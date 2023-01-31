/**
 * Copyright (c) 2014,2015 NetEase, Inc. and other Pomelo contributors
 * MIT Licensed.
 */

#include <pitaya.h>
#include <pc_assert.h>

#include <pc_lib.h>
#include <stdbool.h>

#include "pr_msg.h"
#include "tr_uv_tls.h"
#include "tr_uv_tls_i.h"

static tr_uv_tls_transport_plugin_t instance =
{
    {
        {
            tr_uv_tls_create,
            tr_uv_tls_release,
            tr_uv_tls_plugin_on_register,
            tr_uv_tls_plugin_on_deregister,
            PC_TR_NAME_UV_TLS
        }, /* pc_transport_plugin_t */
        pr_default_msg_encoder, /* encoder */
        pr_default_msg_decoder  /* decoder */
    },
    NULL, /* ssl ctx */
    1 /* enables the verification of the  */
};

pc_transport_plugin_t* pc_tr_uv_tls_trans_plugin()
{
    return (pc_transport_plugin_t* )&instance;
}

int tr_uv_tls_set_ca_file(const char* ca_file, const char* ca_path)
{
    if (instance.ctx) {
        int ret = SSL_CTX_load_verify_locations(instance.ctx, ca_file, ca_path);
        if (!ret) {
            pc_lib_log(PC_LOG_WARN, "tr_uv_tls_set_ca_file - load verify locations error, cafile: %s, capath: %s", ca_file, ca_path);
            return PC_RC_ERROR;
        }
        return PC_RC_OK;
    } else {
        return PC_RC_ERROR;
    }
}

void tr_uv_tls_set_sni_host(const char* host)
{
    instance.host = host;
}
