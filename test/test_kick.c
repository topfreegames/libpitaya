#include <stdlib.h>
#include <stdio.h>
#include <pitaya.h>
#include <stdbool.h>

#include "test_common.h"
#include "flag.h"

static pc_client_t *g_client = NULL;

static int EV_ORDER[] = {
    PC_EV_CONNECTED,
    PC_EV_KICKED_BY_SERVER,
    PC_EV_CONNECTED,
    PC_EV_KICKED_BY_SERVER,
};

static void
event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    Unused(client); Unused(arg1); Unused(arg2);
    flag_t *flag = (flag_t*)ex_data;
    int num_called = flag_get_num_called(flag);
    assert_int(ev_type, ==, EV_ORDER[num_called]);
    assert_int(num_called, <, ArrayCount(EV_ORDER));
    flag_set(flag);
}

MunitResult
test_kick(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);

    const int ports[] = {g_kick_mock_server.tcp_port, g_kick_mock_server.tls_port};
    const int transports[] = {PC_TR_NAME_UV_TCP, PC_TR_NAME_UV_TLS};

    assert_int(tr_uv_tls_set_ca_file(CRT, NULL), ==, PC_RC_OK);

    for (size_t i = 0; i < ArrayCount(ports); i++) {
        flag_t flag = flag_make();

        pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
        config.transport_name = transports[i];

        pc_client_init_result_t res = pc_client_init(NULL, &config);
        g_client = res.client;
        assert_int(res.rc, ==, PC_RC_OK);

        int handler_id = pc_client_add_ev_handler(g_client, event_cb, &flag, NULL);
        assert_int(handler_id, !=, PC_EV_INVALID_HANDLER_ID);

        assert_int(pc_client_connect(g_client, LOCALHOST, ports[i], NULL), ==, PC_RC_OK);

        while (flag_get_num_called(&flag) < 2) {
            assert_int(flag_wait(&flag, 60), ==, FLAG_SET);
        }

        assert_int(pc_client_state(g_client), ==, PC_ST_INITED);

        assert_int(pc_client_connect(g_client, LOCALHOST, ports[i], NULL), ==, PC_RC_OK);

        while (flag_get_num_called(&flag) < ArrayCount(EV_ORDER)) {
            assert_int(flag_wait(&flag, 60), ==, FLAG_SET);
        }

        assert_int(pc_client_state(g_client), ==, PC_ST_INITED);

        assert_int(pc_client_disconnect(g_client), ==, PC_RC_INVALID_STATE);
        assert_int(pc_client_rm_ev_handler(g_client, handler_id), ==, PC_RC_OK);
        assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

        flag_cleanup(&flag);
    }

    return MUNIT_OK;
}

static MunitTest tests[] = {
    {"", test_kick, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
};

const MunitSuite kick_suite = {
    "/kick", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};
