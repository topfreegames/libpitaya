#include <stdlib.h>
#include <stdio.h>
#include <pitaya.h>
#include <stdbool.h>

#include "test_common.h"

static pc_client_t *g_client = NULL;

static void
request_cb(const pc_request_t* req, const pc_buf_t *resp)
{
}

static void
request_error_cb(const pc_request_t* req, const pc_error_t *error)
{
}

static void
event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    Unused(client); Unused(arg1); Unused(arg2);
    // printf("GOT EV: %s\n", pc_client_ev_str(ev_type));
}

static void
notify_error_cb(const pc_notify_t* not, const pc_error_t *error)
{
    bool *called = (bool*)pc_notify_ex_data(not);
    *called = true;
    assert_int(error->code, ==, PC_RC_RESET);
}

static MunitResult
test_reset(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);

    const int ports[] = {g_destroy_socket_mock_server.tcp_port, g_destroy_socket_mock_server.tls_port};
    const int transports[] = {PC_TR_NAME_UV_TCP, PC_TR_NAME_UV_TLS};

    assert_int(tr_uv_tls_set_ca_file(CRT, NULL), ==, PC_RC_OK);

    for (size_t i = 0; i < ArrayCount(ports); i++) {
        pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
        config.transport_name = transports[i];

        pc_client_init_result_t res = pc_client_init(NULL, &config);
        g_client = res.client;
        assert_int(res.rc, ==, PC_RC_OK);

        int handler = pc_client_add_ev_handler(g_client, event_cb, NULL, NULL);

        assert_int(pc_client_connect(g_client, LOCALHOST, ports[i], NULL), ==, PC_RC_OK);

        SLEEP_SECONDS(1);

        bool called = false;
        assert_int(pc_string_notify_with_timeout(g_client, "connector.getsessiondata", "{}", 
                                                 &called, 1, notify_error_cb), ==, PC_RC_OK);
        assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
        SLEEP_SECONDS(1);

        assert_true(called);
        pc_client_rm_ev_handler(g_client, handler);
        assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);
    }

    return MUNIT_OK;
}

static MunitResult
test_called(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);

    const int ports[] = {g_timeout_mock_server.tcp_port, g_timeout_mock_server.tls_port};
    const int transports[] = {PC_TR_NAME_UV_TCP, PC_TR_NAME_UV_TLS};

    assert_int(tr_uv_tls_set_ca_file(CRT, NULL), ==, PC_RC_OK);

    /* for (size_t i = 0; i < ArrayCount(ports); i++) { */
    for (size_t i = 0; i < 1; i++) {
        pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
        config.transport_name = transports[i];

        pc_client_init_result_t res = pc_client_init(NULL, &config);
        g_client = res.client;
        assert_int(res.rc, ==, PC_RC_OK);

        bool called = false;
        assert_int(pc_string_notify_with_timeout(g_client, "connector.getsessiondata", "{}", NULL, 1,
                                                 notify_error_cb), ==, PC_RC_INVALID_STATE);
        SLEEP_SECONDS(1);
        assert_false(called);

        assert_int(pc_client_connect(g_client, LOCALHOST, ports[i], NULL), ==, PC_RC_OK);

        SLEEP_SECONDS(1);

        for (int i = 0; i < 1; i++) {
            assert_int(pc_string_request_with_timeout(g_client, "connector.getsessiondata", "{}", NULL, 1,
                                                      request_cb, request_error_cb), ==, PC_RC_OK);
        }

        assert_int(pc_string_notify_with_timeout(g_client, "connector.getsessiondata", "{}", &called, 1,
                                                 NULL), ==, PC_RC_OK);
        assert_int(pc_string_notify_with_timeout(g_client, "connector.getsessiondata", "{}", &called, 1,
                                                 notify_error_cb), ==, PC_RC_OK);
        SLEEP_SECONDS(1);
        assert_false(called);

        assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
        assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);
    }

    return MUNIT_OK;
}

static MunitTest tests[] = {
    {"/success", test_called, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/reset", test_reset, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
};

const MunitSuite notify_suite = {
    "/notify", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};
