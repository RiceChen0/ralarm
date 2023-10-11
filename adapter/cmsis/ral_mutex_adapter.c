#include "ralarm_def.h"
#include "cmsis_os2.h"

ral_mutex_id ral_mutex_create(void)
{
    osMutexId_t mutex = NULL;
    mutex = osMutexNew(NULL);

    return (ral_mutex_id)mutex;
}

ral_status ral_mutex_lock(ral_mutex_id mutex)
{
    if (mutex == NULL) {
        return RALARM_EINVAL;
    }
    if(osMutexAcquire((osMutexId_t)mutex, osWaitForever) == osOK) {
        return RAL_OK;
    }
    return RAL_ERROR;
}

ral_status ral_mutex_unlock(ral_mutex_id mutex)
{
    if (mutex == NULL) {
        return RAL_INVAL;
    }
    if(osMutexRelease((osMutexId_t)mutex) == osOK) {
        return RAL_OK;
    }
    return RAL_ERROR;
}

void ral_mutex_delete(ral_mutex_id mutex)
{
    if (mutex == NULL) {
        return;
    }
    osMutexDelete(mutex);
}
