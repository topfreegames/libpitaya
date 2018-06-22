#include <stdio.h>
#include <pomelo.h>
#include <stdbool.h>

#include "test_common.h"

static pc_client_t *g_client = NULL;

static int g_num_success_cb_called = 0;
static int g_num_error_cb_called = 0;
static int g_num_timeout_error_cb_called = 0;

static void
event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    // This callback should not be called, therefore called should always be false.
    Unused(arg1); Unused(arg2); Unused(client); Unused(ev_type);
    bool *called = (bool*)ex_data;
    *called = true;

    printf("\nEVENT %s\n", pc_client_ev_str(ev_type));
    printf("received arg1: %s\n", arg1);
    printf("received arg2: %s\n", arg2);
}

static void
request_cb(const pc_request_t* req, const char *resp)
{
    printf("RESP IS: %s\n", resp);
    g_num_success_cb_called++;
}

static void
timeout_error_cb(const pc_request_t* req, pc_error_t error)
{
    g_num_timeout_error_cb_called++;

    assert_string_equal(error.code, "PC_RC_TIMEOUT");
    assert_null(error.msg);
    assert_null(error.metadata);
}

static void
request_error_cb(const pc_request_t* req, pc_error_t error)
{
    g_num_error_cb_called++;

    assert_string_equal(error.code, "PIT-404");
    assert_string_equal(error.msg, "pitaya/handler: connector.invalid.route not found");
    assert_null(error.metadata);
}

static MunitResult
test_request(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);

    const int ports[] = {g_test_protobuf_server.tcp_port, g_test_protobuf_server.tls_port};
    const int transports[] = {PC_TR_NAME_UV_TCP, PC_TR_NAME_UV_TLS};

    assert_int(tr_uv_tls_set_ca_file(CRT, NULL), ==, PC_RC_OK);

    for (size_t i = 0; i < ArrayCount(ports); i++) {
        pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
        config.transport_name = transports[i];

        pc_client_init_result_t res = pc_client_init(NULL, &config);
        g_client = res.client;
        assert_int(res.rc, ==, PC_RC_OK);

        bool ev_cb_called = false;
        pc_client_add_ev_handler(g_client, event_cb, &ev_cb_called, NULL);

        assert_int(pc_client_connect(g_client, LOCALHOST, ports[i], NULL), ==, PC_RC_OK);
        SLEEP_SECONDS(1);

#if 1
        const char *req_msg = "{}";
#else
        const char *req_msg = "{}";
#endif

        assert_int(pc_request_with_timeout(g_client, "connector.getsessiondata", NULL, NULL, REQ_TIMEOUT, request_cb, NULL), ==, PC_RC_OK);

        SLEEP_SECONDS(1);

        // assert_int(g_num_error_cb_called, ==, 1);
        // assert_int(g_num_success_cb_called, ==, 0);

        // g_num_error_cb_called = 0;
        // g_num_success_cb_called = 0;

        assert_true(ev_cb_called);
        assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
        assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);
    }

    return MUNIT_OK;
}

static MunitTest tests[] = {
    {"/request", test_request, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
};

const MunitSuite protobuf_suite = {
    "/protobuf", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};
