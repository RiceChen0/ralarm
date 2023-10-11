/*
 * Change Logs:
 * Date           Author       Notes
 * 2023-05-16     RiceChen     the first version
 */

#ifndef __RALARM_H__
#define __RALARM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ralarm_def.h"

#define RALARM_TASK_PRIORITY                      16
#define RALARM_TASK_STACK_SIZE                    2 * 1024

typedef enum {
    RALARM_ONESHOT,              /* only alarm once */
    RALARM_DAILY,                /* alarm everyday */
} ralarm_flag;

typedef enum {
    RALARM_STATE_INIT  = 1 << 0,
    RALARM_STATE_START = 1 << 1,
    RALARM_STATE_STOP  = 1 << 2,
    RALARM_STATE_WORK  = 1 << 3,
} ralarm_state;

typedef struct ralarm *ralarm_t;

typedef void (*ralarm_response_cb)(ralarm_t alarm);

struct ralarm_time {
    uint16_t hour;
    uint16_t minute;
    uint16_t second;
};
typedef struct ralarm_time *ralarm_time_t;

struct ralarm_setup {
    ralarm_flag flag;
    struct ralarm_time time;
};
typedef struct ralarm_setup *ralarm_setup_t;

struct ralarm_ops{
    ral_status (*time_get)(ralarm_time_t time);
};

struct ralarm {
    ralarm_state state;
    struct ralarm_setup setup;
    ralarm_response_cb cb;
    void *userData;
    ral_list_t list;
};

ral_status ralarm_init(void);
void ralarm_deinit(void);

ralarm_t ralarm_create(ralarm_setup_t setup, ralarm_response_cb cb, void *userData);
ral_status ralarm_start(ralarm_t alarm);
ral_status ralarm_stop(ralarm_t alarm);
ral_status ralarm_modify(ralarm_t alarm, ralarm_setup_t setup);
ral_status ralarm_delete(ralarm_t alarm);
void ralarm_dump(void);

/**
 * Ac register alarm operation
 * 
 * @param ops operation function point
 *
 * @return register result
*/
ral_status ralarm_register_ops(struct ralarm_ops *ops);

/**
 * This function needs to be placed in the RTC interrupt
 * or software timer to provide life for the alarm clock.
*/
void ralarm_refresh(void);

#ifdef __cplusplus
}
#endif

#endif
