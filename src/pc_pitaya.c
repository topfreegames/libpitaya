/**
 * Copyright (c) 2014,2015 NetEase, Inc. and other Pomelo contributors
 * MIT Licensed.
 */

#include <pc_assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include <pitaya.h>
#include <pitaya_trans.h>

#include "pc_lib.h"
#include "pc_pitaya_i.h"
#include "pc_trans_repo.h"
#include "pc_error.h"

/* static int pc__init_magic_num = 0x65521abc; */

static pc_client_config_t pc__default_config = PC_CLIENT_CONFIG_DEFAULT;

size_t pc_client_size()
{
    return sizeof(pc_client_t);
}

pc_client_init_result_t pc_client_init(void* ex_data, const pc_client_config_t* config)
{
    pc_client_init_result_t res = {0};

    res.rc = PC_RC_ERROR;
    res.client = (pc_client_t*)pc_lib_malloc(pc_client_size());
    memset(res.client, 0, pc_client_size());

    if (!config) {
        res.client->config = pc__default_config;
    } else {
        memcpy(&res.client->config, config, sizeof(pc_client_config_t));
    }

    pc_transport_plugin_t *tp = pc__get_transport_plugin(res.client->config.transport_name);

    if (!tp) {
        pc_lib_log(PC_LOG_ERROR, "pc_client_init - no registered transport plugin found, transport plugin: %d", config->transport_name);
        pc_lib_free(res.client);
        res.client = NULL;
        res.rc = PC_RC_NO_TRANS;
        return res;
    }

    pc_assert(tp->transport_create);
    pc_assert(tp->transport_release);

    pc_transport_t *trans = tp->transport_create(tp);
    if (!trans) {
        pc_lib_log(PC_LOG_ERROR, "pc_client_init - create transport error");
        pc_lib_free(res.client);
        res.client = NULL;
        res.rc = PC_RC_ERROR;
        return res;
    }

    res.client->trans = trans;

    pc_assert(res.client->trans->init);

    if (res.client->trans->init(res.client->trans, res.client)) {
        pc_lib_log(PC_LOG_ERROR, "pc_client_init - init transport error");
        tp->transport_release(tp, trans);
        pc_lib_free(res.client);
        res.client = NULL;
        res.rc = PC_RC_ERROR;
        return res;
    }

    pc_mutex_init(&res.client->state_mutex);

    res.client->ex_data = ex_data;

    pc_mutex_init(&res.client->handler_mutex);
    QUEUE_INIT(&res.client->ev_handlers);

    pc_mutex_init(&res.client->req_mutex);
    pc_mutex_init(&res.client->notify_mutex);

    QUEUE_INIT(&res.client->req_queue);
    QUEUE_INIT(&res.client->notify_queue);

    res.client->seq_num = 0;
    res.client->req_id_seq = 1;

    memset(&res.client->requests[0], 0, sizeof(pc_request_t) * PC_PRE_ALLOC_REQUEST_SLOT_COUNT);
    memset(&res.client->notifies[0], 0, sizeof(pc_notify_t) * PC_PRE_ALLOC_NOTIFY_SLOT_COUNT);

    for (int i = 0; i < PC_PRE_ALLOC_REQUEST_SLOT_COUNT; ++i) {
        QUEUE_INIT(&res.client->requests[i].base.queue);
        res.client->requests[i].base.client = res.client;
        res.client->requests[i].base.type = PC_REQ_TYPE_REQUEST | PC_PRE_ALLOC_ST_IDLE | PC_PRE_ALLOC;
    }

    for (int i = 0; i < PC_PRE_ALLOC_NOTIFY_SLOT_COUNT; ++i) {
        QUEUE_INIT(&res.client->notifies[i].base.queue);
        res.client->notifies[i].base.client = res.client;
        res.client->notifies[i].base.type = PC_REQ_TYPE_NOTIFY | PC_PRE_ALLOC_ST_IDLE | PC_PRE_ALLOC;
    }

    pc_mutex_init(&res.client->event_mutex);
    if (res.client->config.enable_polling) {

        QUEUE_INIT(&res.client->pending_ev_queue);

        memset(&res.client->pending_events[0], 0, sizeof(pc_event_t) * PC_PRE_ALLOC_EVENT_SLOT_COUNT);

        for (int i = 0; i < PC_PRE_ALLOC_EVENT_SLOT_COUNT; ++i) {
            QUEUE_INIT(&res.client->pending_events[i].queue);
            res.client->pending_events[i].type = PC_PRE_ALLOC_ST_IDLE | PC_PRE_ALLOC;
        }
    }

    res.client->is_in_poll = 0;
    res.client->state = PC_ST_INITED;
    res.rc = PC_RC_OK;

    pc_lib_log(PC_LOG_DEBUG, "pc_client_init - init ok");
    return res;
}

