#include <stdio.h>
#include <pitaya.h>
#include <stdint.h>
#include <stdbool.h>
#include <pb_decode.h>
#include <pb_encode.h>
#include <math.h>

#include "error.pb.h"
#include "session-data.pb.h"
#include "response.pb.h"
#include "big-message.pb.h"
#include "test_common.h"
#include "flag.h"

static pc_client_t *g_client = NULL;

static void
event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    // This callback should not be called, therefore called should always be false.
    Unused(arg1); Unused(arg2); Unused(client); Unused(ev_type);

    const int EVENTS[] = {
        PC_EV_CONNECTED, PC_EV_DISCONNECT
    };

    flag_t *flag = (flag_t*)ex_data;
    int num_called = flag_get_num_called(flag);
    assert_int(num_called, <, ArrayCount(EVENTS));
    assert_int(ev_type, ==, EVENTS[num_called]);
    flag_set(flag);
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

typedef struct {
    size_t len;
    size_t cap;
    char **data;
} string_array_t;

static bool
read_repeated_string(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    string_array_t *array = (string_array_t*)(*arg);

    uint8_t *buf = calloc(stream->bytes_left+1, 1);
    assert_not_null(buf);

    size_t len = stream->bytes_left;
    assert_true(pb_read(stream, buf, len));

    if (array->cap == array->len) {
        size_t new_cap = array->cap * 1.5;
        char **new_data = (char**)realloc(array->data, sizeof(char*) * new_cap);
        assert_not_null(new_data);

        array->cap = new_cap;
        array->data = new_data;
    }

    array->data[array->len++] = (char*)buf;

    return true;
}

typedef struct {
    size_t cap, len;
    protos_BigMessageResponse_NpcsEntry *data;
} npc_entry_array_t;

static bool
read_npc_entries(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    npc_entry_array_t *array = (npc_entry_array_t*)(*arg);

    protos_BigMessageResponse_NpcsEntry entry = protos_BigMessageResponse_NpcsEntry_init_zero;
    entry.key.funcs.decode = read_string;
    entry.value.name.funcs.decode = read_string;
    entry.value.publicId.funcs.decode = read_string;

    assert_true(pb_decode(stream, protos_BigMessageResponse_NpcsEntry_fields, &entry));

    if (array->cap == array->len) {
        size_t new_cap = array->cap * 1.5;
        protos_BigMessageResponse_NpcsEntry *new_data = realloc(array->data, sizeof(protos_BigMessageResponse_NpcsEntry) * new_cap);
        assert_not_null(new_data);

        array->cap = new_cap;
        array->data = new_data;
    }

    array->data[array->len++] = entry;

    return true;
}

static bool
write_string(pb_ostream_t *stream, const pb_field_t *field, void * const *arg)
{
    return pb_encode_tag_for_field(stream, field) &&
           pb_encode_string(stream, *arg, strlen(*arg));
}

static bool
contains_in_keys(char **keys, size_t len, char *key)
{
    for (int i = 0; i < len; ++i) {
        if (strncmp(key, keys[i], strlen(key)) == 0) {
            return true;
        }
    }
    return false;
}

typedef struct {
    char *name;
    char *publicId;
    double health;
} npc_t;

static bool
contains_in_vals(npc_t *npcs_vals, size_t len, char *name, char *public_id, double health)
{
    for (int i = 0; i < len; ++i) {
        if ((strcmp(name, npcs_vals[i].name) == 0)  &&
            (strcmp(public_id, npcs_vals[i].publicId) == 0) &&
            fabs(health - npcs_vals[i].health) < 0.01) {
            return true;
        }
    }
    return false;
}

