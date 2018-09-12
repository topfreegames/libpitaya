#ifndef PITAYA_UNITY_H
#define PITAYA_UNITY_H

#include <stdint.h>
#include <stdbool.h>

#include "pitaya.h"
#include "pitaya_unity.h"

#ifdef _WIN32
#define CS_PITAYA_EXPORT __declspec(dllexport)
#else
#define CS_PITAYA_EXPORT
#endif

typedef void (*pc_unity_request_success_callback_t)(pc_client_t* client, uint32_t cbid, const pc_buf_t* resp);
typedef void (*pc_unity_request_error_callback_t)(pc_client_t* client, uint32_t cbid, const pc_error_t* error);
typedef void (*pc_unity_assert_t)(const char *e, const char *file, int line);

CS_PITAYA_EXPORT int pc_unity_init_log(const char* path);

CS_PITAYA_EXPORT void pc_unity_native_log(const char* msg);

CS_PITAYA_EXPORT void pc_unity_lib_init(int log_level, const char* ca_file, const char* ca_path, pc_unity_assert_t custom_assert, const char* platform, const char* build_number,const char* version);

CS_PITAYA_EXPORT pc_client_t *pc_unity_create(bool enable_tls, bool enable_poll, bool enable_reconnect);

CS_PITAYA_EXPORT void pc_unity_destroy(pc_client_t *client);

CS_PITAYA_EXPORT int pc_unity_request(pc_client_t *client, const char *route, const char *msg,
                                      uint32_t cbid, int timeout, pc_unity_request_success_callback_t cb, 
                                      pc_unity_request_error_callback_t error_cb);

CS_PITAYA_EXPORT int pc_unity_binary_request(pc_client_t *client, const char *route, uint8_t *data,
                                      int64_t len, uint32_t cbid, int timeout, pc_unity_request_success_callback_t cb,
                                      pc_unity_request_error_callback_t error_cb);

#endif // PITAYA_UNITY_H