int pc_client_connect(pc_client_t* client, const char* host, int port, const char* handshake_opts)
{
    int state;
    int ret;

    if (!client || !host || port < 0 || port > (1 << 16) - 1) {
        pc_lib_log(PC_LOG_ERROR, "pc_client_connect - invalid args");
        return PC_RC_INVALID_ARG;
    }

    if (client->config.enable_polling) {
        pc_client_poll(client);
    }

    state = pc_client_state(client);
    switch(state) {
    case PC_ST_DISCONNECTING:
        pc_lib_log(PC_LOG_ERROR, "pc_client_connect - invalid state, state: %s", pc_client_state_str(state));
        return PC_RC_INVALID_STATE;

    case PC_ST_CONNECTED:
    case PC_ST_CONNECTING:
        pc_lib_log(PC_LOG_INFO, "pc_client_connect - client already connecting or connected");
        return PC_RC_OK;

    case PC_ST_INITED:
        pc_assert(client->trans && client->trans->connect);

        pc_mutex_lock(&client->state_mutex);
        client->state = PC_ST_CONNECTING;
        pc_mutex_unlock(&client->state_mutex);

        ret = client->trans->connect(client->trans, host, port, handshake_opts);

        if (ret != PC_RC_OK) {
            pc_lib_log(PC_LOG_ERROR, "pc_client_connect - transport connect error, rc: %s", pc_client_rc_str(ret));
            pc_mutex_lock(&client->state_mutex);
            client->state = PC_ST_INITED;
            pc_mutex_unlock(&client->state_mutex);
        }

        return ret;
    }
    pc_lib_log(PC_LOG_ERROR, "pc_client_connect - unknown client state found, state: %d", state);
    return PC_RC_ERROR;
}

int pc_client_disconnect(pc_client_t* client)
{
    int state;
    int ret;

    if (!client) {
        pc_lib_log(PC_LOG_ERROR, "pc_client_disconnect - client is null");
        return PC_RC_INVALID_ARG;
    }

    if (client->config.enable_polling) {
        pc_client_poll(client);
    }

    state = pc_client_state(client);
    switch(state) {
        case PC_ST_INITED:
            pc_lib_log(PC_LOG_ERROR, "pc_client_disconnect - invalid state, state: %s",
                    pc_client_state_str(state));
            return PC_RC_INVALID_STATE;

        case PC_ST_CONNECTING:
        case PC_ST_CONNECTED:
            pc_assert(client->trans && client->trans->disconnect);

            pc_mutex_lock(&client->state_mutex);
            client->state = PC_ST_DISCONNECTING;
            pc_mutex_unlock(&client->state_mutex);

            ret = client->trans->disconnect(client->trans);

            if (ret != PC_RC_OK) {
                pc_lib_log(PC_LOG_ERROR, "pc_client_disconnect - transport disconnect error: %s",
                        pc_client_rc_str(ret));
                pc_mutex_lock(&client->state_mutex);
                client->state = state;
                pc_mutex_unlock(&client->state_mutex);
            }
            return ret;

        case PC_ST_DISCONNECTING:
            pc_lib_log(PC_LOG_INFO, "pc_client_disconnect - client is already disconnecting");
            return PC_RC_OK;
    }
    pc_lib_log(PC_LOG_ERROR, "pc_client_disconnect - unknown client state found, %d", state);
    return PC_RC_ERROR;
}

