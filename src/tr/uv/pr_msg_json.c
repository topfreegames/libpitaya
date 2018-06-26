/**
 * Copyright (c) 2014,2015 NetEase, Inc. and other Pomelo contributors
 * MIT Licensed.
 */

#include <pc_assert.h>
#include <string.h>

#include <pc_JSON.h>
#include <pomelo.h>
#include <pc_lib.h>
#include "pr_gzip.h"

#include "pr_msg.h"

pc_buf_t pc_body_json_encode(pc_buf_t buf, bool *was_body_compressed)
{
    pc_buf_t out_buf;
    out_buf.base = NULL;
    out_buf.len = -1;

    pc_assert(buf.base && buf.len > 0);

    // TODO(leo): maybe move compress_data to outside of the function, because otherwise it is a noop.
    int compress_err = pr_compress((unsigned char**)&out_buf.base, (size_t*)&out_buf.len, (unsigned char*)buf.base, buf.len);

    if (compress_err) {
        pc_lib_log(PC_LOG_ERROR, "pc_body_json_encode - error compressing data");
        pc_buf_free(&out_buf); // free the buffers, since it will not be used.
        return buf;
    }

    // TODO, NOTE(leo): This check could be more specialized. For example, the compressed buffer is only used if it 
    // is at least 30% smaller than the original buffer.
    if (out_buf.len >= buf.len) {
        pc_lib_log(PC_LOG_ERROR, "pc_body_json_encode - compressed is larger (%d > %d)", out_buf.len, buf.len);
        pc_buf_free(&out_buf); // free the buffers, since it will not be used.
        if (was_body_compressed) *was_body_compressed = false;
        return buf;
    }
    
    // out_buf is smaller than buf
    if (was_body_compressed) *was_body_compressed = true;
    return out_buf;
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
