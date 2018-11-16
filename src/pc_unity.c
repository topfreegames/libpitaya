/**
 * Copyright (c) 2014-2015 NetEase, Inc. and other Pomelo contributors
 * MIT Licensed.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "pitaya.h"
#include "pc_assert.h"
#include "pc_lib.h"
#include "pitaya_unity.h"

#define __UNITYEDITOR__

#ifdef __UNITYEDITOR__
#include <time.h>
#endif

#ifdef _WIN32
#define CS_PITAYA_EXPORT __declspec(dllexport)
#else
#define CS_PITAYA_EXPORT
#endif

#if defined(__ANDROID__)
#undef __UNITYEDITOR__
#include <android/log.h>
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "cspitaya", __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG ,"cspitaya", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO  ,"cspitaya", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN  ,"cspitaya", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR ,"cspitaya", __VA_ARGS__)

static void 
android_log(int level, const char* msg, ...)
{
    char buf[256];
    va_list va;

    if (level < 0 || level < pc_lib_get_default_log_level() ) {
        return;
    }

    va_start(va, msg);
    vsnprintf(buf, 256, msg, va);
    va_end(va);

    switch(level) {
        case PC_LOG_DEBUG:
            LOGD("%s", buf);
            break;
        case PC_LOG_INFO:
            LOGI("%s", buf);
            break;
        case PC_LOG_WARN:
            LOGW("%s", buf);
            break;
        case PC_LOG_ERROR:
            LOGE("%s", buf);
            break;
        default:
            LOGV("%s", buf);
    }
}

#endif

#if defined(__UNITYEDITOR__)

static FILE *f = NULL;

static void 
unity_log(int level, const char *msg, ...)
{
    if (!f) {
        return;
    }

    time_t t = time(NULL);
    char buf[32];
    va_list va;
    int n;

    if (level < 0 || level < pc_lib_get_default_log_level() ) {
        return;
    }

    n = strftime(buf, 32, "[%Y-%m-%d %H:%M:%S]", localtime(&t));
    fwrite(buf, sizeof(char), n, f);

    switch(level) {
        case PC_LOG_DEBUG:
            fprintf(f, "U[DEBUG] ");
            break;
        case PC_LOG_INFO:
            fprintf(f, "U[INFO] ");
            break;
        case PC_LOG_WARN:
            fprintf(f, "U[WARN] ");
            break;
        case PC_LOG_ERROR:
            fprintf(f, "U[ERROR] ");
            break;
    }

    va_start(va, msg);
    vfprintf(f, msg, va);
    va_end(va);

    fprintf(f, "\n");
}
#endif

int 
pc_unity_init_log(const char *path)
{
#if defined(__UNITYEDITOR__)
    f = fopen(path, "w");
    if (!f) {
        return -1;
    }
#endif
    return 0;
}

void 
pc_unity_native_log(const char *msg)
{
    if (!msg || strlen(msg) == 0) {
        return;
    }
//    pc_lib_log(PC_LOG_DEBUG, msg);
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

void pc_unity_update_client_info( const char* platform, const char* build_number,const char* version) {
    pc_lib_client_info_t client_info;
    client_info.platform = platform;
    client_info.build_number = build_number;
    client_info.version = version;
    
    pc_update_client_info(client_info);
}

void 
pc_unity_lib_init(int log_level, const char* ca_file, const char* ca_path, pc_unity_assert_t custom_assert, const char* platform, const char* build_number,const char* version) {
    if (custom_assert != NULL) {
        update_assert(custom_assert);
    }

    pc_lib_client_info_t client_info;
    client_info.platform = platform;
    client_info.build_number = build_number;
    client_info.version = version;

    pc_lib_set_default_log_level(log_level);
#if defined(__ANDROID__)
    pc_lib_init(android_log, NULL, NULL, NULL, client_info);
#elif defined(__UNITYEDITOR__)
    pc_lib_init(unity_log, NULL, NULL, NULL, client_info);
#else
    pc_lib_init(NULL, NULL, NULL, NULL, client_info);
#endif

#if !defined(PC_NO_UV_TLS_TRANS)
    if (ca_file || ca_path) {
        tr_uv_tls_set_ca_file(ca_file, ca_path);
    }
#endif
}

pc_client_t * 
pc_unity_create(bool enable_tls, bool enable_poll, bool enable_reconnect, int conn_timeout) {
    pc_assert(conn_timeout >= 0);
    pc_client_init_result_t res = {0};
    pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
    if (enable_tls) {
        config.transport_name = PC_TR_NAME_UV_TLS;
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

void 
pc_unity_destroy(pc_client_t *client) 
{
    lc_callback_t* lc_cb;

#if defined(__UNITYEDITOR__)
    if (f) fclose(f);
    f = NULL;
#endif
    
    if (client) {
        lc_cb = (lc_callback_t*)pc_client_config(client)->ls_ex_data;
        if (lc_cb) {
            free(lc_cb);
        }
        pc_client_cleanup(client);
    }
}

int
pc_unity_request(pc_client_t* client, const char* route, const char* msg,
                 uint32_t cbid, int timeout, pc_unity_request_success_callback_t cb, 
                 pc_unity_request_error_callback_t error_cb) 
{
    request_cb_t* rp = (request_cb_t*)pc_lib_malloc(sizeof(request_cb_t));
    if (!rp) {
        return PC_RC_TIMEOUT;
    }
    rp->cb = cb;
    rp->error_cb = error_cb;
    rp->cbid= cbid;
    return pc_string_request_with_timeout(client, route, msg, rp, timeout, default_request_cb, default_error_cb);
}

int
pc_unity_binary_request(pc_client_t* client, const char* route, uint8_t* data,
                 int64_t len, uint32_t cbid, int timeout, pc_unity_request_success_callback_t cb,
                 pc_unity_request_error_callback_t error_cb)
{
    request_cb_t* rp = (request_cb_t*)pc_lib_malloc(sizeof(request_cb_t));
    if (!rp) {
        return PC_RC_TIMEOUT;
    }
    rp->cb = cb;
    rp->error_cb = error_cb;
    rp->cbid= cbid;
    return pc_binary_request_with_timeout(client, route, data, len, rp, timeout, default_request_cb, default_error_cb);
}
