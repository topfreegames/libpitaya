//
// Created by lichong on 2020/12/11.
//

#ifndef PITAYA_TR_KCP_H
#define PITAYA_TR_KCP_H

#include <pitaya_trans.h>
#include <uv.h>
#include <queue.h>
#include "ikcp.h"
#include "pr_msg.h"
#include "pc_mutex.h"

pc_transport_plugin_t *pc_tr_kcp_trans_plugin();

int tr_kcp_init(pc_transport_t *trans, pc_client_t *client);

int tr_kcp_connect(pc_transport_t *trans, const char *host, int port, const char *handshake_opts);

int tr_kcp_send(pc_transport_t *trans, const char *route, unsigned int seq_num,
                pc_buf_t buf, unsigned int req_id, int timeout);

int tr_kcp_disconnect(pc_transport_t *trans);

int tr_kcp_cleanup(pc_transport_t *trans);

const char *tr_kcp_serializer(pc_transport_t *trans);

void *tr_kcp_internal_data(pc_transport_t *trans);

int tr_kcp_quality(pc_transport_t *trans);

pc_transport_plugin_t *tr_kcp_plugin(pc_transport_t *trans);

typedef enum {
    TR_KCP_WI_TYPE_NONE,
    TR_KCP_WI_TYPE_NOTIFY,
    TR_KCP_WI_TYPE_RESP,
    TR_KCP_WI_TYPE_INTERNAL
} tr_kcp_wi_type_t;

typedef struct tr_kcp_wait_item_s {
    QUEUE queue;
    tr_kcp_wi_type_t type;

    unsigned int seq_num;
    unsigned int req_id;

    uv_buf_t buf;
    time_t timestamp;
    int timeout;
} tr_kcp_wait_item_t;

typedef enum {
    TR_KCP_NOT_CONN,
    TR_KCP_CONNECTING,
    TR_KCP_HANDSHAKING,
    TR_KCP_DONE
} tr_kcp_state_t;

typedef struct tr_kcp_transport_s tr_kcp_transport_t;

struct tr_kcp_transport_s {
    pc_transport_t base;
    pc_client_t *client;
    const pc_client_config_t *config;


    const char *host;
    int port;
    struct sockaddr_in *addr4;
    struct sockaddr_in6 *addr6;

    pc_pkg_parser_t pkg_parser;
    uint64_t last_server_packet_time;
    int hb_interval;
    int hb_timeout;
    int reconn_times;
    const char *serializer;
    int disable_compression;
    QUEUE write_wait_queue;
    QUEUE conn_wait_queue;
    QUEUE resp_pending_queue;

    // kcp
    ikcpcb *kcp;

    // libuv
    uv_loop_t loop;
    uv_udp_t send_socket;
    uv_timer_t timer_update;
    uv_timer_t timer_heartbeat;
    uv_timer_t timer_check_timeout;
    uv_timer_t timer_handshake_timeout;
    uv_timer_t timer_reconn_delay;
    uv_thread_t worker;
    pc_JSON *handshake_opts;
    pc_mutex_t wq_mutex;

    int is_connecting;
    volatile tr_kcp_state_t state;

    // async
    uv_async_t conn_async;
    uv_async_t receive_async;
    uv_async_t write_async;

    void (*reconn_fn)(tr_kcp_transport_t *tt);
    void (*reset_fn)(tr_kcp_transport_t *tt);
};

uv_buf_t pr_kcp_default_msg_encoder(tr_kcp_transport_t *tt, const pc_msg_t* msg);
pc_msg_t pr_kcp_default_msg_decoder(tr_kcp_transport_t *tt, const uv_buf_t* buf);

#endif //PITAYA_TR_KCP_H
