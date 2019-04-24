/**
 * Copyright (c) 2014,2015 NetEase, Inc. and other Pomelo contributors
 * MIT Licensed.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <pitaya.h>
#include <pitaya_trans.h>

#include "test_common.h"
#include "pc_assert.h"
#include "flag.h"

static pc_client_t *g_client = NULL;

static int EV_ORDER[] = {
    PC_EV_CONNECTED,
    PC_EV_DISCONNECT,
};

static void
key_not_pinned_event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    Unused(client); Unused(arg1); Unused(arg2);

    static int EVENTS[] = {
        PC_EV_CONNECT_FAILED,
    };

    flag_t *flag = (flag_t*)ex_data;
    int num_called = flag_get_num_called(flag);
    assert_int(num_called, <=, ArrayCount(EVENTS));
    assert_int(ev_type, ==, EVENTS[num_called]);
    flag_set(flag);
}

static void
key_pinned_event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    Unused(client); Unused(arg1); Unused(arg2);
    static int EVENTS[] = {
        PC_EV_CONNECTED,
        PC_EV_DISCONNECT,
    };

    flag_t *flag = (flag_t*)ex_data;
    int num_called = flag_get_num_called(flag);
    assert_int(ev_type, ==, EVENTS[num_called]);
    assert_int(num_called, <, ArrayCount(EVENTS));
    flag_set(flag);
}

static void
event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    Unused(client); Unused(arg1); Unused(arg2);
    flag_t *flag = (flag_t*)ex_data;
    int num_called = flag_get_num_called(flag);
    assert_int(ev_type, ==, EV_ORDER[num_called]);
    assert_int(num_called, <, ArrayCount(EV_ORDER));
    flag_set(flag);
}

static void
request_cb(const pc_request_t* req, const pc_buf_t *resp)
{
    flag_t *flag = (flag_t*)pc_request_ex_data(req);
    assert_not_null(resp);
    assert_ptr_equal(pc_request_client(req), g_client);
    assert_string_equal(pc_request_route(req), REQ_ROUTE);
    assert_string_equal(pc_request_msg(req), REQ_MSG);
    assert_int(pc_request_timeout(req), ==, REQ_TIMEOUT);
    flag_set(flag);
}

static void
notify_cb(const pc_notify_t* noti, const pc_error_t *err)
{
    flag_t *flag= (flag_t*)pc_notify_ex_data(noti);
    flag_set(flag);
}

static MunitResult
test_add_key_pinned_errors(const MunitParameter params[], void *state)
{
    assert_int(pc_lib_add_pinned_public_key_from_certificate_file("fixtures/asoijdoaisjdisajd"), ==, PC_RC_NO_SUCH_FILE);
    assert_int(pc_lib_add_pinned_public_key_from_certificate_file("fixtures/corrupt-ca.crt"), ==, PC_RC_ERROR);
    assert_int(pc_lib_add_pinned_public_key_from_certificate_file("fixtures/myCA.pem"), ==, PC_RC_OK);
    pc_lib_clear_pinned_public_keys();
    return MUNIT_OK;
}

static MunitResult
test_key_pinned(const MunitParameter params[], void *state)
{
    pc_lib_skip_key_pin_check(false);
    for (int i = 0; i < 3; ++i) {
        flag_t flag = flag_make();

        const char *ca_array[] = {
            CRT, INCORRECT_CRT, SERVER_CRT,
        };

        // This switch means that it does not matter how many items are added to the array,
        // if one is valid it always should work.
        switch (i) {
            case 0:
                pc_lib_add_pinned_public_key_from_certificate_file(ca_array[2]);
                break;
            case 1:
                pc_lib_add_pinned_public_key_from_certificate_file(ca_array[0]);
                pc_lib_add_pinned_public_key_from_certificate_file(ca_array[2]);
                break;
            case 2:
                pc_lib_add_pinned_public_key_from_certificate_file(ca_array[0]);
                pc_lib_add_pinned_public_key_from_certificate_file(ca_array[1]);
                pc_lib_add_pinned_public_key_from_certificate_file(ca_array[2]);
                break;
            default:
                assert(false);
        }

        pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
        config.transport_name = PC_TR_NAME_UV_TLS;

        pc_client_init_result_t res = pc_client_init(NULL, &config);
        g_client = res.client;
        assert_int(res.rc, ==, PC_RC_OK);

        int handler_id = pc_client_add_ev_handler(g_client, key_pinned_event_cb, &flag, NULL);
        assert_int(handler_id, !=, PC_EV_INVALID_HANDLER_ID);

        assert_int(tr_uv_tls_set_ca_file(CRT, NULL), ==, PC_RC_OK);

        assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, g_test_server.tls_port, NULL), ==, PC_RC_OK);
        assert_int(flag_wait(&flag, 60), ==, FLAG_SET);
        assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
        assert_int(flag_wait(&flag, 60), ==, FLAG_SET);

        assert_int(pc_client_rm_ev_handler(g_client, handler_id), ==, PC_RC_OK);
        assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

        pc_lib_clear_pinned_public_keys();
        flag_cleanup(&flag);
    }
    pc_lib_skip_key_pin_check(true);
    return MUNIT_OK;
}

static MunitResult
test_key_not_pinned(const MunitParameter params[], void *state)
{
    flag_t flag = flag_make();

    pc_lib_add_pinned_public_key_from_certificate_file("fixtures/server/pitaya.crt");
    pc_lib_clear_pinned_public_keys();
    pc_lib_skip_key_pin_check(false);

    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    config.transport_name = PC_TR_NAME_UV_TLS;

    pc_client_init_result_t res = pc_client_init(NULL, &config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);

    int handler_id = pc_client_add_ev_handler(g_client, key_not_pinned_event_cb, &flag, NULL);
    assert_int(handler_id, !=, PC_EV_INVALID_HANDLER_ID);

    assert_int(tr_uv_tls_set_ca_file(CRT, NULL), ==, PC_RC_OK);

    assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, g_test_server.tls_port, NULL), ==, PC_RC_OK);
    assert_int(flag_wait(&flag, 60), ==, FLAG_SET);

    assert_int(pc_client_state(g_client), ==, PC_ST_INITED);
    assert_int(pc_client_rm_ev_handler(g_client, handler_id), ==, PC_RC_OK);
    assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

    pc_lib_skip_key_pin_check(true);
    flag_cleanup(&flag);
    return MUNIT_OK;
}

static MunitResult
test_successful_handshake(const MunitParameter params[], void *state)
{
    Unused(state); Unused(params);
    flag_t flag_evs = flag_make();
    flag_t flag_req_not = flag_make();

    pc_lib_add_pinned_public_key_from_certificate_file(SERVER_CRT);

    pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
    config.transport_name = PC_TR_NAME_UV_TLS;

    pc_client_init_result_t res = pc_client_init(NULL, &config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);

    int handler_id = pc_client_add_ev_handler(g_client, event_cb, &flag_evs, NULL);
    assert_int(handler_id, !=, PC_EV_INVALID_HANDLER_ID);

    // Set CA file so that the handshake is successful.
    assert_int(tr_uv_tls_set_ca_file(CRT, NULL), ==, PC_RC_OK);

    assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, g_test_server.tls_port, NULL), ==, PC_RC_OK);
    assert_int(flag_wait(&flag_evs, 60), ==, FLAG_SET);

    assert_int(pc_string_request_with_timeout(g_client, REQ_ROUTE, REQ_MSG, &flag_req_not, REQ_TIMEOUT, request_cb, NULL), ==, PC_RC_OK);
    assert_int(flag_wait(&flag_req_not, 60), ==, FLAG_SET);
    assert_int(pc_string_notify_with_timeout(g_client, NOTI_ROUTE, NOTI_MSG, &flag_req_not, NOTI_TIMEOUT, notify_cb), ==, PC_RC_OK);
    assert_int(flag_wait(&flag_req_not, 4), ==, FLAG_TIMEOUT);

    assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
    assert_int(flag_wait(&flag_evs, 60), ==, FLAG_SET);

    assert_int(pc_client_rm_ev_handler(g_client, handler_id), ==, PC_RC_OK);
    assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

    pc_lib_clear_pinned_public_keys();
    flag_cleanup(&flag_evs);
    flag_cleanup(&flag_req_not);
    return MUNIT_OK;
}


static void
connect_failed_event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    Unused(client);
    flag_t *flag = (flag_t*)ex_data;
    assert_int(ev_type, ==, PC_EV_CONNECT_FAILED);
    assert_string_equal(arg1, "TLS Handshake Error");
    assert_null(arg2);
    flag_set(flag);
}

static void
test_invalid_handshake()
{
    flag_t flag = flag_make();
    int handler_id = pc_client_add_ev_handler(g_client, connect_failed_event_cb, &flag, NULL);
    assert_int(handler_id, !=, PC_EV_INVALID_HANDLER_ID);

    assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, g_test_server.tls_port, NULL), ==, PC_RC_OK);
    assert_int(flag_wait(&flag, 60), ==, FLAG_SET);
    assert_int(pc_client_disconnect(g_client), ==, PC_RC_INVALID_STATE);
    assert_int(pc_client_rm_ev_handler(g_client, handler_id), ==, PC_RC_OK);
    flag_cleanup(&flag);
}

static MunitResult
test_no_client_certificate(const MunitParameter params[], void *state)
{
    Unused(state); Unused(params);
    pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
    config.transport_name = PC_TR_NAME_UV_TLS;

    pc_client_init_result_t res = pc_client_init(NULL, &config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);

    // Without setting a CA file, the handshake should fail.
    test_invalid_handshake();
    // Setting an unexistent CA file should fail the function and also fail the handshake.
    assert_int(tr_uv_tls_set_ca_file("./this/ca/does/not/exist.crt", NULL), ==, PC_RC_ERROR);
    test_invalid_handshake();

    assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);
    return MUNIT_OK;
}

static MunitResult
test_wrong_client_certificate(const MunitParameter params[], void *state)
{
    Unused(state); Unused(params);
    pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
    config.transport_name = PC_TR_NAME_UV_TLS;

    pc_client_init_result_t res = pc_client_init(NULL, &config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);

    // Setting the WRONG CA file should not fail the function but make the handshake fail.
    assert_int(tr_uv_tls_set_ca_file(INCORRECT_CRT, NULL), ==, PC_RC_OK);
    test_invalid_handshake();

    assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);
    return MUNIT_OK;
}

static void
custom_assert(const char *e, const char *file, int line)
{
    Unused(e); Unused(file); Unused(line);
    // Do nothing
}

static MunitResult
test_cleanup_before_connection_done(const MunitParameter params[], void *data)
{
    Unused(data); Unused(params);
    update_assert(custom_assert);

    pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
    config.transport_name = PC_TR_NAME_UV_TLS;
    config.reconn_max_retry = 4;
    config.enable_reconn = true;

    pc_client_init_result_t res = pc_client_init(NULL, &config);
    g_client = res.client;

    assert_int(res.rc, ==, PC_RC_OK);
    assert_int(pc_client_connect(g_client, "localhost", 8080, NULL), ==, PC_RC_OK);
    assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
    assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

    update_assert(NULL);

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
    assert_int(num_called, <, ArrayCount(UNEXPECTED_DISCONNECT_EVENTS));
    flag_set(flag);
}

static MunitResult
test_unexpected_disconnect(const MunitParameter params[], void *data)
{
    Unused(data); Unused(params);
    flag_t flag = flag_make();

    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    config.transport_name = PC_TR_NAME_UV_TLS;
    assert_int(tr_uv_tls_set_ca_file(CRT, NULL), ==, PC_RC_OK);

    pc_client_init_result_t res = pc_client_init(NULL, &config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);

    assert_int(pc_client_add_ev_handler(g_client, unexpected_disconnect_event_cb, &flag, NULL), !=, PC_EV_INVALID_HANDLER_ID);

    assert_int(pc_client_connect(g_client, LOCALHOST, g_kill_client_mock_server.tls_port, NULL), ==, PC_RC_OK);

    while (flag_get_num_called(&flag) < ArrayCount(UNEXPECTED_DISCONNECT_EVENTS)) {
        assert_int(flag_wait(&flag, 60), ==, FLAG_SET);
    }

    assert_int(pc_client_disconnect(g_client), ==, PC_RC_INVALID_STATE);
    assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

    flag_cleanup(&flag);
    return MUNIT_OK;
}

static MunitTest tests[] = {
    {"/no_client_certificate", test_no_client_certificate, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/wrong_client_certificate", test_wrong_client_certificate, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/successful_handshake", test_successful_handshake, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/key_pinned", test_key_pinned, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/key_not_pinned", test_key_not_pinned, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/add_pinned_key_errors", test_add_key_pinned_errors, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/cleanup_before_connection_done", test_cleanup_before_connection_done, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/unexpected_disconnect", test_unexpected_disconnect, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
};

const MunitSuite tls_suite = {
    "/tls", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};
