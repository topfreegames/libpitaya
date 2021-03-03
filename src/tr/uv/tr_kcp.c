//
// Created by lichong on 2020/12/11.
//

#include "tr_kcp.h"
#include <pc_assert.h>
#include <pc_lib.h>
#include "pc_JSON.h"
#include "pc_pitaya_i.h"
#include "pr_pkg.h"
#include "pr_gzip.h"
#include "pc_error.h"

#define KCP_CONV 777


static void udp_on_send(uv_udp_send_t *req, int status) {
    pc_lib_free(req);
}

static int udp_output(const char *buf, int len, ikcpcb *kcp, void *p) {
    tr_kcp_transport_t *tt = p;
    uv_udp_send_t *send_req = pc_lib_malloc(sizeof(uv_udp_send_t));
    memset(send_req, 0, sizeof(uv_udp_send_t));
    char *data = pc_lib_malloc(len);
    memcpy(data, buf, len);
    uv_buf_t msg = uv_buf_init(data, len);
    send_req->data = tt;
    int ret = uv_udp_send(send_req, &tt->send_socket, &msg, 1, (const struct sockaddr *) tt->addr, udp_on_send);
    pc_lib_free(data);
    return ret;
}

static void
uv_udp_on_read(uv_udp_t *req, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags) {
    if (nread <= 0) {
        pc_lib_free(buf->base);
        return;
    }
    tr_kcp_transport_t *tt = req->data;
    int ret = ikcp_input(tt->kcp, buf->base, nread);
    if (ret < 0) {
        pc_lib_log(PC_LOG_ERROR, "uv_udp_on_read - kcp input failed: %d", ret);
    } else {
        uv_async_send(&tt->receive_async);
    }
    pc_lib_free(buf->base);
}

static void tr_kcp_thread_fn(void *arg) {
    uv_loop_t *loop = arg;
    pc_lib_log(PC_LOG_INFO, "tr_kcp_thread_fn - start uv loop");
    uv_run(loop, UV_RUN_DEFAULT);
}

static void uv_udp_alloc_buff(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = pc_lib_malloc(suggested_size);
    buf->len = suggested_size;
}


static void kcp__receive_async(uv_async_t *handle) {
    tr_kcp_transport_t *tt = handle->data;
    int size = ikcp_peeksize(tt->kcp);
    if (size < 0) {
        return;
    }
    char *buf = pc_lib_malloc(size);
    int ret = ikcp_recv(tt->kcp, buf, size);
    if (ret < 0) {
        pc_lib_free(buf);
        return;
    }

    pc_pkg_parser_feed(&tt->pkg_parser, buf, ret);
    pc_lib_free(buf);
}

static void kcp__send_heartbeat(tr_kcp_transport_t *tt) {
    uv_buf_t buf;
    pc_lib_log(PC_LOG_DEBUG, "kcp__send_heartbeat - send heartbeat");
    buf = pc_pkg_encode(PC_PKG_HEARBEAT, NULL, 0);

    pc_assert(buf.len && buf.base);
    ikcp_send(tt->kcp, buf.base, buf.len);
    pc_lib_free(buf.base);
}

static void kcp__on_heartbeat(tr_kcp_transport_t *tt) {
    pc_lib_log(PC_LOG_DEBUG, "kcp__on_heartbeat - [Heartbeat] received from server");
    pc_assert(tt->state == TR_KCP_DONE);
}

static void kcp__on_data_received(tr_kcp_transport_t *tt, const char *data, size_t len) {
    uv_buf_t buf;
    buf.base = (char *) data;
    buf.len = len;
    pc_msg_t msg = pr_kcp_default_msg_decoder(tt, &buf);
    if (msg.id == PC_INVALID_REQ_ID || !msg.buf.base) {
        pc_lib_log(PC_LOG_ERROR, "kcp__on_data_recieved - decode error, will reconn");
        pc_trans_fire_event(tt->client, PC_EV_PROTO_ERROR, "Decode Error", NULL);
        tt->reconn_fn(tt);
        return;
    }

    if (msg.id == PC_NOTIFY_PUSH_REQ_ID && !msg.route) {
        pc_lib_log(PC_LOG_ERROR, "kcp__on_data_recieved - push message without route, error, will reconn");
        pc_trans_fire_event(tt->client, PC_EV_PROTO_ERROR, "No Route Specified", NULL);
        tt->reconn_fn(tt);
        return;
    }

    pc_lib_log(PC_LOG_INFO, "kcp__on_data_received - received data, req_id: %d", msg.id);

    if (msg.id != PC_NOTIFY_PUSH_REQ_ID) {
        /* request */
        if (msg.error) {
            pc_error_t err = pc__error_server(&msg.buf);
            pc_trans_resp(tt->client, msg.id, &msg.buf, &err);
            pc__error_free(&err);
        } else {
            pc_trans_resp(tt->client, msg.id, &msg.buf, NULL);
        }

        QUEUE *q;
        tr_kcp_wait_item_t *wi;
        pc_mutex_lock(&tt->wq_mutex);
        QUEUE_FOREACH(q, &tt->resp_pending_queue) {
            wi = (tr_kcp_wait_item_t *) QUEUE_DATA(q, tr_kcp_wait_item_t, queue);
            pc_assert(wi->type == TR_KCP_WI_TYPE_RESP);
            if (wi->req_id != msg.id)
                continue;

            QUEUE_REMOVE(q);
            QUEUE_INIT(q);
            pc_lib_free(wi->buf.base);
            pc_lib_free(wi);
            break;
        }
        pc_mutex_unlock(&tt->wq_mutex);
    } else {
        pc_trans_fire_push_event(tt->client, msg.route, &msg.buf);
    }

    pc_lib_free((char *) msg.route);
    pc_buf_free(&msg.buf);
}

