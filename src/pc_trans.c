/**
 * Copyright (c) 2014,2015 NetEase, Inc. and other Pomelo contributors
 * MIT Licensed.
 */

#include <pc_assert.h>
#include <string.h>

#include <pitaya_trans.h>

#include "pc_lib.h"
#include "pc_pitaya_i.h"
#include "pc_error.h"

static void pc__trans_queue_event(pc_client_t* client, int ev_type, const char* arg1, const char* arg2);

void pc_trans_fire_event(pc_client_t* client, int ev_type, const char* arg1, const char* arg2)
{
    if (!client) {
        pc_lib_log(PC_LOG_ERROR, "pc_client_fire_event - client is null");
        return ;
    }

    if (client->config.enable_polling)
        pc__trans_queue_event(client, ev_type, arg1, arg2);
    else
        pc__trans_fire_event(client, ev_type, arg1, arg2);
}

void pc__trans_queue_event(pc_client_t* client, int ev_type, const char* arg1, const char* arg2)
{

    pc_event_t* ev;
    int i;

    if (ev_type >= PC_EV_COUNT || ev_type < 0) {
        pc_lib_log(PC_LOG_ERROR, "pc__trans_queue_event - error event type");
        return;
    }

    if (ev_type == PC_EV_USER_DEFINED_PUSH && (!arg1 || !arg2)) {
        pc_lib_log(PC_LOG_ERROR, "pc__trans_queue_event - push msg but without a route or msg");
        return;
    }

    if (ev_type == PC_EV_CONNECT_ERROR || ev_type == PC_EV_UNEXPECTED_DISCONNECT
        || ev_type == PC_EV_PROTO_ERROR || ev_type == PC_EV_CONNECT_FAILED || ev_type == PC_EV_RECONNECT_FAILED) {
        if (!arg1) {
            pc_lib_log(PC_LOG_ERROR, "pc__trans_queue_event - event should be with a reason description");
            return ;
        }
    }

    pc_assert(client->config.enable_polling);

    pc_lib_log(PC_LOG_INFO, "pc__trans_queue_event - add pending event: %s", pc_client_ev_str(ev_type));
    pc_mutex_lock(&client->event_mutex);

    ev = NULL;
    for (i = 0; i < PC_PRE_ALLOC_EVENT_SLOT_COUNT; ++i) {
        if (PC_PRE_ALLOC_IS_IDLE(client->pending_events[i].type)) {
            ev = &client->pending_events[i];
            PC_PRE_ALLOC_SET_BUSY(ev->type);
            break;
        }
    }

    if (!ev) {
        ev = (pc_event_t* )pc_lib_malloc(sizeof(pc_event_t));
        memset(ev, 0, sizeof(pc_event_t));

        ev->type = PC_DYN_ALLOC;
    }

    PC_EV_SET_NET_EVENT(ev->type);

    QUEUE_INIT(&ev->queue);
    QUEUE_INSERT_TAIL(&client->pending_ev_queue, &ev->queue);

    ev->data.ev.ev_type = ev_type;

    if (arg1) {
        ev->data.ev.arg1 = pc_lib_strdup(arg1);
    } else {
        ev->data.ev.arg1 = NULL;
    }

    if (arg2) {
        ev->data.ev.arg2 = pc_lib_strdup(arg2);
    } else {
        ev->data.ev.arg2 = NULL;
    }

    pc_mutex_unlock(&client->event_mutex);
}

