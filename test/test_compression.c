#include <stdlib.h>
#include <stdio.h>
#include <pomelo.h>
#include <stdbool.h>

#include "test_common.h"

static pc_client_t *g_client = NULL;

static char *RESPONSES_DISABLED[] = {
    "{\"isCompressed\":false}",
    "{\"isCompressed\":false}",
};

static void
request_cb_disabled(const pc_request_t* req, const char* resp)
{
    int *num_called = (int*)pc_request_ex_data(req);
    assert_string_equal(RESPONSES_DISABLED[*num_called], resp);
    (*num_called)++;
}

static char *RESPONSES_ENABLED[] = {
    "{\"isCompressed\":true}",
    "{\"isCompressed\":false}",
};

static void
request_cb_enabled(const pc_request_t* req, const char* resp)
{
    int *num_called = (int*)pc_request_ex_data(req);
    assert_string_equal(RESPONSES_ENABLED[*num_called], resp);
    (*num_called)++;
}

MunitResult
test_disabled_compression(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);

    const int ports[] = {g_compression_mock_server.tcp_port, g_compression_mock_server.tls_port};
    const int transports[] = {PC_TR_NAME_UV_TCP, PC_TR_NAME_UV_TLS};

    assert_int(tr_uv_tls_set_ca_file(CRT, NULL), ==, PC_RC_OK);

    for (size_t i = 0; i < ArrayCount(ports); i++) {
        pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
        config.transport_name = transports[i];
        config.disable_compression = true;

        pc_client_init_result_t res = pc_client_init(NULL, &config);
        g_client = res.client;
        assert_int(res.rc, ==, PC_RC_OK);

        assert_int(pc_client_connect(g_client, LOCALHOST, ports[i], NULL), ==, PC_RC_OK);
        SLEEP_SECONDS(1);

        int num_called = 0;
        const char *big_json = "{\"Data\":{\"name\":\"PEPE\",\"age\":\"veryyyyyyyyyyy old myyyyyyyyyyyyyyyyyyy boiiiiiiiiiiiiiiiiiiiiiiiiiiiiii\"}}";
        const char *empty_json = "{}";

        assert_int(pc_request_with_timeout(g_client, "irrelevant.route", big_json, &num_called, 12, request_cb_disabled, NULL), ==, PC_RC_OK);
        SLEEP_SECONDS(2);

        assert_int(pc_request_with_timeout(g_client, "irrelevant.route", empty_json, &num_called, REQ_TIMEOUT, request_cb_disabled, NULL), ==, PC_RC_OK);
        SLEEP_SECONDS(2);

        assert_int(num_called, ==, ArrayCount(RESPONSES_DISABLED));
        assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
        assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);
    }

    return MUNIT_OK;
}

MunitResult
test_enabled_compression(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);

    const int ports[] = {g_compression_mock_server.tcp_port, g_compression_mock_server.tls_port};
    const int transports[] = {PC_TR_NAME_UV_TCP, PC_TR_NAME_UV_TLS};

    assert_int(tr_uv_tls_set_ca_file(CRT, NULL), ==, PC_RC_OK);

    for (size_t i = 0; i < ArrayCount(ports); i++) {
        pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
        config.transport_name = transports[i];

        pc_client_init_result_t res = pc_client_init(NULL, &config);
        g_client = res.client;
        assert_int(res.rc, ==, PC_RC_OK);

        assert_int(pc_client_connect(g_client, LOCALHOST, ports[i], NULL), ==, PC_RC_OK);
        SLEEP_SECONDS(1);

        int num_called = 0;

        const char *big_json = "{\"Data\":{\"name\":\"PEPE\",\"age\":\"veryyyyyyyyyyy old myyyyyyyyyyyyyyyyyyy boiiiiiiiiiiiiiiiiiiiiiiiiiiiiii\"}}";
        const char *empty_json = "{}";

        assert_int(pc_request_with_timeout(g_client, "irrelevant.route", big_json, &num_called, 12, request_cb_enabled, NULL), ==, PC_RC_OK);
        SLEEP_SECONDS(2);

        assert_int(pc_request_with_timeout(g_client, "irrelevant.route", empty_json, &num_called, REQ_TIMEOUT, request_cb_enabled, NULL), ==, PC_RC_OK);
        SLEEP_SECONDS(2);

        assert_int(num_called, ==, ArrayCount(RESPONSES_ENABLED));
        assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
        assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);
    }

    return MUNIT_OK;
}

static MunitTest tests[] = {
    {"/enabled", test_enabled_compression, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/disabled", test_disabled_compression, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
};

const MunitSuite compression_suite = {
    "/compression", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};