static void
big_message_request_cb(const pc_request_t* req, const pc_buf_t *resp)
{
    flag_t *flag = (flag_t*)pc_request_ex_data(req);

    assert_not_null(resp);
    assert_not_null(resp);
    assert_int(resp->len, >, 0);

    protos_BigMessage big_message = protos_BigMessage_init_zero;
    big_message.code.funcs.decode = read_string;
    big_message.response.player.publicId.funcs.decode = read_string;
    big_message.response.player.accessToken.funcs.decode = read_string;
    big_message.response.player.name.funcs.decode = read_string;
    big_message.response.player.items.funcs.decode = read_repeated_string;
    {
        string_array_t *array = (string_array_t*)calloc(1, sizeof(string_array_t));
        array->cap = 2;
        array->len = 0;
        array->data = (char**)calloc(array->cap, sizeof(char*));
        assert_not_null(array);
        assert_not_null(array->data);
        big_message.response.player.items.arg = array;
    }
    big_message.response.npcs.funcs.decode = read_npc_entries;
    {
        npc_entry_array_t *array = (npc_entry_array_t*)calloc(1, sizeof(npc_entry_array_t));
        array->cap = 2;
        array->len = 0;
        array->data = (protos_BigMessageResponse_NpcsEntry*)calloc(array->cap, sizeof(protos_BigMessageResponse_NpcsEntry));
        assert_not_null(array);
        assert_not_null(array->data);
        big_message.response.npcs.arg = array;
    }
    big_message.response.chests.funcs.decode = read_repeated_string;
    {
        string_array_t *array = (string_array_t*)calloc(1, sizeof(string_array_t));
        array->cap = 2;
        array->len = 0;
        array->data = (char**)calloc(array->cap, sizeof(char*));
        assert_not_null(array);
        assert_not_null(array->data);
        big_message.response.chests.arg = array;
    }

    // big_message.response.chests.funcs.decode = read_repeated_string;

    pb_istream_t stream = pb_istream_from_buffer(resp->base, resp->len);

    assert_true(pb_decode(&stream, protos_BigMessage_fields, &big_message));

    assert_string_equal((char*)big_message.code.arg, "ABC-200");
    free(big_message.code.arg);

    assert_string_equal((char*)big_message.response.player.publicId.arg,
                        "qwid1ioj230as09d1908309asd12oij3lkjoaisjd21");
    free((char*)big_message.response.player.publicId.arg);

    assert_string_equal((char*)big_message.response.player.accessToken.arg,
                        "oqijwdioj29012i3jlkad012d9j");
    free((char*)big_message.response.player.accessToken.arg);

    assert_string_equal((char*)big_message.response.player.name.arg,
                        "Frederico Castanha");
    free((char*)big_message.response.player.name.arg);

    assert_double_equal(big_message.response.player.health, 87.5, 10);

    char *expected_items[] = {
        "BigItem",
        "SmallItem",
        "InterestingWeirdItem",
        "Another",
        "One",
        "Stuff",
        "MoreStuff",
        "MoarStuff",
        "BigItem",
        "SmallItem",
        "InterestingWeirdItem",
        "Another",
    };

    string_array_t *items = (string_array_t*)big_message.response.player.items.arg;
    assert_not_null(items);
    assert_int(items->len, ==, ArrayCount(expected_items));
    for (int i = 0; i < items->len; ++i) {
        assert_string_equal(items->data[i], expected_items[i]);
        free(items->data[i]);
    }
    free(items->data);
    free(items);

    char *npcs_keys[] = {
        "banana",
        "oqijwdoijdwoij[q12919120120991912191911111",
        "npc201",
        "np20xaiwodj1",
        "qiwjdoqijwd",
    };

    npc_t npcs_vals[] = {
        {"bzbzbbabababqbbwbebqwbebqwbdiasbciuwiqub", "o12e12d-120id-q0-012id0-k-0cc12 1-20d10-2kd-012ie01-i-0i12 -0id-0ic- 01k-0ie-0i12-0i-d0iasidaopjdwqijd-2", 21.43},
        {"UQUQUUQ", "owiqdjoiqjwdoijwoqijdi112123333333333333333333333333312oij2190d1290", 312.00},
        {"this is as annoying enemy", "wow, public id: 20123iq9w0e9012309812039ad", 21.43},
        {"ENEENENNEEMMMMMMMMMMMMMMY", "randonqwdmqwoqowjdoqwjdoqjwdoqqqwoj", 72.01},
        {"QUQUQUAOAOOQOQOQOQOOQOQOQOQOQMCMCMCM", "owijdoiqwjdoqwjdo-021192391231-102dii2-10-0i12d", 99.99},
    };

    npc_entry_array_t *npcs = (npc_entry_array_t*)big_message.response.npcs.arg;
    assert_not_null(npcs);
    assert_int(npcs->len, ==, ArrayCount(npcs_keys));
    assert_int(npcs->len, ==, ArrayCount(npcs_vals));

    for (int i = 0; i < npcs->len; ++i) {
        assert_true(contains_in_keys(npcs_keys, ArrayCount(npcs_keys), (char*)npcs->data[i].key.arg));
        assert_true(contains_in_vals(npcs_vals, ArrayCount(npcs_vals),
                                     (char*)npcs->data[i].value.name.arg,
                                     (char*)npcs->data[i].value.publicId.arg,
                                     npcs->data[i].value.health));

        free(npcs->data[i].key.arg);
        free(npcs->data[i].value.name.arg);
        free(npcs->data[i].value.publicId.arg);
    }
    free(npcs->data);
    free(npcs);

    char *expected_chests[] = {
        "chchche",
        "asbcasc",
        "oijdoijqwodijqwoeijqwioj",
        "ciqwojdoqiwjdiojchchche",
        "1023doi12jdoijdoqwijoi21j",
        "1jd210",
        "c",
        "-12id-10id0-id0-iew-0ic-0i",
        "$#RIOj1j#(joiJDIOJQIWOJD))",
        "dwq0jq-cqw0921jejclsnvwejflk   qwf we  we gw eg ",
        "ijqwoidjoqijc",
        "nvweoifjoweijfoiwje910e23fj",
    };

    string_array_t *chests = (string_array_t*)big_message.response.chests.arg;
    assert_not_null(chests);
    assert_int(chests->len, ==, ArrayCount(expected_chests));
    for (int i = 0; i < chests->len; ++i) {
        assert_string_equal(chests->data[i], expected_chests[i]);
        free(chests->data[i]);
    }
    free(chests->data);
    free(chests);

    flag_set(flag);
}

