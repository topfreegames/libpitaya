//
//  pc_assert.h
//  All
//
//  Created by Guthyerrz Maciel on 06/06/18.
//

#ifndef pc_assert_h
#define pc_assert_h

#include <assert.h>
#include <stddef.h>

#undef assert

static void pc_assert(const char* e, const char* file, int line);

#define assert(e)  \
((void) ((e) ? ((void)0) : pc_assert (#e, __FILE__, __LINE__)))

void (*pc_custom_assert)(const char* e, const char* file, int line) = NULL;

static void pc_assert(const char* e, const char* file, int line) {
    
    if(pc_custom_assert != NULL){
        pc_custom_assert(e,file,line);
    }else{
    #ifdef __assert
        __assert(e,file,line);
    #endif
    }
}

static void update_assert(void (*custom_assert)(const char* e, const char* file, int line)){
    pc_custom_assert = custom_assert;
}


#endif /* pc_assert_h */
