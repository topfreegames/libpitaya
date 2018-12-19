/**
 * Copyright (c) 2014,2015 NetEase, Inc. and other Pomelo contributors
 * MIT Licensed.
 */

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <pc_assert.h>
#include <time.h>

#include <pc_lib.h>
#include <pc_pitaya_i.h>

#include "tr_uv_tcp_aux.h"
#include "tr_uv_tcp_i.h"
#include "pr_pkg.h"
#include "pr_gzip.h"
#include "pc_error.h"

#define GET_TT(x) tr_uv_tcp_transport_t* tt = (tr_uv_tcp_transport_t* )(x->data); pc_assert(tt)

static void tcp__reset_wi(pc_client_t* client, tr_uv_wi_t* wi)
{
    if (TR_UV_WI_IS_RESP(wi->type)) {
        pc_lib_log(PC_LOG_DEBUG, "tcp__reset_wi - reset request, req_id: %u", wi->req_id);
        pc_error_t err = pc__error_reset();
        pc_buf_t empty_buf = {0};
        pc_trans_resp(client, wi->req_id, &empty_buf, &err);
    } else if (TR_UV_WI_IS_NOTIFY(wi->type)) {
        pc_lib_log(PC_LOG_DEBUG, "tcp__reset_wi - reset notify, seq_num: %u", wi->seq_num);
        pc_error_t err = pc__error_reset();
        pc_trans_sent(client, wi->seq_num, &err);
    }
    /* drop internal write item */

    pc_lib_free(wi->buf.base);
    wi->buf.base = NULL;
    wi->buf.len = 0;

    if (PC_IS_PRE_ALLOC(wi->type)) {
        PC_PRE_ALLOC_SET_IDLE(wi->type);
    } else {
        pc_lib_free(wi);
    }
}

void tcp__reset(tr_uv_tcp_transport_t* tt)
{
    tr_uv_wi_t* wi;
    QUEUE* q;

    pc_assert(tt);

    tt->state = TR_UV_TCP_NOT_CONN;

    pc_pkg_parser_reset(&tt->pkg_parser);

    uv_timer_stop(&tt->hb_timeout_timer);
    uv_timer_stop(&tt->hb_timer);

    uv_timer_stop(&tt->check_timeout);

    uv_timer_stop(&tt->reconn_delay_timer);
    uv_timer_stop(&tt->conn_timeout);

    tt->is_waiting_hb = 0;
    tt->hb_rtt = -1;

    uv_read_stop((uv_stream_t* )&tt->socket);

    if (tt->client->state != PC_ST_INITED
            && !uv_is_closing((uv_handle_t*)&tt->socket)) {
        uv_close((uv_handle_t*)&tt->socket, NULL);
    }

    pc_mutex_lock(&tt->wq_mutex);

    /*
     * As the callback invoked in tcp__reset_wi may send notify/request,
     * that means `tcp__reset_wi` will fill conn_pending/write_wait queue,
     * which would lead to infinite loop, if reseting wis directly on
     * conn_pending/write_wait queue.
     *
     * So, we move all the wis inside conn_pending/write_wait queue to
     * writing_queue.
     */
    if (!QUEUE_EMPTY(&tt->conn_pending_queue)) {
        QUEUE_ADD(&tt->writing_queue, &tt->conn_pending_queue);
        QUEUE_INIT(&tt->conn_pending_queue);
    }

    if (!QUEUE_EMPTY(&tt->write_wait_queue)) {
        QUEUE_ADD(&tt->writing_queue, &tt->write_wait_queue);
        QUEUE_INIT(&tt->write_wait_queue);
    }

    while(!QUEUE_EMPTY(&tt->writing_queue)) {
        q = QUEUE_HEAD(&tt->writing_queue);
        QUEUE_REMOVE(q);
        QUEUE_INIT(q);

        wi = (tr_uv_wi_t* )QUEUE_DATA(q, tr_uv_wi_t, queue);
        tcp__reset_wi(tt->client, wi);
    }

    while(!QUEUE_EMPTY(&tt->resp_pending_queue)) {
        q = QUEUE_HEAD(&tt->resp_pending_queue);
        QUEUE_REMOVE(q);
        QUEUE_INIT(q);

        wi = (tr_uv_wi_t* )QUEUE_DATA(q, tr_uv_wi_t, queue);
        tcp__reset_wi(tt->client, wi);
    }

    pc_mutex_unlock(&tt->wq_mutex);

}

void tcp__reconn_delay_timer_cb(uv_timer_t* t)
{
    GET_TT(t);

    pc_assert(t == &tt->reconn_delay_timer);
    uv_timer_stop(t);
    uv_async_send(&tt->conn_async);
}