int pc_client_cleanup(pc_client_t* client)
{
    QUEUE* q;
    int ret;

    if (!client) {
        pc_lib_log(PC_LOG_ERROR, "pc_client_cleanup - client is null");
        return PC_RC_INVALID_ARG;
    }

    pc_assert(client->trans && client->trans->cleanup);

    /*
     * when cleaning transport up, transport should ack all
     * the request it holds from client so that client can release them
     *
     * transport->cleanup may be blocking.
     */
    ret = client->trans->cleanup(client->trans);
    if (ret != PC_RC_OK) {
        pc_lib_log(PC_LOG_ERROR, "pc_client_cleanup - transport cleanup error: %s", pc_client_rc_str(ret));
        return ret;
    }

    pc_transport_plugin_t *plugin = client->trans->plugin(client->trans);
    plugin->transport_release(plugin, client->trans);

    client->trans = NULL;

    if (client->config.enable_polling) {
        pc_client_poll(client);

        pc_assert(QUEUE_EMPTY(&client->pending_events));
    }

    pc_assert(QUEUE_EMPTY(&client->req_queue));
    pc_assert(QUEUE_EMPTY(&client->notify_queue));

    while (!QUEUE_EMPTY(&client->ev_handlers)) {
        q = QUEUE_HEAD(&client->ev_handlers);
        QUEUE_REMOVE(q);
        QUEUE_INIT(q);

        pc_ev_handler_t *handler = QUEUE_DATA(q, pc_ev_handler_t, queue);

        if (handler->destructor) {
            handler->destructor(handler->ex_data);
        }

        pc_lib_free(handler);
    }

    pc_mutex_destroy(&client->req_mutex);
    pc_mutex_destroy(&client->notify_mutex);
    pc_mutex_destroy(&client->event_mutex);

    pc_mutex_destroy(&client->handler_mutex);
    pc_mutex_destroy(&client->state_mutex);

    client->req_id_seq = 1;
    client->seq_num = 0;

    pc_lib_free(client);

    return PC_RC_OK;
}

static void pc__handle_event(pc_client_t* client, pc_event_t* ev)
{
    pc_assert(PC_EV_IS_RESP(ev->type) || PC_EV_IS_NOTIFY_SENT(ev->type) || PC_EV_IS_NET_EVENT(ev->type));

    if (PC_EV_IS_RESP(ev->type)) {
        pc__trans_resp(client, ev->data.req.req_id, &ev->data.req.resp, &ev->data.req.error);
        pc_lib_log(PC_LOG_DEBUG, "pc__handle_event - fire pending trans resp, req_id: %u",
                ev->data.req.req_id);

        pc__error_free(&ev->data.req.error);

        pc_lib_free((char* )ev->data.req.resp.base);
        ev->data.req.resp.base = NULL;
        ev->data.req.resp.len = -1;

    } else if (PC_EV_IS_NOTIFY_SENT(ev->type)) {
        pc__trans_sent(client, ev->data.notify.seq_num, &ev->data.notify.error);
        pc_lib_log(PC_LOG_DEBUG, "pc__handle_event - fire pending trans sent, seq_num: %u, rc: %s",
                ev->data.notify.seq_num, ev->data.notify.error.code);

        pc__error_free(&ev->data.notify.error);
    } else if (PC_EV_IS_PUSH(ev->type)) {
        pc__trans_push(client, ev->data.push.route, &ev->data.push.buf);

        pc_lib_log(PC_LOG_DEBUG, "pc__handle_event - fire pending trans sent, seq_num: %u, rc: %s",
                ev->data.notify.seq_num, ev->data.notify.error.code);

        pc_lib_free((char*)ev->data.push.route);
    } else {
        pc__trans_fire_event(client, ev->data.ev.ev_type, ev->data.ev.arg1, ev->data.ev.arg2);
        pc_lib_log(PC_LOG_DEBUG, "pc__handle_event - fire pending trans event: %s, arg1: %s",
                pc_client_ev_str(ev->data.ev.ev_type), ev->data.ev.arg1 ? ev->data.ev.arg1 : "");
        pc_lib_free((char* )ev->data.ev.arg1);
        pc_lib_free((char* )ev->data.ev.arg2);

        ev->data.ev.arg1 = NULL;
        ev->data.ev.arg2 = NULL;
    }

    if (PC_IS_DYN_ALLOC(ev->type)) {
        pc_lib_free(ev);
    } else {
        PC_PRE_ALLOC_SET_IDLE(ev->type);
    }
}

