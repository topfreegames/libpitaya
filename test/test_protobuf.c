#include <stdio.h>
#include <pomelo.h>
#include <stdint.h>
#include <stdbool.h>
#include <pb_decode.h>
#include <pb_encode.h>

#include "error.pb.h"
#include "session-data.pb.h"
#include "response.pb.h"
#include "test_common.h"

static pc_client_t *g_client = NULL;

static int g_num_success_cb_called = 0;
static int g_num_error_cb_called = 0;
static int g_num_timeout_error_cb_called = 0;

static void
event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    // This callback should not be called, therefore called should always be false.
    Unused(arg1); Unused(arg2); Unused(client); Unused(ev_type);
    // bool *called = (bool*)ex_data;
    // *called = true;
}

static bool 
read_varint(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    uint64_t *value = (uint64_t)(*arg);
    return pb_decode_varint(stream, value);
}

static bool
read_string(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    uint8_t *buf = calloc(stream->bytes_left+1, 1);
    assert_not_null(buf);

    if (!pb_read(stream, buf, stream->bytes_left)) {
        free(buf);
        return false;
    }
    
    *arg = buf;
    return true;
}

static bool
write_string(pb_ostream_t *stream, const pb_field_t *field, void * const *arg)
{
    return pb_encode_tag_for_field(stream, field) && 
           pb_encode_string(stream, *arg, strlen(*arg));
}

static void
encoded_request_cb(const pc_request_t* req, const pc_buf_t *resp)
{
    assert_not_null(resp);
    assert_not_null(resp);
    assert_int(resp->len, >, 0);

    protos_Response response = protos_Response_init_zero;
    response.msg.funcs.decode = read_string;
    response.msg.arg = NULL;

    pb_istream_t stream = pb_istream_from_buffer(resp->base, resp->len);

    assert_true(pb_decode(&stream, protos_Response_fields, &response));

    assert_int(response.code, ==, 200);
    assert_string_equal((char*)response.msg.arg, "success");

    free(response.msg.arg);

    g_num_success_cb_called++;
}

static void
request_cb(const pc_request_t* req, const pc_buf_t *resp)
{
    assert_not_null(resp);
    assert_not_null(resp);
    assert_int(resp->len, >, 0);

    protos_SessionData session_data = protos_SessionData_init_zero;
    session_data.data.funcs.decode = read_string;
    session_data.data.arg = NULL;

    pb_istream_t stream = pb_istream_from_buffer(resp->base, resp->len);

    assert_true(pb_decode(&stream, protos_SessionData_fields, &session_data));
    assert_string_equal((char*)session_data.data.arg, "THIS IS THE SESSION DATA");

    free(session_data.data.arg);

    g_num_success_cb_called++;
}

static void
request_error_cb(const pc_request_t* req, const pc_error_t *error)
{
    assert_not_null(error->payload.base);
    assert_int(error->code, ==, PC_RC_SERVER_ERROR);

    protos_Error decoded_error = protos_Error_init_zero;
    decoded_error.code.funcs.decode = read_string;
    decoded_error.code.arg = NULL;

    decoded_error.msg.funcs.decode = &read_string;
    decoded_error.msg.arg = NULL;

    decoded_error.metadata.funcs.decode = NULL;

    pb_istream_t stream = pb_istream_from_buffer(error->payload.base, error->payload.len);

    assert_true(pb_decode(&stream, protos_Error_fields, &decoded_error));
    assert_string_equal((char*)decoded_error.code.arg, "PIT-404");
    assert_string_equal((char*)decoded_error.msg.arg, "pitaya/handler: connector.connector.route not found");

    free(decoded_error.code.arg);
    free(decoded_error.msg.arg);

    g_num_error_cb_called++;
}