void tcp__reconn(tr_uv_tcp_transport_t* tt)
{
    int timeout;
    const pc_client_config_t* config;
    int i;
    int factor;
    pc_assert(tt && tt->reset_fn);

    tt->reset_fn(tt);

    tt->state = TR_UV_TCP_CONNECTING;

    config = tt->config;

    if (!config->enable_reconn) {
         pc_lib_log(PC_LOG_WARN, "tcp__reconn - trans want to reconn, but reconn is disabled");
         tt->reconn_times = 0;
         tt->state = TR_UV_TCP_NOT_CONN;
         return;
    }

    if (tt->reconn_times == 0) {
        // This is the first time the reconnection is being called, therefore send an event informing the user.
        pc_trans_fire_event(tt->client, PC_EV_RECONNECT_STARTED, "Started the reconnection", NULL);
    } else {
        pc_mutex_lock(&tt->client->state_mutex);
        tt->client->state = PC_ST_CONNECTING;
        pc_mutex_unlock(&tt->client->state_mutex);
    }

    tt->reconn_times++;
    if (config->reconn_max_retry != PC_ALWAYS_RETRY && config->reconn_max_retry < tt->reconn_times) {
        pc_lib_log(PC_LOG_WARN, "tcp__reconn - reconn times exceeded");
        pc_trans_fire_event(tt->client, PC_EV_RECONNECT_FAILED, "Exceed Max Retry", NULL);
        tt->reconn_times = 0;
        tt->state = TR_UV_TCP_NOT_CONN;
        return ;
    }

    if (!tt->max_reconn_incr) {

        if (!config->reconn_delay) {
            factor = 1;
        } else {
            factor = config->reconn_delay_max / config->reconn_delay;
            if (factor <= 0)
                factor = 1;
        }

        if (!config->reconn_exp_backoff) {
            tt->max_reconn_incr = factor + 1;
        } else {
            for (i = 1; ; ++i) {
                if (!(factor >> i)) {
                    break;
                }
            }

            tt->max_reconn_incr = i + 1;
        }
        pc_lib_log(PC_LOG_DEBUG, "tcp__reconn - max reconn delay incr: %d", tt->max_reconn_incr);
    }

    if (tt->reconn_times >= tt->max_reconn_incr) {
        timeout = config->reconn_delay_max;
    } else {
        if (!config->reconn_exp_backoff) {
            timeout = config->reconn_delay * tt->reconn_times;
        } else {
            timeout = config->reconn_delay << (tt->reconn_times - 1);
        }
    }

    timeout = (rand() % timeout) + timeout / 2;

    pc_lib_log(PC_LOG_DEBUG, "tcp__reconn - reconnect, delay: %d", timeout);

    uv_timer_start(&tt->reconn_delay_timer, tcp__reconn_delay_timer_cb, timeout * 1000, 0);
}

static void on_connection_done_cb(uv_connect_t *connect, int status)
{
    tr_uv_tcp_transport_t *tt = (tr_uv_tcp_transport_t*)connect->data;
    pc_assert(tt);

    // NOTE(lhahn): If conn_done_cb is set on the transport, it means the client
    // has not yet cleaned up, otherwise the pointer will be NULL.
    if (tt->conn_done_cb) {
        tt->conn_done_cb(connect, status);
    }
}

void tcp__conn_async_cb(uv_async_t* t)
{
    struct addrinfo hints;
    struct addrinfo* ainfo;
    struct addrinfo* rp;
    struct sockaddr_in* addr4 = NULL;
    struct sockaddr_in6* addr6 = NULL;
    struct sockaddr* addr = NULL;
    int ret;

    GET_TT(t);

    pc_assert(t == &tt->conn_async);

    if (tt->is_connecting)
        return;

    tt->state = TR_UV_TCP_CONNECTING;

    pc_assert(tt->host && tt->reconn_fn);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_ADDRCONFIG;
    hints.ai_socktype = SOCK_STREAM;

    uv_tcp_init(&tt->uv_loop, &tt->socket);
    tt->socket.data = tt;

    ret = getaddrinfo(tt->host, NULL, &hints, &ainfo);

    if (ret) {
        pc_lib_log(PC_LOG_ERROR, "tcp__conn_async_cb - dns resolve error, state: %s", pc_client_state_str(tt->client->state));
        pc_lib_log(PC_LOG_ERROR, "tcp__conn_async_cb - dns resolve error: %s, will reconn", tt->host);
        pc_trans_fire_event(tt->client, PC_EV_CONNECT_ERROR, "DNS Resolve Error", NULL);
        tt->reconn_fn(tt);
        return ;
    }

    for (rp = ainfo; rp; rp = rp->ai_next) {
        if (rp->ai_family == AF_INET) {
            addr4 = (struct sockaddr_in* )rp->ai_addr;
            addr4->sin_port = htons(tt->port);
            break;

        } else if(rp->ai_family == AF_INET6){
            addr6 = (struct sockaddr_in6* )rp->ai_addr;
            addr6->sin6_port = htons(tt->port);
            break;

        } else {
            continue;
        }
    }

    if (!addr4 && !addr6) {
        pc_trans_fire_event(tt->client, PC_EV_CONNECT_ERROR, "DNS Resolve Error", NULL);
        pc_lib_log(PC_LOG_ERROR, "tcp__conn_async_cb - dns resolve error: %s, will reconn", tt->host);
        freeaddrinfo(ainfo);
        tt->reconn_fn(tt);
        return;
    }

    addr = addr4 ? (struct sockaddr* )addr4 : (struct sockaddr* )addr6;
    tt->conn_req.data = tt;
    ret = uv_tcp_connect(&tt->conn_req, &tt->socket, addr, on_connection_done_cb);

    freeaddrinfo(ainfo);

    if (ret) {
        pc_trans_fire_event(tt->client, PC_EV_CONNECT_ERROR, "UV Conn Error", NULL);
        pc_lib_log(PC_LOG_ERROR, "tcp__conn_async_cb - uv tcp connect error: %s, will reconn", uv_strerror(ret));
        tt->reconn_fn(tt);
        return ;
    }

    tt->is_connecting = 1;

    if (tt->config->conn_timeout != PC_WITHOUT_TIMEOUT) {
        pc_lib_log(PC_LOG_DEBUG, "tcp__con_async_cb - start conn timeout timer");
        uv_timer_start(&tt->conn_timeout, tcp__conn_timeout_cb, tt->config->conn_timeout * 1000, 0);
    }
}

void tcp__conn_timeout_cb(uv_timer_t* t)
{
    GET_TT(t);

    pc_assert(&tt->conn_timeout == t);
    pc_assert(tt->is_connecting);
    uv_timer_stop(t);
    pc_lib_log(PC_LOG_INFO, "tcp__conn_timeout_cb - conn timeout, cancel it");

    if (!uv_is_closing((uv_handle_t*)&tt->socket)) {
        uv_close((uv_handle_t* )&tt->socket, NULL);
    }
}