void pc__trans_fire_event(pc_client_t* client, int ev_type, const char* arg1, const char* arg2)
{
    QUEUE* q;
    pc_ev_handler_t* handler;

    if (ev_type >= PC_EV_COUNT || ev_type < 0) {
        pc_lib_log(PC_LOG_ERROR, "pc__transport_fire_event - error event type");
        return;
    }

    if (ev_type == PC_EV_USER_DEFINED_PUSH && (!arg1 || !arg2)) {
        pc_lib_log(PC_LOG_ERROR, "pc__transport_fire_event - push msg but without a route or msg");
        return;
    }

    if (ev_type == PC_EV_CONNECT_ERROR || ev_type == PC_EV_UNEXPECTED_DISCONNECT
        || ev_type == PC_EV_PROTO_ERROR || ev_type == PC_EV_CONNECT_FAILED || ev_type == PC_EV_RECONNECT_FAILED) {
        if (!arg1) {
            pc_lib_log(PC_LOG_ERROR, "pc__transport_fire_event - event should be with a reason description");
            return ;
        }
    }

    pc_lib_log(PC_LOG_INFO, "pc__trans_fire_event - fire event: %s, arg1: %s, arg2: %s",
            pc_client_ev_str(ev_type), arg1 ? arg1 : "", arg2 ? arg2 : "");
    pc_mutex_lock(&client->state_mutex);
    switch(ev_type) {
        case PC_EV_CONNECTED:
            pc_assert(client->state == PC_ST_CONNECTING);
            client->state = PC_ST_CONNECTED;
            break;

        case PC_EV_CONNECT_ERROR:
            pc_assert(client->state == PC_ST_CONNECTING || client->state == PC_ST_DISCONNECTING);
            client->state = PC_ST_INITED;
            break;

        case PC_EV_CONNECT_FAILED:
            pc_assert(client->state == PC_ST_CONNECTING || client->state == PC_ST_DISCONNECTING);
            client->state = PC_ST_INITED;
            break;
            
        case PC_EV_RECONNECT_FAILED:
            pc_assert(client->state == PC_ST_CONNECTING || client->state == PC_ST_DISCONNECTING);
            client->state = PC_ST_INITED;
            break;
            
        case PC_EV_DISCONNECT:
            pc_assert(client->state == PC_ST_DISCONNECTING || client->state == PC_ST_CONNECTED);
            client->state = PC_ST_INITED;
            break;

        case PC_EV_KICKED_BY_SERVER:
            pc_assert(client->state == PC_ST_CONNECTED || client->state == PC_ST_DISCONNECTING);
            client->state = PC_ST_INITED;
            break;

        case PC_EV_RECONNECT_STARTED:
            pc_assert(client->state == PC_ST_CONNECTING || client->state == PC_ST_INITED);
            client->state = PC_ST_CONNECTING;
            break;

        case PC_EV_UNEXPECTED_DISCONNECT:
            pc_assert(client->state == PC_ST_CONNECTED || client->state == PC_ST_DISCONNECTING);
            client->state = PC_ST_INITED;
            break;
        case PC_EV_PROTO_ERROR:
            pc_assert(client->state == PC_ST_CONNECTING || client->state == PC_ST_CONNECTED
                    || client->state == PC_ST_DISCONNECTING);
            client->state = PC_ST_CONNECTING;
            break;
        case PC_EV_USER_DEFINED_PUSH:
            /* do nothing here */
            break;

        default:
            /* never run to here */
            pc_lib_log(PC_LOG_ERROR, "pc__trans_fire_event - unknown network event: %d", ev_type);
    }
    pc_mutex_unlock(&client->state_mutex);

    /* invoke handler */
    pc_mutex_lock(&client->handler_mutex);
    QUEUE_FOREACH(q, &client->ev_handlers) {
        handler = QUEUE_DATA(q, pc_ev_handler_t, queue);
        pc_assert(handler && handler->cb);
        handler->cb(client, ev_type, handler->ex_data, arg1, arg2);
    }
    pc_mutex_unlock(&client->handler_mutex);
}

static void pc__trans_queue_push(pc_client_t *client, const char *route, const pc_buf_t *buf);
void pc__trans_push(pc_client_t *client, const char *route, const pc_buf_t *buf);
void pc_trans_fire_push_event(pc_client_t *client, const char *route, const pc_buf_t *buf)
{
    if (!client) {
        pc_lib_log(PC_LOG_ERROR, "pc_client_fire_event - client is null");
        return ;
    }

    if (client->config.enable_polling)
        pc__trans_queue_push(client, route, buf);
    else
        pc__trans_push(client, route, buf);
}