static void kcp__on_kick_received(tr_kcp_transport_t *tt) {
    pc_lib_log(PC_LOG_INFO, "kcp__on_kick_received - kicked by server");
    pc_trans_fire_event(tt->client, PC_EV_KICKED_BY_SERVER, NULL, NULL);
    tt->reset_fn(tt);
}

static void kcp__heartbeat_timer_cb(uv_timer_t *t) {
    tr_kcp_transport_t *tt = t->data;
    pc_assert(t == &tt->timer_heartbeat);
    pc_assert(tt->state == TR_KCP_DONE);

    uint64_t threshold = (tt->hb_interval * 1000) * (PC_HEARTBEAT_TIMEOUT_FACTOR + 1);
    uint64_t time_elapsed = uv_now(&tt->loop) - tt->last_server_packet_time;
    if (time_elapsed > threshold) {
        pc_lib_log(PC_LOG_WARN, "kcp__heartbeat_time_cb - heartbeat timeout, will reconn");
        pc_trans_fire_event(tt->client, PC_EV_UNEXPECTED_DISCONNECT, "Heartbeat timeout", NULL);
        tt->reconn_fn(tt);
        return;
    }

    kcp__send_heartbeat(tt);
}

static int kcp__check_queue_timeout(QUEUE *ql, pc_client_t *client) {
    int count = 0;
    QUEUE tmp;
    QUEUE *q;
    tr_kcp_wait_item_t *wi;
    time_t now = time(NULL);

    QUEUE_INIT(&tmp);
    while (!QUEUE_EMPTY(ql)) {
        q = QUEUE_HEAD(ql);
        QUEUE_REMOVE(q);
        QUEUE_INIT(q);

        wi = (tr_kcp_wait_item_t *) QUEUE_DATA(q, tr_kcp_wait_item_t, queue);
        if (wi->timeout != PC_WITHOUT_TIMEOUT) {
            if (now > wi->timestamp + wi->timeout) {
                // timeout
                if (wi->type == TR_KCP_WI_TYPE_NOTIFY) {
                    pc_lib_log(PC_LOG_WARN, "kcp__check_queue_timeout - notify timeout, seq_num: %d", wi->seq_num);
                    pc_error_t err = pc__error_timeout();
                    pc_trans_sent(client, wi->seq_num, &err);
                }
                if (wi->type == TR_KCP_WI_TYPE_RESP) {
                    pc_lib_log(PC_LOG_WARN, "kcp__check_queue_timeout - request timeout, req_id: %d", wi->req_id);
                    pc_error_t err = pc__error_timeout();
                    pc_buf_t empty_buf = {0};
                    pc_trans_resp(client, wi->req_id, &empty_buf, &err);
                }

                pc_lib_free(wi->buf.base);
                pc_lib_free(wi);
            } else {
                count += 1;
                QUEUE_INSERT_TAIL(&tmp, q);
            }
        }
    }

    QUEUE_ADD(ql, &tmp);
    QUEUE_INIT(&tmp);
    return count;
}

static void kcp__write_check_timeout(uv_timer_t *t) {
    tr_kcp_transport_t *tt = t->data;
    int count = 0;
    pc_lib_log(PC_LOG_DEBUG, "kcp__write_check_timeout - start check timeout");

    pc_mutex_lock(&tt->wq_mutex);
    count += kcp__check_queue_timeout(&tt->conn_wait_queue, tt->client);
    count += kcp__check_queue_timeout(&tt->write_wait_queue, tt->client);
    count += kcp__check_queue_timeout(&tt->resp_pending_queue, tt->client);
    pc_mutex_unlock(&tt->wq_mutex);
    if (count && !uv_is_active((uv_handle_t *) t)) {
        uv_timer_start(t, kcp__write_check_timeout, PC_TIMEOUT_CHECK_INTERVAL * 1000, 0);
    }
    pc_lib_log(PC_LOG_DEBUG, "kcp__write_check_timeout - finish to check timeout");
}

static void kcp__send_handshake_ack(tr_kcp_transport_t *tt) {
    uv_buf_t buf;
    buf = pc_pkg_encode(PC_PKG_HANDSHAKE_ACK, NULL, 0);
    pc_lib_log(PC_LOG_INFO, "kcp__send_handshake_ack - send handshake ack");
    pc_assert(buf.base && buf.len);
    int ret = ikcp_send(tt->kcp, buf.base, buf.len);
    if (ret < 0) {
        pc_lib_log(PC_LOG_ERROR, "kcp__send_handshake_ack - kcp send failed");
    }
    pc_lib_free(buf.base);
    uv_async_send(&tt->write_async);
}