void tcp__conn_done_cb(uv_connect_t* conn, int status)
{
    int hs_timeout;
    int ret;

    GET_TT(conn);

    pc_assert(&tt->conn_req == conn);
    pc_assert(tt->is_connecting);

    tt->is_connecting = 0;
    if (tt->config->conn_timeout != PC_WITHOUT_TIMEOUT) {

        /*
         * NOTE: we hack uv here to get the rest timeout value of conn_timeout,
         *
         * and use it as the timeout value of handshake.
         *
         * it maybe lead to be non-compatiable to uv in future.
         */
        hs_timeout = (int)(tt->conn_timeout.timeout - tt->uv_loop.time);
        uv_timer_stop(&tt->conn_timeout);
    }

    if (status == 0) {
        /* tcp connected. */
        tt->state = TR_UV_TCP_HANDSHAKEING;

        ret = uv_read_start((uv_stream_t* ) &tt->socket, tcp__alloc_cb, tt->on_tcp_read_cb);

        if (ret) {
            pc_lib_log(PC_LOG_ERROR, "tcp__conn_done_cb - start read from tcp error, reconn");
            tt->reconn_fn(tt);
            return ;
        }

        /* XXX: ignore return of uv_tcp_keepalive */
        uv_tcp_keepalive(&tt->socket, 1, 60);

        pc_lib_log(PC_LOG_INFO, "tcp__conn_done_cb - tcp connected, send handshake");

        tcp__send_handshake(tt);

        if (tt->config->conn_timeout != PC_WITHOUT_TIMEOUT) {
            uv_timer_start( &tt->handshake_timer, tcp__handshake_timer_cb, hs_timeout, 0);
        }
        return ;
    }

    if (status == UV_ECANCELED) {
        pc_lib_log(PC_LOG_DEBUG, "tcp__conn_done_cb - connect timeout");
        pc_trans_fire_event(tt->client, PC_EV_CONNECT_ERROR, "Connect Timeout", NULL);
    } else {
        pc_lib_log(PC_LOG_DEBUG, "tcp__conn_done_cb - connect error, error: %s", uv_strerror(status));
        pc_trans_fire_event(tt->client, PC_EV_CONNECT_ERROR, "Connect Error", NULL);
    }

    tt->reconn_fn(tt);
}

void tcp__write_async_cb(uv_async_t* a)
{
    int buf_cnt;
    int i;
    int ret;
    int need_check = 0;
    QUEUE* q;
    tr_uv_wi_t* wi;
    uv_buf_t* bufs;
    GET_TT(a);

    if (tt->state == TR_UV_TCP_NOT_CONN) {
        return ;
    }

    pc_assert(a == &tt->write_async);

    if (tt->is_writing) {
        return ;
    }

    pc_mutex_lock(&tt->wq_mutex);
    if (tt->state == TR_UV_TCP_DONE) {
        while (!QUEUE_EMPTY(&tt->conn_pending_queue)) {
            q = QUEUE_HEAD(&tt->conn_pending_queue);
            QUEUE_REMOVE(q);
            QUEUE_INIT(q);

            wi = (tr_uv_wi_t* )QUEUE_DATA(q, tr_uv_wi_t, queue);

            if (!TR_UV_WI_IS_INTERNAL(wi->type)) {
                pc_lib_log(PC_LOG_DEBUG, "tcp__write_async_cb - move wi from conn pending to write wait,"
                    "seq_num: %u, req_id: %u", wi->seq_num, wi->req_id);
            }

            QUEUE_INSERT_TAIL(&tt->write_wait_queue, q);
        }
    } else {
        need_check = !QUEUE_EMPTY(&tt->conn_pending_queue);
    }

    buf_cnt = 0;

    QUEUE_FOREACH(q, &tt->write_wait_queue) {
        wi = (tr_uv_wi_t*)QUEUE_DATA(q, tr_uv_wi_t, queue);

        if (!TR_UV_WI_IS_INTERNAL(wi->type) && wi->timeout != PC_WITHOUT_TIMEOUT) {
            need_check = 1;
        }

        buf_cnt++;
    }

    if (buf_cnt == 0) {
        pc_mutex_unlock(&tt->wq_mutex);
        if (need_check) {
            pc_lib_log(PC_LOG_DEBUG, "WEE NEED A CHEECK");
            /* if there are pending req, we should start to check timeout */
            if (!uv_is_active((uv_handle_t* )&tt->check_timeout)) {
                pc_lib_log(PC_LOG_DEBUG, "tcp__write_async_cb - start check timeout timer");
                uv_timer_start(&tt->check_timeout, tt->write_check_timeout_cb,
                        PC_TIMEOUT_CHECK_INTERVAL * 1000, 0);
            }
        }
        return ;
    }

    bufs = (uv_buf_t* )pc_lib_malloc(sizeof(uv_buf_t) * buf_cnt);

    i = 0;
    while (!QUEUE_EMPTY(&tt->write_wait_queue)) {
        q = QUEUE_HEAD(&tt->write_wait_queue);

        QUEUE_REMOVE(q);
        QUEUE_INIT(q);

        wi = (tr_uv_wi_t* )QUEUE_DATA(q, tr_uv_wi_t, queue);

        if (!TR_UV_WI_IS_INTERNAL(wi->type)) {
            pc_lib_log(PC_LOG_DEBUG, "tcp__write_async_cb - move wi from write wait to writing queue,"
                    "seq_num: %u, req_id: %u", wi->seq_num, wi->req_id);
        }

        bufs[i++] = wi->buf;

        QUEUE_INSERT_TAIL(&tt->writing_queue, q);
    }

    pc_assert(i == buf_cnt);

    pc_mutex_unlock(&tt->wq_mutex);

    tt->write_req.data = tt;

    ret = uv_write(&tt->write_req, (uv_stream_t* )&tt->socket, bufs, buf_cnt, tcp__write_done_cb);

    pc_lib_free(bufs);

    if (ret) {
        pc_lib_log(PC_LOG_ERROR, "tcp__write_async_cb - uv write error: %s", uv_strerror(ret));

        pc_mutex_lock(&tt->wq_mutex);
        while (!QUEUE_EMPTY(&tt->writing_queue)) {
            q = QUEUE_HEAD(&tt->writing_queue);
            QUEUE_REMOVE(q);
            QUEUE_INIT(q);

            wi = (tr_uv_wi_t* )QUEUE_DATA(q, tr_uv_wi_t, queue);

            pc_lib_free(wi->buf.base);
            wi->buf.base = NULL;
            wi->buf.len = 0;

            if (TR_UV_WI_IS_NOTIFY(wi->type)) {
                pc_error_t err = pc__error_uv(ret);
                pc_trans_sent(tt->client, wi->seq_num, &err);
            }

            if (TR_UV_WI_IS_RESP(wi->type)) {
                pc_error_t err = pc__error_uv(ret);
                pc_buf_t empty_buf = {0};
                pc_trans_resp(tt->client, wi->req_id, &empty_buf, &err);
            }
            /* if internal, do nothing here. */

            if (PC_IS_PRE_ALLOC(wi->type)) {
                PC_PRE_ALLOC_SET_IDLE(wi->type);
            } else {
                pc_lib_free(wi);
            }
        }
        pc_mutex_unlock(&tt->wq_mutex);
        return ;
    }

    tt->is_writing = 1;

    /* enable check timeout timer */
    if (need_check && !uv_is_active((uv_handle_t* )&tt->check_timeout)) {
        pc_lib_log(PC_LOG_DEBUG, "tcp__write_async_cb - start check timeout timer");
        uv_timer_start(&tt->check_timeout, tt->write_check_timeout_cb,
                PC_TIMEOUT_CHECK_INTERVAL * 1000, 0);
    }

}

