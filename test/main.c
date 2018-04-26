#include <stdio.h>
#include <pomelo.h>
#include "test_common.h"

#define NUM_SUITES 5

extern const MunitSuite pc_client_suite;
extern const MunitSuite tcp_suite;
extern const MunitSuite tls_suite;
extern const MunitSuite session_suite;
extern const MunitSuite reconnection_suite;

const MunitSuite null_suite = {
    NULL, NULL, NULL, 0, MUNIT_SUITE_OPTION_NONE
};

static MunitSuite *
make_suites()
{
    MunitSuite *suites_array = calloc(NUM_SUITES+1, sizeof(MunitSuite));
    size_t i = 0;
    suites_array[i++] = pc_client_suite;
    suites_array[i++] = tcp_suite;
    suites_array[i++] = tls_suite;
    suites_array[i++] = session_suite;
    suites_array[i++] = reconnection_suite;
    suites_array[i++] = null_suite; // always has to end with a null suite
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

    // Run this function only one time, otherwise things break.
    pc_lib_init(quiet_log, NULL, NULL, NULL, NULL);
//    pc_lib_init(NULL, NULL, NULL, NULL, NULL);

    int ret = munit_suite_main(&main_suite, NULL, argc, argv);

    pc_lib_cleanup();

    free(suites_array);
    return ret;
}
