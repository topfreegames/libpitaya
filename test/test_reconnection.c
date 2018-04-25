#include <stdlib.h>
#include <stdio.h>
#include <pomelo.h>
#include <stdbool.h>

#include "test_common.h"

#define MOCK_SERVER_PORT 4000

static pc_client_t *g_client = NULL;

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

static int SUCCESS_RECONNECT_EV_ORDER[] = {
    PC_EV_CONNECTED,
    PC_EV_UNEXPECTED_DISCONNECT,
    PC_EV_RECONNECT_STARTED,
    PC_EV_CONNECTED,
};

static void
reconnect_success_event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    int *num_calls = ex_data;
//    printf("ev_type: %s\n", pc_client_ev_str(ev_type));
//    printf("%s\n", arg1);
//    fflush(stdout);
    assert_int_equal(SUCCESS_RECONNECT_EV_ORDER[*num_calls], ev_type);
    (*num_calls)++;
}

void
test_reconnect_success(void **state)
{
    Unused(state);

    pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;

    assert_int_equal(pc_client_init(g_client, NULL, &config), PC_RC_OK);

    int num_calls = 0;
    int handler_id = pc_client_add_ev_handler(g_client, reconnect_success_event_cb, &num_calls, NULL);
    assert_int_not_equal(handler_id, PC_EV_INVALID_HANDLER_ID);

    assert_int_equal(pc_client_connect(g_client, LOCALHOST, MOCK_SERVER_PORT, NULL), PC_RC_OK);
    SLEEP_SECONDS(10);

    assert_int_equal(num_calls, ArrayCount(SUCCESS_RECONNECT_EV_ORDER));
    assert_int_equal(pc_client_disconnect(g_client), PC_RC_OK);
    assert_int_equal(pc_client_rm_ev_handler(g_client, handler_id), PC_RC_OK);
    assert_int_equal(pc_client_cleanup(g_client), PC_RC_OK);
}

static int RECONNECT_MAX_RETRY_EV_ORDER[] = {
    PC_EV_CONNECT_ERROR,
    PC_EV_RECONNECT_STARTED,
    PC_EV_CONNECT_ERROR,
    PC_EV_CONNECT_ERROR,
    PC_EV_CONNECT_ERROR,
    PC_EV_RECONNECT_FAILED,
};

static void
reconnect_event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    int *num_called = ex_data;
//    printf("ev_type %s\n", pc_client_ev_str(ev_type));
    assert_int_equal(RECONNECT_MAX_RETRY_EV_ORDER[*num_called], ev_type);
    (*num_called)++;
}

void
test_reconnect_max_retry(void **state)
{
    Unused(state);

    pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
    config.reconn_max_retry = 3;
    config.reconn_delay = 1;

    assert_int_equal(pc_client_init(g_client, NULL, &config), PC_RC_OK);

    int num_called = 0;
    int handler_id = pc_client_add_ev_handler(g_client, reconnect_event_cb, &num_called, NULL);
    assert_int_not_equal(handler_id, PC_EV_INVALID_HANDLER_ID);

    assert_int_equal(pc_client_connect(g_client, "invalidhost", TCP_PORT, NULL), PC_RC_OK);
    SLEEP_SECONDS(8);

    // There should be reconn_max_retry reconnections retries. This means the event_cb should be called
    // reconn_max_retry + 1 times with PC_EV_CONNECT_ERROR and one time with PC_EV_RECONNECT_FAILED.
    assert_int_equal(num_called, ArrayCount(RECONNECT_MAX_RETRY_EV_ORDER));
    assert_int_equal(pc_client_rm_ev_handler(g_client, handler_id), PC_RC_OK);
    assert_int_equal(pc_client_cleanup(g_client), PC_RC_OK);
}

int
main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_reconnect_max_retry, setup_global_client, teardown_global_client),
        cmocka_unit_test_setup_teardown(test_reconnect_success, setup_global_client, teardown_global_client),
    };
    return cmocka_run_group_tests(tests, setup_pc_lib, teardown_pc_lib);
}
