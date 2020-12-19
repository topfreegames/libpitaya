//
// Created by lichong on 2020/12/11.
//

#include "tr_kcp.h"
#include <pc_assert.h>
#include "ikcp.h"
#include <uv.h>
#include <pc_lib.h>
#include "pc_JSON.h"
#include "pc_pitaya_i.h"
#include "pr_pkg.h"
#include "pr_gzip.h"

#define KCP_CONV 777

typedef struct kcp_transport_s kcp_transport_t;

struct kcp_transport_s {
    pc_transport_t base;
    pc_client_t *client;

    const char *host;
    int port;
    struct sockaddr_in *addr4;
    struct sockaddr_in6 *addr6;

    pc_pkg_parser_t pkg_parser;
    uint64_t last_server_packet_time;
    int hb_interval;
    int hb_timeout;
    const char *serializer;

    // kcp
    ikcpcb *kcp;

    // libuv
    uv_loop_t loop;
    uv_udp_t send_socket;
    uv_timer_t timer_update;
    uv_timer_t timer_heartbeat;
    uv_thread_t worker;
    pc_JSON *handshake_opts;

    int is_connecting;
    tr_kcp_state_t state;

    // async
    uv_async_t conn_async;
    uv_async_t receive_async;

    void (*reconn_fn)(kcp_transport_t *tt);
};

static void udp_on_send(uv_udp_send_t *req, int status) {
    free(req);
}

