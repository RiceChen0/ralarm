/*
 * Change Logs:
 * Date           Author       Notes
 * 2023-05-12     RiceChen     the first version
 */
#include "ralarm.h"
#include "rtthread.h"
#include <time.h>

static rt_timer_t timer;
ralarm_t alarm_test = NULL;

static void alarm_handler(ralarm_t alarm)
{
    rt_kprintf("Time: %02d:%02d:%02d\r\n", alarm->setup.time.hour, 
                alarm->setup.time.minute, alarm->setup.time.second);
    ralarm_stop(alarm);
    ralarm_dump();
}

static pf_err_t alarm_time_get(ralarm_time_t timer)
{
    time_t current;
    struct tm *local;
    
    time(&current);
    local = localtime(&current);
    timer->hour = local->tm_hour;
    timer->minute = local->tm_min;
    timer->second = local->tm_sec;
    return RAL_OK;
}

static struct ralarm_ops ops = {
    .time_get = alarm_time_get,
};

static void time_handler(void *param)
{
    ralarm_refresh();
}

int ralarm_test(void)
{
    ralarm_init();
    ralarm_register_ops(&ops);

    struct ralarm_setup setup;
    setup.flag = RALARM_DAILY;
    setup.time.hour = 15;
    setup.time.minute = 0;
    setup.time.second = 0;

    alarm_test = ralarm_create(&setup, alarm_handler, NULL);
    ralarm_start(alarm_test);

    ralarm_dump();

    timer = rt_timer_create("timer", time_handler,
                             RT_NULL, 800,
                             RT_TIMER_FLAG_PERIODIC);
    if (timer != RT_NULL) 
        rt_timer_start(timer);

    return RT_EOK;
}