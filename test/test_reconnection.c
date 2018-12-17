#include <stdlib.h>
#include <stdio.h>
#include <pitaya.h>
#include <stdbool.h>

#include "test_common.h"
#include "flag.h"

static pc_client_t *g_client = NULL;

static int SUCCESS_RECONNECT_EV_ORDER[] = {
    PC_EV_CONNECTED,
    PC_EV_UNEXPECTED_DISCONNECT,
    PC_EV_RECONNECT_STARTED,
    PC_EV_CONNECTED,
    PC_EV_DISCONNECT,
};

static void
reconnect_success_event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    Unused(client); Unused(arg1); Unused(arg2);
    flag_t *flag = (flag_t*)ex_data;
    int num_calls = flag_get_num_called(flag);

    assert_int(num_calls, <, ArrayCount(SUCCESS_RECONNECT_EV_ORDER));
    assert_int(ev_type, ==, SUCCESS_RECONNECT_EV_ORDER[num_calls]);
    flag_set(flag);
}

MunitResult
test_success(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);

    int ports[] = {g_disconnect_mock_server.tcp_port, g_disconnect_mock_server.tls_port};
    int transports[] = {PC_TR_NAME_UV_TCP, PC_TR_NAME_UV_TLS};

    assert_int(tr_uv_tls_set_ca_file(CRT, NULL), ==, PC_RC_OK);

    for (size_t i = 0; i < ArrayCount(ports); i++) {
        flag_t flag_evs = flag_make();
        pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
        config.transport_name = transports[i];

        pc_client_init_result_t res = pc_client_init(NULL, &config);
        g_client = res.client;
        assert_int(res.rc, ==, PC_RC_OK);

        int handler_id = pc_client_add_ev_handler(g_client, reconnect_success_event_cb, &flag_evs, NULL);
        assert_int(handler_id, !=, PC_EV_INVALID_HANDLER_ID);

        assert_int(pc_client_connect(g_client, LOCALHOST, ports[i], NULL), ==, PC_RC_OK);

        while (flag_get_num_called(&flag_evs) < ArrayCount(SUCCESS_RECONNECT_EV_ORDER)-1) {
            assert_int(flag_wait(&flag_evs, 60), ==, FLAG_SET);
        }

        assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
        assert_int(flag_wait(&flag_evs, 60), ==, FLAG_SET);

        assert_int(pc_client_rm_ev_handler(g_client, handler_id), ==, PC_RC_OK);
        assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

        flag_cleanup(&flag_evs);
    }

    return MUNIT_OK;
}

static int RECONNECT_MAX_RETRY_EV_ORDER[] = {
    PC_EV_CONNECT_ERROR,
    PC_EV_RECONNECT_STARTED,
    PC_EV_CONNECT_ERROR,
    PC_EV_CONNECT_ERROR,
    PC_EV_CONNECT_ERROR,
    PC_EV_RECONNECT_FAILED,
};

static void
reconnect_event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    Unused(client); Unused(arg1); Unused(arg2);
    flag_t *flag = (flag_t*)ex_data;
    int num_called = flag_get_num_called(flag);
    assert_int(num_called, <, ArrayCount(RECONNECT_MAX_RETRY_EV_ORDER));
    assert_int(ev_type, ==, RECONNECT_MAX_RETRY_EV_ORDER[num_called]);
    flag_set(flag);
}

MunitResult
test_max_retry(const MunitParameter params[], void *data)
{
    Unused(data); Unused(params);
    flag_t flag_evs = flag_make();

    pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
    config.reconn_max_retry = 3;
    config.reconn_delay = 1;

    pc_client_init_result_t res = pc_client_init(NULL, &config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);

    int handler_id = pc_client_add_ev_handler(g_client, reconnect_event_cb, &flag_evs, NULL);
    assert_int(handler_id, !=, PC_EV_INVALID_HANDLER_ID);

    assert_int(pc_client_connect(g_client, "invalidhost", g_test_server.tcp_port, NULL), ==, PC_RC_OK);

    while (flag_get_num_called(&flag_evs) < ArrayCount(RECONNECT_MAX_RETRY_EV_ORDER)) {
        assert_int(flag_wait(&flag_evs, 60), ==, FLAG_SET);
    }

    // There should be reconn_max_retry reconnections retries. This means the event_cb should be called
    // reconn_max_retry + 1 times with PC_EV_CONNECT_ERROR and one time with PC_EV_RECONNECT_FAILED.
    assert_int(pc_client_rm_ev_handler(g_client, handler_id), ==, PC_RC_OK);
    assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

    flag_cleanup(&flag_evs);
    return MUNIT_OK;
}

static MunitTest tests[] = {
    {"/max_retry", test_max_retry, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/success", test_success, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
};

const MunitSuite reconnection_suite = {
    "/reconnection", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};
