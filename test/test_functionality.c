#include <stdlib.h>
#include <stdio.h>
#include <pomelo.h>
#include <stdbool.h>

#include "test_common.h"

#define SESSION_DATA "{\"Data\":{\"animal\":2}}"

static pc_client_t *g_client = NULL;

typedef struct {
    bool called;
    char *data;
} session_cb_data_t;

static int
setup_global_client(void **state)
{
    Unused(state);
    // NOTE: use calloc in order to avoid one of the issues with the api.
    // see `issues.md`.
    g_client = calloc(1, pc_client_size());
    if (!g_client) {
        return -1;
    }
    return 0;
}

static int
teardown_global_client(void **state)
{
    Unused(state);
    free(g_client);
    g_client = NULL;
    return 0;
}

static void
event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    bool *connected = ex_data;
    *connected = true;
    assert_int_equal(ev_type, PC_EV_CONNECTED);
}

static void
set_session_request_cb(const pc_request_t* req, int rc, const char* resp)
{
    bool *called = pc_request_ex_data(req);
    *called = true;
    assert_int_equal(rc, PC_RC_OK);
    assert_string_equal(resp, SUCCESS_RESP);
    assert_non_null(req);
}

static void
get_session_request_cb(const pc_request_t* req, int rc, const char* resp)
{
    session_cb_data_t *scd = pc_request_ex_data(req);
    scd->called = true;
    assert_int_equal(rc, PC_RC_OK);
    assert_string_equal(resp, scd->data);
    assert_non_null(req);
}

void
do_test_session_persistence(pc_client_config_t *config, int port)
{
    assert_int_equal(pc_client_init(g_client, NULL, config), PC_RC_OK);

    bool connected = false;
    int handler_id = pc_client_add_ev_handler(g_client, event_cb, &connected, NULL);
    assert_int_not_equal(handler_id, PC_EV_INVALID_HANDLER_ID);

    assert_int_equal(pc_client_connect(g_client, "127.0.0.1", port, NULL), PC_RC_OK);
    SLEEP_SECONDS(1);

    assert_true(connected);

    session_cb_data_t session_cb_data;
    session_cb_data.called = false;
    session_cb_data.data = EMPTY_RESP;

    // TODO: why passing NULL msg is an invalid argument?
    assert_int_equal(pc_request_with_timeout(g_client, "connector.getsessiondata", NULL, NULL, REQ_TIMEOUT, get_session_request_cb, NULL), PC_RC_INVALID_ARG);

    // Get empty session and check that the callback was called.
    assert_int_equal(pc_request_with_timeout(g_client, "connector.getsessiondata", "{}", &session_cb_data, REQ_TIMEOUT, get_session_request_cb, NULL), PC_RC_OK);
    SLEEP_SECONDS(1);
    assert_true(session_cb_data.called);

    session_cb_data.called = false;
    session_cb_data.data = SESSION_DATA;

    // Set session and check that the callback was called.
    bool set_session_cb_called = false;
    assert_int_equal(pc_request_with_timeout(g_client, "connector.setsessiondata", SESSION_DATA, &set_session_cb_called, REQ_TIMEOUT, set_session_request_cb, NULL), PC_RC_OK);
    SLEEP_SECONDS(1);
    assert_true(set_session_cb_called);

    // Get session and check that the callback was called.
    assert_int_equal(pc_request_with_timeout(g_client, "connector.getsessiondata", "{}", &session_cb_data, REQ_TIMEOUT, get_session_request_cb, NULL), PC_RC_OK);
    SLEEP_SECONDS(1);
    assert_true(session_cb_data.called);

    session_cb_data.called = false;
    assert_int_equal(pc_request_with_timeout(g_client, "connector.getsessiondata", "{}", &session_cb_data, REQ_TIMEOUT, get_session_request_cb, NULL), PC_RC_OK);
    SLEEP_SECONDS(1);
    assert_true(session_cb_data.called);

    pc_client_disconnect(g_client);
    pc_client_rm_ev_handler(g_client, handler_id);
    pc_client_cleanup(g_client);
}

void
test_session_persistence(void **state)
{
    Unused(state);

    pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
    // Test with TCP
    do_test_session_persistence(&config, TCP_PORT);
    // Test with TLS
    config.transport_name = PC_TR_NAME_UV_TLS;
    assert_true(tr_uv_tls_set_ca_file("../../test/server/fixtures/ca.crt", NULL));
    do_test_session_persistence(&config, TLS_PORT);
}

int
main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_session_persistence, setup_global_client, teardown_global_client),
    };
    return cmocka_run_group_tests(tests, setup_pc_lib, teardown_pc_lib);
}
