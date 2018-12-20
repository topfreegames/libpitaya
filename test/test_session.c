#include <stdlib.h>
#include <stdio.h>
#include <pitaya.h>
#include <stdbool.h>

#include "test_common.h"
#include "flag.h"

#define SESSION_DATA "{\"Data\":{\"animal\":2}}"

static pc_client_t *g_client = NULL;

typedef struct {
    char *data;
    flag_t flag;
} session_cb_data_t;

static void
event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    Unused(client); Unused(arg1); Unused(arg2);

    static int EVENTS[] = {
        PC_EV_CONNECTED,
        PC_EV_DISCONNECT,
    };

    flag_t *flag = (flag_t*)ex_data;
    int num_called = flag_get_num_called(flag);

    assert_int(num_called, <, ArrayCount(EVENTS));
    assert_int(ev_type, ==, EVENTS[num_called]);

    flag_set(flag);
}

static void
set_session_request_cb(const pc_request_t* req, const pc_buf_t *resp)
{
    assert_not_null(req);
    flag_t *flag = (flag_t*)pc_request_ex_data(req);
    flag_set(flag);
}

static void
get_session_request_cb(const pc_request_t* req, const pc_buf_t *resp)
{
    assert_not_null(req);
    session_cb_data_t *sd = (session_cb_data_t*)pc_request_ex_data(req);
    flag_set(&sd->flag);
}

static void
do_test_session_persistence(pc_client_config_t *config, int port)
{
    pc_client_init_result_t res = pc_client_init(NULL, config);
    g_client = res.client;
    assert_int(res.rc, ==, PC_RC_OK);

    flag_t ev_flag = flag_make();
    flag_t set_session_flag = flag_make();

    session_cb_data_t session_cb_data;
    session_cb_data.flag = flag_make();
    session_cb_data.data = EMPTY_RESP;

    int handler_id = pc_client_add_ev_handler(g_client, event_cb, &ev_flag, NULL);
    assert_int(handler_id, !=, PC_EV_INVALID_HANDLER_ID);

    assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, port, NULL), ==, PC_RC_OK);
    assert_int(flag_wait(&ev_flag, 60), ==, FLAG_SET);

    // Get empty session and check that the callback was called.
    assert_int(pc_string_request_with_timeout(g_client, "connector.getsessiondata", "{}", &session_cb_data,
                                              REQ_TIMEOUT, get_session_request_cb, NULL), ==, PC_RC_OK);
    assert_int(flag_wait(&session_cb_data.flag, 60), ==, FLAG_SET);

    session_cb_data.data = SESSION_DATA;

    // Set session and check that the callback was called.
    assert_int(pc_string_request_with_timeout(g_client, "connector.setsessiondata", SESSION_DATA, &set_session_flag,
                                              REQ_TIMEOUT, set_session_request_cb, NULL), ==, PC_RC_OK);
    assert_int(flag_wait(&set_session_flag, 60), ==, FLAG_SET);

    // Get session and check that the callback was called.
    assert_int(pc_string_request_with_timeout(g_client, "connector.getsessiondata", "{}", &session_cb_data,
                                              REQ_TIMEOUT, get_session_request_cb, NULL), ==, PC_RC_OK);
    assert_int(flag_wait(&session_cb_data.flag, 60), ==, FLAG_SET);

    assert_int(pc_string_request_with_timeout(g_client, "connector.getsessiondata", "{}", &session_cb_data,
                                              REQ_TIMEOUT, get_session_request_cb, NULL), ==, PC_RC_OK);
    assert_int(flag_wait(&session_cb_data.flag, 60), ==, FLAG_SET);

    pc_client_disconnect(g_client);

    assert_int(flag_wait(&ev_flag, 60), ==, FLAG_SET);

    pc_client_rm_ev_handler(g_client, handler_id);
    pc_client_cleanup(g_client);

    flag_cleanup(&ev_flag);
    flag_cleanup(&set_session_flag);
    flag_cleanup(&session_cb_data.flag);
}

static MunitResult
test_persistence(const MunitParameter params[], void *data)
{
    Unused(data); Unused(params);
    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    // Test with TCP
    do_test_session_persistence(&config, g_test_server.tcp_port);
    // Test with TLS
    config.transport_name = PC_TR_NAME_UV_TLS;
    assert_int(tr_uv_tls_set_ca_file(CRT, NULL), ==, PC_RC_OK);
    do_test_session_persistence(&config, g_test_server.tls_port);
    return MUNIT_OK;
}

static MunitTest tests[] = {
    {"/persistence", test_persistence, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
};

const MunitSuite session_suite = {
    "/session", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};
