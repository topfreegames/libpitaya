/**
 * Copyright (c) 2014,2015 NetEase, Inc. and other Pomelo contributors
 * MIT Licensed.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <pc_assert.h>
#include <pc_mutex.h>
#include <time.h>

#include <pitaya.h>
#include <pitaya_trans.h>

#include "pc_lib.h"

#if !defined(PC_NO_DUMMY_TRANS)
#  include "tr/dummy/tr_dummy.h"
#endif

#if !defined(PC_NO_UV_TCP_TRANS)
#  include "tr/uv/tr_uv_tcp.h"

#  if !defined(PC_NO_UV_TLS_TRANS)
#    include "tr/uv/tr_uv_tls.h"
#  endif /* tls */

#endif /* tcp */

#define PC_MAX_PINNED_KEYS 10

void (*pc_lib_log)(int level, const char* msg, ...) = NULL;
void* (*pc_lib_malloc)(size_t len) = NULL;
void (*pc_lib_free)(void* data) = NULL;
void* (*pc_lib_realloc)(void* ptr, size_t len) = NULL;

const char *pc_lib_platform_str = NULL;
const char *pc_lib_client_build_number_str = NULL;
const char *pc_lib_client_version_str = NULL;

static int pc__default_log_level = 0;

// TODO: Should this array really be dynamic?
// If there was a small upper bound, a static array could be used,
// avoid the need for dynamic memory allocation.
typedef struct {
    uint8_t *key;
    size_t size;
} pc_pinned_key_t;

static pc_pinned_key_t pc__pinned_keys[PC_MAX_PINNED_KEYS];
static pc_mutex_t pc__pinned_keys_mutex;

static int pc_initiateded = 0;


/**
 * default malloc never return NULL
 * so we don't have to check its return value
 *
 * if you customize malloc, please make sure that it never return NULL
 */
static void* default_malloc(size_t len) {
    void* d = malloc(len);

    /* if oom, just abort */
    if (!d)
        abort();

    return d;
}

static void* default_realloc(void* ptr, size_t len) {
    ptr = realloc(ptr, len);
    
    if (!ptr){
        abort();
    }
    
    return ptr;
}

static void default_log(int level, const char* msg, ...) {
    time_t t = time(NULL);
    char buf[32];
    va_list va;

    if (level < pc__default_log_level) {
        return;
    }

    strftime(buf, 32, "[%Y-%m-%d %H:%M:%S]", localtime(&t));
    printf("%s", buf);
    switch(level) {
    case PC_LOG_DEBUG:
        printf("[DEBUG] ");
        break;
    case PC_LOG_INFO:
        printf("[INFO] ");
        break;
    case PC_LOG_WARN:
        printf("[WARN] ");
        break;
    case PC_LOG_ERROR:
        printf("[ERROR] ");
        break;
    }

    va_start(va, msg);
    vprintf(msg, va);
    va_end(va);

    printf("\n");

    fflush(stderr   );
}

void pc_lib_init(void (*pc_log)(int level, const char* msg, ...), 
                 void* (*pc_alloc)(size_t), void (*pc_free)(void* ), 
                 void* (*pc_realloc)(void*, size_t), 
                 pc_lib_client_info_t client_info) {
    if(pc_initiateded == 1) {
        return; // init function already called
    }
    pc_initiateded = 1;

    pc_mutex_init(&pc__pinned_keys_mutex);
    pc_lib_clear_pinned_public_keys();

    pc_transport_plugin_t* tp;

    pc_lib_log = pc_log ? pc_log : default_log;
    pc_lib_malloc = pc_alloc ? pc_alloc : default_malloc;
    pc_lib_realloc = pc_realloc ? pc_realloc : default_realloc;
    pc_lib_free = pc_free ? pc_free: free;

    pc_lib_platform_str = client_info.platform 
        ? pc_lib_strdup(client_info.platform) 
        : pc_lib_strdup("desktop");
    pc_lib_client_build_number_str = client_info.build_number
        ? pc_lib_strdup(client_info.build_number)
        : pc_lib_strdup("1");
    pc_lib_client_version_str = client_info.version
        ? pc_lib_strdup(client_info.version)
        : pc_lib_strdup("0.1");

#if !defined(PC_NO_DUMMY_TRANS)
    tp = pc_tr_dummy_trans_plugin();
    pc_transport_plugin_register(tp);
    pc_lib_log(PC_LOG_INFO, "pc_lib_init - register dummy plugin");
#endif

#if !defined(PC_NO_UV_TCP_TRANS)
    tp = pc_tr_uv_tcp_trans_plugin();
    pc_transport_plugin_register(tp);
    pc_lib_log(PC_LOG_INFO, "pc_lib_init - register tcp plugin");
#if !defined(PC_NO_UV_TLS_TRANS)
    tp = pc_tr_uv_tls_trans_plugin();
    pc_transport_plugin_register(tp);
    pc_lib_log(PC_LOG_INFO, "pc_lib_init - register tls plugin");
#endif
    srand((unsigned int)time(0));

#endif
}

