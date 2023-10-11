#include "ralarm_def.h"
#include "rtthread.h"

ral_mutex_id ral_mutex_create(void)
{
    rt_mutex_t mutex = NULL;

    mutex = rt_mutex_create("ral", RT_IPC_FLAG_FIFO);
    return (ral_mutex_id)mutex;
}

ral_status ral_mutex_lock(ral_mutex_id mutex)
{
    if (mutex == NULL) {
        return RAL_INVAL;
    }
    rt_mutex_take((rt_mutex_t)mutex, RT_WAITING_FOREVER);
    return RAL_ERROR;
}

ral_status ral_mutex_unlock(ral_mutex_id mutex)
{
    if (mutex == NULL) {
        return RAL_INVAL;
    }
    rt_mutex_release((rt_mutex_t)mutex);
    return RAL_ERROR;
}

void ral_mutex_delete(ral_mutex_id mutex)
{
    rt_mutex_delete((rt_mutex_t)mutex);
}