static void
encoded_request_cb(const pc_request_t* req, const pc_buf_t *resp)
{
    flag_t *flag = (flag_t*)pc_request_ex_data(req);
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
    flag_set(flag);
}

static void
request_cb(const pc_request_t* req, const pc_buf_t *resp)
{
    flag_t *flag = (flag_t*)pc_request_ex_data(req);
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
    flag_set(flag);
}

static void
request_error_cb(const pc_request_t* req, const pc_error_t *error)
{
    assert_not_null(error->payload.base);
    assert_int(error->code, ==, PC_RC_SERVER_ERROR);

    flag_t *flag = pc_request_ex_data(req);

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

    flag_set(flag);
}

static MunitResult
test_big_message(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);

    const int ports[] = {g_test_protobuf_server.tcp_port, g_test_protobuf_server.tls_port};
    const int transports[] = {PC_TR_NAME_UV_TCP, PC_TR_NAME_UV_TLS};

    assert_int(tr_uv_tls_set_ca_file(CRT, NULL), ==, PC_RC_OK);

    for (size_t i = 0; i < ArrayCount(ports); ++i) {
        flag_t flag_evs = flag_make();
        flag_t flag_req = flag_make();

        pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
        config.transport_name = transports[i];

        pc_client_init_result_t res = pc_client_init(NULL, &config);
        g_client = res.client;
        assert_int(res.rc, ==, PC_RC_OK);

        pc_client_add_ev_handler(g_client, event_cb, &flag_evs, NULL);

        assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, ports[i], NULL), ==, PC_RC_OK);
        assert_int(flag_wait(&flag_evs, 60), ==, FLAG_SET);

        const char *serializer = pc_client_serializer(g_client);
        assert_not_null(serializer);
        assert_string_equal(serializer, "protobuf");
        pc_client_free_serializer(serializer);

        assert_int(pc_string_request_with_timeout(g_client, "connector.getbigmessage", "", &flag_req,
                                                  REQ_TIMEOUT, big_message_request_cb, NULL), ==, PC_RC_OK);

        assert_int(flag_wait(&flag_req, 60), ==, FLAG_SET);

        assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
        assert_int(flag_wait(&flag_evs, 60), ==, FLAG_SET);

        assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);
        flag_cleanup(&flag_req);
        flag_cleanup(&flag_evs);
    }

    return MUNIT_OK;
}

