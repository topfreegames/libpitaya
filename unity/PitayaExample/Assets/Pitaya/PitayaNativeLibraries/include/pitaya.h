/**
 * Copyright (c) 2014 NetEase, Inc. and other Pomelo contributors
 * MIT Licensed.
 */

#ifndef PC_PITAYA_H
#define PC_PITAYA_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#  define PC_EXPORT __declspec(dllexport)
#else
#  define PC_EXPORT
#endif

#include "pitaya_version.h"

#define PC_T(x) PC_T2(x)
#define PC_T2(x) #x

#define PC_VERSION_NUM (PC_VERSION_MAJOR * 10000 + PC_VERSION_MINOR * 100 + PC_VERSION_REVISION)
#define PC_VERSION_STR ( PC_T(PC_VERSION_MAJOR) "." PC_T(PC_VERSION_MINOR) \
        "." PC_T(PC_VERSION_REVISION) "-" PC_VERSION_SUFFIX )

/**
 * error code
 */
#define PC_RC_OK 0
#define PC_RC_ERROR -1
#define PC_RC_TIMEOUT -2
#define PC_RC_INVALID_JSON -3
#define PC_RC_INVALID_ARG -4
#define PC_RC_NO_TRANS -5
#define PC_RC_INVALID_THREAD -6
#define PC_RC_TRANS_ERROR -7
#define PC_RC_INVALID_ROUTE -8
#define PC_RC_INVALID_STATE -9
#define PC_RC_NOT_FOUND -10
#define PC_RC_RESET -11
#define PC_RC_SERVER_ERROR -12
#define PC_RC_UV_ERROR -13
#define PC_RC_NO_SUCH_FILE -14
#define PC_RC_MIN -15

typedef struct pc_client_s pc_client_t;
typedef struct pc_request_s pc_request_t;
typedef struct pc_notify_s pc_notify_t;

/**
 * client state
 */
#define PC_ST_INITED 0
#define PC_ST_CONNECTING 1
#define PC_ST_CONNECTED 2
#define PC_ST_DISCONNECTING 3
#define PC_ST_UNKNOWN 4
#define PC_ST_COUNT 5


/**
 * log level
 */
#define PC_LOG_DEBUG 0
#define PC_LOG_INFO 1
#define PC_LOG_WARN 2
#define PC_LOG_ERROR 3
#define PC_LOG_DISABLE 4

/**
 * some tunable arguments
 */
#define PC_TRANSPORT_PLUGIN_SLOT_COUNT 8
#define PC_PRE_ALLOC_REQUEST_SLOT_COUNT 4 
#define PC_PRE_ALLOC_NOTIFY_SLOT_COUNT 4
#define PC_TIMEOUT_CHECK_INTERVAL 2
#define PC_HEARTBEAT_TIMEOUT_FACTOR 2
#define PC_TCP_READ_BUFFER_SIZE (1 << 16)

/**
 * builtin transport name
 */
#define PC_TR_NAME_UV_TCP 0
#define PC_TR_NAME_UV_TLS 1
#define PC_TR_NAME_DUMMY 7

/**
 * reconnect max retry
 */
#define PC_ALWAYS_RETRY -1 

/**
 * disable timeout
 */
#define PC_WITHOUT_TIMEOUT -1

typedef enum { 
    PC_LOCAL_STORAGE_OP_READ = 0,
    PC_LOCAL_STORAGE_OP_WRITE = 1,
} pc_local_storage_op_t;
typedef int (*pc_local_storage_cb_t)(pc_local_storage_op_t op,
        char* data, size_t* len, void* ex_data);

/**
 * Binary buffer
 */
typedef struct {
    uint8_t *base;
    int64_t  len;
} pc_buf_t;

PC_EXPORT pc_buf_t pc_buf_empty(void);
PC_EXPORT pc_buf_t pc_buf_copy(const pc_buf_t *buf);
PC_EXPORT void pc_buf_free(pc_buf_t *buf);
PC_EXPORT pc_buf_t pc_buf_from_string(const char *str);
PC_EXPORT void pc_buf_debug_print(const pc_buf_t *buf);

/**
 * Push
 */
typedef void (*pc_push_handler_cb_t)(pc_client_t *client, const char *route, const pc_buf_t *payload);

PC_EXPORT void pc_client_set_push_handler(pc_client_t *client, pc_push_handler_cb_t cb);

