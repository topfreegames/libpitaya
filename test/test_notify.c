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
}

static void
request_error_cb(const pc_request_t* req, const pc_error_t *error)
{
}

static void
event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    Unused(client); Unused(arg1); Unused(arg2);
    static int EV_ORDER[] = {
        PC_EV_CONNECTED,
        PC_EV_DISCONNECT,
    };

    flag_t *flag = (flag_t*)ex_data;
    int num_called = flag_get_num_called(flag);
    assert_int(num_called, <, ArrayCount(EV_ORDER));
    assert_int(ev_type, ==, EV_ORDER[num_called]);
    flag_set(flag);
}

static void
notify_error_cb(const pc_notify_t* not, const pc_error_t *error)
{
    assert_int(error->code, ==, PC_RC_RESET);
    flag_t *flag = (flag_t*)pc_notify_ex_data(not);
    flag_set(flag);
}

//static MunitResult
//test_reset(const MunitParameter params[], void *data)
//{
//    Unused(params); Unused(data);
//
//    const int ports[] = {g_destroy_socket_mock_server.tcp_port, g_destroy_socket_mock_server.tls_port};
//    const int transports[] = {PC_TR_NAME_UV_TCP, PC_TR_NAME_UV_TLS};
//
//    assert_int(tr_uv_tls_set_ca_file(CRT, NULL), ==, PC_RC_OK);
//
//    for (size_t i = 0; i < ArrayCount(ports); i++) {
//        flag_t flag_evs = flag_make();
//        flag_t flag_notify = flag_make();
//
//        pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
//        config.transport_name = transports[i];
//
//        pc_client_init_result_t res = pc_client_init(NULL, &config);
//        g_client = res.client;
//        assert_int(res.rc, ==, PC_RC_OK);
//
//        int handler = pc_client_add_ev_handler(g_client, event_cb, &flag_evs, NULL);
//
//        assert_int(pc_client_connect(g_client, LOCALHOST, ports[i], NULL), ==, PC_RC_OK);
//        assert_int(flag_wait(&flag_evs, 60), ==, FLAG_SET);
//
//        assert_int(pc_string_notify_with_timeout(g_client, "connector.getsessiondata", "{}",
//                                                 &flag_notify, 1, notify_error_cb), ==, PC_RC_OK);
//        assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
//
//        assert_int(flag_wait(&flag_notify, 60), ==, FLAG_SET);
//        assert_int(flag_wait(&flag_evs, 60), ==, FLAG_SET);
//
//        pc_client_rm_ev_handler(g_client, handler);
//        assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);
//
//        flag_cleanup(&flag_notify);
//        flag_cleanup(&flag_evs);
//    }
//
//    return MUNIT_OK;
//}

static MunitResult
test_success(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);

    const int ports[] = {g_timeout_mock_server.tcp_port, g_timeout_mock_server.tls_port};
    const int transports[] = {PC_TR_NAME_UV_TCP, PC_TR_NAME_UV_TLS};

    assert_int(tr_uv_tls_set_ca_file(CRT, NULL), ==, PC_RC_OK);

    for (size_t i = 0; i < ArrayCount(ports); i++) {
        flag_t flag_notify = flag_make();
        flag_t flag_evs = flag_make();

        pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
        config.transport_name = transports[i];

        pc_client_init_result_t res = pc_client_init(NULL, &config);
        g_client = res.client;
        assert_int(res.rc, ==, PC_RC_OK);

        pc_client_add_ev_handler(g_client, event_cb, &flag_evs, NULL);

        assert_int(pc_string_notify_with_timeout(g_client, "connector.getsessiondata", "{}", NULL, 1,
                                                 notify_error_cb), ==, PC_RC_INVALID_STATE);
        assert_int(flag_wait(&flag_notify, 3), ==, FLAG_TIMEOUT);

        assert_int(pc_client_connect(g_client, LOCALHOST, ports[i], NULL), ==, PC_RC_OK);
        assert_int(flag_wait(&flag_evs, 60), ==, FLAG_SET);

        for (int i = 0; i < 1; i++) {
            assert_int(pc_string_request_with_timeout(g_client, "connector.getsessiondata", "{}", NULL, 1,
                                                      request_cb, request_error_cb), ==, PC_RC_OK);
        }

        assert_int(pc_string_notify_with_timeout(g_client, "connector.getsessiondata", "{}", &flag_notify, 1,
                                                 NULL), ==, PC_RC_OK);
        assert_int(pc_string_notify_with_timeout(g_client, "connector.getsessiondata", "{}", &flag_notify, 1,
                                                 notify_error_cb), ==, PC_RC_OK);
        assert_int(flag_wait(&flag_notify, 3), ==, FLAG_TIMEOUT);

        assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
        assert_int(flag_wait(&flag_evs, 60), ==, FLAG_SET);

        assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

        flag_cleanup(&flag_evs);
        flag_cleanup(&flag_notify);
    }

    return MUNIT_OK;
}

static MunitTest tests[] = {
    {"/success", test_success, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    //{"/reset", test_reset, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
};

const MunitSuite notify_suite = {
    "/notify", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};
