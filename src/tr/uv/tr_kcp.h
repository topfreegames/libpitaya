//
// Created by lichong on 2020/12/11.
//

#ifndef PITAYA_TR_KCP_H
#define PITAYA_TR_KCP_H

#include <pitaya_trans.h>

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


#endif //PITAYA_TR_KCP_H
