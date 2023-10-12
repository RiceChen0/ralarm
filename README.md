# ralarm
1. 大部分单片机都提供了rtc alarm硬件闹钟，但是实际很少人使用，就举个简单的例子，rt-thread的BSP中也没有几个芯片适配了alarm硬件闹钟。但是我们要使用怎么办？？
2. 我受到RTOS的调度的启发，像M3/M4这种内核都是SysTick产生时钟节拍，以供系统处理所有和时间有关的事情，如线程延时，线程的时间片轮转，以及定时器超时等。
3. 有了第3点的经验，那么我们可以写一个软件闹钟功能就容易多了，只需要提供一个刷新节拍，定时查看哪一个闹钟需要唤醒，就可以解决闹钟的管理了

## raralarm接口说明：

#### 跨平台

- RTOS的种类很多，接口差异性打，所以raralarm为了解决这个问题，统一为上层提供一整套接口。

1. 线程接口。

``` C
typedef void *ral_task_id;

typedef struct ral_task_attr{
    char *name;             // name of the task
    uint32_t stack_size;    // size of stack
    uint8_t priority;       // initial task priority
} ral_task_attr;

typedef void(*ral_task_func)(void *arg);

ral_task_id ral_task_create(ral_task_func func, void *arg, ral_task_attr *attr);
void ral_task_delete(ral_task_id thread);
```

2. 互斥量接口。

``` C
typedef void *ral_mutex_id;

ral_mutex_id ral_mutex_create(void);
ral_status ral_mutex_lock(ral_mutex_id mutex);
ral_status ral_mutex_unlock(ral_mutex_id mutex);
void ral_mutex_delete(ral_mutex_id mutex);
```

3. 事件接口。

``` C
typedef void *ral_event_id;

ral_event_id ral_event_create(void);
uint32_t ral_event_recv(ral_event_id event, uint32_t flags);
ral_status ral_event_send(ral_event_id event, uint32_t flags);
void ral_event_delete(ral_event_id event);
```

- RAlarm目前已经提供了两个环境的适配，如cmsis，rtthread。

#### 接口使用简单

| 接口          | 说明     |
| ------------- | -------- |
| ralarm_init   | 初始化   |
| ralarm_deinit | 去初始化 |
| ralarm_create | 创建闹钟 |
| ralarm_start  | 启动闹钟 |
| ralarm_stop   | 停止闹钟 |
| ralarm_modify | 修改闹钟 |
| ralarm_delete | 删除闹钟 |


- 闹钟初始化接口：初始化闹钟的链表，闹钟任务，事件，互斥锁；去初始化接口：注销闹钟组件

``` C
/* 闹钟初始化 */
ralarm_err_t ralarm_init(void);

/* 闹钟去初始化 */
void ralarm_deinit(void);
```

- 闹钟创建：

1. 参数说明：

| **参数** | **描述**                                                     |
| -------- | ------------------------------------------------------------ |
| setup    | 闹钟的时间和标志，flag可为：RALARM_ONESHOT(只设置一次)和RALARM_DAILY(每天都设置) |
| cb       | 闹钟时间到了，唤醒的回调函数指针：typedef void (*ralarm_response_cb)(ralarm_t alarm) |
| userData | 设置闹钟时，自带的用户数据的指针                             |
| **返回** | ——                                                           |
| ralarm_t | 闹钟创建成功，放回闹钟句柄                                   |
| NULL     | 闹钟创建失败                                                 |

2. 函数说明：
   - ①申请闹钟控制块的空间。
   - ②设置闹钟参数到控制块中。
   - ③将闹钟加入到闹钟链表中。

``` C
struct ralarm_setup {
    ralarm_flag flag;
    struct ralarm_time time;
};
typedef struct ralarm_setup *ralarm_setup_t;

struct ralarm {
    ralarm_state state;
    struct ralarm_setup setup;
    ralarm_response_cb cb;
    void *userData;
    ral_list_t list;
};
typedef struct ralarm *ralarm_t;

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
```

- 闹钟启动：将闹钟的状态的start bit置为1。

