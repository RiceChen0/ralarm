#include "ralarm_def.h"
#include "rtthread.h"

ral_task_id ral_task_create(ral_task_func func, void *arg, ral_task_attr *attr)
{
    rt_thread_t thread = NULL;

    thread = rt_thread_create(attr->name, func, arg, attr->stack_size, attr->priority, 20);
    rt_thread_startup(thread);

    return (ral_task_id)thread;
}

void ral_task_delete(ral_task_id thread)
{
    rt_thread_delete((rt_thread_t)thread);
}