static void kcp__on_handshake_resp(tr_kcp_transport_t *tt, const char *data, size_t len) {
    pc_assert(tt->state == TR_KCP_HANDSHAKING);
    pc_JSON *res = NULL;

    if (is_compressed((unsigned char *) data, len)) {
        char *uncompressed_data = NULL;
        size_t uncompressed_len;
        pr_decompress((unsigned char **) &uncompressed_data, &uncompressed_len, (unsigned char *) data, len);
        if (uncompressed_data) {
            pc_lib_log(PC_LOG_INFO, "data: %.*s", uncompressed_len, uncompressed_data);
            res = pc_JSON_Parse(uncompressed_data);
            pc_lib_free(uncompressed_data);
        } else {
            pc_lib_log(PC_LOG_ERROR, "tcp__on_handshake_resp - failed to uncompress handshake data");
        }
    } else {
        pc_lib_log(PC_LOG_INFO, "data: %.*s", len, data);
        res = pc_JSON_Parse(data);
    }

    if (tt->config->conn_timeout != PC_WITHOUT_TIMEOUT) {
        uv_timer_stop(&tt->timer_handshake_timeout);
    }

    pc_lib_log(PC_LOG_INFO, "kcp__on_handshake_resp - kcp get handshake resp");

    if (!res) {
        pc_lib_log(PC_LOG_ERROR, "tcp__on_handshake_resp - handshake resp is not valid json");
        pc_trans_fire_event(tt->client, PC_EV_CONNECT_FAILED, "Handshake Error", NULL);
        return;
    }

    pc_JSON *tmp = pc_JSON_GetObjectItem(res, "code");
    int code = -1;
    if (!tmp || tmp->type != pc_JSON_Number || (code = tmp->valueint) != 200) {
        pc_lib_log(PC_LOG_ERROR, "kcp__on_handshake_resp - handshake fail, code: %d", code);
        pc_trans_fire_event(tt->client, PC_EV_CONNECT_FAILED, "Handshake Error", NULL);
        pc_JSON_Delete(res);
        return;
    }

    pc_JSON *sys = pc_JSON_GetObjectItem(res, "sys");
    if (!sys) {
        pc_lib_log(PC_LOG_ERROR, "kcp__on_handshake_resp - handshake fail, no sys field");
        pc_trans_fire_event(tt->client, PC_EV_CONNECT_FAILED, "Handshake Error", NULL);
        pc_JSON_Delete(res);
        return;
    }

    pc_lib_log(PC_LOG_INFO, "tcp__on_handshake_resp - handshake ok");
    tmp = pc_JSON_GetObjectItem(sys, "heartbeat");
    int heartbeat_interval = -1;
    if (!tmp || tmp->type != pc_JSON_Number) {
        heartbeat_interval = -1;
    } else {
        heartbeat_interval = tmp->valueint;
    }

    if (heartbeat_interval <= 0) {
        tt->hb_interval = -1;
        tt->hb_timeout = -1;
        pc_lib_log(PC_LOG_INFO, "kcp__on_handshake_resp - no heartbeat specified");
    } else {
        tt->hb_interval = heartbeat_interval;
        pc_lib_log(PC_LOG_INFO, "kcp__on_handshake_resp - set heartbeat interval: %d", heartbeat_interval);
        tt->hb_timeout = tt->hb_interval * 2;
    }

    tmp = pc_JSON_GetObjectItem(sys, "serializer");
    if (!tmp || tmp->type != pc_JSON_String) {
        pc_lib_log(PC_LOG_WARN, "kcp__on_handshake_resp - invalid seriaizer field send by server, defaulting to json");
        tt->serializer = pc_lib_strdup("json");
    } else {
        tt->serializer = pc_lib_strdup(tmp->valuestring);
    }

    pc_JSON_Delete(res);
    res = NULL;

    kcp__send_handshake_ack(tt);
    if (tt->hb_interval != -1) {
        pc_lib_log(PC_LOG_INFO, "kcp__on_handshake_resp - start heartbeat");
        uv_timer_start(&tt->timer_heartbeat, kcp__heartbeat_timer_cb, tt->hb_interval * 1000, tt->hb_interval * 1000);
    }

    tt->state = TR_KCP_DONE;
    pc_lib_log(PC_LOG_INFO, "kcp__on_handshake_resp - handshake completely");
    pc_trans_fire_event(tt->client, PC_EV_CONNECTED, NULL, NULL);
}

