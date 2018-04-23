/**
 * Copyright (c) 2014,2015 NetEase, Inc. and other Pomelo contributors
 * MIT Licensed.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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
    Unused(state);
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
    Unused(state);
    free(g_client);
    g_client = NULL;
    return 0;
}

static void
event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    g_event_cb_called = true;
    assert_ptr_equal(ex_data, EV_HANDLER_EX);
//    printf("test: get event %s, arg1: %s, arg2: %s\n", pc_client_ev_str(ev_type),
//            arg1 ? arg1 : "", arg2 ? arg2 : "");
}

static void
request_cb(const pc_request_t* req, int rc, const char* resp)
{
    g_request_cb_called = true;
    assert_int_equal(rc, PC_RC_OK);
    assert_non_null(resp);
//    printf("test: get resp %s\n", resp);
//    fflush(stdout);

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

void
test_tls_connection(void **state)
{
    Unused(state);
    pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
    config.transport_name = PC_TR_NAME_UV_TLS;

    assert_non_null(g_client);

#define RANDOM_PTR (void*)0xdeadbeef
    pc_client_init(g_client, RANDOM_PTR, &config);

    assert_ptr_equal(pc_client_ex_data(g_client), RANDOM_PTR);
    assert_int_equal(pc_client_state(g_client), PC_ST_INITED);

    int handler_id = pc_client_add_ev_handler(g_client, event_cb, EV_HANDLER_EX, NULL);

    pc_client_connect(g_client, "127.0.0.1", TLS_PORT, NULL);
    SLEEP_SECONDS(1);
    pc_request_with_timeout(g_client, REQ_ROUTE, REQ_MSG, REQ_EX, REQ_TIMEOUT, request_cb);
    pc_notify_with_timeout(g_client, NOTI_ROUTE, NOTI_MSG, NOTI_EX, NOTI_TIMEOUT, notify_cb);
    SLEEP_SECONDS(2);

    assert_true(g_event_cb_called);
    assert_true(g_notify_cb_called);
    assert_true(g_request_cb_called);

    pc_client_disconnect(g_client);
    pc_client_rm_ev_handler(g_client, handler_id);
    pc_client_cleanup(g_client);

    assert_int_equal(pc_client_state(g_client), PC_ST_NOT_INITED);

#undef RANDOM_PTR
}

int
main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_tls_connection, setup_global_client, teardown_global_client),
    };
    return cmocka_run_group_tests(tests, setup_pc_lib, teardown_pc_lib);
}
