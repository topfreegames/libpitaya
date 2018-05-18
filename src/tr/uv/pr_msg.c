/**
 * Copyright (c) 2014,2015 NetEase, Inc. and other Pomelo contributors
 * MIT Licensed.
 */

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include <pc_lib.h>

#include "pr_msg.h"
#include "tr_uv_tcp_i.h"
#include "pr_gzip.h"

#define PC_MSG_FLAG_BYTES 1
#define PC_MSG_ROUTE_LEN_BYTES 1
#define PC_MSG_ROUTE_CODE_BYTES 2

#define PC_MSG_HAS_ID(TYPE) ((TYPE) == PC_MSG_REQUEST ||                      \
        (TYPE) == PC_MSG_RESPONSE)

#define PC_MSG_HAS_ROUTE(TYPE) ((TYPE) != PC_MSG_RESPONSE)

#define PC_IS_VALID_TYPE(TYPE) ((TYPE) == PC_MSG_REQUEST ||                    \
        (TYPE) == PC_MSG_NOTIFY ||                                            \
        (TYPE) == PC_MSG_RESPONSE ||                                          \
        (TYPE) == PC_MSG_PUSH)

/**
 * message type.
 */
typedef enum {
    PC_MSG_REQUEST = 0,
    PC_MSG_NOTIFY,
    PC_MSG_RESPONSE,
    PC_MSG_PUSH
} pc_msg_type;

typedef union {
    uint16_t route_code;
    char *route_str;
} pc_message_route;

typedef struct {
    uint32_t id;
    pc_msg_type type;
    uint8_t is_route_compressed;
    uint8_t is_gzipped;
    int error;
    pc_message_route route;
    pc_buf_t body;
} pc__msg_raw_t;

static PC_INLINE const char *pc__resolve_dictionary(const pc_JSON* code2route, uint16_t code)
{
    pc_JSON* tmp;
    char code_str[16];
    memset(code_str, 0, 16);
    sprintf(code_str, "%u", code);
    tmp = pc_JSON_GetObjectItem(code2route, code_str);
    assert(tmp && tmp->type == pc_JSON_String);
    return tmp->valuestring;
}

static void* length_error() {
    pc_lib_log(PC_LOG_ERROR, "pc_msg_decode_to_raw - invalid length");
    return NULL;
}

static pc__msg_raw_t *pc_msg_decode_to_raw(const pc_buf_t* buf)
{
    int len = buf->len;
    if (len < PC_MSG_FLAG_BYTES) return length_error();

    const char* data = buf->base;
    int offset = 0;
    pc_message_flag* flag = (pc_message_flag*) &(data[offset++]);
    
    if (!PC_IS_VALID_TYPE(flag->message_type)) {
        pc_lib_log(PC_LOG_ERROR, "pc_msg_decode_to_raw - unknow message type");
        return NULL;
    }

    uint32_t id = PC_NOTIFY_PUSH_REQ_ID;
    if (PC_MSG_HAS_ID(flag->message_type)) {
        id = 0;
        for (int i = offset; i < len; ++i) {
            uint8_t byte = data[i];
            uint8_t value = byte & 0x7F;
            id += value << (7*(i-offset));
            if (byte < 128) { // most significant bit not set.
                offset = i+1;
                break;
            }
        }
    }

    // route
    pc_message_route route = { 0 };
    if (PC_MSG_HAS_ROUTE(flag->message_type)) {
        if (flag->route_compressed) {
            if (offset + PC_MSG_ROUTE_CODE_BYTES - 1 >= len) return length_error();

            route.route_code = (uint16_t)data[offset];
            offset += 2;
        } else {
            size_t route_len;
            if (offset + PC_MSG_ROUTE_LEN_BYTES - 1 >= len || (route_len = data[offset++]) >= len) return length_error();
            
            route.route_str = (char *)pc_lib_malloc(route_len + 1);
            route.route_str[route_len] = '\0';
            strncpy(route.route_str, &data[offset], route_len);
            offset += route_len;
        }
    }

    pc__msg_raw_t *msg = (pc__msg_raw_t *)pc_lib_malloc(sizeof(pc__msg_raw_t));

    msg->type = (pc_msg_type)flag->message_type;
    msg->is_gzipped = flag->data_compressed;
    msg->is_route_compressed = flag->route_compressed;
    msg->error = flag->error;
    msg->id = id;
    memcpy(&msg->route, &route, sizeof(route));

    // borrow memory from original pc_buf_t
    msg->body.base = (char* )data + offset;
    msg->body.len = len - offset;

    assert(msg->id != PC_INVALID_REQ_ID);
    
    return msg;
}