static void kcp__write_async(uv_async_t *t) {
    tr_kcp_transport_t *tt = t->data;
    if (tt->state == TR_KCP_NOT_CONN) {
        return;
    }
    int need_check = 0;
    tr_kcp_wait_item_t *wi;
    int buf_cnt = 0;
    QUEUE *q;

    pc_mutex_lock(&tt->wq_mutex);
    if (tt->state == TR_KCP_DONE) {
        while (!QUEUE_EMPTY(&tt->conn_wait_queue)) {
            q = QUEUE_HEAD(&tt->conn_wait_queue);
            QUEUE_REMOVE(q);
            QUEUE_INIT(q);

            wi = (tr_kcp_wait_item_t *) QUEUE_DATA(q, tr_kcp_wait_item_t, queue);
            QUEUE_INSERT_TAIL(&tt->write_wait_queue, q);
        }
    } else {
        need_check = !QUEUE_EMPTY(&tt->conn_wait_queue);
    }

    QUEUE_FOREACH(q, &tt->write_wait_queue) {
        wi = (tr_kcp_wait_item_t *) QUEUE_DATA(q, tr_kcp_wait_item_t, queue);
        if (wi->timeout != PC_WITHOUT_TIMEOUT) {
            need_check = 1;
        }
        buf_cnt++;
    }

    if (buf_cnt == 0) {
        pc_mutex_unlock(&tt->wq_mutex);
        if (need_check) {
            pc_lib_log(PC_LOG_DEBUG, "NEED A CHECK");
            if (!uv_is_active((uv_handle_t *) &tt->timer_check_timeout)) {
                pc_lib_log(PC_LOG_DEBUG, "kcp__write_async - start check timeout timer");
                uv_timer_start(&tt->timer_check_timeout, kcp__write_check_timeout, PC_TIMEOUT_CHECK_INTERVAL * 1000, 0);
            }
        }
        return;
    }
    int ret;
    while (!QUEUE_EMPTY(&tt->write_wait_queue)) {
        q = QUEUE_HEAD(&tt->write_wait_queue);
        QUEUE_REMOVE(q);
        QUEUE_INIT(q);

        wi = (tr_kcp_wait_item_t *) QUEUE_DATA(q, tr_kcp_wait_item_t, queue);
        ret = ikcp_send(tt->kcp, wi->buf.base, wi->buf.len);
        if (ret) {
            // failed
            pc_lib_log(PC_LOG_ERROR, "kcp__write_async - kcp write error: %d", ret);
            if (wi->type == TR_KCP_WI_TYPE_NOTIFY) {
                pc_error_t err = pc__error_kcp(ret);
                pc_trans_sent(tt->client, wi->seq_num, &err);
            }
            if (wi->type == TR_KCP_WI_TYPE_RESP) {
                pc_error_t err = pc__error_kcp(ret);
                pc_buf_t empty_buf = {0};
                pc_trans_resp(tt->client, wi->req_id, &empty_buf, &err);
            }
            pc_lib_free(wi->buf.base);
            pc_lib_free(wi);
        } else {
            // send success
            pc_lib_log(PC_LOG_DEBUG, "kcp__write_async - kcp write success");
            if (wi->type == TR_KCP_WI_TYPE_NOTIFY) {
                // notify has no response
                pc_trans_sent(tt->client, wi->seq_num, NULL);
                pc_lib_free(wi->buf.base);
                pc_lib_free(wi);
            } else {
                // waiting for response
                QUEUE_INSERT_TAIL(&tt->resp_pending_queue, q);
            }
        }
    }

    pc_mutex_unlock(&tt->wq_mutex);

    if (need_check) {
        pc_lib_log(PC_LOG_DEBUG, "NEED A CHECK");
        if (!uv_is_active((uv_handle_t *) &tt->timer_check_timeout)) {
            pc_lib_log(PC_LOG_DEBUG, "kcp__write_async - start check timeout timer");
            uv_timer_start(&tt->timer_check_timeout, kcp__write_check_timeout, PC_TIMEOUT_CHECK_INTERVAL * 1000, 0);
        }
    }
}

static void walk_cb(uv_handle_t *handle, void *arg) {
    if (!uv_is_closing(handle)) {
        uv_close(handle, NULL);
    }
}

static void kcp__disconnect_async(uv_async_t *t) {
    tr_kcp_transport_t *tt = t->data;
    tt->reset_fn(tt);
    tt->reconn_times = 0;
    pc_lib_log(PC_LOG_DEBUG, "kcp__disconnect_async - send disconnect event");
    pc_trans_fire_event(tt->client, PC_EV_DISCONNECT, NULL, NULL);
}

static void kcp__clean_async(uv_async_t *t) {
    tr_kcp_transport_t *tt = t->data;
    tt->reset_fn(tt);
    if (tt->addr) {
        pc_lib_free(tt->addr);
        tt->addr = NULL;
    }
    tt->reconn_times = 0;
    uv_stop(&tt->loop);
    uv_walk(&tt->loop, walk_cb, NULL);
//    uv_run(&tt->loop, UV_RUN_DEFAULT);
}

static void tr_kcp_on_pkg_handler(pc_pkg_type type, const char *data, size_t len, void *ex_data) {
    tr_kcp_transport_t *tt = ex_data;
    pc_assert(type == PC_PKG_HANDSHAKE || type == PC_PKG_HEARBEAT
              || type == PC_PKG_DATA || type == PC_PKG_KICK);
    pc_lib_log(PC_LOG_DEBUG, "tr_kcp_on_pkg_handler - updating last server packet time");
    tt->last_server_packet_time = uv_now(&tt->loop);

    switch (type) {
        case PC_PKG_HANDSHAKE:
            kcp__on_handshake_resp(tt, data, len);
            break;
        case PC_PKG_HEARBEAT:
            kcp__on_heartbeat(tt);
            break;
        case PC_PKG_DATA:
            kcp__on_data_received(tt, data, len);
            break;
        case PC_PKG_KICK:
            kcp__on_kick_received(tt);
            break;
        default:
            break;
    }
}

static void kcp__reset_wi(pc_client_t *client, tr_kcp_wait_item_t *wi) {
    if (wi->type == TR_KCP_WI_TYPE_RESP) {
        pc_lib_log(PC_LOG_DEBUG, "kcp__reset_wi - reset request, req_id: %u", wi->req_id);
        pc_error_t err = pc__error_reset();
        pc_buf_t empty_buf = {0};
        pc_trans_resp(client, wi->req_id, &empty_buf, &err);
    }
    if (wi->type == TR_KCP_WI_TYPE_NOTIFY) {
        pc_lib_log(PC_LOG_DEBUG, "kcp__reset_wi - reset notify, seq_num: %u", wi->seq_num);
        pc_error_t err = pc__error_reset();
        pc_trans_sent(client, wi->seq_num, &err);
    }

    pc_lib_free(wi->buf.base);
    pc_lib_free(wi);
}

