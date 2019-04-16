/**
 * Copyright (c) 2014,2015 NetEase, Inc. and other Pomelo contributors
 * MIT Licensed.
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>
#include <pitaya.h>
#include <pitaya_trans.h>

#include "test_common.h"
#include "flag.h"

static pc_client_t* g_client = NULL;

static void
request_cb(const pc_request_t* req, const pc_buf_t *resp)
{
    flag_t *flag = (flag_t*)pc_request_ex_data(req);

    char *str = calloc(resp->len+1, 1);
    assert_not_null(str);
    memcpy(str, resp->base, resp->len);

    assert_not_null(resp);

    char *first_part = "{\"sys\":{\"platform\":\"mac\",\"libVersion\":\"";
    char *last_part = "\",\"clientBuildNumber\":\"20\",\"clientVersion\":\"2.1\"},\"user\":{\"age\":30}}";
    char *expected_json = calloc(strlen(pc_lib_version_str()) + strlen(first_part) + strlen(last_part) + 1, 1);
    assert_not_null(expected_json);

    strcat(expected_json, first_part);
    strcat(expected_json, pc_lib_version_str());
    strcat(expected_json, last_part);

    assert_string_equal(str, expected_json);
    assert_ptr_equal(pc_request_client(req), g_client);
    assert_string_equal(pc_request_route(req), "connector.gethandshakedata");
    assert_string_equal(pc_request_msg(req), REQ_MSG);
    assert_int(pc_request_timeout(req), ==, REQ_TIMEOUT);

    free(expected_json);
    free(str);
    flag_set(flag);
}

static void
request_event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    Unused(client); Unused(arg1); Unused(arg2);
    flag_t *flag = (flag_t*)ex_data;

    static int num_called = 0;
    const int EVENTS[] = {
        PC_EV_CONNECTED, PC_EV_DISCONNECT
    };

    assert_int(ev_type, ==, EVENTS[num_called++]);
    assert_int(num_called, <=, ArrayCount(EVENTS));
    flag_set(flag);
}

static MunitResult
test_request_cb(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);
    flag_t flag = flag_make();

    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    config.transport_name = PC_TR_NAME_UV_TCP;

    pc_client_init_result_t res = pc_client_init(NULL, &config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);

    pc_client_add_ev_handler(g_client, request_event_cb, &flag, NULL);

    const char *handshake_opts = "{\"age\":30}";
    assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, g_test_server.tcp_port, handshake_opts), ==, PC_RC_OK);
    assert_int(flag_wait(&flag, 60), ==, FLAG_SET);
    assert_int(pc_string_request_with_timeout(g_client, "connector.gethandshakedata", REQ_MSG, &flag, REQ_TIMEOUT, request_cb, NULL), ==, PC_RC_OK);
    assert_int(flag_wait(&flag, 60), ==, FLAG_SET);
    assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
    assert_int(flag_wait(&flag, 60), ==, FLAG_SET);
    assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

    flag_cleanup(&flag);
    return MUNIT_OK;
}

static int CB_EVENTS[] = {
    PC_EV_CONNECTED, PC_EV_DISCONNECT,
};

static void
event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    Unused(client); Unused(arg1); Unused(arg2);
    flag_t *flag = (flag_t*)ex_data;
    int num_called = flag_get_num_called(flag);
    assert_int(ev_type, ==, CB_EVENTS[num_called]);
    assert_int(num_called, <, ArrayCount(CB_EVENTS));
    flag_set(flag);
}

static MunitResult
test_event_callback(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);
    flag_t flag = flag_make();

    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    config.transport_name = PC_TR_NAME_UV_TCP;

    pc_client_init_result_t res = pc_client_init(NULL, &config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);

    pc_client_add_ev_handler(g_client, event_cb, &flag, NULL);

    assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, g_test_server.tcp_port, NULL), ==, PC_RC_OK);
    assert_int(flag_wait(&flag, 60), ==, FLAG_SET);
    assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
    assert_int(flag_wait(&flag, 60), ==, FLAG_SET);
    assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);
    flag_cleanup(&flag);
    return MUNIT_OK;
}

static void
invalid_ev_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    Unused(client); Unused(arg2);
    flag_t *flag = (flag_t*)ex_data;

    assert(ev_type == PC_EV_CONNECT_ERROR || ev_type == PC_EV_CONNECT_FAILED);
    bool is_connect_error = strcmp("Connect Error", arg1) == 0;
    bool is_connect_timeout = strcmp("Connect Timeout", arg1) == 0;
    bool is_reconn_disabled = strcmp("Reconn Disabled", arg1) == 0;
    assert(is_connect_error || is_connect_timeout || is_reconn_disabled);

    flag_set(flag);
}

static MunitResult
test_connect_errors(const MunitParameter params[], void *data)
{
    Unused(data); Unused(params);
    flag_t flag = flag_make();

    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    config.transport_name = PC_TR_NAME_UV_TCP;
    config.conn_timeout = 3;

    // Initializing the client
    pc_client_init_result_t res = pc_client_init(NULL, &config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);

    const int handler_id = pc_client_add_ev_handler(g_client, invalid_ev_cb, &flag, NULL);
    assert_int(handler_id, !=, PC_EV_INVALID_HANDLER_ID);

    // Invalid arguments for pc_client_connect
    assert_int(pc_client_connect(NULL, PITAYA_SERVER_URL, g_test_server.tcp_port, NULL), ==, PC_RC_INVALID_ARG);
    assert_int(pc_client_connect(g_client, NULL, g_test_server.tcp_port, NULL), ==, PC_RC_INVALID_ARG);
    assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, -1, NULL), ==, PC_RC_INVALID_ARG);
    assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, (1 << 16), NULL), ==, PC_RC_INVALID_ARG);

    // Invalid JSON errors
    const char *invalid_handshake_opts = "wqdojh";
    const int invalid_port = g_test_server.tcp_port + 50;
    assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, g_test_server.tcp_port, invalid_handshake_opts), ==, PC_RC_INVALID_JSON);

    const char *valid_handshake_opts = "{ \"oi\": 2 }";
    assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, invalid_port, valid_handshake_opts), ==, PC_RC_OK);

    assert_int(flag_wait(&flag, 60), ==, FLAG_SET);

    assert_int(pc_client_rm_ev_handler(g_client, handler_id), ==, PC_RC_OK);
    assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

    flag_cleanup(&flag);
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

    assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

    return MUNIT_OK;
}

static int CONNECT_TLS_EVENTS[] = {
    PC_EV_CONNECT_FAILED,
};

static void
connect_tls_server_event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    Unused(client); Unused(arg1); Unused(arg2);
    flag_t *flag = (flag_t*)ex_data;
    int num_called = flag_get_num_called(flag);
    assert_int(ev_type, ==, CONNECT_TLS_EVENTS[num_called]);
    flag_set(flag);
}

static MunitResult
test_fails_to_connect_to_tls_server(const MunitParameter params[], void *data)
{
    Unused(data); Unused(params);
    flag_t flag = flag_make();

    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    pc_client_init_result_t res = pc_client_init(NULL, &config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);

    assert_int(pc_client_add_ev_handler(g_client, connect_tls_server_event_cb, &flag, NULL), !=, PC_EV_INVALID_HANDLER_ID);
    assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, g_test_server.tls_port, NULL), ==, PC_RC_OK);
    while (flag_get_num_called(&flag) < ArrayCount(CONNECT_TLS_EVENTS)) {
        assert_int(flag_wait(&flag, 60), ==, FLAG_SET);
    }
    assert_int(pc_client_disconnect(g_client), ==, PC_RC_INVALID_STATE);
    assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

    flag_cleanup(&flag);
    return MUNIT_OK;
}

static int UNEXPECTED_DISCONNECT_EVENTS[] = {
    PC_EV_CONNECTED,
    PC_EV_UNEXPECTED_DISCONNECT,
};

static void
unexpected_disconnect_event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    Unused(client); Unused(arg1); Unused(arg2);
    flag_t *flag = (flag_t*)ex_data;
    int num_called = flag_get_num_called(flag);
    assert_int(ev_type, ==, UNEXPECTED_DISCONNECT_EVENTS[num_called]);
    flag_set(flag);
}

static MunitResult
test_unexpected_disconnect(const MunitParameter params[], void *data)
{
    Unused(data); Unused(params);
    flag_t flag = flag_make();

    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    pc_client_init_result_t res = pc_client_init(NULL, &config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);
    assert_int(pc_client_add_ev_handler(g_client, unexpected_disconnect_event_cb, &flag, NULL), !=, PC_EV_INVALID_HANDLER_ID);
    assert_int(pc_client_connect(g_client, LOCALHOST, g_kill_client_mock_server.tcp_port, NULL), ==, PC_RC_OK);
    while (flag_get_num_called(&flag) < ArrayCount(UNEXPECTED_DISCONNECT_EVENTS)) {
        assert_int(flag_wait(&flag, 60), ==, FLAG_SET);
    }
    assert_int(pc_client_disconnect(g_client), ==, PC_RC_INVALID_STATE);
    assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

    flag_cleanup(&flag);
    return MUNIT_OK;
}

static void
test_connection_with_options_event_cb(pc_client_t* client,
                                      int ev_type,
                                      void* ex_data,
                                      const char* arg1,
                                      const char* arg2)
{
    static int EXPECTED_EVENTS[] = {
        PC_EV_CONNECTED,
        PC_EV_DISCONNECT,
    };

    Unused(client); Unused(arg1); Unused(arg2);
    flag_t *flag = (flag_t*)ex_data;
    int num_called = flag_get_num_called(flag);
    assert_int(ev_type, ==, EXPECTED_EVENTS[num_called]);
    flag_set(flag);
}

static MunitResult
test_connection_with_options(const MunitParameter params[], void *data)
{
    Unused(data); Unused(params);
    flag_t flag = flag_make();

    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    pc_client_init_result_t res = pc_client_init(NULL, &config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);
    assert_int(pc_client_add_ev_handler(g_client, test_connection_with_options_event_cb, &flag, NULL),
               !=, PC_EV_INVALID_HANDLER_ID);

    const char* handshake_opts = "{\"my-options\": \"dog-evenry\"}";
    assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, g_test_server.tcp_port, handshake_opts),
               ==, PC_RC_OK);
    assert_int(flag_wait(&flag, 60), ==, FLAG_SET);

    assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
    assert_int(flag_wait(&flag, 60), ==, FLAG_SET);

    assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

    flag_cleanup(&flag);
    return MUNIT_OK;
}

static MunitResult
test_connection_with_options_fails_with_invalid_data(const MunitParameter params[], void *data)
{
    Unused(data); Unused(params);

    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    pc_client_init_result_t res = pc_client_init(NULL, &config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);

    const char* handshake_opts = "{\"my-options\": dog-evenry\"}";
    assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, g_test_server.tcp_port, handshake_opts),
               ==, PC_RC_INVALID_JSON);

    return MUNIT_OK;
}

static MunitTest tests[] = {
    {"/invalid_disconnect", test_invalid_disconnect, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/event_cb", test_event_callback, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/request_cb", test_request_cb, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/connect_errors", test_connect_errors, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/fails_to_connect_to_tls_server", test_fails_to_connect_to_tls_server, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/unexpected_disconnect", test_unexpected_disconnect, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/connection_with_options", test_connection_with_options, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/connection_with_options_fails_with_invalid_data",
     test_connection_with_options_fails_with_invalid_data, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
};

const MunitSuite tcp_suite = {
    "/tcp", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};
