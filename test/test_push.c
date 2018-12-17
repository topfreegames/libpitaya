#include <stdlib.h>
#include <stdio.h>
#include <pitaya.h>
#include <stdbool.h>

#include "test_common.h"
#include "flag.h"

static pc_client_t *g_client = NULL;

static void
request_cb(const pc_request_t* req, const pc_buf_t *resp)
{
    flag_t *flag = (flag_t*)pc_request_ex_data(req);
    flag_set(flag);
}

static int EV_ORDER[] = {
    PC_EV_CONNECTED,
    PC_EV_DISCONNECT,
};

static void
event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    Unused(client); Unused(arg1); Unused(arg2);
    flag_t *flag = (flag_t*)ex_data;
    int num_called = flag_get_num_called(flag);
    assert_int(num_called, <, ArrayCount(EV_ORDER));
    assert_int(ev_type, ==, EV_ORDER[num_called]);
    flag_set(flag);
}

void
push_handler(pc_client_t *client, const char *route, const pc_buf_t *payload)
{
    assert_not_null(payload);
    assert_not_null(payload->base);
    assert_string_equal(route, "some.push.route");

    char *str = calloc(payload->len+1, 1);
    assert_not_null(str);
    memcpy(str, payload->base, payload->len);

    assert_string_equal(str, "{\"key1\":10,\"key2\":true}");

    free(str);

    flag_t *flag = (flag_t*)pc_client_ex_data(client);
    flag_set(flag);
}

static MunitResult
test_success(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);

    const int ports[] = {g_test_server.tcp_port, g_test_server.tls_port};
    const int transports[] = {PC_TR_NAME_UV_TCP, PC_TR_NAME_UV_TLS};

    assert_int(tr_uv_tls_set_ca_file(CRT, NULL), ==, PC_RC_OK);

    for (size_t i = 0; i < ArrayCount(ports); i++) {
        flag_t flag_evs = flag_make();
        flag_t flag_req = flag_make();
        flag_t flag_push = flag_make();

        pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
        config.transport_name = transports[i];

        pc_client_init_result_t res = pc_client_init(&flag_push, &config);
        g_client = res.client;
        assert_int(res.rc, ==, PC_RC_OK);

        pc_client_set_push_handler(g_client, push_handler);
        int handler_id = pc_client_add_ev_handler(g_client, event_cb, &flag_evs, NULL);

        assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, ports[i], NULL), ==, PC_RC_OK);
        assert_int(flag_wait(&flag_evs, 60), ==, FLAG_SET);

        assert_int(pc_string_request_with_timeout(g_client, "connector.sendpush", "{}", &flag_req, REQ_TIMEOUT,
                                                  request_cb, NULL), ==, PC_RC_OK);
        assert_int(flag_wait(&flag_req, 60), ==, FLAG_SET);
        assert_int(flag_wait(&flag_push, 60), ==, FLAG_SET);

        assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
        assert_int(flag_wait(&flag_evs, 60), ==, FLAG_SET);

        pc_client_rm_ev_handler(g_client, handler_id);
        assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

        flag_cleanup(&flag_push);
        flag_cleanup(&flag_req);
        flag_cleanup(&flag_evs);
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
