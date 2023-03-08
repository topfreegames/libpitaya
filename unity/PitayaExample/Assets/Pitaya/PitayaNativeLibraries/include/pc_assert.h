//
//  pc_assert.h
//  All
//
//  Created by Guthyerrz Maciel on 06/06/18.
//

#ifndef pc_assert_h
#define pc_assert_h

#include <stddef.h>

#include <pitaya.h>

#ifdef __cplusplus
extern "C" {
#endif

PC_EXPORT void __pc_assert(const char* e, const char* file, int line);

#define pc_assert(e)  \
((void) ((e) ? ((void)0) : __pc_assert (#e, __FILE__, __LINE__)))

PC_EXPORT void update_assert(void (*custom_assert)(const char* e, const char* file, int line));

#ifdef __cplusplus
}
#endif

#endif /* pc_assert_h */
