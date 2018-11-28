#include <stdio.h>
#include <pitaya.h>
#include "test_common.h"

static pc_client_t *g_client = NULL;

static void
empty_event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    // This callback should not be called, therefore called should always be false.
    Unused(arg1); Unused(arg2); Unused(client); Unused(ev_type);
    bool *called = (bool*)ex_data;
    *called = true;
}

static MunitResult
test_polling(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);
    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    config.enable_polling = true;

    bool called = false;

    pc_client_init_result_t res = pc_client_init(NULL, &config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);

    int handler_id = pc_client_add_ev_handler(g_client, empty_event_cb, &called, NULL);
    assert_int(handler_id, !=, PC_EV_INVALID_HANDLER_ID);

    assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, g_test_server.tcp_port, NULL), ==, PC_RC_OK);
    SLEEP_SECONDS(1);
    assert_false(called);

    assert_int(pc_client_poll(NULL), ==, PC_RC_INVALID_ARG);
    assert_int(pc_client_poll(g_client), ==, PC_RC_OK);

    assert_true(called);
    assert_int(pc_client_rm_ev_handler(g_client, handler_id), ==, PC_RC_OK);
    assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);
    return MUNIT_OK;
}

static MunitResult
test_pc_client_ex_data(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);
#define RANDOM_PTR (void*)0xdeadbeef
    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;

    pc_client_init_result_t res = pc_client_init(RANDOM_PTR, &config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);

    assert_ptr_equal(pc_client_ex_data(g_client), RANDOM_PTR);
    assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);
    return MUNIT_OK;
#undef RANDOM_PTR
}

static MunitResult
test_pc_client_config(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);
    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    pc_client_init_result_t res = pc_client_init(NULL, &config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);

    assert_int(pc_client_state(g_client), ==, PC_ST_INITED);
    assert_memory_equal(sizeof(pc_client_config_t), pc_client_config(g_client), &config);
    assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);
    return MUNIT_OK;
}

static MunitResult
test_pc_client_conn_quality(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);
    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    pc_client_init_result_t res = pc_client_init(NULL, &config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);

    pc_client_connect(g_client, PITAYA_SERVER_URL, g_test_server.tcp_port, NULL);

//    for (int i = 0; i < 300; i++)
//    {
//        SLEEP_SECONDS(1);
//        int cq = pc_client_conn_quality(g_client);
//        printf("Connection quality: %d\n", cq);
//        fflush(stdout);
//    }

    pc_client_cleanup(g_client);
    return MUNIT_SKIP;
}

static MunitResult
test_serializer(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);

    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    pc_client_init_result_t res = pc_client_init(NULL, &config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);

    assert_null(pc_client_serializer(g_client));
    assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, g_test_server.tcp_port, NULL), ==, PC_RC_OK);

    SLEEP_SECONDS(1);

    assert_string_equal(pc_client_serializer(g_client), "json");

    assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);
    return MUNIT_OK;
}

static MunitResult
test_pc_client_trans_data(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);
    // TODO
    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    pc_client_init_result_t res = pc_client_init(NULL, &config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);

    assert_null(pc_client_trans_data(NULL));
    assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);
    return MUNIT_SKIP;
}

static MunitTest tests[] = {
    {"/ex_data", test_pc_client_ex_data, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/client_config", test_pc_client_config, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/conn_quality", test_pc_client_conn_quality, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/trans_data", test_pc_client_trans_data, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/polling", test_polling, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/serializer", test_serializer, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
};

const MunitSuite pc_client_suite = {
    "/pc_client", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};