static MunitResult
test_request_encoding(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);

    const int ports[] = {g_test_protobuf_server.tcp_port, g_test_protobuf_server.tls_port};
    const int transports[] = {PC_TR_NAME_UV_TCP, PC_TR_NAME_UV_TLS};

    assert_int(tr_uv_tls_set_ca_file(CRT, NULL), ==, PC_RC_OK);

    for (size_t i = 0; i < ArrayCount(ports); ++i) {
        pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
        config.transport_name = transports[i];

        pc_client_init_result_t res = pc_client_init(NULL, &config);
        g_client = res.client;
        assert_int(res.rc, ==, PC_RC_OK);

        int handler = pc_client_add_ev_handler(g_client, event_cb, NULL, NULL);

        SLEEP_SECONDS(1);

        assert_null(pc_client_serializer(g_client));

        assert_int(pc_client_connect(g_client, LOCALHOST, ports[i], NULL), ==, PC_RC_OK);
        SLEEP_SECONDS(1);

        assert_string_equal(pc_client_serializer(g_client), "protobuf");

        protos_SessionData session_data = protos_SessionData_init_zero;
        session_data.data.funcs.encode = write_string;
        session_data.data.arg = "Joelho";

        uint8_t buf[256];
        pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(buf));

        assert_true(pb_encode(&stream, protos_SessionData_fields, &session_data));
        assert_int(stream.bytes_written, <=, sizeof(buf));
        assert_int(pc_binary_request_with_timeout(g_client, "connector.setsessiondata", buf, stream.bytes_written, NULL, 
                                                  REQ_TIMEOUT, encoded_request_cb, request_error_cb), ==, PC_RC_OK);

        SLEEP_SECONDS(1);

        assert_int(g_num_error_cb_called, ==, 0);
        assert_int(g_num_success_cb_called, ==, 1);

        g_num_success_cb_called = 0;

        // assert_true(ev_cb_called);
        assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
        pc_client_rm_ev_handler(g_client, handler);
        assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);
    }

    return MUNIT_OK;
}

static MunitResult
test_response_decoding(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);

    const int ports[] = {g_test_protobuf_server.tcp_port, g_test_protobuf_server.tls_port};
    const int transports[] = {PC_TR_NAME_UV_TCP, PC_TR_NAME_UV_TLS};

    assert_int(tr_uv_tls_set_ca_file(CRT, NULL), ==, PC_RC_OK);

    for (size_t i = 0; i < ArrayCount(ports); ++i) {
        pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
        config.transport_name = transports[i];

        pc_client_init_result_t res = pc_client_init(NULL, &config);
        g_client = res.client;
        assert_int(res.rc, ==, PC_RC_OK);

        int handler = pc_client_add_ev_handler(g_client, event_cb, NULL, NULL);

        SLEEP_SECONDS(1);

        assert_null(pc_client_serializer(g_client));

        assert_int(pc_client_connect(g_client, LOCALHOST, ports[i], NULL), ==, PC_RC_OK);
        SLEEP_SECONDS(1);

        assert_string_equal(pc_client_serializer(g_client), "protobuf");

        assert_int(pc_string_request_with_timeout(g_client, "connector.getsessiondata", "{}", NULL, 
                                                  REQ_TIMEOUT, request_cb, request_error_cb), ==, PC_RC_OK);

        SLEEP_SECONDS(1);

        assert_int(g_num_error_cb_called, ==, 0);
        assert_int(g_num_success_cb_called, ==, 1);

        g_num_success_cb_called = 0;

        // assert_true(ev_cb_called);
        assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
        pc_client_rm_ev_handler(g_client, handler);
        assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);
    }

    return MUNIT_OK;
}

static MunitResult
test_error_decoding(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);

    const int ports[] = {g_test_protobuf_server.tcp_port, g_test_protobuf_server.tls_port};
    const int transports[] = {PC_TR_NAME_UV_TCP, PC_TR_NAME_UV_TLS};

    assert_int(tr_uv_tls_set_ca_file(CRT, NULL), ==, PC_RC_OK);

    for (size_t i = 0; i < ArrayCount(ports); ++i) {
        pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
        config.transport_name = transports[i];

        pc_client_init_result_t res = pc_client_init(NULL, &config);
        g_client = res.client;
        assert_int(res.rc, ==, PC_RC_OK);

        int handler = pc_client_add_ev_handler(g_client, event_cb, NULL, NULL);

        SLEEP_SECONDS(1);

        assert_null(pc_client_serializer(g_client));

        assert_int(pc_client_connect(g_client, LOCALHOST, ports[i], NULL), ==, PC_RC_OK);
        SLEEP_SECONDS(1);

        assert_string_equal(pc_client_serializer(g_client), "protobuf");

        assert_int(pc_string_request_with_timeout(g_client, "connector.route", "{dqwdwqd}", NULL, 
                                                  REQ_TIMEOUT, request_cb, request_error_cb), ==, PC_RC_OK);

        SLEEP_SECONDS(1);

        assert_int(g_num_error_cb_called, ==, 1);
        assert_int(g_num_success_cb_called, ==, 0);

        g_num_error_cb_called = 0;

        // assert_true(ev_cb_called);
        assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
        pc_client_rm_ev_handler(g_client, handler);
        assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);
    }

    return MUNIT_OK;
}

static MunitTest tests[] = {
    {"/error_decoding", test_error_decoding, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/response_decoding", test_response_decoding, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/request_encoding", test_request_encoding, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
};

const MunitSuite protobuf_suite = {
    "/protobuf", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};