static MunitResult
test_request_encoding(const MunitParameter params[], void *data)
{
    Unused(params); Unused(data);

    const int ports[] = {g_test_protobuf_server.tcp_port, g_test_protobuf_server.tls_port};
    const int transports[] = {PC_TR_NAME_UV_TCP, PC_TR_NAME_UV_TLS};

    assert_int(tr_uv_tls_set_ca_file(CRT, NULL), ==, PC_RC_OK);

    for (size_t i = 0; i < ArrayCount(ports); ++i) {
        flag_t flag_evs = flag_make();
        flag_t flag_req = flag_make();

        pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
        config.transport_name = transports[i];

        pc_client_init_result_t res = pc_client_init(NULL, &config);
        g_client = res.client;
        assert_int(res.rc, ==, PC_RC_OK);

        int handler = pc_client_add_ev_handler(g_client, event_cb, &flag_evs, NULL);
        assert_null(pc_client_serializer(g_client));

        assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, ports[i], NULL), ==, PC_RC_OK);
        assert_int(flag_wait(&flag_evs, 60), ==, FLAG_SET);

        const char *serializer = pc_client_serializer(g_client);
        assert_not_null(serializer);
        assert_string_equal(serializer, "protobuf");
        pc_client_free_serializer(serializer);

        protos_SessionData session_data = protos_SessionData_init_zero;
        session_data.data.funcs.encode = write_string;
        session_data.data.arg = "Joelho";

        uint8_t buf[256];
        pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(buf));

        assert_true(pb_encode(&stream, protos_SessionData_fields, &session_data));
        assert_int(stream.bytes_written, <=, sizeof(buf));
        assert_int(pc_binary_request_with_timeout(g_client, "connector.setsessiondata", buf, stream.bytes_written, &flag_req,
                                                  REQ_TIMEOUT, encoded_request_cb, request_error_cb), ==, PC_RC_OK);
        assert_int(flag_wait(&flag_req, 60), ==, FLAG_SET);

        // assert_true(ev_cb_called);
        assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
        assert_int(flag_wait(&flag_evs, 60), ==, FLAG_SET);
        pc_client_rm_ev_handler(g_client, handler);
        assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);
        flag_cleanup(&flag_req);
        flag_cleanup(&flag_evs);
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
        flag_t flag_evs = flag_make();
        flag_t flag_req = flag_make();

        pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
        config.transport_name = transports[i];

        pc_client_init_result_t res = pc_client_init(NULL, &config);
        g_client = res.client;
        assert_int(res.rc, ==, PC_RC_OK);

        int handler = pc_client_add_ev_handler(g_client, event_cb, &flag_evs, NULL);
        assert_null(pc_client_serializer(g_client));

        assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, ports[i], NULL), ==, PC_RC_OK);
        assert_int(flag_wait(&flag_evs, 60), ==, FLAG_SET);

        const char *serializer = pc_client_serializer(g_client);
        assert_not_null(serializer);
        assert_string_equal(serializer, "protobuf");
        pc_client_free_serializer(serializer);

        assert_int(pc_string_request_with_timeout(g_client, "connector.getsessiondata", "{}", &flag_req,
                                                  REQ_TIMEOUT, request_cb, NULL), ==, PC_RC_OK);

        assert_int(flag_wait(&flag_req, 60), ==, FLAG_SET);

        // assert_true(ev_cb_called);
        assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
        pc_client_rm_ev_handler(g_client, handler);
        assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);

        flag_cleanup(&flag_evs);
        flag_cleanup(&flag_req);
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
        flag_t flag_evs = flag_make();
        flag_t flag_req = flag_make();

        pc_client_config_t config = PC_CLIENT_CONFIG_TEST;
        config.transport_name = transports[i];

        pc_client_init_result_t res = pc_client_init(NULL, &config);
        g_client = res.client;
        assert_int(res.rc, ==, PC_RC_OK);

        int handler = pc_client_add_ev_handler(g_client, event_cb, &flag_evs, NULL);
        assert_null(pc_client_serializer(g_client));

        assert_int(pc_client_connect(g_client, PITAYA_SERVER_URL, ports[i], NULL), ==, PC_RC_OK);
        assert_int(flag_wait(&flag_evs, 60), ==, FLAG_SET);

        const char *serializer = pc_client_serializer(g_client);
        assert_not_null(serializer);
        assert_string_equal(serializer, "protobuf");
        pc_client_free_serializer(serializer);

        assert_int(pc_string_request_with_timeout(g_client, "connector.route", "{dqwdwqd}", &flag_req,
                                                  REQ_TIMEOUT, request_cb, request_error_cb), ==, PC_RC_OK);

        assert_int(flag_wait(&flag_req, 60), ==, FLAG_SET);

        assert_int(pc_client_disconnect(g_client), ==, PC_RC_OK);
        assert_int(flag_wait(&flag_evs, 60), ==, FLAG_SET);

        pc_client_rm_ev_handler(g_client, handler);
        assert_int(pc_client_cleanup(g_client), ==, PC_RC_OK);
        flag_cleanup(&flag_req);
        flag_cleanup(&flag_evs);
    }

    return MUNIT_OK;
}

static MunitTest tests[] = {
    {"/error_decoding", test_error_decoding, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/response_decoding", test_response_decoding, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/request_encoding", test_request_encoding, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/big_message", test_big_message, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
};

const MunitSuite protobuf_suite = {
    "/protobuf", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};
