//
// Created by lichong on 2020/12/11.
//

#ifndef PITAYA_TR_KCP_H
#define PITAYA_TR_KCP_H

#include <pitaya_trans.h>
#include <uv.h>
#include "ikcp.h"
#include "pr_msg.h"

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
    TR_KCP_NOT_CONN,
    TR_KCP_CONNECTING,
    TR_KCP_HANDSHAKING,
    TR_KCP_DONE
} tr_kcp_state_t;


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
    int disable_compression;

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

uv_buf_t pr_kcp_default_msg_encoder(kcp_transport_t *tt, const pc_msg_t* msg);
pc_msg_t pr_kcp_default_msg_decoder(kcp_transport_t *tt, const uv_buf_t* buf);

#endif //PITAYA_TR_KCP_H