int pc_client_poll(pc_client_t* client)
{
    pc_event_t* ev;
    QUEUE* q;

    if (!client) {
        pc_lib_log(PC_LOG_ERROR, "pc_client_poll - client is null");
        return PC_RC_INVALID_ARG;
    }

    if (!client->config.enable_polling) {
        pc_lib_log(PC_LOG_ERROR, "pc_client_poll - client did not enable polling");
        return PC_RC_ERROR;
    }

    pc_mutex_lock(&client->event_mutex);

    /*
     * `is_in_poll` is used to avoid recursive invocation of pc_client_poll
     * by identical thread as `pc_mutex_t` is recursive.
     *
     * `is_in_poll` can be protected by `event_mutex` too, so no extra mutex
     * is needed here.
     */
    if (!client->is_in_poll) {
        client->is_in_poll = 1;

        while(!QUEUE_EMPTY(&client->pending_ev_queue)) {
            q = QUEUE_HEAD(&client->pending_ev_queue);
            ev = (pc_event_t*) QUEUE_DATA(q, pc_event_t, queue);

            QUEUE_REMOVE(&ev->queue);
            QUEUE_INIT(&ev->queue);

            pc_assert((PC_IS_PRE_ALLOC(ev->type) && PC_PRE_ALLOC_IS_BUSY(ev->type)) || PC_IS_DYN_ALLOC(ev->type));

            pc__handle_event(client, ev);
        }
        client->is_in_poll = 0;
    }

    pc_mutex_unlock(&client->event_mutex);

    return PC_RC_OK;
}

int pc_client_add_ev_handler(pc_client_t* client, pc_event_cb_t cb,
                             void* ex_data, void (*destructor)(void* ex_data))
{
    pc_ev_handler_t* handler;
    static int handler_id = 0;

    if (!client || !cb) {
        pc_lib_log(PC_LOG_ERROR, "pc_client_add_ev_handler - invalid args");
        return PC_EV_INVALID_HANDLER_ID;
    }

    handler = (pc_ev_handler_t*)pc_lib_malloc(sizeof(pc_ev_handler_t));
    memset(handler, 0, sizeof(pc_ev_handler_t));

    QUEUE_INIT(&handler->queue);
    handler->ex_data = ex_data;
    handler->cb = cb;
    handler->handler_id = handler_id++;
    handler->destructor = destructor;

    pc_mutex_lock(&client->handler_mutex);

    QUEUE_INSERT_TAIL(&client->ev_handlers, &handler->queue);
    pc_lib_log(PC_LOG_INFO, "pc_client_add_ev_handler -"
            " add event handler, handler id: %d", handler->handler_id);

    pc_mutex_unlock(&client->handler_mutex);

    return handler->handler_id;
}