typedef struct {
    int conn_timeout;

    int enable_reconn;
    int reconn_max_retry;
    int reconn_delay;
    int reconn_delay_max;
    int reconn_exp_backoff;

    int enable_polling;

    pc_local_storage_cb_t local_storage_cb;
    void* ls_ex_data;

    int transport_name;
    
    int disable_compression;
} pc_client_config_t;

#define PC_CLIENT_CONFIG_DEFAULT                      \
{                                                     \
    30, /* conn_timeout */                            \
    1, /* enable_reconn */                            \
    PC_ALWAYS_RETRY, /* reconn_max_retry */           \
    2, /* reconn_delay */                             \
    30, /* reconn_delay_max */                        \
    1, /* reconn_exp_backoff */                       \
    0, /* enable_polling */                           \
    NULL, /* local_storage_cb */                      \
    NULL, /* ls_ex_data */                            \
    PC_TR_NAME_UV_TCP, /* transport_name */           \
    0 /* disable_compression */                       \
}

PC_EXPORT int pc_lib_version(void);
PC_EXPORT const char* pc_lib_version_str(void);

/**
 * If you do use default log callback,
 * this function will change the level of log out.
 *
 * Otherwise, this function does nothing.
 */
PC_EXPORT void pc_lib_set_default_log_level(int level);
PC_EXPORT int pc_lib_get_default_log_level(void);

/**
 * pc_lib_init and pc_lib_cleanup both should be invoked only once.
 */
typedef struct {
    const char *platform;
    const char *build_number;
    const char *version;
} pc_lib_client_info_t;

PC_EXPORT void pc_lib_init(void (*pc_log)(int level, const char* msg, ...), 
                           void* (*pc_alloc)(size_t), 
                           void (*pc_free)(void* ), 
                           void* (*pc_realloc)(void*, size_t), 
                           pc_lib_client_info_t client_info);
    
PC_EXPORT void pc_update_client_info(pc_lib_client_info_t client_info);

/**
 * Pins a public key globally for all clients.
 */
PC_EXPORT int pc_lib_add_pinned_public_key_from_certificate_string(const char *ca_string);
PC_EXPORT int pc_lib_add_pinned_public_key_from_certificate_file(const char *ca_path);
PC_EXPORT void pc_lib_skip_key_pin_check(bool should_skip);
    
/**
 * Remote all pinned public keys.
 */
PC_EXPORT void pc_lib_clear_pinned_public_keys(void);

PC_EXPORT void pc_lib_cleanup(void);

typedef struct {
    pc_client_t *client;
    int rc;
} pc_client_init_result_t;

PC_EXPORT size_t pc_client_size(void);
PC_EXPORT pc_client_init_result_t pc_client_init(void* ex_data, const pc_client_config_t* config);
PC_EXPORT int pc_client_connect(pc_client_t* client, const char* host, int port, const char* handshake_opts);
PC_EXPORT int pc_client_disconnect(pc_client_t* client);
PC_EXPORT int pc_client_cleanup(pc_client_t* client);
PC_EXPORT int pc_client_poll(pc_client_t* client);

/**
 * pc_client_t getters
 */
PC_EXPORT void* pc_client_ex_data(const pc_client_t* client);
PC_EXPORT const pc_client_config_t* pc_client_config(const pc_client_t* client);
PC_EXPORT int pc_client_state(pc_client_t* client);
PC_EXPORT int pc_client_conn_quality(pc_client_t* client);
PC_EXPORT void* pc_client_trans_data(pc_client_t* client);
PC_EXPORT const char *pc_client_serializer(pc_client_t *client);

// Free serializer
PC_EXPORT void pc_client_free_serializer(const char *serializer);

/**
 * Event
 */

/**
 * event handler callback and event types
 *
 * arg1 and arg2 are significant for the following events:
 *   PC_EV_USER_DEFINED_PUSH - arg1 as push route, arg2 as push msg
 *   PC_EV_CONNECT_ERROR - arg1 as short error description
 *   PC_EV_CONNECT_FAILED - arg1 as short reason description
 *   PC_EV_UNEXPECTED_DISCONNECT - arg1 as short reason description
 *   PC_EV_PROTO_ERROR - arg1 as short reason description
 *   PC_EV_RECONNECT_FAILED - arg1 as short reason description
 *
 * For other events, arg1 and arg2 will be set to NULL.
 */
typedef void (*pc_event_cb_t)(pc_client_t *client, int ev_type, void* ex_data,
                              const char* arg1, const char* arg2);