pc_msg_t pc_default_msg_decode(const pc_JSON* code2route, const pc_buf_t* buf)
{
    const char *route_str = NULL;
    const char *origin_route = NULL;

    pc_JSON* json_msg = NULL;
    const char* data = NULL;
    pc_buf_t body;
    pc_msg_t msg;
    pc__msg_raw_t* raw_msg = NULL;

    memset(&msg, 0, sizeof(pc_msg_t));
    msg.id = PC_INVALID_REQ_ID;

    assert(buf && buf->base);

    raw_msg = pc_msg_decode_to_raw(buf);

    if (!raw_msg) {
        return msg;
    }

    assert(raw_msg->id != PC_INVALID_REQ_ID);

    msg.id = raw_msg->id;

    /* route */
    if (PC_MSG_HAS_ROUTE(raw_msg->type)) {
        /* uncompress route dictionary */
        route_str = NULL;
        if (raw_msg->is_route_compressed) {
            origin_route = pc__resolve_dictionary(code2route, raw_msg->route.route_code);
            if (!origin_route) {
                pc_lib_log(PC_LOG_ERROR, "pc_default_msg_decode - fail to uncompress route dictionary: %d",
                        raw_msg->route.route_code);
            } else {
                route_str = (char* )pc_lib_malloc(strlen(origin_route) + 1);
                memset((char*)route_str, 0, strlen(origin_route) + 1);
                strcpy((char*)route_str, origin_route);
            }
        } else {
            /* till now, raw_msg->route.route_str is hold by pc_msg_t */
            route_str = raw_msg->route.route_str;
            raw_msg->route.route_str = NULL;
        }

        msg.route = route_str;
    } else {
        /* FIXME: for resp, we can not get route here, so just set it to NULL */
        msg.route = NULL;
    }

    if (PC_MSG_HAS_ROUTE(raw_msg->type) && !msg.route) {
        msg.id = PC_INVALID_REQ_ID;
        pc_lib_free(raw_msg);
        return msg;
    }

    /* body.base is within msg */
    body = raw_msg->body;
    if (body.len > 0) {
        json_msg = NULL;
        if (!msg.route) {
            json_msg = pc_body_json_decode(body.base, 0, body.len, raw_msg->is_gzipped);
        } else {
            // TODO prever deconding raw
            // } else {
            json_msg = pc_body_json_decode(body.base, 0, body.len, raw_msg->is_gzipped);
            // }
        }

        if (!json_msg) {
            pc_lib_free((char*)msg.route);
            msg.id = PC_INVALID_REQ_ID;
            msg.route = NULL;
        } else {
            data = pc_JSON_PrintUnformatted(json_msg);

            assert(data);

            msg.msg = data;
            /* FIXME: review */
            pc_JSON_Delete(json_msg);
        }
    }
    msg.error = raw_msg->error;

    pc_lib_free(raw_msg);

    return msg;
}


static pc_buf_t pc_msg_encode_route(uint32_t id, pc_msg_type type,
        const char *route, int compressed, const pc_buf_t msg);
static pc_buf_t pc_msg_encode_code(uint32_t id, pc_msg_type type,
        int route_code, int compressed, const pc_buf_t msg);

static uint8_t pc__msg_id_length(uint32_t id);
static PC_INLINE size_t pc__msg_encode_flag(pc_msg_type type, int compressRoute,
        int compressed, char *base, size_t offset);
static PC_INLINE size_t pc__msg_encode_id(uint32_t id, char *base, size_t offset);
static PC_INLINE size_t pc__msg_encode_route(const char *route, uint16_t route_len,
        char *base, size_t offset);

pc_buf_t pc_msg_encode_route(uint32_t id, pc_msg_type type,
        const char *route, int compressed, const pc_buf_t msg)
{
    pc_buf_t buf;
    uint8_t id_len = PC_MSG_HAS_ID(type) ? pc__msg_id_length(id) : 0;
    uint16_t route_len = PC_MSG_HAS_ROUTE(type) ? strlen(route) : 0;

    size_t msg_len = PC_MSG_FLAG_BYTES + id_len +
        PC_MSG_ROUTE_LEN_BYTES + route_len + msg.len;
    char *base = NULL;
    size_t offset = 0;

    buf.base = NULL;
    buf.len = -1;

    base = buf.base = (char *)pc_lib_malloc(msg_len);
    buf.len = (int)msg_len;

    /* flag */
    offset = pc__msg_encode_flag(type, 0, compressed, base, offset);

    /* message id */
    if(PC_MSG_HAS_ID(type)) {
        offset = pc__msg_encode_id(id, base, offset);
    }

    /* route */
    if(PC_MSG_HAS_ROUTE(type)) {
        offset = pc__msg_encode_route(route, route_len, base, offset);
    }

    /* body */
    memcpy(base + offset, msg.base, msg.len);
    return buf;
}

pc_buf_t pc_msg_encode_code(uint32_t id, pc_msg_type type,
        int compressed, int route_code, const pc_buf_t body)
{
    pc_buf_t buf;

    uint8_t id_len = PC_MSG_HAS_ID(type) ? pc__msg_id_length(id) : 0;
    uint16_t route_len = PC_MSG_HAS_ROUTE(type) ? PC_MSG_ROUTE_CODE_BYTES : 0;
    size_t msg_len = PC_MSG_FLAG_BYTES + id_len + route_len + body.len;
    char* base = NULL;
    size_t offset = 0;

    buf.base = NULL;
    buf.len = -1;

    base = buf.base = (char *)pc_lib_malloc(msg_len);
    buf.len = (int)msg_len;

    /* flag */
    offset = pc__msg_encode_flag(type, 1, compressed, base, offset);

    /* message id */
    if(PC_MSG_HAS_ID(type)) {
        offset = pc__msg_encode_id(id, base, offset);
    }

    /* route code */
    if(PC_MSG_HAS_ROUTE(type)) {
        base[offset++] = (route_code >> 8) & 0xff;
        base[offset++] = route_code & 0xff;
    }

    /* body */
    memcpy(base + offset, body.base, body.len);
    return buf;
}

