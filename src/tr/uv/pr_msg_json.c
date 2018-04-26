/**
 * Copyright (c) 2014,2015 NetEase, Inc. and other Pomelo contributors
 * MIT Licensed.
 */

#include <assert.h>
#include <string.h>

#include <pc_JSON.h>
#include <pomelo.h>
#include <pc_lib.h>
#include "pr_gzip.h"

#include "pr_msg.h"

pc_buf_t pc_body_json_encode(const pc_JSON* msg, int compress_data)
{
    pc_buf_t buf;
    char* res;

    buf.base = NULL;
    buf.len = -1;

    assert(msg);
    
    res = pc_JSON_PrintUnformatted(msg);
    if (!res) {
        pc_lib_log(PC_LOG_ERROR, "pc_body_json_encode - json encode error");
        return buf;
    }
    
    if (compress_data) {
        const size_t res_len = strlen(res);
        if (pr_compress((unsigned char**)&buf.base, (size_t*)&buf.len, (unsigned char*)res, res_len) != 0 || buf.len >= res_len) {
            buf.base = res;
            buf.len = (int)strlen(res);
        }
    }
    
    return buf;
}

pc_JSON* pc_body_json_decode(const char *data, size_t offset, size_t len, int gzipped)
{
    const char* end = NULL;
    const char* finalData = data;
    unsigned char * out = NULL;
    size_t outLen = len;

    if (gzipped){
        int decomprRet = pr_decompress(&out, &outLen, (unsigned char*)(data + offset), len);
        
        if (decomprRet != 0) {
            pc_lib_log(PC_LOG_ERROR, "pc_body_gzip_inflate - gzip inflate error");
            pc_lib_free(out);
            return NULL;
        }
        
        finalData = (const char*) out;
    }
    
    pc_JSON* res = pc_JSON_ParseWithOpts((char *)finalData, &end, 0);

    pc_lib_free(out);
    if (!res || end != (const char*) finalData + outLen) {
        pc_JSON_Delete(res);
        res = NULL;
        pc_lib_log(PC_LOG_ERROR, "pc_body_json_decode - json decode error");
    }

    return res;
}
