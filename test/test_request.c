#include <stdlib.h>
#include <stdio.h>
#include <pomelo.h>
#include <stdbool.h>

#include "test_common.h"

static pc_client_t *g_client = NULL;

static void *
setup(const MunitParameter params[], void *data)
{
    Unused(data); Unused(params);
    // NOTE: use calloc in order to avoid one of the issues with the api.
    // see `issues.md`.
    g_client = calloc(1, pc_client_size());
    assert_not_null(g_client);
    return NULL;
}

static void
teardown(void *data)
{
    Unused(data);
    free(g_client);
    g_client = NULL;
}

static bool g_success_cb_called = false;
static bool g_error_cb_called = false;

static void
request_cb(const pc_request_t* req, int rc, const char* resp)
{
    g_success_cb_called = true;
}

static void
request_error_cb(const pc_request_t* req, int rc, const char* resp)
{
    g_error_cb_called = true;

    // TODO: Instead of returning a json string, return an Error object, so that it is easier to deal with.
    assert_int(rc, ==, PC_RC_OK);
    assert_string_equal(resp, "{\"Code\":\"PIT-404\",\"Msg\":\"pitaya/handler: connector.invalid.route not found\"}");
}

MunitResult
test_invalid_route(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);

    const int ports[] = {g_test_server.tcp_port, g_test_server.tls_port};
    const int transports[] = {PC_TR_NAME_UV_TCP, PC_TR_NAME_UV_TLS};

    assert_int(tr_uv_tls_set_ca_file("../../test/server/fixtures/ca.crt", NULL), ==, PC_RC_OK);

    for (size_t i = 0; i < ArrayCount(ports); i++) {
        pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
        config.transport_name = transports[i];

        assert_int(pc_client_init(g_client, NULL, &config), ==, PC_RC_OK);

        assert_int(pc_client_connect(g_client, LOCALHOST, ports[i], NULL), ==, PC_RC_OK);
        SLEEP_SECONDS(2);

        assert_int(pc_request_with_timeout(g_client, "invalid.route", REQ_MSG, NULL, REQ_TIMEOUT, request_cb, request_error_cb), ==, PC_RC_OK);

        SLEEP_SECONDS(2);

        assert_true(g_error_cb_called);
        assert_false(g_success_cb_called);

        assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
        assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);
    }

    return MUNIT_OK;
}

static MunitTest tests[] = {
    {"/invalid_route", test_invalid_route, setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
};

const MunitSuite request_suite = {
    "/request", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};
