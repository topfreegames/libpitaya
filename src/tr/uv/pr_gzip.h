//
//  pr_gzip.h
//  libuv
//
//  Created by Felipe Cavalcanti on 12/04/18.
//

#ifndef pr_gzip_h
#define pr_gzip_h

#include <stdio.h>

#endif /* pr_gzip_h */

int pr_compress(unsigned char** output,
             size_t* output_size,
             unsigned char* data,
             size_t size);

int pr_decompress(unsigned char** output,
               size_t* output_size,
               unsigned char* data,
               size_t size);

// Compressed data will return true.
// It is possible non-compressed data also returns true,
// but not in our case (our data is JSON).
int is_compressed(unsigned char* data, size_t size);