void pc__trans_queue_push(pc_client_t *client, const char *route, const pc_buf_t *buf)
{
    pc_mutex_lock(&client->event_mutex);

    pc_lib_log(PC_LOG_INFO, "pc__trans_queue_push - add pending push event, route: %s", route);

    pc_event_t *ev = NULL;
    for (int i = 0; i < PC_PRE_ALLOC_EVENT_SLOT_COUNT; ++i) {
        if (PC_PRE_ALLOC_IS_IDLE(client->pending_events[i].type)) {
            ev = &client->pending_events[i];
            PC_PRE_ALLOC_SET_BUSY(ev->type);
            break;
        }
    }

    if (!ev) {
        ev = (pc_event_t* )pc_lib_malloc(sizeof(pc_event_t));
        memset(ev, 0, sizeof(pc_event_t));
        ev->type = PC_DYN_ALLOC;
    }

    QUEUE_INIT(&ev->queue);

    PC_EV_SET_PUSH(ev->type);
    ev->data.push.route = pc_lib_strdup(route);
    ev->data.push.buf = pc_buf_copy(buf);

    QUEUE_INSERT_TAIL(&client->pending_ev_queue, &ev->queue);

    pc_mutex_unlock(&client->event_mutex);
}

void pc__trans_push(pc_client_t *client, const char *route, const pc_buf_t *buf)
{
    pc_assert(client);
    if (!client || !buf->base || buf->len < 0) {
        pc_lib_log(PC_LOG_ERROR, "pc__trans_push - error parameters");
        return;
    }

    if (buf->len == 0) {
        pc_lib_log(PC_LOG_ERROR, "pc__trans_push - empty buffer");
        return;
    }

    pc_lib_log(PC_LOG_INFO, "pc__trans_push - route: %s", route);

    /* invoke handler */
    if (client->push_handler) {
        client->push_handler(client, route, buf);
    }
}

void pc__trans_sent(pc_client_t* client, unsigned int seq_num, const pc_error_t *error)
{
    QUEUE* q;
    pc_notify_t* notify;
    pc_notify_t* target;

    /* callback immediately */
    pc_mutex_lock(&client->notify_mutex);
    target = NULL;
    QUEUE_FOREACH(q, &client->notify_queue) {
        notify = (pc_notify_t* )QUEUE_DATA(q, pc_common_req_t, queue);
        if (notify->base.seq_num == seq_num) {

            pc_lib_log(PC_LOG_INFO, "pc__trans_sent - fire sent event, seq_num: %u", seq_num);

            target = notify;
            QUEUE_REMOVE(q);
            QUEUE_INIT(q);
            break;
        }
    }
    pc_mutex_unlock(&client->notify_mutex);

    if (target) {
        // Do not call the callback if there is no error.
        if (target->cb && error) {
            target->cb(target, error);
        }

        pc_buf_free(&target->base.msg_buf);

        pc_lib_free((char*)target->base.route);
        target->base.route = NULL;

        if (PC_IS_PRE_ALLOC(target->base.type)) {

            pc_mutex_lock(&client->notify_mutex);
            PC_PRE_ALLOC_SET_IDLE(target->base.type);
            pc_mutex_unlock(&client->notify_mutex);

        } else {
            pc_lib_free(target);
        }

    } else {
        pc_lib_log(PC_LOG_ERROR, "pc__trans_sent - no pending notify found"
                " when transport has sent it, seq num: %u", seq_num);
    }
}

static void pc__trans_queue_sent(pc_client_t* client, unsigned int seq_num, const pc_error_t *error);
void pc_trans_sent(pc_client_t* client, unsigned int seq_num, const pc_error_t *error)
{
    if (!client) {
        pc_lib_log(PC_LOG_ERROR, "pc_trans_sent - client is null");
        return ;
    }

    if (client->config.enable_polling) {
        pc__trans_queue_sent(client, seq_num, error);
    } else {
        pc__trans_sent(client, seq_num, error);
    }
}

void pc__trans_queue_sent(pc_client_t* client, unsigned int seq_num, const pc_error_t *error)
{
    pc_event_t* ev;
    int i;

    pc_mutex_lock(&client->event_mutex);

    if (error) {
        pc_lib_log(PC_LOG_INFO, "pc__trans_queue_sent - add pending sent event, seq_num: %u, rc: %d",
                seq_num, error->code);
    } else {
        pc_lib_log(PC_LOG_INFO, "pc__trans_queue_sent - add pending sent event, seq_num: %u", seq_num);
    }

    ev = NULL;
    for (i = 0; i < PC_PRE_ALLOC_EVENT_SLOT_COUNT; ++i) {
        if (PC_PRE_ALLOC_IS_IDLE(client->pending_events[i].type)) {
            ev = &client->pending_events[i];
            PC_PRE_ALLOC_SET_BUSY(ev->type);
            break;
        }
    }

    if (!ev) {
        ev = (pc_event_t* )pc_lib_malloc(sizeof(pc_event_t));
        memset(ev, 0, sizeof(pc_event_t));
        ev->type = PC_DYN_ALLOC;
    }

    QUEUE_INIT(&ev->queue);

    PC_EV_SET_NOTIFY_SENT(ev->type);
    ev->data.notify.seq_num = seq_num;
    ev->data.notify.error = pc__error_dup(error);

    QUEUE_INSERT_TAIL(&client->pending_ev_queue, &ev->queue);

    pc_mutex_unlock(&client->event_mutex);
}