void tcp__write_done_cb(uv_write_t* w, int status)
{
    GET_TT(w);

    pc_assert(tt->is_writing);
    pc_assert(w == &tt->write_req);

    tt->is_writing = 0;

    if (status) {
        pc_lib_log(PC_LOG_ERROR, "tcp__write_done_cb - uv_write callback error: %s", uv_strerror(status));
    }

    status = status == 0 ? PC_RC_OK : PC_RC_ERROR;

    pc_mutex_lock(&tt->wq_mutex);

    while (!QUEUE_EMPTY(&tt->writing_queue)) {
        QUEUE *q = QUEUE_HEAD(&tt->writing_queue);
        QUEUE_REMOVE(q);
        QUEUE_INIT(q);

        tr_uv_wi_t *wi = (tr_uv_wi_t* )QUEUE_DATA(q, tr_uv_wi_t, queue);

        if (!status && TR_UV_WI_IS_RESP(wi->type)) {

            pc_lib_log(PC_LOG_DEBUG, "tcp__write_done_cb - move wi from writing to resp pending queue,"
                " req_id: %u", wi->req_id);

            QUEUE_INSERT_TAIL(&tt->resp_pending_queue, q);
            continue;
        }

        pc_lib_free(wi->buf.base);
        wi->buf.base = NULL;
        wi->buf.len = 0;

        if (TR_UV_WI_IS_NOTIFY(wi->type)) {
            if (status) {
                pc_error_t err = pc__error_uv(status);
                pc_trans_sent(tt->client, wi->seq_num, &err);
            } else {
                pc_trans_sent(tt->client, wi->seq_num, NULL);
            }
        }

        if (TR_UV_WI_IS_RESP(wi->type)) {
            if (status) {
                pc_error_t err = pc__error_uv(status);
                pc_buf_t empty_buf = {0};
                pc_trans_resp(tt->client, wi->req_id, &empty_buf, &err);
            }
        }
        /* if internal, do nothing here. */

        if (PC_IS_PRE_ALLOC(wi->type)) {
            PC_PRE_ALLOC_SET_IDLE(wi->type);
        } else {
            pc_lib_free(wi);
        }
    }
    pc_mutex_unlock(&tt->wq_mutex);

    uv_async_send(&tt->write_async);
}

int tcp__check_queue_timeout(QUEUE* ql, pc_client_t* client, int cont)
{

    pc_lib_log(PC_LOG_WARN, "tcp__check_queue_timeout ");
    QUEUE tmp;
    QUEUE* q;
    tr_uv_wi_t* wi;
    time_t ct = time(0);

    QUEUE_INIT(&tmp);
    while (!QUEUE_EMPTY(ql)) {
        q = QUEUE_HEAD(ql);
        QUEUE_REMOVE(q);
        QUEUE_INIT(q);

        wi = (tr_uv_wi_t* )QUEUE_DATA(q, tr_uv_wi_t, queue);

        if (wi->timeout != PC_WITHOUT_TIMEOUT) {
            if (ct > wi->ts + wi->timeout) {
                if (TR_UV_WI_IS_NOTIFY(wi->type)) {
                    pc_lib_log(PC_LOG_WARN, "tcp__check_queue_timeout - notify timeout, seq num: %u", wi->seq_num);
                    pc_error_t err = pc__error_timeout();
                    pc_trans_sent(client, wi->seq_num, &err);
                } else if (TR_UV_WI_IS_RESP(wi->type)) {
                    pc_lib_log(PC_LOG_WARN, "tcp__check_queue_timeout - request timeout, req id: %u", wi->req_id);
                    pc_error_t err = pc__error_timeout();
                    pc_buf_t empty_buf = {0};
                    pc_trans_resp(client, wi->req_id, &empty_buf, &err);
                }

                /* if internal, just drop it. */

                pc_lib_free(wi->buf.base);
                wi->buf.base = NULL;
                wi->buf.len = 0;

                if (PC_IS_PRE_ALLOC(wi->type)) {
                    PC_PRE_ALLOC_SET_IDLE(wi->type);
                } else {
                    pc_lib_free(wi);
                }
                continue;
            } else {
                /*
                 * continue to check timeout next tick
                 * if there are wis has timeout configured but not triggered this time.
                 */
                cont = 1;
            }
        }
        /* add the non-timeout wi to queue tmp */
        QUEUE_INSERT_TAIL(&tmp, q);
    } /* while */
    /* restore ql with the non-timeout wi */
    QUEUE_ADD(ql, &tmp);
    QUEUE_INIT(&tmp);
    return cont;
}

