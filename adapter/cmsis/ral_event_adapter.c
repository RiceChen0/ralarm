#include "ralarm_def.h"
#include "cmsis_os2.h"

ral_event_id ral_event_create(void)
{
    osEventFlagsId_t event = NULL;
    event = osEventFlagsNew(NULL);

    return (ral_event_id)event;
}

uint32_t ral_event_recv(ral_event_id event, uint32_t flags)
{
    uint32_t flag = 0;
    if (event == NULL) {
        return 0;
    }
    flag = osEventFlagsWait((osEventFlagsId_t)event, flags, osFlagsWaitAny, osWaitForever);
    return flag;
}

ral_status ral_event_send(ral_event_id event, uint32_t flags)
{
    if (event == NULL) {
        return RAL_ERROR;
    }
    if((osEventFlagsSet((osEventFlagsId_t)event, flags) & flags) == flags) {
        return RAL_OK;
    }
    return RAL_ERROR;
}

void ral_event_delete(ral_event_id event)
{
    if (event == NULL) {
        return;
    }
    osEventFlagsDelete(event);
}