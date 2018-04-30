#ifndef PC_REQUEST_ERROR_H
#define PC_REQUEST_ERROR_H

#include <assert.h>
#include <pomelo.h>
#include "pc_lib.h"
#include "pc_JSON.h"

static pc_request_error_t
pc__request_error_uv(const char *uv_str)
{
    assert(uv_str);
    pc_request_error_t err = {
        .code = (char*)pc_lib_strdup(uv_str),
        .msg = NULL,
        .metadata = NULL,
    };
    return err;
}

static pc_request_error_t
pc__request_error_json(pc_JSON *json)
{
    assert(json->type == pc_JSON_Object);

    pc_JSON *code = pc_JSON_GetObjectItem(json, "Code");
    if (!code) {
        pc_lib_log(PC_LOG_ERROR, "pc__request_error_json - invalid json 'no Code'");
        return (pc_request_error_t){ .code = NULL, .msg = NULL };
    }
    pc_JSON *msg = pc_JSON_GetObjectItem(json, "Msg");
    if (!msg) {
        pc_lib_log(PC_LOG_ERROR, "pc__request_error_json - invalid json 'no Msg'");
        return (pc_request_error_t){ .code = NULL, .msg = NULL };
    }
    pc_JSON *metadata = pc_JSON_GetObjectItem(json, "Metadata");

    assert(code->type == pc_JSON_String);
    assert(msg->type == pc_JSON_String);

    pc_request_error_t err;
    err.code = pc_JSON_PrintUnformatted(code);
    err.msg = pc_JSON_PrintUnformatted(msg);
    err.metadata = NULL;

    if (metadata) {
        assert(metadata->type == pc_JSON_Object);
        err.metadata = pc_JSON_PrintUnformatted(msg);
    }

    return err;
}

static pc_request_error_t
pc__request_error_timeout()
{
    pc_request_error_t err = {
        .code = (char*)pc_lib_strdup("PC_RC_TIMEOUT"),
        .msg = NULL,
        .metadata = NULL,
    };
    return err;
}

static pc_request_error_t
pc__request_error_reset()
{
    pc_request_error_t err = {
        .code = (char*)pc_lib_strdup("PC_RC_RESET"),
        .msg = NULL,
        .metadata = NULL,
    };
    return err;
}

static pc_request_error_t
pc__request_error_dup(const pc_request_error_t *err)
{
    pc_request_error_t new_err = {
        .code = err->code ? (char*)pc_lib_strdup(err->code) : NULL,
        .msg = err->msg ? (char*)pc_lib_strdup(err->msg) : NULL,
        .metadata = err->metadata ? (char*)pc_lib_strdup(err->metadata) : NULL,
    };
    return new_err;
}

static inline void
pc__request_error_free(pc_request_error_t err)
{
    pc_lib_free(err.code);
    pc_lib_free(err.msg);
    pc_lib_free(err.metadata);
}

#endif // PC_REQUEST_ERROR_H
