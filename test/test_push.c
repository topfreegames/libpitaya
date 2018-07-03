#include <stdlib.h>
#include <stdio.h>
#include <pitaya.h>
#include <stdbool.h>

#include "test_common.h"

static pc_client_t *g_client = NULL;
static int g_num_success_cb_called = 0;
static int g_num_error_cb_called = 0;
static int g_num_ev_cb_called = 0;
static int g_num_push_cb_called = 0;

static void
request_cb(const pc_request_t* req, const pc_buf_t *resp)
{
    ++g_num_success_cb_called;
}

static void
request_error_cb(const pc_request_t* req, const pc_error_t *error)
{
    ++g_num_error_cb_called;
}

static int EV_ORDER[] = {
    PC_EV_CONNECTED,
};

static void
event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    Unused(client); Unused(arg1); Unused(arg2);
    assert_int(ev_type, ==, EV_ORDER[g_num_ev_cb_called++]);
}

void 
push_handler(const char *route, const pc_buf_t *payload)
{
    assert_not_null(payload);
    assert_not_null(payload->base);
    assert_string_equal(route, "some.push.route");

    char *str = calloc(payload->len+1, 1);
    assert_not_null(str);
    memcpy(str, payload->base, payload->len);

    assert_string_equal(str, "{\"key1\":10,\"key2\":true}");

    free(str);

    ++g_num_push_cb_called;
}

static MunitResult
test_success(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);

    const int ports[] = {g_test_server.tcp_port, g_test_server.tls_port};
    const int transports[] = {PC_TR_NAME_UV_TCP, PC_TR_NAME_UV_TLS};

    assert_int(tr_uv_tls_set_ca_file(CRT, NULL), ==, PC_RC_OK);

    for (size_t i = 0; i < ArrayCount(ports); i++) {
        pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
        config.transport_name = transports[i];
        config.push_handler = push_handler;

        pc_client_init_result_t res = pc_client_init(NULL, &config);
        g_client = res.client;
        assert_int(res.rc, ==, PC_RC_OK);

        int handler_id = pc_client_add_ev_handler(g_client, event_cb, NULL, NULL);

        assert_int(pc_client_connect(g_client, LOCALHOST, ports[i], NULL), ==, PC_RC_OK);

        SLEEP_SECONDS(1);

        assert_int(pc_string_request_with_timeout(g_client, "connector.sendpush", "{}", NULL, REQ_TIMEOUT,
                                                  request_cb, request_error_cb), ==, PC_RC_OK);
        SLEEP_SECONDS(2);

        assert_int(g_num_success_cb_called, ==, 1);
        assert_int(g_num_error_cb_called, ==, 0);
        assert_int(g_num_ev_cb_called, ==, ArrayCount(EV_ORDER)); 
        assert_int(g_num_push_cb_called, ==, 1);

        g_num_success_cb_called = 0;
        g_num_error_cb_called = 0;
        g_num_ev_cb_called = 0;
        g_num_push_cb_called = 0;

        pc_client_rm_ev_handler(g_client, handler_id);
        assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
        assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);
    }

    return MUNIT_OK;
}

static MunitTest tests[] = {
    {"/success", test_success, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
};

const MunitSuite push_suite = {
    "/push", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};
