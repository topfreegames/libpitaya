#include <stdio.h>
#include <pitaya.h>
#include "test_common.h"
#include "flag.h"

static pc_client_t *g_client = NULL;

static void
empty_event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    // This callback should not be called, therefore called should always be false.
    Unused(arg1); Unused(arg2); Unused(client); Unused(ev_type);
    flag_t *flag = (flag_t*)ex_data;
    if (ev_type == PC_EV_CONNECTED) {
        flag_set(flag);
    }
}

static MunitResult
test_polling(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);

    flag_t flag = flag_make();

    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    config.enable_polling = true;

    pc_client_init_result_t res = pc_client_init(NULL, &config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);

    int handler_id = pc_client_add_ev_handler(g_client, empty_event_cb, &flag, NULL);
    assert_int(handler_id, !=, PC_EV_INVALID_HANDLER_ID);

    assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, g_test_server.tcp_port, NULL), ==, PC_RC_OK);
    assert_int(flag_wait(&flag, 4), ==, FLAG_TIMEOUT);

    assert_int(pc_client_poll(NULL), ==, PC_RC_INVALID_ARG);
    assert_int(pc_client_poll(g_client), ==, PC_RC_OK);

    assert_int(flag_wait(&flag, 60), ==, FLAG_SET);
    assert_int(pc_client_rm_ev_handler(g_client, handler_id), ==, PC_RC_OK);
    assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

    flag_cleanup(&flag);
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

static void
connect_event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    Unused(arg1); Unused(arg2); Unused(client); Unused(ev_type);
    flag_t *flag = (flag_t*)ex_data;
    if (ev_type == PC_EV_CONNECTED) {
        flag_set(flag);
    }
}

static MunitResult
test_serializer(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);
    flag_t flag = flag_make();

    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    pc_client_init_result_t res = pc_client_init(NULL, &config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);

    pc_client_add_ev_handler(g_client, connect_event_cb, &flag, NULL);

    assert_null(pc_client_serializer(g_client));
    assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, g_test_server.tcp_port, NULL), ==, PC_RC_OK);
    assert_int(flag_wait(&flag, 60), ==, FLAG_SET);

    const char *serializer = pc_client_serializer(g_client);
    assert_not_null(serializer);
    assert_string_equal(serializer, "json");
    pc_client_free_serializer(serializer);

    assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

    flag_cleanup(&flag);
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

static void
error_event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    Unused(arg1); Unused(arg2); Unused(client); Unused(ev_type);
    flag_t *flag = (flag_t*)ex_data;
    assert_int(ev_type, ==, PC_EV_CONNECT_ERROR);
    flag_set(flag);
}

static MunitResult
test_creating_and_deleting(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);
    flag_t flag = flag_make();

    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;

    const int num_connections = 8;
    for (int i = 0; i < num_connections; ++i) {
        pc_client_init_result_t res = pc_client_init(NULL, &config);
        g_client = res.client;
        assert_int(res.rc, ==, PC_RC_OK);

        assert_int(pc_client_add_ev_handler(g_client, error_event_cb, &flag, NULL), !=, PC_EV_INVALID_HANDLER_ID);

        assert_int(pc_client_connect(g_client, LOCALHOST, 29301, NULL), ==, PC_RC_OK);
        assert_int(flag_wait(&flag, 60), ==, FLAG_SET);
        assert_int(pc_client_disconnect(g_client), ==, PC_RC_INVALID_STATE);

        assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);
    }

    flag_cleanup(&flag);
    return MUNIT_OK;
}

//static MunitResult
//test_disconnect_right_after_connect(const MunitParameter params[], void *data)
//{
//    Unused(params); Unused(data);
//
//    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
//    pc_client_init_result_t res = pc_client_init(NULL, &config);
//    g_client = res.client;
//    assert_int(res.rc, ==, PC_RC_OK);
//    assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, 3251, NULL), ==, PC_RC_OK);
//
//    assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
//    assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);
//
//    SLEEP_SECONDS(1);
//    return MUNIT_OK;
//}

static MunitTest tests[] = {
    {"/ex_data", test_pc_client_ex_data, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/client_config", test_pc_client_config, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/conn_quality", test_pc_client_conn_quality, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/trans_data", test_pc_client_trans_data, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/polling", test_polling, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/serializer", test_serializer, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/creating_and_deleting", test_creating_and_deleting, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
//    {"/disconnect_right_after_connect", test_disconnect_right_after_connect, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
};

const MunitSuite pc_client_suite = {
    "/pc_client", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};
