//
//  pc_assert.c
//  All
//
//  Created by Guthyerrz Maciel on 06/06/18.
//

#include "pc_assert.h"
#include <assert.h>

void (*pc_custom_assert)(const char* e, const char* file, int line) = NULL;

void __pc_assert(const char* e, const char* file, int line) {
    
    if(pc_custom_assert != NULL){
        pc_custom_assert(e,file,line);
    }else{
#ifdef __assert
        __assert(e,file,line);
#endif
    }
}

void update_assert(void (*custom_assert)(const char* e, const char* file, int line)) {
    pc_custom_assert = custom_assert;
}