int pc_client_rm_ev_handler(pc_client_t* client, int id)
{
    QUEUE* q;
    pc_ev_handler_t* handler;
    int flag = 0;

    pc_mutex_lock(&client->handler_mutex);

    QUEUE_FOREACH(q, &client->ev_handlers) {
        handler = QUEUE_DATA(q, pc_ev_handler_t, queue);

        if (handler->handler_id != id)
            continue;

        pc_lib_log(PC_LOG_INFO, "pc_client_rm_ev_handler - rm handler, handler_id: %d", id);
        flag = 1;
        QUEUE_REMOVE(q);
        QUEUE_INIT(q);

        if (handler->destructor) {
            handler->destructor(handler->ex_data);
        }

        pc_lib_free(handler);
        break;
    }

    pc_mutex_unlock(&client->handler_mutex);

    if (!flag) {
        pc_lib_log(PC_LOG_WARN, "pc_client_rm_ev_handler - no matched event handler found, handler id: %d", id);
    }

    return PC_RC_OK;
}

void* pc_client_ex_data(const pc_client_t* client)
{
    return client->ex_data;
}

const pc_client_config_t* pc_client_config(const pc_client_t* client)
{
    return &client->config;
}

int pc_client_state(pc_client_t* client)
{
    int state;

    if (!client) {
        pc_lib_log(PC_LOG_DEBUG, "pc_client_state - client is null");
        return PC_ST_UNKNOWN;
    }

    pc_mutex_lock(&client->state_mutex);
    state = client->state;
    pc_mutex_unlock(&client->state_mutex);

    return state;
}

int pc_client_conn_quality(pc_client_t* client)
{
    if (!client) {
        pc_lib_log(PC_LOG_ERROR, "pc_client_conn_quality - client is null, invalid arg");
        return PC_RC_INVALID_ARG;
    }

    pc_assert(client->trans);

    if (client->trans->quality) {
        return client->trans->quality(client->trans);
    } else {
        pc_lib_log(PC_LOG_ERROR, "pc_client_conn_quality - transport doesn't support qulity query");
    }

    return PC_RC_ERROR;
}

void* pc_client_trans_data(pc_client_t* client)
{
    if (!client) {
        pc_lib_log(PC_LOG_ERROR, "pc_client_trans_data - client is null, invalid arg");
        return NULL;
    }

    pc_assert(client->trans);

    if (client->trans->internal_data) {
        return client->trans->internal_data(client->trans);
    } else {
        pc_lib_log(PC_LOG_ERROR, "pc_client_trans_data - transport doesn't support internal data");
    }

    return NULL;
}

const char *pc_client_serializer(pc_client_t *client)
{
    if (!client) {
        pc_lib_log(PC_LOG_ERROR, "pc_client_serializer - client is null, invalid arg");
        return NULL;
    }

    pc_assert(client->trans);
    return client->trans->serializer(client->trans);
}

void pc_client_free_serializer(const char *serializer)
{
    pc_lib_free((void*)serializer);
}

static int pc__request_with_timeout(pc_client_t* client, const char* route, 
                                    pc_buf_t msg_buf, void* ex_data, int timeout, 
                                    pc_request_success_cb_t success_cb, pc_request_error_cb_t error_cb);

int pc_string_request_with_timeout(pc_client_t* client, const char* route, 
                                   const char *str, void* ex_data, int timeout, 
                                   pc_request_success_cb_t success_cb, pc_request_error_cb_t error_cb)
{
    pc_buf_t buf = pc_buf_from_string(str);
    return pc__request_with_timeout(client, route, buf, ex_data, timeout, success_cb, error_cb);
}

int pc_binary_request_with_timeout(pc_client_t* client, const char* route, 
                                   uint8_t *data, int64_t len, void* ex_data, int timeout,
                                   pc_request_success_cb_t success_cb, pc_request_error_cb_t error_cb)
{
    if (len < 0) {
        return PC_RC_INVALID_ARG;
    }

    pc_buf_t buf;
    buf.len = len;
    buf.base = pc_lib_malloc((size_t)len);
    memcpy(buf.base, data, len);
    return pc__request_with_timeout(client, route, buf, ex_data, timeout, success_cb, error_cb);
}

