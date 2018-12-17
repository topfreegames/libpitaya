#include <stdlib.h>
#include <stdio.h>
#include <pitaya.h>
#include <stdbool.h>

#include "test_common.h"
#include "flag.h"

static pc_client_t *g_client = NULL;

static void
event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    Unused(arg1); Unused(arg2); Unused(client); Unused(ev_type);

    const int EVENTS[] = {
        PC_EV_CONNECTED, PC_EV_DISCONNECT
    };

    flag_t *flag = (flag_t*)ex_data;
    int num_called = flag_get_num_called(flag);
    assert_int(num_called, <, ArrayCount(EVENTS));
    assert_int(ev_type, ==, EVENTS[num_called]);
    flag_set(flag);
}

static char *RESPONSES_DISABLED[] = {
    "{\"isCompressed\":false}",
    "{\"isCompressed\":false}",
};

static void
request_cb_disabled(const pc_request_t* req, const pc_buf_t *resp)
{
    flag_t *flag = (flag_t*)pc_request_ex_data(req);
    flag_set(flag);
}

static char *RESPONSES_ENABLED[] = {
    "{\"isCompressed\":true}",
    "{\"isCompressed\":false}",
};

static void
request_cb_enabled(const pc_request_t* req, const pc_buf_t *resp)
{
    flag_t *flag = (flag_t*)pc_request_ex_data(req);
    flag_set(flag);
}

MunitResult
test_disabled_compression(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);

    const int ports[] = {g_compression_mock_server.tcp_port, g_compression_mock_server.tls_port};
    const int transports[] = {PC_TR_NAME_UV_TCP, PC_TR_NAME_UV_TLS};

    assert_int(tr_uv_tls_set_ca_file(CRT, NULL), ==, PC_RC_OK);

    for (size_t i = 0; i < ArrayCount(ports); i++) {
        flag_t flag_evs = flag_make();
        flag_t flag_req = flag_make();

        pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
        config.transport_name = transports[i];
        config.disable_compression = true;

        pc_client_init_result_t res = pc_client_init(NULL, &config);
        g_client = res.client;
        assert_int(res.rc, ==, PC_RC_OK);

        pc_client_add_ev_handler(g_client, event_cb, &flag_evs, NULL);

        assert_int(pc_client_connect(g_client, LOCALHOST, ports[i], NULL), ==, PC_RC_OK);
        assert_int(flag_wait(&flag_evs, 60), ==, FLAG_SET);

        const char *big_json = "{\"Data\":{\"name\":\"PEPE\",\"age\":\"veryyyyyyyyyyy old myyyyyyyyyyyyyyyyyyy boiiiiiiiiiiiiiiiiiiiiiiiiiiiiii\"}}";
        const char *empty_json = "{}";

        assert_int(pc_string_request_with_timeout(g_client, "irrelevant.route", big_json, &flag_req, 12, request_cb_disabled, NULL), ==, PC_RC_OK);
        assert_int(flag_wait(&flag_req, 60), ==, FLAG_SET);

        assert_int(pc_string_request_with_timeout(g_client, "irrelevant.route", empty_json, &flag_req, REQ_TIMEOUT, request_cb_disabled, NULL), ==, PC_RC_OK);
        assert_int(flag_wait(&flag_req, 60), ==, FLAG_SET);

        assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
        assert_int(flag_wait(&flag_evs, 60), ==, FLAG_SET);

        assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

        flag_cleanup(&flag_req);
        flag_cleanup(&flag_evs);
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
        flag_t flag_evs = flag_make();
        flag_t flag_req = flag_make();

        pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
        config.transport_name = transports[i];

        pc_client_init_result_t res = pc_client_init(NULL, &config);
        g_client = res.client;
        assert_int(res.rc, ==, PC_RC_OK);

        pc_client_add_ev_handler(g_client, event_cb, &flag_evs, NULL);

        assert_int(pc_client_connect(g_client, LOCALHOST, ports[i], NULL), ==, PC_RC_OK);
        assert_int(flag_wait(&flag_evs, 60), ==, FLAG_SET);

        const char *big_json = "{\"Data\":{\"name\":\"PEPE\",\"age\":\"veryyyyyyyyyyy old myyyyyyyyyyyyyyyyyyy boiiiiiiiiiiiiiiiiiiiiiiiiiiiiii\"}}";
        const char *empty_json = "{}";

        assert_int(pc_string_request_with_timeout(g_client, "irrelevant.route", big_json, &flag_req, 12, request_cb_enabled, NULL), ==, PC_RC_OK);
        assert_int(flag_wait(&flag_req, 60), ==, FLAG_SET);

        assert_int(pc_string_request_with_timeout(g_client, "irrelevant.route", empty_json, &flag_req, REQ_TIMEOUT, request_cb_enabled, NULL), ==, PC_RC_OK);
        assert_int(flag_wait(&flag_req, 60), ==, FLAG_SET);

        assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
        assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

        flag_cleanup(&flag_req);
        flag_cleanup(&flag_evs);
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
