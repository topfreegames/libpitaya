/**
 * Copyright (c) 2014-2015 NetEase, Inc. and other Pomelo contributors
 * MIT Licensed.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include "pitaya.h"
#include "pc_assert.h"
#include "pc_lib.h"

#ifdef _WIN32
#define CS_PITAYA_EXPORT __declspec(dllexport)
#else
#define CS_PITAYA_EXPORT
#endif

typedef void (*pc_unity_log_function_t)(int, char*);
typedef void (*pc_unity_request_success_callback_t)(pc_client_t* client, uint32_t cbid, const pc_buf_t* resp);
typedef void (*pc_unity_request_error_callback_t)(pc_client_t* client, uint32_t cbid, const pc_error_t* error);
typedef void (*pc_unity_assert_t)(const char *e, const char *file, int line);

static pc_unity_log_function_t g_log_function = NULL;

static void
custom_log(int level, const char *msg, ...)
{
    if (g_log_function == NULL) {
        return;
    }

    if (level < 0 || level < pc_lib_get_default_log_level() ) {
        return;
    }

    const int max_size_for_time = 32;
    char log_buf[256];

    time_t t = time(NULL);
    int bytes_written = strftime(log_buf, max_size_for_time, "[%Y-%m-%d %H:%M:%S]", localtime(&t));

    va_list va;
    va_start(va, msg);
    vsnprintf(log_buf + bytes_written, 256-bytes_written, msg, va);
    va_end(va);

    g_log_function(level, log_buf);
}

CS_PITAYA_EXPORT int
pc_unity_init_log_function(pc_unity_log_function_t log_function)
{
    if (log_function == NULL) {
        return -1;
    }

    g_log_function = log_function;
    return 0;
}

typedef struct {
    char* (* read) ();
    int   (* write)(char* data);
} lc_callback_t;

typedef struct {
    pc_unity_request_success_callback_t cb;
    pc_unity_request_error_callback_t error_cb;
    uint32_t cbid;
} request_cb_t;

static void
default_request_cb(const pc_request_t *req, const pc_buf_t *resp)
{
    request_cb_t* rp = (request_cb_t*)pc_request_ex_data(req);
    pc_client_t* client = pc_request_client(req);
    pc_assert(rp);
    request_cb_t r = *rp;
    free(rp);

    r.cb(client, r.cbid, resp);
}

static void
default_error_cb(const pc_request_t *req, const pc_error_t *error)
{
    request_cb_t* rp = (request_cb_t*)pc_request_ex_data(req);
    pc_client_t* client = pc_request_client(req);
    pc_assert(rp);
    request_cb_t r = *rp;
    free(rp);
    r.error_cb(client, r.cbid, error);
}

CS_PITAYA_EXPORT void 
pc_unity_update_client_info( const char* platform, const char* build_number,const char* version)
{
    pc_lib_client_info_t client_info;
    client_info.platform = platform;
    client_info.build_number = build_number;
    client_info.version = version;

    pc_update_client_info(client_info);
}

CS_PITAYA_EXPORT void
pc_unity_lib_init(int log_level, const char* ca_file, const char* ca_path, pc_unity_assert_t custom_assert, const char* platform, const char* build_number,const char* version) 
{
    if (custom_assert != NULL) {
        update_assert(custom_assert);
    }

    pc_lib_client_info_t client_info;
    client_info.platform = platform;
    client_info.build_number = build_number;
    client_info.version = version;

    pc_lib_set_default_log_level(log_level);

    pc_lib_init(custom_log, NULL, NULL, NULL, client_info);

#if !defined(PC_NO_UV_TLS_TRANS)
    if (ca_file || ca_path) {
        tr_uv_tls_set_ca_file(ca_file, ca_path);
    }
#endif
}

CS_PITAYA_EXPORT pc_client_t *
pc_unity_create(int trans_mode, bool enable_poll, bool enable_reconnect, int conn_timeout) {
    pc_assert(conn_timeout >= 0);
    pc_client_init_result_t res = {0};
    pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
    if (trans_mode == 1) {
        config.transport_name = PC_TR_NAME_UV_TLS;
    } else if (trans_mode == 2) {
        config.transport_name = PC_TR_NAME_KCP;
    }

    config.enable_polling = enable_poll;
    config.enable_reconn = enable_reconnect;
    config.conn_timeout = conn_timeout;

    res = pc_client_init(NULL, &config);
    if (res.rc == PC_RC_OK) {
        return res.client;
    }

    return NULL;
}

CS_PITAYA_EXPORT void
pc_unity_destroy(pc_client_t *client)
{
    lc_callback_t* lc_cb;

    if (client) {
        lc_cb = (lc_callback_t*)pc_client_config(client)->ls_ex_data;
        if (lc_cb) {
            free(lc_cb);
        }
        pc_client_cleanup(client);
    }
}

CS_PITAYA_EXPORT int
pc_unity_request(pc_client_t* client, const char* route, const char* msg,
                 uint32_t cbid, int timeout, pc_unity_request_success_callback_t cb,
                 pc_unity_request_error_callback_t error_cb)
{
    request_cb_t* rp = (request_cb_t*)pc_lib_malloc(sizeof(request_cb_t));
    if (!rp) {
        // TODO: consider creating a new return code like PC_RC_NO_MEM
        return PC_RC_ERROR;
    }
    rp->cb = cb;
    rp->error_cb = error_cb;
    rp->cbid= cbid;
    return pc_string_request_with_timeout(client, route, msg, rp, timeout, default_request_cb, default_error_cb);
}

CS_PITAYA_EXPORT int
pc_unity_binary_request(pc_client_t* client, const char* route, uint8_t* data,
                 int64_t len, uint32_t cbid, int timeout, pc_unity_request_success_callback_t cb,
                 pc_unity_request_error_callback_t error_cb)
{
    request_cb_t* rp = (request_cb_t*)pc_lib_malloc(sizeof(request_cb_t));
    if (!rp) {
        // TODO: consider creating a new return code like PC_RC_NO_MEM
        return PC_RC_ERROR;
    }
    rp->cb = cb;
    rp->error_cb = error_cb;
    rp->cbid= cbid;
    return pc_binary_request_with_timeout(client, route, data, len, rp, timeout, default_request_cb, default_error_cb);
}
