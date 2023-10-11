#include "ralarm_def.h"
#include "rtthread.h"

ral_event_id ral_event_create(void)
{
    rt_event_t event = NULL;

    event = rt_event_create("ral", RT_IPC_FLAG_FIFO);
    return (ral_event_id)event;
}

uint32_t ral_event_recv(ral_event_id event, uint32_t flags)
{
    uint32_t flag = 0;
    if (event == NULL) {
        return 0;
    }
    rt_event_recv((rt_event_t)event, flags, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, &flag);
    return flag;
}

ral_status ral_event_send(ral_event_id event, uint32_t flags)
{
    if (event == NULL) {
        return RAL_ERROR;
    }
    rt_event_send((rt_event_t)event, flags);

    return RAL_OK;
}

void ral_event_delete(ral_event_id event)
{
    rt_event_delete((rt_event_t)event);
}
