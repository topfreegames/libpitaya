#include "pc_assert.h"
#include <stdlib.h>
#include <zlib.h>

#include "pc_lib.h"
#include "pr_gzip.h"

int pr_decompress(unsigned char** output,
               size_t* output_size,
               unsigned char* data,
               size_t size)
{
    z_stream inflate_s;
    
    inflate_s.zalloc = Z_NULL;
    inflate_s.zfree = Z_NULL;
    inflate_s.opaque = Z_NULL;
    inflate_s.avail_in = 0;
    inflate_s.next_in = Z_NULL;
    
    // The windowBits parameter is the base two logarithm of the window size (the size of the history buffer).
    // It should be in the range 8..15 for this version of the library.
    // Larger values of this parameter result in better compression at the expense of memory usage.
    // This range of values also changes the decoding type:
    //  -8 to -15 for raw deflate
    //  8 to 15 for zlib
    // (8 to 15) + 16 for gzip
    // (8 to 15) + 32 to automatically detect gzip/zlib header
    const int window_bits = 15 + 32; // auto with windowbits of 15
    
    if (inflateInit2(&inflate_s, window_bits) != Z_OK)
    {
        return 1;
    }

    inflate_s.next_in = (Bytef *)data;
    inflate_s.avail_in = (unsigned int)size;

    size_t size_uncompressed = 0;
    do
    {
        size_t resize_to = size_uncompressed + 2 * size;
        *output = (unsigned char*) pc_lib_realloc(*output, resize_to);
        inflate_s.avail_out = (unsigned int)(2 * size);
        inflate_s.next_out = (Bytef *)(*output + size_uncompressed);
        int ret = inflate(&inflate_s, Z_FINISH);
        if (ret != Z_STREAM_END && ret != Z_OK && ret != Z_BUF_ERROR)
        {
            char * error_msg = inflate_s.msg;
            printf("error decompressing: %s\n", error_msg);
            return inflateEnd(&inflate_s);
        }
        
        size_uncompressed += (2 * size - inflate_s.avail_out);
        *output_size = size_uncompressed;
    } while (inflate_s.avail_out == 0);
    return inflateEnd(&inflate_s);
}

#define CHUNK 64

int pr_compress(unsigned char** output,
                size_t* output_size,
                unsigned char* data,
                size_t size)
{
    int ret;
    z_stream strm;
    
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK)
        return ret;
    
    *output_size = CHUNK;
    
    strm.avail_in = (unsigned int)size;
    strm.next_in = data;
    strm.next_out = *output = (unsigned char*) pc_lib_realloc(*output, *output_size);
    strm.avail_out = (unsigned int)*output_size;
    
    int flush = strm.next_in + strm.avail_in == data + size ? Z_FINISH : Z_NO_FLUSH;
    
    do {
        
        do {
            ret = deflate(&strm, flush);
            while (ret == Z_BUF_ERROR) { // buffer too small, must realloc it
                long offset = strm.next_out - *output;
                strm.avail_out += CHUNK;
                *output_size += CHUNK;
                *output = (unsigned char*)realloc(*output, *output_size);
                strm.next_out = *output + offset;
                ret = deflate(&strm, flush);
            }
            if (ret != Z_STREAM_END && ret != Z_OK) {
                char * error_msg = strm.msg;
                printf("error compressing data: %s; ret: %d\n", error_msg, ret);
                return deflateEnd(&strm);
            }
        } while (strm.avail_out == 0);
        
        *output_size -= strm.avail_out;
    } while (flush != Z_FINISH);
    
    deflateEnd(&strm);
    *output = (unsigned char*) realloc(*output, *output_size);

    return Z_OK;
}

int is_compressed(unsigned char* data, size_t size)
{
    return size > 2 &&
    (
     // zlib
     (
      data[0] == 0x78 &&
      (data[1] == 0x9C ||
       data[1] == 0x01 ||
       data[1] == 0xDA ||
       data[1] == 0x5E)) ||
     // gzip
     (data[0] == 0x1F && data[1] == 0x8B));
}