void tcp__write_check_timeout_cb(uv_timer_t* w)
{
    int cont;
    GET_TT(w);

    pc_assert(w == &tt->check_timeout);

    cont = 0;

    pc_lib_log(PC_LOG_DEBUG, "tcp__write_check_timeout_cb - start to check timeout");

    pc_mutex_lock(&tt->wq_mutex);
    cont = tcp__check_queue_timeout(&tt->conn_pending_queue, tt->client, cont);
    cont = tcp__check_queue_timeout(&tt->write_wait_queue, tt->client, cont);
    cont = tcp__check_queue_timeout(&tt->resp_pending_queue, tt->client, cont);
    pc_mutex_unlock(&tt->wq_mutex);

    if (cont && !uv_is_active((uv_handle_t* )w)) {
        uv_timer_start(w, tt->write_check_timeout_cb, PC_TIMEOUT_CHECK_INTERVAL* 1000, 0);
    }
    pc_lib_log(PC_LOG_DEBUG, "tcp__write_check_timeout_cb - finish to check timeout");
}

static void tcp__cleanup_pc_json(pc_JSON** j)
{
    if (*j) {
        pc_JSON_Delete(*j);
        *j = NULL;
    }
}

static void walk_cb(uv_handle_t *handle, void *arg)
{
    uv_close(handle, NULL);
}

void tcp__cleanup_async_cb(uv_async_t* a)
{
    GET_TT(a);

    pc_assert(a == &tt->cleanup_async);

    tt->reset_fn(tt);

    if (tt->host) {
        pc_lib_free((char *)tt->host);
        tt->host = NULL;
    }

    tt->reconn_times = 0;
    // This ensures that the callback will not be called after everything is
    // cleaned up.
    tt->conn_done_cb = NULL;

    // Walk all handles in the loop closing each one of them.
    // libuv will call for each handle the walk_cb function.
    uv_walk(&tt->uv_loop, walk_cb, NULL);

    tcp__cleanup_pc_json(&tt->handshake_opts);
    tcp__cleanup_pc_json(&tt->route_to_code);
    tcp__cleanup_pc_json(&tt->code_to_route);
}

void tcp__disconnect_async_cb(uv_async_t* a)
{
    GET_TT(a);

    pc_assert(a == &tt->disconnect_async);
    tt->reset_fn(tt);
    tt->reconn_times = 0;
    pc_lib_log(PC_LOG_DEBUG, "tcp__disconnect_async_cb - sending disconnect event");
    pc_trans_fire_event(tt->client, PC_EV_DISCONNECT, NULL, NULL);
}

void tcp__send_heartbeat(tr_uv_tcp_transport_t* tt)
{
    uv_buf_t buf;
    int i;
    tr_uv_wi_t* wi;
    wi = NULL;

    pc_assert(tt->state == TR_UV_TCP_DONE);

    pc_lib_log(PC_LOG_DEBUG, "tcp__send__heartbeat - send heartbeat");

    buf = pc_pkg_encode(PC_PKG_HEARBEAT, NULL, 0);

    pc_assert(buf.len && buf.base);

    pc_mutex_lock(&tt->wq_mutex);
    for (i = 0; i < TR_UV_PRE_ALLOC_WI_SLOT_COUNT; ++i) {
        if (PC_PRE_ALLOC_IS_IDLE(tt->pre_wis[i].type)) {
            wi = &tt->pre_wis[i];
            PC_PRE_ALLOC_SET_BUSY(wi->type);
            break;
        }
    }

    if (!wi) {
        wi = (tr_uv_wi_t* )pc_lib_malloc(sizeof(tr_uv_wi_t));
        memset(wi, 0, sizeof(tr_uv_wi_t));
        wi->type = PC_DYN_ALLOC;
    }

    QUEUE_INIT(&wi->queue);
    TR_UV_WI_SET_INTERNAL(wi->type);

    wi->buf = buf;
    wi->seq_num = -1; /* internal data */
    wi->req_id = -1; /* internal data */
    wi->timeout = PC_WITHOUT_TIMEOUT; /* internal timeout */
    wi->ts = time(NULL);

    QUEUE_INSERT_TAIL(&tt->write_wait_queue, &wi->queue);

    pc_mutex_unlock(&tt->wq_mutex);

    uv_async_send(&tt->write_async);
}

void tcp__on_heartbeat(tr_uv_tcp_transport_t* tt)
{
    int rtt = 0;
    int start = 0;

    if (!tt->is_waiting_hb) {
        pc_lib_log(PC_LOG_WARN, "tcp__on_heartbeat - tcp is not waiting for heartbeat, ignore");
        return;
    }

    pc_lib_log(PC_LOG_DEBUG, "tcp__on_heartbeat - tcp get heartbeat");
    pc_assert(tt->state == TR_UV_TCP_DONE);
    pc_assert(uv_is_active((uv_handle_t*)&tt->hb_timeout_timer));

    /*
     * we hacking uv timer to get the heartbeat rtt, rtt in millisec
     * int is enough to hold the value
     */
    start = (int)(tt->hb_timeout_timer.timeout - tt->hb_timeout * 1000);

    rtt = (int)(tt->uv_loop.time - start);

    uv_timer_stop(&tt->hb_timeout_timer);

    tt->is_waiting_hb = 0;

    if (tt->hb_rtt == -1 ) {
        tt->hb_rtt = rtt;
    } else {
        tt->hb_rtt = (tt->hb_rtt * 2 + rtt) / 3;
        pc_lib_log(PC_LOG_INFO, "tcp__on_heartbeat - calc rtt: %d", tt->hb_rtt);
    }

    uv_timer_start(&tt->hb_timer, tcp__heartbeat_timer_cb, tt->hb_interval * 1000, 0);
}

