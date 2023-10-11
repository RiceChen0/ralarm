/*
 * Change Logs:
 * Date           Author       Notes
 * 2023-05-12     RiceChen     the first version
 */
#include "ralarm.h"

#define RALARM_DELAY                     1
#define RALARM_REFRESH_EVENT             0x00000001

struct ralarm_container{
    ral_list_t list;
    ral_task_id task;
    ral_event_id event;
    ral_mutex_id mutex;
    struct ralarm_ops *ops;
};
typedef struct ralarm_container *ralarm_container_t;

static struct ralarm_container g_container;

uint32_t ralarm_mk_second(ralarm_time_t time)
{
    uint32_t second = 0;

    second = time->second;
    second += time->minute * 60;
    second += time->hour * 3600;

    return second;
}

ralarm_t ralarm_create(ralarm_setup_t setup, ralarm_response_cb cb, void *userData)
{
    ralarm_t alarm = NULL;
    
    if(setup == NULL) {
        RAL_LOGE("Create alarm failed, Setup param is NULL");
        return NULL;
    }
    alarm = RAL_MALLOC(sizeof(struct ralarm));
    if(alarm == NULL) {
        RAL_LOGE("Malloc alarm memory failed");
        return NULL;
    }

    ral_list_init(&alarm->list);
    memset((void *)alarm, 0, sizeof(struct ralarm));
    memcpy((void *)&alarm->setup, setup, sizeof(struct ralarm_setup));
    alarm->cb = cb;
    alarm->userData = userData;

    ral_mutex_lock(g_container.mutex);
    ral_list_insert_after(&g_container.list, &alarm->list);
    ral_mutex_unlock(g_container.mutex);
    return alarm;
}

ral_status ralarm_start(ralarm_t alarm)
{
    if(alarm == NULL) {
        return RAL_ERROR;
    }
    ral_mutex_lock(g_container.mutex);
    alarm->state |= RALARM_STATE_START;

    ral_mutex_unlock(g_container.mutex);
    return RAL_OK;
}

ral_status ralarm_stop(ralarm_t alarm)
{
    if(alarm == NULL) {
        return RAL_ERROR;
    }
    ral_mutex_lock(g_container.mutex);
    alarm->state &= ~RALARM_STATE_START;

    ral_mutex_unlock(g_container.mutex);
    return RAL_OK;
}

ral_status ralarm_modify(ralarm_t alarm, ralarm_setup_t setup)
{
    if(alarm == NULL) {
        return RAL_ERROR;
    }
    ral_mutex_lock(g_container.mutex);

    memcpy((void *)&alarm->setup, setup, sizeof(struct ralarm_setup));

    ral_mutex_unlock(g_container.mutex);
    return RAL_OK;
}

ral_status ralarm_delete(ralarm_t alarm)
{
    if(alarm == NULL) {
        return RAL_ERROR;
    }
    ral_mutex_lock(g_container.mutex);

    alarm->state &= ~RALARM_STATE_START;
    ral_list_remove(&alarm->list);
    RAL_FREE(alarm);

    ral_mutex_unlock(g_container.mutex);
    return RAL_OK;
}

void ralarm_dump(void)
{
    ral_list_t *node = NULL;
    ralarm_t alarm_node = NULL;

    RAL_LOGI("ralarm list: ");
    ral_list_for_each(node, &g_container.list) {
        alarm_node = ral_list_entry(node, struct ralarm, list);
        if(alarm_node != NULL) {
            RAL_LOGI("State: %d", alarm_node->state);
            RAL_LOGI("Flag: %d", alarm_node->setup.flag);
            RAL_LOGI("Time: %02d:%02d:%02d", 
                            alarm_node->setup.time.hour, 
                            alarm_node->setup.time.minute, 
                            alarm_node->setup.time.second);
        }
    }    
}

static void ralarm_wakeup(ralarm_t alarm, ralarm_time_t now)
{
    uint32_t alarm_sec = 0, now_sec = 0;
    bool wakeup = false;
    
    if(alarm == NULL) {
        return;
    }

    alarm_sec = ralarm_mk_second(&alarm->setup.time);
    now_sec = ralarm_mk_second(now);

    if(alarm->state & RALARM_STATE_START) {
        if(((now_sec - alarm_sec ) < RALARM_DELAY) && (now_sec >= alarm_sec)) {
            wakeup = true;
        }
    }
    if((wakeup == true) && (alarm->cb != NULL)) {
        alarm->cb(alarm);

        if(alarm->setup.flag == RALARM_ONESHOT) {
            alarm->state &= ~RALARM_STATE_START;
        }
    }
}

static void ralarm_update(void)
{
    ral_list_t *node = NULL;
    ralarm_t alarm_node = NULL;
    struct ralarm_time now_time = {0};

    if(g_container.ops->time_get == NULL) {
        return;
    }
    g_container.ops->time_get(&now_time);

    ral_mutex_lock(g_container.mutex);
    if(ral_list_is_empty(&g_container.list)) {
        ral_mutex_unlock(g_container.mutex);
        return;
    }
    ral_list_for_each(node, &g_container.list) {
        alarm_node = ral_list_entry(node, struct ralarm, list);
        if(alarm_node != NULL) {
            ralarm_wakeup(alarm_node, &now_time);
        }
    }
    ral_mutex_unlock(g_container.mutex);
}

static void ralarm_handler(void *arg)
{
    ralarm_container_t container = (ralarm_container_t)arg;
    uint32_t event = 0;
    for(;;) {
        event = ral_event_recv(container->event, RALARM_REFRESH_EVENT);
        if(event & RALARM_REFRESH_EVENT) {
            ralarm_update();
        }
    }
}

void ralarm_refresh(void)
{
    if(!ral_list_is_empty(&g_container.list)) {
        ral_event_send(g_container.event, RALARM_REFRESH_EVENT);
    }
}

ral_status ralarm_register_ops(struct ralarm_ops *ops)
{
    g_container.ops = ops;
    return RAL_OK;
}

ral_status ralarm_init(void)
{
    struct ral_task_attr attr = {
        .name = "ralarm",
        .stack_size = RALARM_TASK_STACK_SIZE,
        .priority = RALARM_TASK_PRIORITY,
    };

    ral_list_init(&g_container.list);

    g_container.event = ral_event_create();
    if(g_container.event == NULL) {
        RAL_LOGE("Create event failed");
        return RAL_ERROR;
    }

    g_container.mutex = ral_mutex_create();
    if(g_container.mutex == NULL) {
        RAL_LOGE("Create mutex failed");
        return RAL_ERROR;
    }

    g_container.task = ral_task_create(ralarm_handler, &g_container, &attr);
    if(g_container.task == NULL) {
        ral_event_delete(g_container.event);
        ral_mutex_delete(g_container.mutex);
        RAL_LOGE("Create task failed");
        return RAL_ERROR;
    }

    return RAL_OK;
}

void ralarm_deinit(void)
{

}
