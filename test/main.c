#include <stdio.h>
#include <pitaya.h>
#include "test_common.h"

// HACK(leo): DO NOT EDIT THE LINES BETWEEN SUITES_START AND SUITES_END
// In order to update the number of suites automatically when a new suite is added,
// we compute the number of suites based on the number of lines between SUITES_START
// and SUITES_END. Therefore, adding a blank line between the two delimiters will incorrectly
// count the number of suites. So do not edit it!
static const int SUITES_START = __LINE__;
extern const MunitSuite pc_client_suite;
extern const MunitSuite tcp_suite;
extern const MunitSuite tls_suite;
extern const MunitSuite session_suite;
extern const MunitSuite reconnection_suite;
extern const MunitSuite compression_suite;
extern const MunitSuite kick_suite;
extern const MunitSuite request_suite;
extern const MunitSuite notify_suite;
extern const MunitSuite stress_suite;
extern const MunitSuite protobuf_suite;
extern const MunitSuite push_suite;
static const int SUITES_END = __LINE__;

const MunitSuite null_suite = {
    NULL, NULL, NULL, 0, MUNIT_SUITE_OPTION_NONE
};

static void
quiet_log(int level, const char *msg, ...)
{
    Unused(level); Unused(msg);
    // Use an empty log to avoid messing up the output of the tests.
    // TODO: maybe print only logs of a certain level?
}

static MunitSuite *
make_suites()
{
    const int NUM_SUITES = SUITES_END - SUITES_START - 1;
    MunitSuite *suites_array = (MunitSuite*)calloc(NUM_SUITES+1, sizeof(MunitSuite));
    size_t i = 0;
    suites_array[i++] = pc_client_suite;
    suites_array[i++] = tcp_suite;
    suites_array[i++] = tls_suite;
    suites_array[i++] = session_suite;
    suites_array[i++] = reconnection_suite;
    suites_array[i++] = compression_suite;
    suites_array[i++] = kick_suite;
    suites_array[i++] = request_suite;
    suites_array[i++] = notify_suite;
    suites_array[i++] = stress_suite;
    suites_array[i++] = protobuf_suite;
    suites_array[i++] = push_suite;
    // IMPORTANT: always has to end with a null suite
    suites_array[i++] = null_suite;
    return suites_array;
}

int main(int argc, char **argv)
{
    MunitSuite *suites_array = make_suites();

    MunitSuite main_suite;
    main_suite.prefix = "";
    main_suite.tests = NULL;
    main_suite.suites = suites_array;
    main_suite.iterations = 1;
    main_suite.options = MUNIT_SUITE_OPTION_NONE;

    pc_lib_client_info_t client_info;
    client_info.platform = "mac";
    client_info.build_number = "20";
    client_info.version = "2.1";

    // Run this function only one time, otherwise things break.
#if 0
    pc_lib_init(quiet_log, NULL, NULL, NULL, client_info);
#else
    pc_lib_init(NULL, NULL, NULL, NULL, client_info);
#endif
    // By default, skip all checks of key pinning on tests.
    // If the tests wants to check the key pinning functionality,
    // it will explicitly set the check to false.
    pc_lib_skip_key_pin_check(true);

    int ret = munit_suite_main(&main_suite, NULL, argc, argv);

    pc_lib_cleanup();

    free(suites_array);
    return ret;
}