void tcp__heartbeat_timer_cb(uv_timer_t* t)
{
    GET_TT(t);

    pc_assert(t == &tt->hb_timer);
    pc_assert(tt->is_waiting_hb == 0);
    pc_assert(tt->state == TR_UV_TCP_DONE);

    tcp__send_heartbeat(tt);
    tt->is_waiting_hb = 1;
    pc_lib_log(PC_LOG_DEBUG, "tcp__heartbeat_timer_cb - start heartbeat timeout timer");

    uv_timer_start(&tt->hb_timeout_timer, tcp__heartbeat_timeout_cb, tt->hb_timeout * 1000, 0);
}

void tcp__heartbeat_timeout_cb(uv_timer_t* t)
{
    GET_TT(t);

    pc_assert(tt->is_waiting_hb);
    pc_assert(t == &tt->hb_timeout_timer);

    pc_lib_log(PC_LOG_WARN, "tcp__heartbeat_timeout_cb - will reconn, hb timeout");
    pc_trans_fire_event(tt->client, PC_EV_UNEXPECTED_DISCONNECT, "HB Timeout", NULL);
    tt->reconn_fn(tt);
}

void tcp__handshake_timer_cb(uv_timer_t* t)
{
    GET_TT(t);

    pc_assert(t == &tt->handshake_timer);

    pc_lib_log(PC_LOG_ERROR, "tcp__handshake_timer_cb - tcp handshake timeout, will reconn");
    pc_trans_fire_event(tt->client, PC_EV_CONNECT_ERROR, "Connect Timeout", NULL);
    tt->reconn_fn(tt);
}

void tcp__on_tcp_read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
    GET_TT(stream);

    if (nread < 0) {
        pc_lib_log(PC_LOG_ERROR, "tcp__on_tcp_read_cb - read from tcp error: %s,"
                   "will reconn", uv_strerror(nread));

        if (tt->state == TR_UV_TCP_DONE) {
            // If connection is completed, there was an unexpected disconnect
            pc_trans_fire_event(tt->client, PC_EV_UNEXPECTED_DISCONNECT, "Read Error Or Close", NULL);
        } else {
            // Otherwise, the client failed to connect.
            pc_trans_fire_event(tt->client, PC_EV_CONNECT_FAILED, "Failed to complete pitaya connection", NULL);
        }

        tt->reconn_fn(tt);
        return;
    }

    pc_pkg_parser_feed(&tt->pkg_parser, buf->base, nread);
}

void tcp__alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    size_t len;
    GET_TT(handle);

    len = suggested_size < PC_TCP_READ_BUFFER_SIZE ? suggested_size : PC_TCP_READ_BUFFER_SIZE;

    buf->base = tt->tcp_read_buf;
    buf->len = len;
}

void tcp__on_data_recieved(tr_uv_tcp_transport_t* tt, const char* data, size_t len)
{
    QUEUE* q;
    tr_uv_wi_t* wi = NULL;
    tr_uv_tcp_transport_plugin_t* plugin = (tr_uv_tcp_transport_plugin_t* )tt->base.plugin((pc_transport_t*)tt);

    uv_buf_t buf;
    buf.base = (char*)data;
    buf.len = len;

    pc_msg_t msg = plugin->pr_msg_decoder(tt, &buf);

    if (msg.id == PC_INVALID_REQ_ID || !msg.buf.base) {
        pc_lib_log(PC_LOG_ERROR, "tcp__on_data_recieved - decode error, will reconn");
        pc_trans_fire_event(tt->client, PC_EV_PROTO_ERROR, "Decode Error", NULL);
        tt->reconn_fn(tt);
        return ;
    }

    if (msg.id == PC_NOTIFY_PUSH_REQ_ID && !msg.route) {
        pc_lib_log(PC_LOG_ERROR, "tcp__on_data_recieved - push message without route, error, will reconn");
        pc_trans_fire_event(tt->client, PC_EV_PROTO_ERROR, "No Route Specified", NULL);
        tt->reconn_fn(tt);
        return ;
    }

    pc_assert((msg.id == PC_NOTIFY_PUSH_REQ_ID && msg.route)
            || (msg.id != PC_NOTIFY_PUSH_REQ_ID && !msg.route));

    pc_lib_log(PC_LOG_INFO, "tcp__on_data_recieved - recived data, req_id: %d", msg.id);

    if (msg.id != PC_NOTIFY_PUSH_REQ_ID) {
        /* request */
        if (msg.error) {
            pc_error_t err = pc__error_server(&msg.buf);
            pc_trans_resp(tt->client, msg.id, &msg.buf, &err);
            pc__error_free(&err);
        } else {
            pc_trans_resp(tt->client, msg.id, &msg.buf, NULL);
        }

        /*
         * As we will stop iterating if matched wi found,
         * so it is safe to use `QUEUE_FOREACH` here.
         */
        QUEUE_FOREACH(q, &tt->resp_pending_queue) {
            wi = (tr_uv_wi_t* )QUEUE_DATA(q, tr_uv_wi_t, queue);
            pc_assert(TR_UV_WI_IS_RESP(wi->type));

            if (wi->req_id != msg.id)
                continue;

            QUEUE_REMOVE(q);
            QUEUE_INIT(q);

            pc_lib_free(wi->buf.base);
            wi->buf.base = NULL;
            wi->buf.len = 0;

            if (PC_IS_PRE_ALLOC(wi->type)) {
                pc_mutex_lock(&tt->wq_mutex);
                PC_PRE_ALLOC_SET_IDLE(wi->type);
                pc_mutex_unlock(&tt->wq_mutex);
            } else {
                pc_lib_free(wi);
            }
            break;
        }
    } else {
        pc_trans_fire_push_event(tt->client, msg.route, &msg.buf);
    }

    pc_lib_free((char *)msg.route);
    pc_buf_free(&msg.buf);
}

