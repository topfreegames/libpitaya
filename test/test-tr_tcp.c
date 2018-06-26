/**
 * Copyright (c) 2014,2015 NetEase, Inc. and other Pomelo contributors
 * MIT Licensed.
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>
#include <pomelo.h>
#include <pomelo_trans.h>

#include "test_common.h"

static pc_client_t* g_client = NULL;

static void
request_cb(const pc_request_t* req, const pc_buf_t *resp)
{
    bool *called = (bool*)pc_request_ex_data(req);
    *called = true;

    assert_not_null(resp);
    assert_ptr_equal(pc_request_client(req), g_client);
    assert_string_equal(pc_request_route(req), REQ_ROUTE);
    assert_string_equal(pc_request_msg(req), REQ_MSG);
    assert_int(pc_request_timeout(req), ==, REQ_TIMEOUT);
}

static MunitResult
test_request_callback(const MunitParameter params[], void *data)
{
    Unused(params);
    Unused(data);

    bool called = false;
    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    config.transport_name = PC_TR_NAME_UV_TCP;

    pc_client_init_result_t res = pc_client_init(NULL, &config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);

    assert_int(pc_client_connect(g_client, LOCALHOST, g_test_server.tcp_port, NULL), ==, PC_RC_OK);

    SLEEP_SECONDS(1);
    assert_int(pc_string_request_with_timeout(g_client, REQ_ROUTE, REQ_MSG, &called, REQ_TIMEOUT, request_cb, NULL), ==, PC_RC_OK);
    SLEEP_SECONDS(1);

    assert_true(called);

    assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
    SLEEP_SECONDS(1);
    assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

    return MUNIT_OK;
}

static int EV_ORDER[] = {
    PC_EV_CONNECTED, PC_EV_DISCONNECT,
};

static void
event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    Unused(client); Unused(arg1); Unused(arg2);
    int *num_called = (int*)ex_data;
    assert_int(ev_type, ==, EV_ORDER[*num_called]);
    (*num_called)++;
}

static MunitResult
test_event_callback(const MunitParameter params[], void *data)
{
    Unused(params);
    Unused(data);

    int num_called = 0;
    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    config.transport_name = PC_TR_NAME_UV_TCP;

    pc_client_init_result_t res = pc_client_init(NULL, &config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);

    int handler_id = pc_client_add_ev_handler(g_client, event_cb, &num_called, NULL);
    assert_int(handler_id, !=, PC_EV_INVALID_HANDLER_ID);

    assert_int(pc_client_connect(g_client, LOCALHOST, g_test_server.tcp_port, NULL), ==, PC_RC_OK);
    SLEEP_SECONDS(1);

    assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
    SLEEP_SECONDS(1);

    assert_int(num_called, ==, ArrayCount(EV_ORDER));
    assert_int(pc_client_rm_ev_handler(g_client, handler_id), ==, PC_RC_OK);
    assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

    return MUNIT_OK;
}

static void
invalid_ev_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    Unused(client); Unused(arg2);
    bool *called = (bool*)ex_data;
    *called = true;

    assert(ev_type == PC_EV_CONNECT_ERROR || ev_type == PC_EV_CONNECT_FAILED);
    bool is_connect_error = strcmp("Connect Error", arg1) == 0;
    bool is_reconn_disabled = strcmp("Reconn Disabled", arg1) == 0;
    assert(is_connect_error || is_reconn_disabled);
}

static MunitResult
test_connect_errors(const MunitParameter params[], void *data)
{
    Unused(data); Unused(params);
    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    config.transport_name = PC_TR_NAME_UV_TCP;

    // Initializing the client
    pc_client_init_result_t res = pc_client_init(NULL, &config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);

    bool cb_called = false;
    const int handler_id = pc_client_add_ev_handler(g_client, invalid_ev_cb, &cb_called, NULL);
    assert_int(handler_id, !=, PC_EV_INVALID_HANDLER_ID);

    // Invalid arguments for pc_client_connect
    assert_int(pc_client_connect(NULL, LOCALHOST, g_test_server.tcp_port, NULL), ==, PC_RC_INVALID_ARG);
    assert_int(pc_client_connect(g_client, NULL, g_test_server.tcp_port, NULL), ==, PC_RC_INVALID_ARG);
    assert_int(pc_client_connect(g_client, LOCALHOST, -1, NULL), ==, PC_RC_INVALID_ARG);
    assert_int(pc_client_connect(g_client, LOCALHOST, (1 << 16), NULL), ==, PC_RC_INVALID_ARG);

    // Invalid JSON errors
    const char *invalid_handshake_opts = "wqdojh";
    const int invalid_port = g_test_server.tcp_port + 50;
    assert_int(pc_client_connect(g_client, LOCALHOST, g_test_server.tcp_port, invalid_handshake_opts), ==, PC_RC_INVALID_JSON);

    const char *valid_handshake_opts = "{ \"oi\": 2 }";
    assert_int(pc_client_connect(g_client, LOCALHOST, invalid_port, valid_handshake_opts), ==, PC_RC_OK);
    SLEEP_SECONDS(2);

    assert_true(cb_called);
    assert_int(pc_client_rm_ev_handler(g_client, handler_id), ==, PC_RC_OK);
    assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

    return MUNIT_OK;
}

static MunitResult
test_invalid_disconnect(const MunitParameter params[], void *data)
{
    Unused(data); Unused(params);
    assert_int(pc_client_disconnect(NULL), ==, PC_RC_INVALID_ARG);

    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    pc_client_init_result_t res = pc_client_init(NULL, &config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);

    assert_int(pc_client_disconnect(g_client), ==, PC_RC_INVALID_STATE);

    return MUNIT_OK;
}

static MunitTest tests[] = {
    {"/invalid_disconnect", test_invalid_disconnect, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/event_cb", test_event_callback, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/request_cb", test_request_callback, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/connect_errors", test_connect_errors, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
};

const MunitSuite tcp_suite = {
    "/tcp", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};