static PC_INLINE size_t pc__msg_encode_flag(pc_msg_type type, int compress_route,
        int compress, char *base, size_t offset)
{
    base[offset++] = (type << 1) | (compress_route ? 1 : 0) | compress << 4;
    return offset;
}

static PC_INLINE size_t pc__msg_encode_id(uint32_t id, char *base, size_t offset)
{
    do{
        uint32_t tmp = id & 0x7f;
        uint32_t next = id >> 7;

        if(next != 0){
            tmp = tmp + 128;
        }
        base[offset++] = tmp;
        id = next;
    } while(id != 0);

    return offset;
}

static PC_INLINE size_t pc__msg_encode_route(const char *route, uint16_t route_len,
        char *base, size_t offset)
{
    base[offset++] = route_len & 0xff;

    memcpy(base + offset, route, route_len);

    return offset + route_len;
}

static uint8_t pc__msg_id_length(uint32_t id)
{
    uint8_t len = 0;
    do {
        len += 1;
        id >>= 7;
    } while(id > 0);
    return len;
}

pc_buf_t pc_default_msg_encode(const pc_JSON* route2code, const pc_msg_t* msg, int compress_data)
{
    pc_buf_t msg_buf;
    pc_buf_t body_buf;
    pc_JSON* json_msg;
    int route_code = -1;
    pc_JSON* code = NULL;
    pc_JSON* pb_def = NULL;
    pc_msg_type type;

    msg_buf.base = NULL;
    msg_buf.len = -1;
    body_buf.base = NULL;
    body_buf.len = -1;

    assert(msg && msg->msg && msg->route);

    json_msg = pc_JSON_ParseWithOpts(msg->msg, 0, 1);
    if (!json_msg) {
        pc_lib_log(PC_LOG_ERROR, "pc_default_msg_encode - the msg is not invalid json");
        return msg_buf;
    }

    assert(json_msg);

    //TODO prever encode n json
    // } else {
        body_buf = pc_body_json_encode(json_msg, compress_data);
        if(body_buf.len == -1) {
            assert(body_buf.base == NULL);
            pc_lib_log(PC_LOG_ERROR, "pc_default_msg_encode - fail to encode message with json: %s\n", msg->route);
        }
    //}

    if (body_buf.len == -1) {
        assert(body_buf.base == NULL);
        pc_lib_log(PC_LOG_ERROR, "pc_default_msg_encode - fail to encode message with json: %s\n", msg->route);
    }

    pc_JSON_Delete(json_msg);
    json_msg = NULL;

    if (body_buf.len == -1) {
        return msg_buf;
    }

    assert(body_buf.base && body_buf.len != -1);

    type = msg->id == PC_NOTIFY_PUSH_REQ_ID ? PC_MSG_NOTIFY : PC_MSG_REQUEST;

    if (route2code && (code = pc_JSON_GetObjectItem(route2code, msg->route))
            && code->type == pc_JSON_Number) {
        route_code = code->valueint;
    }

    int compressed = is_compressed((unsigned char*)body_buf.base, body_buf.len);
    
    if (route_code > 0) {
        msg_buf = pc_msg_encode_code(msg->id, type, route_code, compressed, body_buf);
        if(msg_buf.len == -1) {
            assert(msg_buf.base == NULL);
            pc_lib_log(PC_LOG_ERROR, "pc_default_msg_encode - failed to encode message with route code: %d\n",
                    route_code);
        }
    } else {
        msg_buf = pc_msg_encode_route(msg->id, type, msg->route, compressed, body_buf);
        if(msg_buf.len == -1) {
            assert(msg_buf.base == NULL);
            pc_lib_log(PC_LOG_ERROR, "pc_default_msg_encode - failed to encode message with route string: %s\n",
                    msg->route);
        }
    }

    pc_lib_free(body_buf.base);
    body_buf.base = NULL;
    body_buf.len = -1;

    return msg_buf;
}

/* for transport plugin */
uv_buf_t pr_default_msg_encoder(tr_uv_tcp_transport_t* tt, const pc_msg_t* msg)
{
    pc_buf_t pb;
    uv_buf_t ub;

    pb = pc_default_msg_encode(tt->route_to_code, msg, !tt->config->disable_compression);
    ub.base = pb.base;
    ub.len = pb.len;
    return ub;
}

pc_msg_t pr_default_msg_decoder(tr_uv_tcp_transport_t* tt, const uv_buf_t* buf)
{
    pc_buf_t pb;
    pb.base = buf->base;
    pb.len = buf->len;

    return pc_default_msg_decode(tt->code_to_route, &pb);
}