static int pc__request_with_timeout(pc_client_t* client, const char* route, 
                                    pc_buf_t msg_buf, void* ex_data, int timeout, 
                                    pc_request_success_cb_t cb, pc_request_error_cb_t error_cb)
{
    if (!client || !route || !cb) {
        pc_lib_log(PC_LOG_ERROR, "pc_request_with_timeout - invalid args");
        return PC_RC_INVALID_ARG;
    }

    int state = pc_client_state(client);
    if (state != PC_ST_CONNECTED && state != PC_ST_CONNECTING) {
        pc_lib_log(PC_LOG_ERROR, "pc_request_with_timeout - invalid state, state: %s", pc_client_state_str(state));
        return PC_RC_INVALID_STATE;
    }

    if (timeout != PC_WITHOUT_TIMEOUT && timeout <= 0) {
        pc_lib_log(PC_LOG_ERROR, "pc_request_with_timeout - timeout value is invalid");
        return PC_RC_INVALID_ARG;
    }

    pc_assert(client->trans && client->trans->send);

    pc_mutex_lock(&client->req_mutex);

    pc_request_t* req = NULL;
    for (int i = 0; i < PC_PRE_ALLOC_REQUEST_SLOT_COUNT; ++i) {
        if (PC_PRE_ALLOC_IS_IDLE(client->requests[i].base.type)) {
            req = &client->requests[i];

            PC_PRE_ALLOC_SET_BUSY(req->base.type);
            pc_assert(!req->base.route && !req->base.msg_buf.base);
            pc_assert(PC_IS_PRE_ALLOC(req->base.type));
            pc_lib_log(PC_LOG_DEBUG, "pc_request_with_timeout - use pre alloc request");

            break;
        }
    }

    if (!req) {
        req = (pc_request_t* )pc_lib_malloc(sizeof(pc_request_t));
        memset(req, 0, sizeof(pc_request_t));

        pc_lib_log(PC_LOG_DEBUG, "pc_request_with_timeout - use dynamic alloc request");
        req->base.type = PC_DYN_ALLOC | PC_REQ_TYPE_REQUEST;
        req->base.client = client;
    }

    QUEUE_INIT(&req->base.queue);
    QUEUE_INSERT_TAIL(&client->req_queue, &req->base.queue);

    req->base.route = pc_lib_strdup(route);
    req->base.msg_buf = msg_buf;

    req->base.seq_num = client->seq_num++;
    req->base.timeout = timeout;
    req->base.ex_data = ex_data;

    if (client->req_id_seq == PC_NOTIFY_PUSH_REQ_ID || client->req_id_seq == PC_INVALID_REQ_ID)
        client->req_id_seq = 1;
    req->req_id = client->req_id_seq++;
    req->cb = cb;
    req->error_cb = error_cb;

    pc_mutex_unlock(&client->req_mutex);

    pc_lib_log(PC_LOG_INFO, "pc_request_with_timeout - add request to queue, req id: %u", req->req_id);

    int ret = client->trans->send(client->trans, req->base.route, req->base.seq_num, req->base.msg_buf, req->req_id, req->base.timeout);

    pc_lib_log(PC_LOG_DEBUG, "pc_request_with_timeout - transport send function CALLED");

    if (ret != PC_RC_OK) {
        pc_lib_log(PC_LOG_ERROR, "pc_request_with_timeout - send to transport error,"
                " req id: %u, error: %s", req->req_id, pc_client_rc_str(ret));

        pc_mutex_lock(&client->req_mutex);

        pc_lib_free((char* )req->base.msg_buf.base);
        pc_lib_free((char* )req->base.route);

        req->base.msg_buf.base = NULL;
        req->base.msg_buf.len = -1;
        req->base.route = NULL;

        QUEUE_REMOVE(&req->base.queue);
        QUEUE_INIT(&req->base.queue);

        if (PC_IS_PRE_ALLOC(req->base.type)) {
            PC_PRE_ALLOC_SET_IDLE(req->base.type);
        } else {
            pc_lib_free(req);
        }

        pc_mutex_unlock(&client->req_mutex);
    }

    return ret;
}

pc_client_t* pc_request_client(const pc_request_t* req)
{
    pc_assert(req);
    return req->base.client;
}