void tcp__on_kick_recieved(tr_uv_tcp_transport_t* tt)
{
    pc_lib_log(PC_LOG_INFO, "tcp__on_kick_recieved - kicked by server");
    pc_trans_fire_event(tt->client, PC_EV_KICKED_BY_SERVER, NULL, NULL);
    tt->reset_fn(tt);
}

void tcp__send_handshake(tr_uv_tcp_transport_t* tt)
{
    uv_buf_t buf;
    tr_uv_wi_t* wi;
    pc_JSON* sys;
    pc_JSON* body;

    char* data;
    int i;

    body = pc_JSON_CreateObject();
    sys = pc_JSON_CreateObject();

    pc_assert(tt->state == TR_UV_TCP_HANDSHAKEING);

    pc_assert((tt->route_to_code && tt->code_to_route)
            || (!tt->route_to_code && !tt->code_to_route));

    pc_JSON_AddItemToObject(sys, "platform", pc_JSON_CreateString(pc_lib_platform_str));
    pc_JSON_AddItemToObject(sys, "libVersion", pc_JSON_CreateString(pc_lib_version_str()));
    pc_JSON_AddItemToObject(sys, "clientBuildNumber", pc_JSON_CreateString(pc_lib_client_build_number_str));
    pc_JSON_AddItemToObject(sys, "clientVersion", pc_JSON_CreateString(pc_lib_client_version_str));

    pc_JSON_AddItemToObject(body, "sys", sys);

    if (tt->handshake_opts) {
        pc_JSON_AddItemReferenceToObject(body, "user", tt->handshake_opts);
    }

    data = pc_JSON_PrintUnformatted(body);
    pc_lib_log(PC_LOG_DEBUG, "tcp__send_handshake -- sending handshake: %s", data);

    buf = pc_pkg_encode(PC_PKG_HANDSHAKE, data, strlen(data));

    pc_lib_free(data);
    pc_JSON_Delete(body);

    wi = NULL;
    pc_mutex_lock(&tt->wq_mutex);
    for (i = 0; i < TR_UV_PRE_ALLOC_WI_SLOT_COUNT; ++i) {
        if (PC_PRE_ALLOC_IS_IDLE(tt->pre_wis[i].type)) {
            wi = &tt->pre_wis[i];
            PC_PRE_ALLOC_SET_BUSY(wi->type);
            break;
        }
    }

    if (!wi) {
        wi = (tr_uv_wi_t* )pc_lib_malloc(sizeof(tr_uv_wi_t));
        memset(wi, 0, sizeof(tr_uv_wi_t));
        wi->type = PC_DYN_ALLOC;
    }

    QUEUE_INIT(&wi->queue);
    TR_UV_WI_SET_INTERNAL(wi->type);

    wi->buf = buf;
    wi->seq_num = -1; /* internal data */
    wi->req_id = -1; /* internal data */
    wi->timeout = PC_WITHOUT_TIMEOUT; /* internal timeout */
    wi->ts = time(NULL); /* TODO: time() */

    /*
     * insert to head, because handshake req should be sent
     * before any application data.
     */
    QUEUE_INSERT_HEAD(&tt->write_wait_queue, &wi->queue);
    pc_mutex_unlock(&tt->wq_mutex);

    uv_async_send(&tt->write_async);
}

#define PC_HANDSHAKE_OK 200

