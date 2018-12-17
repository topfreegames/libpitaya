#include <stdlib.h>
#include <stdio.h>
#include <pitaya.h>
#include <stdbool.h>

#include "test_common.h"
#include "flag.h"

#define NUM_REQUESTS_AT_ONCE 40

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

static void
request_cb(const pc_request_t* req, const pc_buf_t *resp)
{
    flag_t *flag = (flag_t*)pc_request_ex_data(req);
    flag_set(flag);
}

static void
request_error_cb(const pc_request_t* req, const pc_error_t *error)
{
    flag_t *flag = (flag_t*)pc_request_ex_data(req);
    flag_set(flag);
}

static MunitResult
test_multiple_requests(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);

    const int ports[] = {g_test_server.tcp_port, g_test_server.tls_port};
    const int transports[] = {PC_TR_NAME_UV_TCP, PC_TR_NAME_UV_TLS};

    assert_int(tr_uv_tls_set_ca_file(CRT, NULL), ==, PC_RC_OK);

    for (size_t i = 0; i < 2; i++) {
        flag_t flag_evs = flag_make();
        flag_t flag_req = flag_make();

        pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
        config.transport_name = transports[i];

        pc_client_init_result_t res = pc_client_init(NULL, &config);
        g_client = res.client;
        assert_int(res.rc, ==, PC_RC_OK);

        pc_client_add_ev_handler(g_client, event_cb, &flag_evs, NULL);

        assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, ports[i], NULL), ==, PC_RC_OK);
        assert_int(flag_wait(&flag_evs, 60), ==, FLAG_SET);

        for (int y = 0; y < NUM_REQUESTS_AT_ONCE; ++y) {
            pc_string_request_with_timeout(g_client, "connector.getsessiondata", "{}", &flag_req, 30,
                                           request_cb, request_error_cb);
        }

        while (flag_get_num_called(&flag_req) < NUM_REQUESTS_AT_ONCE) {
            assert_int(flag_wait(&flag_req, 60), ==, FLAG_SET);
        }

        assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
        assert_int(flag_wait(&flag_evs, 60), ==, FLAG_SET);

        assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

        flag_cleanup(&flag_evs);
        flag_cleanup(&flag_req);
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