void pc_lib_cleanup() {
    pc_lib_free((char*)pc_lib_platform_str);
    pc_lib_free((char*)pc_lib_client_build_number_str);
    pc_lib_free((char*)pc_lib_client_version_str);

    pc_lib_log(PC_LOG_INFO, "pc_lib_cleanup - free pinned public keys array");

    // Clear pinned keys
    pc_lib_clear_pinned_public_keys();
    pc_mutex_destroy(&pc__pinned_keys_mutex);

    return; // TODO: Should this return be here?
#if !defined(PC_NO_DUMMY_TRANS)
    pc_transport_plugin_deregister(PC_TR_NAME_DUMMY);
    pc_lib_log(PC_LOG_INFO, "pc_lib_cleanup - deregister dummy plugin");
#endif

#if !defined(PC_NO_UV_TCP_TRANS)
    pc_transport_plugin_deregister(PC_TR_NAME_UV_TCP);
    pc_lib_log(PC_LOG_INFO, "pc_lib_cleanup - deregister tcp plugin");

#if !defined(PC_NO_UV_TLS_TRANS)
    pc_transport_plugin_deregister(PC_TR_NAME_UV_TLS);
    pc_lib_log(PC_LOG_INFO, "pc_lib_cleanup - deregister tls plugin");
#endif

#endif
}

const char* pc_lib_strdup(const char* str) {
    char* buf;
    size_t len;

    if (!str)
        return NULL;

    len = strlen(str);

    buf = (char*)pc_lib_malloc(len + 1);
    strcpy(buf, str);
    buf[len] = '\0';

    return buf;
}

static const char* state_str[] = {
    "PC_ST_INITED",
    "PC_ST_CONNECTING",
    "PC_ST_CONNECTED",
    "PC_ST_DISCONNECTING",
    "PC_ST_UNKNOWN",
    NULL
};


const char* pc_client_state_str(int state) {
    pc_assert(state < PC_ST_COUNT && state >= 0);
    return state_str[state];
}

static const char* ev_str[] = {
    "PC_EV_USER_DEFINED_PUSH",
    "PC_EV_CONNECTED",
    "PC_EV_CONNECT_ERROR",
    "PC_EV_CONNECT_FAILED",
    "PC_EV_DISCONNECT",
    "PC_EV_KICKED_BY_SERVER",
    "PC_EV_UNEXPECTED_DISCONNECT",
    "PC_EV_PROTO_ERROR",
    "PC_EV_RECONNECT_FAILED",
    "PC_EV_RECONNECT_STARTED",
    "PC_EV_UNPINNED_KEY",
    NULL
};

const char* pc_client_ev_str(int ev_type) {
    pc_assert(ev_type >= 0 && ev_type < PC_EV_COUNT);
    return ev_str[ev_type];
}

static const char* rc_str[] = {
    "PC_RC_OK",
    "PC_RC_ERROR",
    "PC_RC_TIMEOUT",
    "PC_RC_INVALID_JSON",
    "PC_RC_INVALID_ARG",
    "PC_RC_NO_TRANS",
    "PC_RC_INVALID_THREAD",
    "PC_RC_TRANS_ERROR",
    "PC_RC_INVALID_ROUTE",
    "PC_RC_INVALID_STATE",
    "PC_RC_NOT_FOUND",
    "PC_RC_RESET",
    "PC_RC_SERVER_ERROR",
    "PC_RC_UV_ERROR",
    NULL
};

const char* pc_client_rc_str(int rc) {
    pc_assert(rc <= 0 && rc > PC_RC_MIN);
    return rc_str[-rc];
}

int pc_lib_get_default_log_level() {
    return pc__default_log_level;
}

void pc_lib_set_default_log_level(int level) {
    pc__default_log_level = level;
}

// ---------------------------------
// Pinned keys
// ---------------------------------

bool pc_lib_is_key_pinned(uint8_t *key, size_t key_size)
{
    bool result = false;
    pc_mutex_lock(&pc__pinned_keys_mutex);
    for (size_t i = 0; i < PC_MAX_PINNED_KEYS; ++i) {
        pc_pinned_key_t curr_key = pc__pinned_keys[i];
        if (curr_key.size == key_size) {
            if (memcmp(key, curr_key.key, key_size) == 0) {
                result = true;
                goto out;
            }
        }
    }
out:
    pc_mutex_unlock(&pc__pinned_keys_mutex);
    return result;
}

void pc_lib_add_pinned_public_key(uint8_t *public_key, size_t size)
{
    pc_assert(public_key);
    if (!public_key || size == 0) {
        pc_lib_log(PC_LOG_ERROR, "The public_key should not be a NULL pointer");
        return;
    }

    bool found = false;
    for (size_t i = 0; i < PC_MAX_PINNED_KEYS; ++i) {
        if (pc__pinned_keys[i].key == NULL) {
            pc__pinned_keys[i].key = public_key;
            pc__pinned_keys[i].size = size;
            found = true;
            break;
        }
    }

    if (!found) {
        pc_lib_free(public_key);
        pc_lib_log(PC_LOG_ERROR, "Maximum amount of pinned keys (%d) is already used", PC_MAX_PINNED_KEYS);
    }
}

void pc_lib_clear_pinned_public_keys(void)
{
    pc_mutex_lock(&pc__pinned_keys_mutex);
    for (size_t i = 0; i < PC_MAX_PINNED_KEYS; ++i) {
        if (pc__pinned_keys[i].key) {
            pc_lib_free(pc__pinned_keys[i].key);
            pc__pinned_keys[i].key = NULL;
            pc__pinned_keys[i].size = 0;
        }
        pc__pinned_keys[i].key = NULL;
        pc__pinned_keys[i].size = 0;
    }
    pc_mutex_unlock(&pc__pinned_keys_mutex);
}
