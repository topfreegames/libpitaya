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

#include "test_common.h"

static pc_client_t* g_client = NULL;

static bool g_event_cb_called = false;
static bool g_request_cb_called = false;
static bool g_notify_cb_called = false;

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

static inline void
reset_cb_flags()
{
    g_event_cb_called = false;
    g_request_cb_called = false;
    g_notify_cb_called = false;
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

    assert_int_equal(pc_client_connect(g_client, "127.0.0.1", TCP_PORT, NULL), PC_RC_OK);
    SLEEP_SECONDS(1);
    assert_false(called);

    assert_int_equal(pc_client_poll(NULL), PC_RC_INVALID_ARG);
    assert_int_equal(pc_client_poll(g_client), PC_RC_OK);

    assert_true(called);

    assert_int_equal(pc_client_rm_ev_handler(g_client, handler_id), PC_RC_OK);
    assert_int_equal(pc_client_cleanup(g_client), PC_RC_OK);
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

    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    config.transport_name = PC_TR_NAME_UV_TCP;

    assert_int_equal(pc_client_init(g_client, NULL, &config), PC_RC_OK);

    int handler_id = pc_client_add_ev_handler(g_client, event_cb, EV_HANDLER_EX, NULL);
    assert_int_not_equal(handler_id, PC_EV_INVALID_HANDLER_ID);

    assert_int_equal(pc_client_connect(g_client, "127.0.0.1", TCP_PORT, NULL), PC_RC_OK);

    SLEEP_SECONDS(1);
    assert_int_equal(pc_request_with_timeout(g_client, REQ_ROUTE, REQ_MSG, REQ_EX, REQ_TIMEOUT, request_cb, NULL), PC_RC_OK);
    assert_int_equal(pc_notify_with_timeout(g_client, NOTI_ROUTE, NOTI_MSG, NOTI_EX, NOTI_TIMEOUT, notify_cb), PC_RC_OK);
    SLEEP_SECONDS(1);

    assert_true(g_event_cb_called);
    assert_true(g_notify_cb_called);
    assert_true(g_request_cb_called);
    reset_cb_flags();

    assert_int_equal(pc_client_disconnect(g_client), PC_RC_OK);
    assert_int_equal(pc_client_rm_ev_handler(g_client, handler_id), PC_RC_OK);
    assert_int_equal(pc_client_cleanup(g_client), PC_RC_OK);
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
    config.transport_name = PC_TR_NAME_UV_TCP;

    assert_int_equal(pc_client_connect(g_client, "127.0.0.1", TCP_PORT, NULL), PC_RC_INVALID_STATE);

    // Initializing the client
    assert_int_equal(pc_client_init(g_client, NULL, &config), PC_RC_OK);

    bool cb_called = false;
    const int handler_id = pc_client_add_ev_handler(g_client, invalid_ev_cb, &cb_called, NULL);
    assert_int_not_equal(handler_id, PC_EV_INVALID_HANDLER_ID);

    // Invalid arguments for pc_client_connect
    assert_int_equal(pc_client_connect(NULL, "127.0.0.1", TCP_PORT, NULL), PC_RC_INVALID_ARG);
    assert_int_equal(pc_client_connect(g_client, NULL, TCP_PORT, NULL), PC_RC_INVALID_ARG);
    assert_int_equal(pc_client_connect(g_client, "127.0.0.1", -1, NULL), PC_RC_INVALID_ARG);
    assert_int_equal(pc_client_connect(g_client, "127.0.0.1", (1 << 16), NULL), PC_RC_INVALID_ARG);

    // Invalid JSON errors
    const char *invalid_handshake_opts = "wqdojh";
    const int invalid_port = TCP_PORT + 50;
    assert_int_equal(pc_client_connect(g_client, "127.0.0.1", TCP_PORT, invalid_handshake_opts), PC_RC_INVALID_JSON);

    const char *valid_handshake_opts = "{ \"oi\": 2 }";
    assert_int_equal(pc_client_connect(g_client, "127.0.0.1", invalid_port, valid_handshake_opts), PC_RC_OK);
    SLEEP_SECONDS(2);

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