void tcp__on_handshake_resp(tr_uv_tcp_transport_t* tt, const char* data, size_t len)
{
    int code = -1;
    pc_JSON* res = NULL;
    pc_JSON* tmp = NULL;
    pc_JSON* sys = NULL;
    int i;
    int need_sync = 0;

    pc_assert(tt->state == TR_UV_TCP_HANDSHAKEING);

    tt->reconn_times = 0;

    if (is_compressed((unsigned char*)data, len)) {
        char* uncompressed_data = (char*)pc_lib_malloc(1);
        size_t uncompressed_len;
        pr_decompress((unsigned char**)&uncompressed_data, &uncompressed_len, (unsigned char*) data, len);

        pc_lib_log(PC_LOG_INFO, "data: %.*s", uncompressed_len, uncompressed_data);
        res = pc_JSON_Parse(uncompressed_data);
        pc_lib_free(uncompressed_data);
        
    } else {
        pc_lib_log(PC_LOG_INFO, "data: %.*s", len, data);
        res = pc_JSON_Parse(data);
    }

    pc_lib_log(PC_LOG_INFO, "tcp__on_handshake_resp - tcp get handshake resp");

    if (tt->config->conn_timeout != PC_WITHOUT_TIMEOUT) {
        uv_timer_stop(&tt->handshake_timer);
    }

    if (!res) {
        pc_lib_log(PC_LOG_ERROR, "tcp__on_handshake_resp - handshake resp is not valid json");
        pc_trans_fire_event(tt->client, PC_EV_CONNECT_FAILED, "Handshake Error", NULL);
        tt->reset_fn(tt);
        return ;
    }

    tmp = pc_JSON_GetObjectItem(res, "code");

    if (!tmp || tmp->type != pc_JSON_Number || (code = tmp->valueint) != PC_HANDSHAKE_OK) {
        pc_lib_log(PC_LOG_ERROR, "tcp__on_handshake_resp - handshake fail, code: %d", code);
        pc_trans_fire_event(tt->client, PC_EV_CONNECT_FAILED, "Handshake Error", NULL);
        pc_JSON_Delete(res);
        tt->reset_fn(tt);
        return ;
    }

    /* we just use sys here, ignore user field. */
    sys = pc_JSON_GetObjectItem(res, "sys");
    if (!sys) {
        pc_lib_log(PC_LOG_ERROR, "tcp__on_handshake_resp - handshake fail, no sys field");
        pc_trans_fire_event(tt->client, PC_EV_CONNECT_FAILED, "Handshake Error", NULL);
        pc_JSON_Delete(res);
        tt->reset_fn(tt);
        return ;
    }

    pc_lib_log(PC_LOG_INFO, "tcp__on_handshake_resp - handshake ok");
    /* setup heartbeat */
    tmp = pc_JSON_GetObjectItem(sys, "heartbeat");
    if (!tmp || tmp->type != pc_JSON_Number) {
        i = -1;
    } else {
        i = tmp->valueint;
    }

    if (i <= 0) {
        /* no need heartbeat */
        tt->hb_interval= -1;
        tt->hb_timeout = -1;
        pc_lib_log(PC_LOG_INFO, "tcp__on_handshake_resp - no heartbeat specified");
    } else {
        tt->hb_interval = (int)i;
        pc_lib_log(PC_LOG_INFO, "tcp__on_handshake_resp - set heartbeat interval: %d", i);
        tt->hb_timeout = tt->hb_interval * PC_HEARTBEAT_TIMEOUT_FACTOR;
    }

    tmp = pc_JSON_GetObjectItem(sys, "serializer");
    if (!tmp || tmp->type != pc_JSON_String) {
        pc_lib_log(PC_LOG_WARN, "tcp__on_handshake_resp - invalid serializer field sent by the server, defaulting to 'json'");

        pc_mutex_lock(&tt->serializer_mutex);
        tt->serializer = pc_lib_strdup("json");
        pc_mutex_unlock(&tt->serializer_mutex);
    } else {
        pc_mutex_lock(&tt->serializer_mutex);
        tt->serializer = pc_lib_strdup(tmp->valuestring);
        pc_mutex_unlock(&tt->serializer_mutex);
    }

    tmp = pc_JSON_GetObjectItem(sys, "useDict");
    if (!tmp || tmp->type == pc_JSON_False) {
        if (tt->route_to_code && tt->code_to_route) {
            pc_JSON_Delete(tt->route_to_code);
            pc_JSON_Delete(tt->code_to_route);

            tt->route_to_code = NULL;
            tt->code_to_route = NULL;
            need_sync = 1;
        }
    } else {
        /* pc_JSON* route2code = pc_JSON_DetachItemFromObject(sys, "routeToCode"); */
        /* pc_JSON* code2route = pc_JSON_DetachItemFromObject(sys, "codeToRoute"); */
        pc_assert(tt->route_to_code && tt->code_to_route);
    }
    pc_JSON_Delete(res);
    res = NULL;

    if (tt->config->local_storage_cb && need_sync) {
        pc_JSON* lc = pc_JSON_CreateObject();
        char* data;
        size_t len;

        if (tt->route_to_code) {
            pc_JSON_AddItemReferenceToObject(lc, TR_UV_LCK_ROUTE_2_CODE, tt->route_to_code);
        }

        if (tt->code_to_route) {
            pc_JSON_AddItemReferenceToObject(lc, TR_UV_LCK_CODE_2_ROUTE, tt->code_to_route);
        }

        data = pc_JSON_PrintUnformatted(lc);
        pc_JSON_Delete(lc);

        if (!data) {
            pc_lib_log(PC_LOG_WARN,
                    "tcp__on_handshake_resp - serialize handshake data failed");
        } else {
            len = strlen(data);

            if (tt->config->local_storage_cb(PC_LOCAL_STORAGE_OP_WRITE, data,
                        &len, tt->config->ls_ex_data) != 0) {
                pc_lib_log(PC_LOG_WARN,
                        "tcp__on_handshake_resp - write data to local storage error");
            }
            pc_lib_free(data);
        }
    }

    tcp__send_handshake_ack(tt);
    if (tt->hb_interval != -1) {
        pc_lib_log(PC_LOG_INFO, "tcp__on_handshake_resp - start heartbeat interval timer");
        uv_timer_start(&tt->hb_timer, tcp__heartbeat_timer_cb, tt->hb_interval * 1000, 0);
    }

    tt->state = TR_UV_TCP_DONE;
    pc_lib_log(PC_LOG_INFO, "tcp__on_handshake_resp - handshake completely");
    pc_lib_log(PC_LOG_INFO, "tcp__on_handshake_resp - client connected");
    pc_trans_fire_event(tt->client, PC_EV_CONNECTED, NULL, NULL);
    uv_async_send(&tt->write_async);
}

void tcp__send_handshake_ack(tr_uv_tcp_transport_t* tt)
{
    uv_buf_t buf;
    int i;
    tr_uv_wi_t* wi;

    buf = pc_pkg_encode(PC_PKG_HANDSHAKE_ACK, NULL, 0);

    pc_lib_log(PC_LOG_INFO, "tcp__send_handshake_ack - send handshake ack");

    pc_assert(buf.base && buf.len);

    wi = NULL;
    pc_mutex_lock(&tt->wq_mutex);
    for (i = 0; i < TR_UV_PRE_ALLOC_WI_SLOT_COUNT; ++i) {
        if (PC_PRE_ALLOC_IS_IDLE(tt->pre_wis[i].type)) {
            wi = &tt->pre_wis[i];
            PC_PRE_ALLOC_SET_BUSY(wi->type);
            break;
        }
    }

    if (!wi) {
        wi = (tr_uv_wi_t* )pc_lib_malloc(sizeof(tr_uv_wi_t));
        memset(wi, 0, sizeof(tr_uv_wi_t));
        wi->type = PC_DYN_ALLOC;
    }

    QUEUE_INIT(&wi->queue);

    wi->buf = buf;
    wi->seq_num = -1; /* internal data */
    wi->req_id = -1; /* internal data */
    wi->timeout = PC_WITHOUT_TIMEOUT; /* internal timeout */
    wi->ts = time(NULL);
    TR_UV_WI_SET_INTERNAL(wi->type);

    /*
     * insert to head, because handshake ack should be sent
     * before any application data.
     */
    QUEUE_INSERT_HEAD(&tt->write_wait_queue, &wi->queue);

    pc_mutex_unlock(&tt->wq_mutex);

    uv_async_send(&tt->write_async);
}

#undef GET_TT

