/*
 * Change Logs:
 * Date           Author       Notes
 * 2023-05-12     RiceChen     the first version
 */

#ifndef __PLATFORM_DEF_H__
#define __PLATFORM_DEF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef enum {
    RAL_OK = 0,                         /**< There is no error */
    RAL_ERROR,                          /**< A generic error happens */
    RAL_FULL,                           /**< The resource is full */
    RAL_EMPTY,                          /**< The resource is empty */
    RAL_NOMEM,                          /**< No memory */
    RAL_INVAL,                          /**< Invalid argument */
} ral_status;

#define RAL_INLINE                      static __inline

/**
 * memory API
*/
#ifndef RAL_MALLOC
    #define RAL_MALLOC                  malloc
#endif

#ifndef RAL_FREE
    #define RAL_FREE                    free
#endif

#ifndef RAL_PRINT
    #define RAL_PRINT                   printf
#endif

#ifndef RAL_PRINT_TAG
    #define RAL_PRINT_TAG               "RAL"
#endif

#define RAL_LOGE(...)                   RAL_PRINT("\033[31;22m[E/%s](%s:%d) ", RAL_PRINT_TAG, __FUNCTION__, __LINE__);     \
                                        RAL_PRINT(__VA_ARGS__);                                                           \
                                        RAL_PRINT("\033[0m\n")
#define RAL_LOGI(...)                   RAL_PRINT("\033[32;22m[I/%s](%s:%d) ", RAL_PRINT_TAG, __FUNCTION__, __LINE__);     \
                                        RAL_PRINT(__VA_ARGS__);                                                           \
                                        RAL_PRINT("\033[0m\n")
#define RAL_LOGD(...)                   RAL_PRINT("[D/%s](%s:%d) ", RAL_PRINT_TAG, __FUNCTION__, __LINE__);                \
                                        RAL_PRINT(__VA_ARGS__);                                                           \
                                        RAL_PRINT("\n")

typedef void *ral_task_id;
typedef void *ral_mutex_id;
typedef void *ral_event_id;

/**
 * Task API
*/
typedef struct ral_task_attr{
    char *name;             // name of the task
    uint32_t stack_size;    // size of stack
    uint8_t priority;       // initial task priority
} ral_task_attr;

typedef void(*ral_task_func)(void *arg);

ral_task_id ral_task_create(ral_task_func func, void *arg, ral_task_attr *attr);
void ral_task_delete(ral_task_id thread);

/**
 * Mutex API
*/
ral_mutex_id ral_mutex_create(void);
ral_status ral_mutex_lock(ral_mutex_id mutex);
ral_status ral_mutex_unlock(ral_mutex_id mutex);
void ral_mutex_delete(ral_mutex_id mutex);

/**
 * Event API
*/
ral_event_id ral_event_create(void);
uint32_t ral_event_recv(ral_event_id event, uint32_t flags);
ral_status ral_event_send(ral_event_id event, uint32_t flags);
void ral_event_delete(ral_event_id event);

struct ral_list_node {
    struct ral_list_node *next;
    struct ral_list_node *prev;
};
typedef struct ral_list_node ral_list_t;

RAL_INLINE void ral_list_init(ral_list_t *l)
{
    l->next = l->prev = l;
}

RAL_INLINE void ral_list_insert_after(ral_list_t *l, ral_list_t *n)
{
    l->next->prev = n;
    n->next = l->next;
    l->next = n;
    n->prev = l;
}

RAL_INLINE void ral_list_insert_before(ral_list_t *l, ral_list_t *n)
{
    l->prev->next = n;
    n->prev = l->prev;
    l->prev = n;
    n->next = l;
}

RAL_INLINE void ral_list_remove(ral_list_t *n)
{
    n->next->prev = n->prev;
    n->prev->next = n->next;
    n->next = n->prev = n;
}

RAL_INLINE int ral_list_is_empty(const ral_list_t *l)
{
    return l->next == l;
}

RAL_INLINE int ral_list_len(const ral_list_t *l)
{
    int len = 0;
    const ral_list_t *p = l;
    while (p->next != l) {
        p = p->next;
        len ++;
    }
    return len;
}

#define ral_container_of(ptr, type, member)                                   \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

#define ral_list_obj_init(obj) {&(obj), &(obj)}

#define ral_list_entry(node, type, member)                                    \
    ral_container_of(node, type, member)

#define ral_list_for_each(pos, head)                                           \
    for (pos = (head)->next; pos != (head); pos = pos->next)


#ifdef __cplusplus
}
#endif

#endif