static void kcp__reset(tr_kcp_transport_t *tt) {
    tr_kcp_wait_item_t *wi;
    QUEUE *q;

    pc_pkg_parser_reset(&tt->pkg_parser);
    uv_timer_stop(&tt->timer_heartbeat);
    uv_timer_stop(&tt->timer_reconn_delay);
    uv_timer_stop(&tt->timer_check_timeout);
    uv_timer_stop(&tt->timer_update);
    pc_lib_free((char *) tt->serializer);
    tt->serializer = NULL;
    uv_udp_recv_stop(&tt->send_socket);
    if (tt->state != TR_KCP_NOT_CONN && !uv_is_closing((const uv_handle_t *) &tt->send_socket)) {
        uv_close((uv_handle_t *) &tt->send_socket, NULL);
    }
    ikcp_release(tt->kcp);
    tt->kcp = NULL;

    if (tt->host) {
        pc_lib_free((void *) tt->host);
        tt->host = NULL;
    }

    pc_mutex_lock(&tt->wq_mutex);
    if (!QUEUE_EMPTY(&tt->conn_wait_queue)) {
        QUEUE_ADD(&tt->write_wait_queue, &tt->conn_wait_queue);
        QUEUE_INIT(&tt->conn_wait_queue);
    }
    while (!QUEUE_EMPTY(&tt->write_wait_queue)) {
        q = QUEUE_HEAD(&tt->write_wait_queue);
        QUEUE_REMOVE(q);
        QUEUE_INIT(q);
        wi = (tr_kcp_wait_item_t *) QUEUE_DATA(q, tr_kcp_wait_item_t, queue);
        kcp__reset_wi(tt->client, wi);
    }
    while (!QUEUE_EMPTY(&tt->resp_pending_queue)) {
        q = QUEUE_HEAD(&tt->resp_pending_queue);
        QUEUE_REMOVE(q);
        QUEUE_INIT(q);
        wi = (tr_kcp_wait_item_t *) QUEUE_DATA(q, tr_kcp_wait_item_t, queue);
        kcp__reset_wi(tt->client, wi);
    }
    pc_mutex_unlock(&tt->wq_mutex);

    tt->state = TR_KCP_NOT_CONN;
}

static void kcp__reconn_delay_timer_cb(uv_timer_t *t) {
    tr_kcp_transport_t *tt = t->data;
    uv_timer_stop(t);
    uv_async_send(&tt->conn_async);
}

static void kcp__reconn(tr_kcp_transport_t *tt) {
    tt->reset_fn(tt);
    tt->state = TR_KCP_CONNECTING;
    const pc_client_config_t *config = tt->config;
    if (!config->enable_reconn) {
        pc_lib_log(PC_LOG_WARN, "tcp__reconn - trans want to reconnect, but is disabled");
        tt->reconn_times = 0;
        tt->state = TR_KCP_NOT_CONN;
        return;
    }

    if (tt->reconn_times == 0) {
        pc_trans_fire_event(tt->client, PC_EV_RECONNECT_STARTED, "kcp__reconn - Start reconn", NULL);
    } else {
        pc_mutex_lock(&tt->client->state_mutex);
        tt->client->state = PC_ST_CONNECTING;
        pc_mutex_unlock(&tt->client->state_mutex);
    }
    tt->reconn_times++;
    if (config->reconn_max_retry != PC_ALWAYS_RETRY && config->reconn_max_retry < tt->reconn_times) {
        pc_lib_log(PC_LOG_WARN, "kcp__reconn - reconn times exceeded");
        tt->reconn_times = 0;
        tt->state = TR_KCP_NOT_CONN;
        return;
    }

    int timeout = config->reconn_delay * tt->reconn_times;
    if (timeout > config->reconn_delay_max) {
        timeout = config->reconn_delay_max;
    }
    timeout = (rand() % timeout) + timeout / 2;
    pc_lib_log(PC_LOG_DEBUG, "kcp__reconn - reconnect, delay: %d", timeout);

    uv_timer_start(&tt->timer_reconn_delay, kcp__reconn_delay_timer_cb, timeout * 1000, 0);
}

static void kcp__send_handshake(tr_kcp_transport_t *tt) {
    uv_buf_t buf;
    pc_JSON *sys;
    pc_JSON *body;

    body = pc_JSON_CreateObject();
    sys = pc_JSON_CreateObject();

    pc_JSON_AddItemToObject(sys, "platform", pc_JSON_CreateString(pc_lib_platform_str));
    pc_JSON_AddItemToObject(sys, "libVersion", pc_JSON_CreateString(pc_lib_version_str()));
    pc_JSON_AddItemToObject(sys, "clientBuildNumber", pc_JSON_CreateString(pc_lib_client_build_number_str));
    pc_JSON_AddItemToObject(sys, "clientVersion", pc_JSON_CreateString(pc_lib_client_version_str));

    pc_JSON_AddItemToObject(body, "sys", sys);
    if (tt->handshake_opts) {
        pc_JSON_AddItemReferenceToObject(body, "user", tt->handshake_opts);
    }

    char *data = pc_JSON_PrintUnformatted(body);
    pc_lib_log(PC_LOG_DEBUG, "kcp__send_handshake -- sending handshake: %s", data);

    buf = pc_pkg_encode(PC_PKG_HANDSHAKE, data, strlen(data));
    pc_lib_free(data);
    pc_JSON_Delete(body);

    int ret = ikcp_send(tt->kcp, buf.base, buf.len);
    pc_assert(!ret);
    pc_lib_free(buf.base);
}