const char* pc_request_route(const pc_request_t* req)
{
    pc_assert(req);
    return req->base.route;
}

const char* pc_request_msg(const pc_request_t* req)
{
    pc_assert(req);
    return (const char*)req->base.msg_buf.base;
}

int pc_request_timeout(const pc_request_t* req)
{
    pc_assert(req);
    return req->base.timeout;
}

void* pc_request_ex_data(const pc_request_t* req)
{
    pc_assert(req);
    return req->base.ex_data;
}

static int pc__notify_with_timeout(pc_client_t* client, const char* route, pc_buf_t msg_buf, void* ex_data,
                                   int timeout, pc_notify_error_cb_t cb);

int pc_binary_notify_with_timeout(pc_client_t* client, const char* route, uint8_t *data, int64_t len,
                                  void* ex_data, int timeout, pc_notify_error_cb_t cb)
{
    pc_buf_t buf;
    buf.len = len;
    buf.base = pc_lib_malloc(len);
    memcpy(buf.base, data, len);
    return pc__notify_with_timeout(client, route, buf, ex_data, timeout, cb);
}

int pc_string_notify_with_timeout(pc_client_t* client, const char* route, const char *str, 
                                  void* ex_data, int timeout, pc_notify_error_cb_t cb)
{
    pc_buf_t buf = pc_buf_from_string(str);
    return pc__notify_with_timeout(client, route, buf, ex_data, timeout, cb);
}

static int pc__notify_with_timeout(pc_client_t* client, const char* route, pc_buf_t msg_buf, void* ex_data,
                                   int timeout, pc_notify_error_cb_t cb)
{
    pc_notify_t* notify;
    int i;
    int ret;
    int state;

    if (!client || !route || msg_buf.len == -1) {
        pc_lib_log(PC_LOG_ERROR, "pc_notify_with_timeout - invalid args");
        return PC_RC_INVALID_ARG;
    }

    if (timeout != PC_WITHOUT_TIMEOUT && timeout <= 0) {
        pc_lib_log(PC_LOG_ERROR, "pc_notify_with_timeout - invalid timeout value");
        return PC_RC_INVALID_ARG;
    }

    state = pc_client_state(client);
    if(state != PC_ST_CONNECTED && state != PC_ST_CONNECTING) {
        pc_lib_log(PC_LOG_ERROR, "pc_request_with_timeout - invalid state, state: %s", pc_client_state_str(state));
        return PC_RC_INVALID_STATE;
    }


    pc_assert(client->trans && client->trans->send);

    pc_mutex_lock(&client->req_mutex);

    notify = NULL;
    for (i = 0; i < PC_PRE_ALLOC_NOTIFY_SLOT_COUNT; ++i) {
        if (PC_PRE_ALLOC_IS_IDLE(client->notifies[i].base.type)) {
            notify = &client->notifies[i];

            PC_PRE_ALLOC_SET_BUSY(notify->base.type);

            pc_lib_log(PC_LOG_DEBUG, "pc_notify_with_timeout - use pre alloc notify");
            pc_assert(!notify->base.route && !notify->base.msg_buf.base);
            pc_assert(PC_IS_PRE_ALLOC(notify->base.type));

            break;
        }
    }

    if (!notify) {
        notify = (pc_notify_t* )pc_lib_malloc(sizeof(pc_notify_t));
        memset(notify, 0, sizeof(pc_notify_t));

        pc_lib_log(PC_LOG_DEBUG, "pc_notify_with_timeout - use dynamic alloc notify");
        notify->base.type = PC_REQ_TYPE_NOTIFY | PC_DYN_ALLOC;
        notify->base.client = client;
    }

    QUEUE_INIT(&notify->base.queue);
    QUEUE_INSERT_TAIL(&client->notify_queue, &notify->base.queue);

    notify->base.route = pc_lib_strdup(route);
    notify->base.msg_buf = msg_buf;

    notify->base.seq_num = client->seq_num++;

    notify->base.timeout = timeout;
    notify->base.ex_data = ex_data;

    notify->cb = cb;

    pc_mutex_unlock(&client->req_mutex);

    pc_lib_log(PC_LOG_INFO, "pc_notify_with_timeout - add notify to queue, seq num: %u", notify->base.seq_num);

    ret = client->trans->send(client->trans, notify->base.route, notify->base.seq_num,
                              notify->base.msg_buf, PC_NOTIFY_PUSH_REQ_ID, notify->base.timeout);

    if (ret != PC_RC_OK) {
        pc_lib_log(PC_LOG_ERROR, "pc_notify_with_timeout - send to transport error,"
                " seq num: %u, error: %s", notify->base.seq_num, pc_client_rc_str(ret));

        pc_mutex_lock(&client->req_mutex);

        pc_lib_free((char* )notify->base.msg_buf.base);
        pc_lib_free((char* )notify->base.route);

        notify->base.msg_buf.base = NULL;
        notify->base.msg_buf.len = -1;
        notify->base.route = NULL;

        QUEUE_REMOVE(&notify->base.queue);
        QUEUE_INIT(&notify->base.queue);

        if (PC_IS_PRE_ALLOC(notify->base.type)) {
            PC_PRE_ALLOC_SET_IDLE(notify->base.type);
        } else {
            pc_lib_free(notify);
        }

        pc_mutex_unlock(&client->req_mutex);
    }
    return ret;
}