static void pc__trans_queue_resp(pc_client_t* client, unsigned int req_id,
                                 const pc_buf_t *resp, const pc_error_t *error);

void pc_trans_resp(pc_client_t* client, unsigned int req_id, const pc_buf_t *resp, const pc_error_t *error)
{
    if (!client) {
        pc_lib_log(PC_LOG_ERROR, "pc_trans_resp - client is null");
        return ;
    }

    if (client->config.enable_polling) {
        pc__trans_queue_resp(client, req_id, resp, error);
    } else {
        pc__trans_resp(client, req_id, resp, error);
    }
}

void pc__trans_queue_resp(pc_client_t* client, unsigned int req_id, const pc_buf_t *resp, const pc_error_t *error)
{
    pc_mutex_lock(&client->event_mutex);

    pc_lib_log(PC_LOG_INFO, "pc__trans_queue_resp - add pending resp event, req_id: %u", req_id);

    pc_event_t *ev = NULL;
    for (int i = 0; i < PC_PRE_ALLOC_EVENT_SLOT_COUNT; ++i) {
        if (PC_PRE_ALLOC_IS_IDLE(client->pending_events[i].type)) {
            ev = &client->pending_events[i];
            PC_PRE_ALLOC_SET_BUSY(ev->type);
            break;
        }
    }

    if (!ev) {
        ev = (pc_event_t* )pc_lib_malloc(sizeof(pc_event_t));
        memset(ev, 0, sizeof(pc_event_t));

        ev->type = PC_DYN_ALLOC;
    }

    PC_EV_SET_RESP(ev->type);

    QUEUE_INIT(&ev->queue);
    ev->data.req.req_id = req_id;
    ev->data.req.resp = pc_buf_copy(resp);
    ev->data.req.error = pc__error_dup(error);

    QUEUE_INSERT_TAIL(&client->pending_ev_queue, &ev->queue);

    pc_mutex_unlock(&client->event_mutex);
}

void pc__trans_resp(pc_client_t* client, unsigned int req_id, const pc_buf_t *resp, const pc_error_t *error)
{
    QUEUE* q = NULL;

    /* invoke callback immediately */
    pc_request_t *target = NULL;
    pc_mutex_lock(&client->req_mutex);
    QUEUE_FOREACH(q, &client->req_queue) {
        pc_request_t *req = (pc_request_t* )QUEUE_DATA(q, pc_common_req_t, queue);
        if (req->req_id == req_id) {
            if (error) {
                pc_lib_log(PC_LOG_INFO, "pc__trans_resp - fire resp event, req_id: %u, error: %d",
                       req_id, error->code);
            } else {
                pc_lib_log(PC_LOG_INFO, "pc__trans_resp - fire resp event, req_id: %u", req_id);
            }

            target = req;
            QUEUE_REMOVE(q);
            QUEUE_INIT(q);
            break;
        }
    }

    pc_mutex_unlock(&client->req_mutex);

    if (target) {
        if (error && target->error_cb) {
            target->error_cb(target, error);
        } else if (!error) {
            target->cb(target, resp);
        }

        pc_buf_free(&target->base.msg_buf);
        pc_lib_free((char*)target->base.route);

        target->base.route = NULL;

        if (PC_IS_PRE_ALLOC(target->base.type)) {
            pc_mutex_lock(&client->req_mutex);
            PC_PRE_ALLOC_SET_IDLE(target->base.type);
            pc_mutex_unlock(&client->req_mutex);

        } else {
            pc_lib_free(target);
        }
    } else {
        pc_lib_log(PC_LOG_ERROR, "pc__trans_resp - no pending request found when"
            " get a response, req id: %u", req_id);
    }
}