static int udp_output(const char *buf, int len, ikcpcb *kcp, void *p) {
    kcp_transport_t *trans = p;
    uv_udp_send_t *send_req = malloc(sizeof(uv_udp_send_t));
    memset(send_req, 0, sizeof(uv_udp_send_t));
    char *data = malloc(len);
    memcpy(data, buf, len);
    uv_buf_t msg = uv_buf_init(data, len);
    send_req->data = trans;
    struct sockaddr_in send_addr;
    uv_ip4_addr(trans->host, trans->port, &send_addr);
    int ret = uv_udp_send(send_req, &trans->send_socket, &msg, 1, (const struct sockaddr *) &send_addr, udp_on_send);
    free(data);
    return ret;
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

static void
uv_udp_on_read(uv_udp_t *req, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags) {
    if (nread <= 0) {
        return;
    }
    kcp_transport_t *tt = req->data;
    ikcp_input(tt->kcp, buf->base, nread);
    uv_async_send(&tt->receive_async);
}

static void kcp__receive_async(uv_async_t *handle) {
    kcp_transport_t *tt = handle->data;
    int size = ikcp_peeksize(tt->kcp);
    char *buf = pc_lib_malloc(size);
    int ret = ikcp_recv(tt->kcp, buf, size);
    if (ret < 0) {
        return;
    }
}


static void kcp__send_handshake_ack(kcp_transport_t *tt) {
    uv_buf_t buf;
    buf = pc_pkg_encode(PC_PKG_HANDSHAKE_ACK, NULL, 0);
    pc_lib_log(PC_LOG_INFO, "kcp__send_handshake_ack - send handshake ack");
    pc_assert(buf.base && buf.len);
    int ret = ikcp_send(tt->kcp, buf.base, buf.len);
    if (ret < 0) {
        pc_lib_log(PC_LOG_ERROR, "kcp__send_handshake_ack - kcp send failed");
    }
}

static void kcp__on_handshake_resp(kcp_transport_t *tt, const char *data, size_t len) {
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

    // todo
    tmp = pc_JSON_GetObjectItem(sys, "useDict");
    if (!tmp || tmp->type == pc_JSON_False) {

    }

    pc_JSON_Delete(res);
    res = NULL;

    kcp__send_handshake_ack(tt);
    // todo: add heartbeat

    tt->state = TR_KCP_DONE;
    pc_lib_log(PC_LOG_INFO, "kcp__on_handshake_resp - handshake completely");
    pc_trans_fire_event(tt->client, PC_EV_CONNECTED, NULL, NULL);
}

static void tr_kcp_on_pkg_handler(pc_pkg_type type, const char *data, size_t len, void *ex_data) {
    kcp_transport_t *tt = ex_data;
    pc_assert(type == PC_PKG_HANDSHAKE || type == PC_PKG_HEARBEAT
              || type == PC_PKG_DATA || type == PC_PKG_KICK);
    pc_lib_log(PC_LOG_DEBUG, "tr_kcp_on_pkg_handler - updating last server packet time");
    tt->last_server_packet_time = uv_now(&tt->loop);

    switch (type) {
        case PC_PKG_HANDSHAKE:
            kcp__on_handshake_resp(tt, data, len);
            break;
        case PC_PKG_HEARBEAT:
            break;
        case PC_PKG_DATA:
            break;
        case PC_PKG_KICK:
            break;
        default:
            break;
    }
}

static void kcp__reconn(kcp_transport_t *tt) {

}

static void kcp__send_handshake(kcp_transport_t *tt) {
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

    ikcp_send(tt->kcp, buf.base, buf.len);
    pc_lib_free(buf.base);
}

static void kcp__conn_async(uv_async_t *handle) {
    kcp_transport_t *tt = handle->data;

    pc_assert(handle == &tt->conn_async);
    if (tt->is_connecting) {
        return;
    }

    struct addrinfo hints;
    struct addrinfo *ainfo;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_ADDRCONFIG;
    hints.ai_socktype = SOCK_STREAM;

    uv_udp_init(&tt->loop, &tt->send_socket);
    tt->send_socket.data = tt;
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

    if (tt->addr4) {
        pc_lib_free(tt->addr4);
        tt->addr4 = NULL;
    }
    if (tt->addr6) {
        pc_lib_free(tt->addr6);
        tt->addr6 = NULL;
    }

    if (addr4) {
        tt->addr4 = pc_lib_malloc(sizeof(struct sockaddr_in));
        memcpy(tt->addr4, addr4, sizeof(struct sockaddr_in));
    } else {
        tt->addr6 = pc_lib_malloc(sizeof(struct sockaddr_in6));
        memcpy(tt->addr6, addr6, sizeof(struct sockaddr_in6));
    }

    struct sockaddr_in broadcast_addr;
    uv_ip4_addr("0.0.0.0", 0, &broadcast_addr);
    uv_udp_bind(&tt->send_socket, (const struct sockaddr *) &broadcast_addr, 0);
    uv_udp_set_broadcast(&tt->send_socket, 1);
    tt->is_connecting = TRUE;

    ret = uv_udp_recv_start(&tt->send_socket, uv_udp_alloc_buff, uv_udp_on_read);
    if (ret) {
        pc_lib_log(PC_LOG_ERROR, "kcp__conn_async_cb - start read from udp error");
        tt->reconn_fn(tt);
        return;
    }

    tt->state = TR_KCP_HANDSHAKING;
    kcp__send_handshake(tt);
}

int tr_kcp_init(pc_transport_t *trans, pc_client_t *client) {
    kcp_transport_t *k_tr = (kcp_transport_t *) trans;
    pc_assert(k_tr && client);


    // libuv
    if (uv_loop_init(&k_tr->loop)) {
        pc_lib_log(PC_LOG_ERROR, "kcp_init - init uv loop error");
        return PC_RC_ERROR;
    }

    // kcp
    ikcpcb *kcp = ikcp_create(KCP_CONV, k_tr);
    kcp->output = udp_output;
    k_tr->kcp = kcp;

    int ret;
    k_tr->loop.data = k_tr;
    ret = uv_udp_init(&k_tr->loop, &k_tr->send_socket);
    pc_assert(!ret);
    k_tr->send_socket.data = k_tr;

    ret = uv_timer_init(&k_tr->loop, &k_tr->timer_update);
    pc_assert(!ret);
    k_tr->timer_update.data = k_tr;

    ret = uv_timer_init(&k_tr->loop, &k_tr->timer_heartbeat);
    pc_assert(!ret);
    k_tr->timer_heartbeat.data = k_tr;

    ret = uv_async_init(&k_tr->loop, &k_tr->conn_async, kcp__conn_async);
    pc_assert(!ret);
    k_tr->conn_async.data = k_tr;

    ret = uv_async_init(&k_tr->loop, &k_tr->receive_async, kcp__receive_async);
    pc_assert(!ret);
    k_tr->receive_async.data = k_tr;

    k_tr->host = NULL;
    k_tr->port = 0;

    k_tr->client = client;
    k_tr->state = TR_KCP_NOT_CONN;

    pc_pkg_parser_init(&k_tr->pkg_parser, tr_kcp_on_pkg_handler, k_tr);

    uv_thread_create(&k_tr->worker, tr_kcp_thread_fn, &k_tr->loop);

    return PC_RC_OK;
}

int tr_kcp_connect(pc_transport_t *trans, const char *host, int port, const char *handshake_opts) {
    pc_JSON *handshake;
    kcp_transport_t *tt = (kcp_transport_t *) trans;
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
    return 0;
}


static pc_transport_t *kcp_trans_create(pc_transport_plugin_t *plugin) {
    size_t len = sizeof(kcp_transport_t);
    kcp_transport_t *tt = (kcp_transport_t *) pc_lib_malloc(len);
    memset(tt, 0, len);

    tt->base.init = tr_kcp_init;
    tt->base.connect = tr_kcp_connect;
    tt->base.send = tr_kcp_send;
    tt->base.serializer = tr_kcp_serializer;
    tt->reconn_fn = kcp__reconn;

    return (pc_transport_t *) tt;
}

static void kcp_trans_release(pc_transport_plugin_t *plugin, pc_transport_t *trans) {
    kcp_transport_t *tt = (kcp_transport_t *) trans;

    if (tt->kcp) {
        ikcp_release(tt->kcp);
        tt->kcp = NULL;
    }
    if (tt->addr4) {
        pc_lib_free(tt->addr4);
        tt->addr4 = NULL;
    }
    if (tt->addr6) {
        pc_lib_free(tt->addr6);
        tt->addr6 = NULL;
    }

    pc_lib_free(trans);
}

static pc_transport_plugin_t instance =
        {
                kcp_trans_create,
                kcp_trans_release,
                NULL,
                NULL,
                PC_TR_NAME_KCP
        };

pc_transport_plugin_t *pc_tr_kcp_trans_plugin() {
    return &instance;
}