pc_client_t* pc_notify_client(const pc_notify_t* notify)
{
    pc_assert(notify);
    return notify->base.client;
}

const char* pc_notify_route(const pc_notify_t* notify)
{
    pc_assert(notify);
    return notify->base.route;
}

const pc_buf_t *pc_notify_msg(const pc_notify_t* notify)
{
    pc_assert(notify);
    return &notify->base.msg_buf;
}

int pc_notify_timeout(const pc_notify_t* notify)
{
    pc_assert(notify);
    return notify->base.timeout;
}

void* pc_notify_ex_data(const pc_notify_t* notify)
{
    pc_assert(notify);
    return notify->base.ex_data;
}

pc_buf_t pc_buf_copy(const pc_buf_t *buf)
{
    if (!buf->base) {
        pc_buf_t empty_buf = {0};
        return empty_buf;
    }

    pc_assert(buf->base);
    pc_assert(buf->len >= 0);

    pc_buf_t new_buf;
    new_buf.base = pc_lib_malloc((size_t)buf->len);
    new_buf.len = buf->len;

    if (!new_buf.base) {
        new_buf.len = -1;
        return new_buf;
    }

    memcpy(new_buf.base, buf->base, buf->len);

    new_buf.len = buf->len;
    return new_buf;
}

void pc_buf_free(pc_buf_t *buf)
{
    if (buf->base) {
        pc_lib_free(buf->base);
        buf->base = NULL;
        buf->len = 0;
    }
}

pc_buf_t pc_buf_empty()
{
    pc_buf_t buf;
    memset(&buf, 0, sizeof(pc_buf_t));
    return buf;
}

pc_buf_t pc_buf_from_string(const char *str)
{
    pc_buf_t buf = {0};
    if (str) {
        size_t str_len = strlen(str);
        
        if (str_len > 0) {
            buf.base = pc_lib_malloc(str_len+1);
            buf.len = (int64_t)str_len;
            
            buf.base[buf.len] = '\0';
            strncpy((char*)buf.base, str, buf.len);
        }
    }
    return buf;
}

void pc_buf_debug_print(const pc_buf_t *buf)
{
    printf("[");
    for (int i = 0; i < buf->len; ++i) {
        if (i == buf->len-1) {
            printf("%d", buf->base[i]);
        } else {
            printf("%d ", buf->base[i]);
        }
    }
    printf("]\n");
}

void pc_client_set_push_handler(pc_client_t *client, pc_push_handler_cb_t cb)
{
    client->push_handler = cb;
}
