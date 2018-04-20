/**
 * Copyright (c) 2014,2015 NetEase, Inc. and other Pomelo contributors
 * MIT Licensed.
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <pomelo.h>
#include <pomelo_trans.h>

#include <setjmp.h>
#include <stddef.h>
#include <cmocka.h>

// Macro to sign to the compiler that the parameter is unused, avoiding a warning.
#define Unused(x) ((void)(x))

#define REQ_ROUTE "connector.getsessiondata"
#define REQ_MSG "{\"name\": \"test\"}"
#define REQ_EX ((void*)0x22)
#define REQ_TIMEOUT 10

#define NOTI_ROUTE "test.testHandler.notify"
#define NOTI_MSG "{\"content\": \"test content\"}"
#define NOTI_EX ((void*)0x33)
#define NOTI_TIMEOUT 30

#define EV_HANDLER_EX ((void*)0x44)
#define SERVER_PUSH "onPush"

#define PORT 3251

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


// QUESTION: Do we need ifdefs for windows since we do not use them?
#ifdef _WIN32
#include <windows.h>
#define SLEEP(x) Sleep(x * 1000);
#else
#include <unistd.h>
#define SLEEP(x) sleep(x)
#endif

static pc_client_t* g_client = NULL;

static void quiet_log(int level, const char *msg, ...)
{
    // Use an empty log to avoid messing up the output of the tests.
    // TODO: maybe print only logs of a certain level?
}

static int
setup_pc_lib(void **state)
{
    pc_lib_init(quiet_log, NULL, NULL, NULL);
    //pc_lib_init(NULL, NULL, NULL, NULL);
    return 0;
}

static int
teardown_pc_lib(void **state)
{
    pc_lib_cleanup();
    return 0;
}

static int
setup_global_client(void **state)
{
    // NOTE: use calloc in order to avoid one of the issues with the api.
    // see `issues.md`.
    g_client = calloc(1, pc_client_size());
    if (!g_client) {
        return -1;
    }
    return 0;
}

static int
teardown_global_client(void **state)
{
    free(g_client);
    g_client = NULL;
    return 0;
}

static bool g_event_cb_called = false;
static bool g_request_cb_called = false;
static bool g_notify_cb_called = false;
static bool g_local_storage_cb_called = false;

static inline void
reset_cb_flags()
{
    g_event_cb_called = false;
    g_request_cb_called = false;
    g_notify_cb_called = false;
    g_local_storage_cb_called = false;
}

static void
empty_event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    // This callback should not be called, therefore called should always be false.
    Unused(arg1); Unused(arg2); Unused(client); Unused(ev_type);
    bool *called = (bool*)ex_data;
    *called = true;
}

static void
test_client_polling()
{
    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    config.enable_polling = true;

    bool called = false;

    assert_int_equal(pc_client_init(g_client, NULL, &config), PC_RC_OK);
    int handler_id = pc_client_add_ev_handler(g_client, empty_event_cb, &called, NULL);
    assert_int_not_equal(handler_id, PC_EV_INVALID_HANDLER_ID);

    assert_int_equal(pc_client_connect(g_client, "127.0.0.1", PORT, NULL), PC_RC_OK);
    SLEEP(1);
    assert_false(called);

    assert_int_equal(pc_client_poll(NULL), PC_RC_INVALID_ARG);
    assert_int_equal(pc_client_poll(g_client), PC_RC_OK);

    assert_true(called);

    assert_int_equal(pc_client_rm_ev_handler(g_client, handler_id), PC_RC_OK);
    assert_int_equal(pc_client_cleanup(g_client), PC_RC_OK);
}

static int
local_storage_cb(pc_local_storage_op_t op, char* data, size_t* len, void* ex_data)
{
    // 0 - success, -1 - fail
    g_local_storage_cb_called = true;
    char buf[1024];
    size_t length;
    size_t offset;
    FILE* f;

    if (op == PC_LOCAL_STORAGE_OP_WRITE) {
      f = fopen("pomelo.dat", "w");
      if (!f) {
          return -1;
      }
      fwrite(data, 1, *len, f);
      fclose(f);
      return 0;

    } else {
        f = fopen("pomelo.dat", "r");
        if (!f) {
            *len = 0;
            return -1;
        }
        *len = 0;
        offset = 0;

        while((length = fread(buf, 1, 1024, f))) {
            *len += length;
            if (data) {
                memcpy(data + offset, buf, length);
            }
            offset += length;
        }

        fclose(f);

        return 0;
    }
}

static void
event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    g_event_cb_called = true;
    assert_ptr_equal(ex_data, EV_HANDLER_EX);
}

static void
request_cb(const pc_request_t* req, int rc, const char* resp)
{
    g_request_cb_called = true;
    assert_int_equal(rc, PC_RC_OK);
    assert_non_null(resp);
    assert_ptr_equal(pc_request_client(req), g_client);
    assert_string_equal(pc_request_route(req), REQ_ROUTE);
    assert_string_equal(pc_request_msg(req), REQ_MSG);
    assert_ptr_equal(pc_request_ex_data(req), REQ_EX);
    assert_int_equal(pc_request_timeout(req), REQ_TIMEOUT);
}

static void
notify_cb(const pc_notify_t* noti, int rc)
{
    g_notify_cb_called = true;
    assert_int_equal(rc, PC_RC_OK);
    assert_ptr_equal(pc_notify_client(noti), g_client);
    assert_string_equal(pc_notify_route(noti), NOTI_ROUTE);
    assert_string_equal(pc_notify_msg(noti), NOTI_MSG);
    assert_ptr_equal(pc_notify_ex_data(noti), NOTI_EX);
    assert_int_equal(pc_notify_timeout(noti), NOTI_TIMEOUT);
}

static void
test_event_notify_and_request_callbacks(void **state)
{
    Unused(state);

    {
        pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
        config.local_storage_cb = local_storage_cb;
        config.transport_name = PC_TR_NAME_UV_TCP;

        assert_int_equal(pc_client_init(g_client, NULL, &config), PC_RC_OK);

        int handler_id = pc_client_add_ev_handler(g_client, event_cb, EV_HANDLER_EX, NULL);
        assert_int_not_equal(handler_id, PC_EV_INVALID_HANDLER_ID);

        assert_int_equal(pc_client_connect(g_client, "127.0.0.1", PORT, NULL), PC_RC_OK);

        SLEEP(1);
        assert_int_equal(pc_request_with_timeout(g_client, REQ_ROUTE, REQ_MSG, REQ_EX, REQ_TIMEOUT, request_cb), PC_RC_OK);
        assert_int_equal(pc_notify_with_timeout(g_client, NOTI_ROUTE, NOTI_MSG, NOTI_EX, NOTI_TIMEOUT, notify_cb), PC_RC_OK);
        SLEEP(1);

        assert_true(g_event_cb_called);
        assert_true(g_notify_cb_called);
        assert_true(g_request_cb_called);
        assert_true(g_local_storage_cb_called);
        reset_cb_flags();

        assert_int_equal(pc_client_disconnect(g_client), PC_RC_OK);
        assert_int_equal(pc_client_rm_ev_handler(g_client, handler_id), PC_RC_OK);
        assert_int_equal(pc_client_cleanup(g_client), PC_RC_OK);
    }
    // TODO: consider adding these assertions when the server supports tls.
#if 0
    {
        pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
        config.local_storage_cb = local_storage_cb;
        config.transport_name = PC_TR_NAME_UV_TLS;

        assert_int_equal(pc_client_init(g_client, NULL, &config), PC_RC_OK);

        int handler_id = pc_client_add_ev_handler(g_client, event_cb, EV_HANDLER_EX, NULL);
        assert_int_not_equal(handler_id, PC_EV_INVALID_HANDLER_ID);

        assert_int_equal(pc_client_connect(g_client, "127.0.0.1", PORT, NULL), PC_RC_OK);

        SLEEP(1);
        assert_int_equal(pc_request_with_timeout(g_client, REQ_ROUTE, REQ_MSG, REQ_EX, REQ_TIMEOUT, request_cb), PC_RC_OK);
        assert_int_equal(pc_notify_with_timeout(g_client, NOTI_ROUTE, NOTI_MSG, NOTI_EX, NOTI_TIMEOUT, notify_cb), PC_RC_OK);
        SLEEP(2);

        assert_true(g_event_cb_called);
        assert_true(g_notify_cb_called);
        assert_true(g_request_cb_called);
        assert_true(g_local_storage_cb_called);
        reset_cb_flags();

        assert_int_equal(pc_client_disconnect(g_client), PC_RC_OK);
        assert_int_equal(pc_client_rm_ev_handler(g_client, handler_id), PC_RC_OK);
        assert_int_equal(pc_client_cleanup(g_client), PC_RC_OK);
    }
#endif
}

static void
invalid_ev_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    bool *called = (bool*)ex_data;
    *called = true;

    assert_true(ev_type == PC_EV_CONNECT_ERROR || ev_type == PC_EV_CONNECT_FAILED);
    assert_true(strcmp("Connect Error", arg1) == 0 || strcmp("Reconn Disabled", arg1) == 0);
}

static void
test_connect_errors(void **state)
{
    Unused(state);

    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    config.local_storage_cb = local_storage_cb;
    config.transport_name = PC_TR_NAME_UV_TCP;

    assert_int_equal(pc_client_connect(g_client, "127.0.0.1", PORT, NULL), PC_RC_INVALID_STATE);

    // Initializing the client
    assert_int_equal(pc_client_init(g_client, NULL, &config), PC_RC_OK);

    bool cb_called = false;
    const int handler_id = pc_client_add_ev_handler(g_client, invalid_ev_cb, &cb_called, NULL);
    assert_int_not_equal(handler_id, PC_EV_INVALID_HANDLER_ID);

    // Invalid arguments for pc_client_connect
    assert_int_equal(pc_client_connect(NULL, "127.0.0.1", PORT, NULL), PC_RC_INVALID_ARG);
    assert_int_equal(pc_client_connect(g_client, NULL, PORT, NULL), PC_RC_INVALID_ARG);
    assert_int_equal(pc_client_connect(g_client, "127.0.0.1", -1, NULL), PC_RC_INVALID_ARG);
    assert_int_equal(pc_client_connect(g_client, "127.0.0.1", (1 << 16), NULL), PC_RC_INVALID_ARG);

    // Invalid JSON errors
    const char *invalid_handshake_opts = "wqdojh";
    const int invalid_port = PORT + 50;
    assert_int_equal(pc_client_connect(g_client, "127.0.0.1", PORT, invalid_handshake_opts), PC_RC_INVALID_JSON);

    const char *valid_handshake_opts = "{ \"oi\": 2 }";
    assert_int_equal(pc_client_connect(g_client, "127.0.0.1", invalid_port, valid_handshake_opts), PC_RC_OK);
    SLEEP(2);

    assert_true(cb_called);

    assert_int_equal(pc_client_rm_ev_handler(g_client, handler_id), PC_RC_OK);
    assert_int_equal(pc_client_cleanup(g_client), PC_RC_OK);
}

static void
test_invalid_disconnect(void **state)
{
    Unused(state);

    assert_int_equal(pc_client_disconnect(NULL), PC_RC_INVALID_ARG);
    assert_int_equal(pc_client_disconnect(g_client), PC_RC_INVALID_STATE);

    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    assert_int_equal(pc_client_init(g_client, NULL, &config), PC_RC_OK);

    assert_int_equal(pc_client_disconnect(g_client), PC_RC_INVALID_STATE);
}

static void
test_pc_client_getters(void **state)
{
    Unused(state);

    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;

    config.local_storage_cb = local_storage_cb;

#define RANDOM_PTR (void*)0xdeadbeef

    pc_client_init(g_client, RANDOM_PTR, &config);

    // external data
    assert_ptr_equal(pc_client_ex_data(g_client), RANDOM_PTR);
    // state
    assert_int_equal(pc_client_state(g_client), PC_ST_INITED);
    // config
    assert_memory_equal(pc_client_config(g_client), &config, sizeof(pc_client_config_t));
    // conn quality
    assert_int_equal(pc_client_conn_quality(NULL), PC_RC_INVALID_ARG);
    // TODO: Testing connection quality does not seem to make a lot of sense.
    // trans data
    assert_null(pc_client_trans_data(NULL));
    // TODO: Testing the internal data depends on the transport type.

    pc_client_cleanup(g_client);
    assert_int_equal(pc_client_state(g_client), PC_ST_NOT_INITED);

#undef RANDOM_PTR
}

static void
test_pc_client_incorrectly_initialized(void **state)
{
    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;

    // Return invalid argument when NULL is passed as a client pointer.
    int rc = pc_client_init(NULL, NULL, NULL);
    assert_int_equal(rc, PC_RC_INVALID_ARG);

    pc_client_cleanup(g_client);
}

int main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_pc_client_getters, setup_global_client, teardown_global_client),
        cmocka_unit_test_setup_teardown(test_client_polling, setup_global_client, teardown_global_client),
        cmocka_unit_test_setup_teardown(test_event_notify_and_request_callbacks, setup_global_client, teardown_global_client),
        cmocka_unit_test_setup_teardown(test_connect_errors, setup_global_client, teardown_global_client),
        cmocka_unit_test_setup_teardown(test_invalid_disconnect, setup_global_client, teardown_global_client),
    };
    return cmocka_run_group_tests(tests, setup_pc_lib, teardown_pc_lib);
}