``` C
ralarm_err_t ralarm_start(ralarm_t alarm)
{
    if(alarm == NULL) {
        return RAL_ERROR;
    }
    ral_mutex_lock(g_container.mutex);
    alarm->state |= RALARM_STATE_START;

    ral_mutex_unlock(g_container.mutex);
    return RAL_OK;
}
```

- 闹钟停止：将闹钟的状态的start bit置为0。

``` C
ralarm_err_t ralarm_stop(ralarm_t alarm)
{
    if(alarm == NULL) {
        return RAL_ERROR;
    }
    ral_mutex_lock(g_container.mutex);
    alarm->state &= ~RALARM_STATE_START;

    ral_mutex_unlock(g_container.mutex);
    return RAL_OK;
}
```

- 闹钟修改：修改闹钟的标志和闹钟的时间

1. 参数说明：

| **参数**     | **描述**                   |
| ------------ | -------------------------- |
| alarm        | 闹钟的句柄                 |
| setup        | 要修改闹钟的时间和标志参数 |
| **返回**     | ——                         |
| RALARM_EOK   | 修改成功                   |
| RALARM_ERROR | 修改失败                   |

``` C
ralarm_err_t ralarm_modify(ralarm_t alarm, ralarm_setup_t setup)
{
    if(alarm == NULL) {
        return RAL_ERROR;
    }
    ral_mutex_lock(g_container.mutex);

    memcpy((void *)&alarm->setup, setup, sizeof(struct ralarm_setup));

    ral_mutex_unlock(g_container.mutex);
    return RAL_OK;
}
```

- 删除闹钟：

1. 函数说明：
   - ①将闹钟的状态的start bit置为0。
   - ②将闹钟从闹钟链表中移除。
   - ③释放闹钟的内存。

``` C
ralarm_err_t ralarm_delete(ralarm_t alarm)
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
```

#### 适配简单

- 根据系统能力，提供获取时间方法，创建ralarm的ops并注册获取时间接口。

``` C
struct ralarm_ops{
    ral_status (*time_get)(ralarm_time_t time);
};

ral_status ralarm_register_ops(struct ralarm_ops *ops);
```

- 提供刷新节拍，然后调用刷新接口。

``` C
void ralarm_refresh(void);
```

## RAlarm运行逻辑：

1. 闹钟的refresh接口需要用户提供一个刷新节拍，以提供闹钟的生命。
2. refresh皆苦根据闹钟链表是否存在已设置的闹钟，选择发送事件给更新任务，更新检测闹钟的状态。
3. 如下图：当检测闹钟链表无设置的闹钟，则不会发送事件给更新任务

![](https://ricechen0.gitee.io/picture/ralarm/3.png)

4. 如下图：
   - 当用户创建了闹钟，则会将闹钟挂在闹钟量表中。
   - 刷新节拍调用refresh之后，发送事件给更新任务，然后调用wakeup检测闹钟的状态。
   - 如果某个闹钟时间到，则会调用对应闹钟的回调函数。

![](https://ricechen0.gitee.io/picture/ralarm/4.png)

# RAlarm的使用

- 在RT-Thread下使用ralarm组件：
  - ① 闹钟的处理函数，当闹钟时间到了，则会调用这个函数。
  - ② 提供给ralarm组件时间接口。
  - ③ 创建ops，提供时间接口。
  - ④ 软件定时器的处理函数，调用ralarm的刷新函数，提供刷新节拍。
  - ⑤ ralarm组件初始化，注册ops。
  - ⑥ 创建闹钟。
  - ⑦ 创建一个软件定时器，为ralarm组件提供刷新节拍。

``` C
static rt_timer_t timer;
ralarm_t alarm_test = NULL;

static void alarm_handler(ralarm_t alarm)
{
    rt_kprintf("Time: %02d:%02d:%02d\r\n", alarm->setup.time.hour, 
                alarm->setup.time.minute, alarm->setup.time.second);
    ralarm_stop(alarm);
    ralarm_dump();
}

static ral_status alarm_time_get(ralarm_time_t timer)
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
MSH_CMD_EXPORT(ralarm_test, ralarm test);
```

- 验证结果：

![](https://ricechen0.gitee.io/picture/ralarm/1.png)