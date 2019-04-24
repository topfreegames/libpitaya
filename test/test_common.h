/*
 * File defining common macros that are going to be used across multiple test files.
 */

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#define MUNIT_ENABLE_ASSERT_ALIASES
#include <munit.h>
#include "pc_mutex.h"

// Macro to sign to the compiler that the parameter is unused, avoiding a warning.
#define Unused(x) ((void)(x))

// QUESTION: Do we need ifdefs for windows since we do not use them?
#ifdef _WIN32
#include <windows.h>
#define SLEEP_SECONDS(x) Sleep(x * 1000);
#else
#include <unistd.h>
#define SLEEP_SECONDS(x) sleep(x)
#endif

#define ArrayCount(arr) (sizeof(arr)/sizeof((arr)[0]))

#define LOCALHOST "127.0.0.1"
#define PITAYA_SERVER_URL "libpitaya-tests.tfgco.com"

#define REQ_ROUTE "connector.getsessiondata"
#define REQ_MSG "{\"name\": \"test\"}"
#define REQ_EX ((void*)0x22)
#define REQ_TIMEOUT 10

#define EMPTY_RESP "{\"Data\":{}}"
#define SUCCESS_RESP "{\"Code\":200,\"Msg\":\"success\"}"

#define NOTI_ROUTE "test.testHandler.notify"
#define NOTI_MSG "{\"content\": \"test content\"}"
#define NOTI_EX ((void*)0x33)
#define NOTI_TIMEOUT 30

#define CRT "fixtures/myCA.pem"
#define INCORRECT_CRT "fixtures/ca_incorrect.crt"
#define SERVER_CRT "fixtures/server/pitaya.crt"

typedef struct {
    int tcp_port;
    int tls_port;
} test_server_t;

// Mock servers
static test_server_t g_disconnect_mock_server = {4000, 4001};
static test_server_t g_compression_mock_server = {4100, 4101};
static test_server_t g_kick_mock_server = {4200, 4201};
static test_server_t g_timeout_mock_server = {4300, 4301};
static test_server_t g_destroy_socket_mock_server = {4400, 4401};
static test_server_t g_kill_client_mock_server = {4500, 4501};
// Pitaya servers
static test_server_t g_test_server = {3251, 3252};
static test_server_t g_test_protobuf_server = {3351, 3352};

#define EV_HANDLER_EX ((void*)0x44)
#define SERVER_PUSH "onPush"

#define PC_CLIENT_CONFIG_TEST                    \
{                                               \
    30, /* conn_timeout */                      \
    false, /* enable_reconn */                  \
    0, /* reconn_max_retry */ \
    0, /* reconn_delay */                   \
    0, /* reconn_delay_max */              \
    0, /* reconn_exp_backoff */             \
    0, /* enable_polling */                 \
    NULL, /* local_storage_cb */            \
    NULL, /* ls_ex_data */                  \
    PC_TR_NAME_UV_TCP, /* transport_name */ \
    0 /* disable_compression */             \
}

#endif // TEST_COMMON_H