#define PC_EV_USER_DEFINED_PUSH 0
#define PC_EV_CONNECTED 1
#define PC_EV_CONNECT_ERROR 2
#define PC_EV_CONNECT_FAILED 3
#define PC_EV_DISCONNECT 4
#define PC_EV_KICKED_BY_SERVER 5
#define PC_EV_UNEXPECTED_DISCONNECT 6
#define PC_EV_PROTO_ERROR 7
#define PC_EV_RECONNECT_FAILED 8
#define PC_EV_RECONNECT_STARTED 9
#define PC_EV_COUNT 10

#define PC_EV_INVALID_HANDLER_ID -1

PC_EXPORT int pc_client_add_ev_handler(pc_client_t* client, pc_event_cb_t cb,
        void* ex_data, void (*destructor)(void* ex_data));
PC_EXPORT int pc_client_rm_ev_handler(pc_client_t* client, int id);

/**
 * Error
 */
typedef struct {
    int code;
    pc_buf_t payload;
    int uv_code;
} pc_error_t;

/**
 * Request
 */

typedef void (*pc_request_success_cb_t)(const pc_request_t* req, const pc_buf_t *resp);
typedef void (*pc_request_error_cb_t)(const pc_request_t* req, const pc_error_t *error);

/**
 * pc_request_t getters.
 *
 * All the getters should be called in pc_request_cb_t to access read-only
 * properties of the current pc_request_t.
 *
 * User should not hold any references to pc_request_t.
 */
PC_EXPORT pc_client_t* pc_request_client(const pc_request_t* req);
PC_EXPORT const char* pc_request_route(const pc_request_t* req);
PC_EXPORT const char* pc_request_msg(const pc_request_t* req);
PC_EXPORT int pc_request_timeout(const pc_request_t* req);
PC_EXPORT void* pc_request_ex_data(const pc_request_t* req);

/**
 * Initiate a request.
 */
PC_EXPORT int pc_string_request_with_timeout(pc_client_t* client, const char* route,
                                             const char *str, void* ex_data, int timeout, 
                                             pc_request_success_cb_t success_cb, pc_request_error_cb_t error_cb);

PC_EXPORT int pc_binary_request_with_timeout(pc_client_t* client, const char* route,
                                             uint8_t *data, int64_t len, void* ex_data, int timeout,
                                             pc_request_success_cb_t success_cb, pc_request_error_cb_t error_cb);

/**
 * Notify
 */

typedef void (*pc_notify_error_cb_t)(const pc_notify_t* req, const pc_error_t *error);

/**
 * pc_notify_t getters.
 *
 * All the getters should be called in pc_notify_cb_t to access read-only 
 * properties of the current pc_notify_t. 
 *
 * User should not hold any references to pc_notify_t.
 */
PC_EXPORT pc_client_t* pc_notify_client(const pc_notify_t* notify);
PC_EXPORT const char* pc_notify_route(const pc_notify_t* notify);
PC_EXPORT const pc_buf_t *pc_notify_msg(const pc_notify_t* notify);
PC_EXPORT int pc_notify_timeout(const pc_notify_t* notify);
PC_EXPORT void* pc_notify_ex_data(const pc_notify_t* notify);

/**
 * Initiate a notify.
 */
PC_EXPORT int pc_binary_notify_with_timeout(pc_client_t* client, const char* route, uint8_t *data, int64_t len,
                                            void* ex_data, int timeout, pc_notify_error_cb_t cb);
PC_EXPORT int pc_string_notify_with_timeout(pc_client_t* client, const char* route, const char *str, 
                                            void* ex_data, int timeout, pc_notify_error_cb_t cb);

/**
 * Utilities
 */
PC_EXPORT const char* pc_client_state_str(int state);
PC_EXPORT const char* pc_client_ev_str(int ev_type);
PC_EXPORT const char* pc_client_rc_str(int rc);

/**
 * set ca file for tls transports
 */

#if !defined(PC_NO_UV_TCP_TRANS) && !defined(PC_NO_UV_TLS_TRANS)

/**
 * Sets the certificates that the client trust in order to verify the server for TLS communication.
 * Returns `PC_RC_OK` in case of success otherwise `PC_RC_ERROR`. For more information, see
 * https://www.openssl.org/docs/man1.0.2/ssl/SSL_CTX_load_verify_locations.html
 */
PC_EXPORT int tr_uv_tls_set_ca_file(const char* ca_file, const char* ca_path);

#endif /* uv_tls */

/**
 * Macro implementation
 */
#define pc_lib_version() PC_VERSION_NUM
#define pc_lib_version_str() PC_VERSION_STR 

#ifdef __cplusplus
}
#endif

#endif /* PC_PITAYA_H */
