/**
 * Copyright (c) 2014,2015 NetEase, Inc. and other Pomelo contributors
 * MIT Licensed.
 */

#ifndef PR_MSG_H
#define PR_MSG_H

#include <stdint.h>

#include "pr_pkg.h"
#include <pc_JSON.h>

typedef struct tr_uv_tcp_transport_s tr_uv_tcp_transport_t;

typedef struct {
    unsigned int id;
    int error;
    const char* route;
    pc_JSON   *json_msg;
} pc_msg_t;

uv_buf_t pr_default_msg_encoder(tr_uv_tcp_transport_t* tt, const pc_msg_t* msg);
pc_msg_t pr_default_msg_decoder(tr_uv_tcp_transport_t* tt, const uv_buf_t* buf);

/**
 * internal use
 */
typedef struct {
    char* base;
    int len;
} pc_buf_t;

typedef struct {
    char route_compressed:1;
    char message_type:3;
    char data_compressed:1;
    char error:1;
    char unused:2;
} pc_message_flag;

typedef struct {
    char value:7;
    char continues:1;
} pc_message_id_byte;

pc_buf_t pc_default_msg_encode(const pc_JSON* route2code, const pc_msg_t* msg, int compress_data);
pc_msg_t pc_default_msg_decode(const pc_JSON* code2route, const pc_buf_t* buf);

pc_buf_t pc_body_json_encode(const pc_JSON* msg, int compress_data);
pc_JSON* pc_body_json_decode(const char *data, size_t offset, size_t len, int gzipped);

#endif

