#ifndef PC_ERROR_H
#define PC_ERROR_H

#include <assert.h>
#include <pomelo.h>
#include "pc_lib.h"
#include "pc_JSON.h"

static pc_error_t
pc__error_uv(int uv_code)
{
    pc_error_t err = {0};
    err.code = PC_RC_UV_ERROR;
    err.uv_code = uv_code;
    return err;
}

static pc_error_t
pc__error_server(const pc_buf_t *payload)
{
    pc_assert(payload->base && payload->len > 0);
    pc_error_t err = {0};
    err.code = PC_RC_SERVER_ERROR;
    err.payload = pc_buf_copy(payload);
    return err;
}

static pc_error_t
pc__error_timeout()
{
    pc_error_t err = {0};
    err.code = PC_RC_TIMEOUT;
    err.payload.len = -1;
    return err;
}

static pc_error_t
pc__error_reset()
{
    pc_error_t err = {0};
    err.code = PC_RC_TIMEOUT;
    err.payload.len = -1;
    return err;
}

static pc_error_t
pc__error_dup(const pc_error_t *err)
{
    pc_error_t new_err = {0};
    new_err.code = err->code;
    if (err->payload.base) {
        new_err.payload = pc_buf_copy(&err->payload);
    }
    return new_err;
}

static inline void
pc__error_free(pc_error_t *err)
{
    if (err->payload.base) {
        pc_lib_free(err->payload.base);
        memset(err, 0, sizeof(pc_error_t));
    }
}

#endif // PC_ERROR_H
