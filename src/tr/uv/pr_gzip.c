//
//  pr_gzip.c
//  libuv
//
//  Created by Felipe Cavalcanti on 12/04/18.
//

#include "pr_gzip.h"
#include <zlib.h>
#include "pc_lib.h"
#include <stdlib.h>

int decompress(unsigned char** output,
               size_t* output_size,
               unsigned char* data,
               size_t size)
{
    z_stream inflate_s;
    
    //*output = (unsigned char *)pc_lib_malloc(CHUNK);
    
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
    
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
    if (inflateInit2(&inflate_s, window_bits) != Z_OK)
    {
        return 1;
    }
#pragma GCC diagnostic pop
    inflate_s.next_in = (Bytef *)data;
    
    inflate_s.avail_in = size;
    size_t size_uncompressed = 0;
    do
    {
        size_t resize_to = size_uncompressed + 2 * size;
        *output = (unsigned char*) pc_lib_realloc(*output, resize_to);
        inflate_s.avail_out = 2 * size;
        inflate_s.next_out = (Bytef *)(*output + size_uncompressed);
        int ret = inflate(&inflate_s, Z_FINISH);
        if (ret != Z_STREAM_END && ret != Z_OK && ret != Z_BUF_ERROR)
        {
            char * error_msg = inflate_s.msg;
            printf("%s\n", error_msg);
            return inflateEnd(&inflate_s);
        }
        
        size_uncompressed += (2 * size - inflate_s.avail_out);
        *output_size = size_uncompressed;
    } while (inflate_s.avail_out == 0);
    return inflateEnd(&inflate_s);
}