static void kcp__update(uv_timer_t *handle) {
    tr_kcp_transport_t *tt = handle->data;
    ikcp_update(tt->kcp, time(NULL) * 1000);
}

static void kcp__handshake_timer_cb(uv_timer_t *t) {
    tr_kcp_transport_t *tt = t->data;
    pc_lib_log(PC_LOG_ERROR, "kcp__handshake_timer_cb - handshake timeout, will reconn");
    pc_trans_fire_event(tt->client, PC_EV_CONNECT_ERROR, "Connect Timeout", NULL);
    tt->reconn_fn(tt);
}

static void kcp__log(const char *log, ikcpcb *kcp, void *user) {
    pc_lib_log(PC_LOG_DEBUG, log);
}

static void kcp__config_kcp(ikcpcb *kcp) {
    kcp->writelog = kcp__log;
    ikcp_nodelay(kcp, 1, 10, 2, 1);
    ikcp_wndsize(kcp, 64, 64);
}

static void kcp__conn_async(uv_async_t *handle) {
    tr_kcp_transport_t *tt = handle->data;

    pc_assert(handle == &tt->conn_async);

    // kcp
    ikcpcb *kcp = ikcp_create(KCP_CONV, tt);
    kcp->output = udp_output;
    kcp__config_kcp(kcp);
    tt->kcp = kcp;

    memset(&tt->send_socket, 0, sizeof(uv_udp_t));
    uv_udp_init(&tt->loop, &tt->send_socket);
    tt->send_socket.data = tt;

    struct addrinfo hints;
    struct addrinfo *ainfo;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_ADDRCONFIG;
    hints.ai_socktype = SOCK_DGRAM;

    int ret;
    ret = getaddrinfo(tt->host, NULL, &hints, &ainfo);
    if (ret) {
        pc_lib_log(PC_LOG_ERROR, "kcp__conn_async - dns resolve error, state: %s",
                   pc_client_state_str(tt->client->state));
        pc_lib_log(PC_LOG_ERROR, "kcp__conn_async - dns resolve error: %s, will reconn", tt->host);
        pc_trans_fire_event(tt->client, PC_EV_CONNECT_ERROR, "DNS Resolve Error", NULL);
        tt->reconn_fn(tt);
        return;
    }

    struct addrinfo *rp;
    struct sockaddr_in *addr4 = NULL;
    struct sockaddr_in6 *addr6 = NULL;
    for (rp = ainfo; rp; rp = rp->ai_next) {
        if (rp->ai_family == AF_INET) {
            addr4 = (struct sockaddr_in *) rp->ai_addr;
            addr4->sin_port = htons(tt->port);
            break;
        } else if (rp->ai_family == AF_INET6) {
            addr6 = (struct sockaddr_in6 *) rp->ai_addr;
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

    if (tt->addr) {
        pc_lib_free(tt->addr);
        tt->addr = NULL;
    }

    tt->addr = pc_lib_malloc(sizeof(struct sockaddr_storage));
    memset(tt->addr, 0, sizeof(struct sockaddr_storage));

    if (addr4) {
        memcpy(tt->addr, addr4, sizeof(struct sockaddr_in));
        // udp bind
        struct sockaddr_in broadcast_addr;
        uv_ip4_addr("0.0.0.0", 0, &broadcast_addr);
        ret = uv_udp_bind(&tt->send_socket, (const struct sockaddr *) &broadcast_addr, 0);
    } else {
        memcpy(tt->addr, addr6, sizeof(struct sockaddr_in6));
        // udp bind
        struct sockaddr_in6 broadcast_addr;
        uv_ip6_addr("0.0.0.0", 0, &broadcast_addr);
        ret = uv_udp_bind(&tt->send_socket, (const struct sockaddr *) &broadcast_addr, 0);
    }
    freeaddrinfo(ainfo);

    if (ret) {
        pc_lib_log(PC_LOG_ERROR, "kcp__conn - bind addr failed");
        pc_trans_fire_event(tt->client, PC_EV_CONNECT_ERROR, "Bind udp error", NULL);
        tt->reconn_fn(tt);
        return;
    }

    ret = uv_udp_recv_start(&tt->send_socket, uv_udp_alloc_buff, uv_udp_on_read);
    if (ret) {
        pc_lib_log(PC_LOG_ERROR, "kcp__conn - start read from udp error");
        pc_trans_fire_event(tt->client, PC_EV_CONNECT_ERROR, "Udp receive error", NULL);
        tt->reconn_fn(tt);
        return;
    }
    uv_timer_start(&tt->timer_update, kcp__update, 0, 10);

    tt->last_server_packet_time = uv_now(&tt->loop);
    tt->state = TR_KCP_HANDSHAKING;
    kcp__send_handshake(tt);

    if (tt->config->conn_timeout != PC_WITHOUT_TIMEOUT) {
        uv_timer_start(&tt->timer_handshake_timeout, kcp__handshake_timer_cb, tt->config->conn_timeout * 1000, 0);
    }
}

int tr_kcp_init(pc_transport_t *trans, pc_client_t *client) {
    tr_kcp_transport_t *k_tr = (tr_kcp_transport_t *) trans;
    pc_assert(k_tr && client);


    // libuv
    if (uv_loop_init(&k_tr->loop)) {
        pc_lib_log(PC_LOG_ERROR, "kcp_init - init uv loop error");
        return PC_RC_ERROR;
    }

    int ret;
    k_tr->reconn_times = 0;
    k_tr->loop.data = k_tr;

    pc_mutex_init(&k_tr->wq_mutex);

    ret = uv_timer_init(&k_tr->loop, &k_tr->timer_update);
    pc_assert(!ret);
    k_tr->timer_update.data = k_tr;

    ret = uv_timer_init(&k_tr->loop, &k_tr->timer_heartbeat);
    pc_assert(!ret);
    k_tr->timer_heartbeat.data = k_tr;

    ret = uv_timer_init(&k_tr->loop, &k_tr->timer_check_timeout);
    pc_assert(!ret);
    k_tr->timer_check_timeout.data = k_tr;

    ret = uv_timer_init(&k_tr->loop, &k_tr->timer_handshake_timeout);
    pc_assert(!ret);
    k_tr->timer_handshake_timeout.data = k_tr;

    ret = uv_timer_init(&k_tr->loop, &k_tr->timer_reconn_delay);
    pc_assert(!ret);
    k_tr->timer_reconn_delay.data = k_tr;

    ret = uv_async_init(&k_tr->loop, &k_tr->conn_async, kcp__conn_async);
    pc_assert(!ret);
    k_tr->conn_async.data = k_tr;

    ret = uv_async_init(&k_tr->loop, &k_tr->receive_async, kcp__receive_async);
    pc_assert(!ret);
    k_tr->receive_async.data = k_tr;

    ret = uv_async_init(&k_tr->loop, &k_tr->write_async, kcp__write_async);
    pc_assert(!ret);
    k_tr->write_async.data = k_tr;

    ret = uv_async_init(&k_tr->loop, &k_tr->clean_async, kcp__clean_async);
    pc_assert(!ret);
    k_tr->clean_async.data = k_tr;

    ret = uv_async_init(&k_tr->loop, &k_tr->disconnect_async, kcp__disconnect_async);
    pc_assert(!ret);
    k_tr->disconnect_async.data = k_tr;

    QUEUE_INIT(&k_tr->write_wait_queue);
    QUEUE_INIT(&k_tr->conn_wait_queue);
    QUEUE_INIT(&k_tr->resp_pending_queue);

    k_tr->host = NULL;
    k_tr->port = 0;

    k_tr->client = client;
    k_tr->config = pc_client_config(client);
    k_tr->serializer = NULL;
    k_tr->state = TR_KCP_NOT_CONN;

    k_tr->last_server_packet_time = uv_now(&k_tr->loop);

    pc_pkg_parser_init(&k_tr->pkg_parser, tr_kcp_on_pkg_handler, k_tr);

    uv_thread_create(&k_tr->worker, tr_kcp_thread_fn, &k_tr->loop);

    return PC_RC_OK;
}

int tr_kcp_connect(pc_transport_t *trans, const char *host, int port, const char *handshake_opts) {
    pc_JSON *handshake;
    tr_kcp_transport_t *tt = (tr_kcp_transport_t *) trans;
    pc_assert(tt);
    pc_assert(host);

    if (tt->handshake_opts) {
        pc_JSON_Delete(tt->handshake_opts);
        tt->handshake_opts = NULL;
    }

    if (handshake_opts) {
        handshake = pc_JSON_Parse(handshake_opts);
        if (!handshake) {
            pc_lib_log(PC_LOG_ERROR, "tr_kcp_connect - handshake_opts is invalid json string");
            return PC_RC_INVALID_JSON;
        }
        tt->handshake_opts = handshake;
    }

    if (tt->host) {
        pc_lib_free((char *) tt->host);
        tt->host = NULL;
    }

    tt->host = pc_lib_strdup(host);
    tt->port = port;


    uv_async_send(&tt->conn_async);

    return PC_RC_OK;
}

int tr_kcp_send(pc_transport_t *trans, const char *route, unsigned int seq_num,
                pc_buf_t buf, unsigned int req_id, int timeout) {
    pc_lib_log(PC_LOG_DEBUG, "kcp_send - Entered");
    tr_kcp_transport_t *tt = (tr_kcp_transport_t *) trans;
    if (tt->state == TR_KCP_NOT_CONN) {
        return PC_RC_INVALID_STATE;
    }

    pc_assert(trans && route && req_id != PC_INVALID_REQ_ID);

    pc_msg_t m;
    m.id = req_id;
    m.buf = buf;
    m.route = route;
    uv_buf_t uv_buf = pr_kcp_default_msg_encoder(tt, &m);

    pc_lib_log(PC_LOG_DEBUG, "tr_kcp_send - encoded msg length = %lu", uv_buf.len);

    if (uv_buf.len == (unsigned int) -1) {
        pc_assert(uv_buf.base == NULL && "uv_buf should be empty here");
        pc_lib_log(PC_LOG_ERROR, "tr_kcp_send - encode msg failed, route: %s", route);
        return PC_RC_ERROR;
    }

    uv_buf_t pkg_buf = pc_pkg_encode(PC_PKG_DATA, uv_buf.base, uv_buf.len);

    pc_lib_log(PC_LOG_DEBUG, "tr_uv_tcp_send - encoded pkg length = %lu", pkg_buf.len);

    pc_lib_free(uv_buf.base);

    if (pkg_buf.len == (unsigned int) -1) {
        pc_lib_log(PC_LOG_ERROR, "tr_uv_tcp_send - encode package failed");
        return PC_RC_ERROR;
    }

    tr_kcp_wait_item_t *item = pc_lib_malloc(sizeof(tr_kcp_wait_item_t));
    memset(item, 0, sizeof(tr_kcp_wait_item_t));
    pc_mutex_lock(&tt->wq_mutex);
    QUEUE_INIT(&item->queue);
    if (tt->state == TR_KCP_DONE) {
        QUEUE_INSERT_TAIL(&tt->write_wait_queue, &item->queue);
    } else {
        QUEUE_INSERT_TAIL(&tt->conn_wait_queue, &item->queue);
    }
    pc_mutex_unlock(&tt->wq_mutex);

    if (PC_NOTIFY_PUSH_REQ_ID == req_id) {
        item->type = TR_KCP_WI_TYPE_NOTIFY;
    } else {
        item->type = TR_KCP_WI_TYPE_RESP;
    }
    item->buf = pkg_buf;
    item->seq_num = seq_num;
    item->req_id = req_id;
    item->timestamp = time(NULL);
    item->timeout = timeout;
    if (tt->state == TR_KCP_CONNECTING || tt->state == TR_KCP_HANDSHAKING || tt->state == TR_KCP_DONE) {
        uv_async_send(&tt->write_async);
    }

//    ikcp_send(tt->kcp, pkg_buf.base, pkg_buf.len);
//    pc_lib_free(pkg_buf.base);

    return PC_RC_OK;
}

int tr_kcp_disconnect(pc_transport_t *trans) {
    tr_kcp_transport_t *tt = (tr_kcp_transport_t *) trans;
    uv_async_send(&tt->disconnect_async);
    return PC_RC_OK;
}

int tr_kcp_cleanup(pc_transport_t *trans) {
    tr_kcp_transport_t *tt = (tr_kcp_transport_t *) trans;

    uv_async_send(&tt->clean_async);
    if (uv_thread_join(&tt->worker)) {
        pc_lib_log(PC_LOG_ERROR, "tr_kcp_cleanup - join uv thread error");
        return PC_RC_ERROR;
    }

    pc_mutex_destroy(&tt->wq_mutex);
    if (tt->serializer) {
        pc_lib_free((char *) tt->serializer);
        tt->serializer = NULL;
    }

    // After the thread exits, run pending close callbacks to avoid
    // memory leaks.
    uv_run(&tt->loop, UV_RUN_DEFAULT);
    if (uv_loop_close(&tt->loop) == UV_EBUSY) {
        pc_lib_log(PC_LOG_ERROR, "tr_kcp_cleanup - failed to close loop, it is busy");
        return PC_RC_ERROR;
    }
    return PC_RC_OK;
}

void *tr_kcp_internal_data(pc_transport_t* trans)
{
    tr_kcp_transport_t *tt = (tr_kcp_transport_t *) trans;
    return &tt->loop;
}

int tr_kcp_quality(pc_transport_t *trans) {
    return 11;
}

static pc_transport_t *kcp_trans_create(pc_transport_plugin_t *plugin) {
    size_t len = sizeof(tr_kcp_transport_t);
    tr_kcp_transport_t *tt = (tr_kcp_transport_t *) pc_lib_malloc(len);
    memset(tt, 0, len);

    tt->base.init = tr_kcp_init;
    tt->base.connect = tr_kcp_connect;
    tt->base.send = tr_kcp_send;
    tt->base.serializer = tr_kcp_serializer;
    tt->base.disconnect = tr_kcp_disconnect;
    tt->base.cleanup = tr_kcp_cleanup;
    tt->base.quality = tr_kcp_quality;
    tt->base.plugin = tr_kcp_plugin;
    tt->base.internal_data = tr_kcp_internal_data;
    tt->reconn_fn = kcp__reconn;
    tt->reset_fn = kcp__reset;

    return (pc_transport_t *) tt;
}

static void kcp_trans_release(pc_transport_plugin_t *plugin, pc_transport_t *trans) {
    tr_kcp_transport_t *tt = (tr_kcp_transport_t *) trans;

    pc_lib_free(trans);
}

static void kcp_trans_on_register(pc_transport_plugin_t *plugin) {
}

static pc_transport_plugin_t instance =
        {
                kcp_trans_create,
                kcp_trans_release,
                kcp_trans_on_register,
                NULL,
                PC_TR_NAME_KCP
        };

pc_transport_plugin_t *pc_tr_kcp_trans_plugin() {
    return &instance;
}


pc_transport_plugin_t *tr_kcp_plugin(pc_transport_t *trans) {
    return &instance;
}

const char *tr_kcp_serializer(pc_transport_t *trans) {
    tr_kcp_transport_t *tt = (tr_kcp_transport_t *) trans;
    const char *serializer = NULL;
    if (tt->serializer) {
        serializer = pc_lib_strdup(tt->serializer);
    }
    return serializer;
}

uv_buf_t pr_kcp_default_msg_encoder(tr_kcp_transport_t *tt, const pc_msg_t *msg) {
    pc_buf_t pb = pc_default_msg_encode(NULL, msg, !tt->disable_compression);
    uv_buf_t ub;
    ub.base = (char *) pb.base;
    ub.len = pb.len;
    return ub;
}

pc_msg_t pr_kcp_default_msg_decoder(tr_kcp_transport_t *tt, const uv_buf_t *buf) {
    pc_buf_t pb;
    pb.base = (uint8_t *) buf->base;
    pb.len = buf->len;
    return pc_default_msg_decode(NULL, &pb);
}
