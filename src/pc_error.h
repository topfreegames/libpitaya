#ifndef PC_ERROR_H
#define PC_ERROR_H

#include <assert.h>
#include <pomelo.h>
#include "pc_lib.h"
#include "pc_JSON.h"

static pc_error_t
pc__error_uv(const char *uv_str)
{
    assert(uv_str);
    pc_error_t err = {0};
    err.code = (char*)pc_lib_strdup(uv_str);
    err.msg = NULL;
    err.metadata = NULL;
    return err;
}

static pc_error_t
pc__error_json(pc_JSON *json)
{
    assert(json->type == pc_JSON_Object);

    pc_JSON *code = pc_JSON_GetObjectItem(json, "Code");
    if (!code) {
        pc_lib_log(PC_LOG_ERROR, "pc__error_json - invalid json 'no Code'");
        pc_error_t err = {0};
        return err;
    }
    pc_JSON *msg = pc_JSON_GetObjectItem(json, "Msg");
    if (!msg) {
        pc_lib_log(PC_LOG_ERROR, "pc__error_json - invalid json 'no Msg'");
        pc_error_t err = {0};
        return err;
    }
    pc_JSON *metadata = pc_JSON_GetObjectItem(json, "Metadata");

    assert(code->type == pc_JSON_String);
    assert(msg->type == pc_JSON_String);

    pc_error_t err = {0};
    err.code = (char*)pc_lib_strdup(code->valuestring);
    err.msg = (char*)pc_lib_strdup(msg->valuestring);
    err.metadata = NULL;

    assert(err.code != NULL);

    if (metadata) {
        assert(metadata->type == pc_JSON_Object);
        err.metadata = pc_JSON_PrintUnformatted(msg);
    }

    return err;
}

static pc_error_t
pc__error_timeout()
{
    pc_error_t err = {0};
    err.code = (char*)pc_lib_strdup("PC_RC_TIMEOUT");
    err.msg = NULL;
    err.metadata = NULL;
    return err;
}

static pc_error_t
pc__error_reset()
{
    pc_error_t err = {0};
    err.code = (char*)pc_lib_strdup("PC_RC_RESET");
    err.msg = NULL;
    err.metadata = NULL;
    return err;
}

static pc_error_t
pc__error_dup(const pc_error_t *err)
{
    pc_error_t new_err = {0};
    new_err.code = err->code ? (char*)pc_lib_strdup(err->code) : NULL;
    new_err.msg = err->msg ? (char*)pc_lib_strdup(err->msg) : NULL;
    new_err.metadata = err->metadata ? (char*)pc_lib_strdup(err->metadata) : NULL;
    return new_err;
}

static inline void
pc__error_free(pc_error_t err)
{
    pc_lib_free(err.code);
    pc_lib_free(err.msg);
    pc_lib_free(err.metadata);
}

#endif // PC_ERROR_H
