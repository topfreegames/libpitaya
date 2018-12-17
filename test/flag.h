#ifndef PITAYA_FLAG_H
#define PITAYA_FLAG_H

#include "pc_mutex.h"
#include <stdbool.h>

typedef struct flag_t {
    pc_mutex_t mutex;
    int num_called;
    bool val;
} flag_t;

typedef enum flag_wait_result_t {
    FLAG_TIMEOUT,
    FLAG_SET,
} flag_wait_result_t;

flag_t flag_make();
void flag_cleanup(flag_t *flag);
void flag_set(flag_t *flag);
void flag_reset(flag_t *flag);
int flag_get_num_called(flag_t *flag);
bool flag_get_val(flag_t *flag);
flag_wait_result_t flag_wait(flag_t *flag, int timeout);

#endif // PITAYA_FLAG_H
