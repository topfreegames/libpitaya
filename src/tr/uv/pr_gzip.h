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

int decompress(unsigned char** output,
               size_t* output_size,
               unsigned char* data,
               size_t size);

int is_compressed(unsigned char* data, size_t size);
