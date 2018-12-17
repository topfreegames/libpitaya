#include "flag.h"
#include "test_common.h"

flag_t flag_make()
{
    flag_t flag;
    flag.val = false;
    flag.num_called = 0;
    pc_mutex_init(&flag.mutex);
    return flag;
}

void flag_cleanup(flag_t *flag)
{
    flag->val = false;
    flag->num_called = 0;
    pc_mutex_destroy(&flag->mutex);
}

int flag_get_num_called(flag_t *flag)
{
    pc_mutex_lock(&flag->mutex);
    int num_called = flag->num_called;
    pc_mutex_unlock(&flag->mutex);
    return num_called;
}

void flag_reset(flag_t *flag)
{
    pc_mutex_lock(&flag->mutex);
    flag->num_called = 0;
    flag->val = false;
    pc_mutex_unlock(&flag->mutex);
}

bool flag_get_val(flag_t *flag)
{
    pc_mutex_lock(&flag->mutex);
    bool val = flag->val;
    pc_mutex_unlock(&flag->mutex);
    return val;
}

void flag_set(flag_t *flag)
{
    pc_mutex_lock(&flag->mutex);
    flag->val = true;
    flag->num_called++;
    pc_mutex_unlock(&flag->mutex);
}

flag_wait_result_t flag_wait(flag_t *flag, int timeout)
{
    int waited = 0;

    for (;;) {
        if (waited >= timeout) {
            return FLAG_TIMEOUT;
        }

        pc_mutex_lock(&flag->mutex);
        if (flag->val) {
            flag->val = false;
            pc_mutex_unlock(&flag->mutex);
            return FLAG_SET;
        }
        pc_mutex_unlock(&flag->mutex);

        SLEEP_SECONDS(1);
        waited++;
    }
}
