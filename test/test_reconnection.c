#include <stdlib.h>
#include <stdio.h>
#include <pomelo.h>
#include <stdbool.h>

#include "test_common.h"

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

typedef struct {
    int num_reconnect_failed_events;
    int num_connect_error_events;
} event_cb_ctx_t;

static void
reconnect_event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    event_cb_ctx_t *ctx = ex_data;

    assert_true(ev_type == PC_EV_CONNECT_ERROR || ev_type == PC_EV_RECONNECT_FAILED);

    if (ev_type == PC_EV_CONNECT_ERROR) {
        assert_string_equal(arg1, "DNS Resolve Error");
        ctx->num_connect_error_events++;
    } else if (ev_type == PC_EV_RECONNECT_FAILED) {
        assert_string_equal(arg1, "Exceed Max Retry");
        ctx->num_reconnect_failed_events++;
    }
}

void
test_reconnect_success(void **state)
{
    Unused(state);
    // Guarantee that the server is not running.
    system("pkill server-exe");

    pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
    config.reconn_max_retry = 4;

//    assert_int_equal(pc_client_init(g_client, NULL, &config), PC_RC_OK);

//    assert_int_equal(pc_client_rm_ev_handler(g_client, handler_id), PC_RC_OK);
//    assert_int_equal(pc_client_cleanup(g_client), PC_RC_OK);
    // TODO: Make a mock server that allows for this kind of behaviour
}

void
test_reconnect_max_retry(void **state)
{
    Unused(state);

    pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
    config.reconn_max_retry = 3;
    config.reconn_delay = 1;

    assert_int_equal(pc_client_init(g_client, NULL, &config), PC_RC_OK);

    event_cb_ctx_t ctx;
    ctx.num_connect_error_events = 0;
    ctx.num_reconnect_failed_events = 0;

    int handler_id = pc_client_add_ev_handler(g_client, reconnect_event_cb, &ctx, NULL);
    assert_int_not_equal(handler_id, PC_EV_INVALID_HANDLER_ID);

    assert_int_equal(pc_client_connect(g_client, "invalidhost", TCP_PORT, NULL), PC_RC_OK);
    SLEEP_SECONDS(10);

    // There should be reconn_max_retry reconnections retries. This means the event_cb should be called
    // reconn_max_retry + 1 times with PC_EV_CONNECT_ERROR and one time with PC_EV_RECONNECT_FAILED.
    assert_true(ctx.num_connect_error_events == config.reconn_max_retry+1);
    assert_true(ctx.num_reconnect_failed_events == 1);

    assert_int_equal(pc_client_rm_ev_handler(g_client, handler_id), PC_RC_OK);
    assert_int_equal(pc_client_cleanup(g_client), PC_RC_OK);
}

int
main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_reconnect_max_retry, setup_global_client, teardown_global_client),
    };
    return cmocka_run_group_tests(tests, setup_pc_lib, teardown_pc_lib);
}
