#include <stdlib.h>
#include <stdio.h>
#include <pitaya.h>
#include <stdbool.h>

#include "test_common.h"

static pc_client_t *g_client = NULL;
static int g_num_success_cb_called = 0;
static int g_num_error_cb_called = 0;

static void
request_cb(const pc_request_t* req, const pc_buf_t *resp)
{
    ++g_num_success_cb_called;
}

static void
request_error_cb(const pc_request_t* req, const pc_error_t *error)
{
    printf("Error called: %s\n", pc_client_rc_str(error->code));
    ++g_num_error_cb_called;
}

static MunitResult
test_multiple_requests(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);

    const int ports[] = {g_test_server.tcp_port, g_test_server.tls_port};
    const int transports[] = {PC_TR_NAME_UV_TCP, PC_TR_NAME_UV_TLS};

    assert_int(tr_uv_tls_set_ca_file(CRT, NULL), ==, PC_RC_OK);
    bool called = false;

    for (size_t i = 0; i < 2; i++) {
        pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
        config.transport_name = transports[i];

        pc_client_init_result_t res = pc_client_init(NULL, &config);
        g_client = res.client;
        assert_int(res.rc, ==, PC_RC_OK);

        assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, ports[i], NULL), ==, PC_RC_OK);
        SLEEP_SECONDS(1);

        static const int num_requests_at_once = 50;
        for (int y = 0; y < num_requests_at_once; ++y) {
            pc_string_request_with_timeout(g_client, "connector.getsessiondata", "{}", &called, 10,
                                           request_cb, request_error_cb);
        }

        SLEEP_SECONDS(6);

        assert_int(g_num_error_cb_called, ==, 0);
        assert_int(g_num_success_cb_called, ==, num_requests_at_once);
        assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
        assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

        g_num_error_cb_called = 0;
        g_num_success_cb_called = 0;
    }

    return MUNIT_OK;
}

static MunitTest tests[] = {
    {"/multiple_requests", test_multiple_requests, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
};

const MunitSuite stress_suite = {
    "/stress", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};
