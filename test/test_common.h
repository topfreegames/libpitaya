/*
 * File defining common macros that are going to be used across multiple test files.
 */

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#define MUNIT_ENABLE_ASSERT_ALIASES
#include <munit.h>

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

#define LOCALHOST "127.0.0.1"

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

typedef struct {
    int tcp_port;
    int tls_port;
} test_server_t;

static test_server_t g_disconnect_mock_server = {4000, 4001};
static test_server_t g_compression_mock_server = {4100, 4101};
static test_server_t g_kick_mock_server = {4200, 4201};
static test_server_t g_test_server = {3251, 3252};

#define ArrayCount(arr) (sizeof(arr)/sizeof((arr)[0]))

#define EV_HANDLER_EX ((void*)0x44)
#define SERVER_PUSH "onPush"

#define PC_CLIENT_CONFIG_TEST                         \
{                                                     \
    30, /* conn_timeout */                            \
    false, /* enable_reconn */                            \
    0, /* reconn_max_retry */           \
    0, /* reconn_delay */                             \
    0, /* reconn_delay_max */                        \
    0, /* reconn_exp_backoff */                       \
    0, /* enable_polling */                           \
    NULL, /* local_storage_cb */                      \
    NULL, /* ls_ex_data */                               \
    PC_TR_NAME_UV_TCP /* transport_name */            \
}

static void
quiet_log(int level, const char *msg, ...)
{
    // Use an empty log to avoid messing up the output of the tests.
    // TODO: maybe print only logs of a certain level?
}

static int
setup_pc_lib(void **state)
{
    pc_lib_init(quiet_log, NULL, NULL, NULL, NULL);
//    pc_lib_init(NULL, NULL, NULL, NULL, NULL);
    return 0;
}

static int
teardown_pc_lib(void **state)
{
    pc_lib_cleanup();
    return 0;
}

#endif // TEST_COMMON_H
