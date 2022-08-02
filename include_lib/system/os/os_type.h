#ifndef __OS_TYPE_H
#define __OS_TYPE_H


#define OS_TICKS_PER_SEC          100

#if defined CONFIG_UCOS_ENABLE
#include <os/ucos_ii.h>


#define  OS_EVENT_TYPE_UNUSED         0u
#define  OS_EVENT_TYPE_Q              1u
#define  OS_EVENT_TYPE_SEM            2u
#define  OS_EVENT_TYPE_MUTEX          3u

typedef struct {
    unsigned char OSEventType;
    int aa;
    void *bb;
    unsigned char value;
    unsigned char prio;
    unsigned short cc;
} OS_SEM, OS_MUTEX, OS_QUEUE;

typedef struct  {
    u8         OSFlagType;
    void         *OSFlagWaitList;
    u32      OSFlagFlags;
} OS_EVENT_GRP;


#ifdef CONFIG_THREAD_ENABLE
#ifndef IN_UCOS
typedef OS_TCB StaticTask_t ;
struct task_queue {
    void *q;
};
#define configMAX_TASK_NAME_LEN 64
#define portMAX_DELAY 0xffff
struct thread_parm {
    char static_thread_flag;
    char thread_name[configMAX_TASK_NAME_LEN];
    OS_TCB task_tcb;
    void *parm;
    void (*thread_func)(void *);
    void (*thread_complete_cb)(void *);
    int (*rpc_condition)(void);
    void *thread_complete_cb_parm;
};
#endif
#endif

#elif defined CONFIG_FREE_RTOS_ENABLE

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/semphr.h"
#include "FreeRTOS/task.h"

typedef StaticSemaphore_t OS_SEM, OS_MUTEX;
typedef StaticQueue_t OS_QUEUE;
typedef StaticEventGroup_t OS_EVENT_GRP;

#ifdef CONFIG_THREAD_ENABLE
#include "generic/list.h"
struct task_queue {
    struct list_head entry;
    TaskHandle_t task;
    StaticQueue_t queue;
};
struct thread_parm {
    char static_thread_flag;
    char thread_name[configMAX_TASK_NAME_LEN];
    StaticTask_t task_tcb;
    void *parm;
    void (*thread_func)(void *);
    void (*thread_complete_cb)(void *);
    int (*rpc_condition)(void);
    void *thread_complete_cb_parm;
};
#endif


#else
#error "no_os_defined"
#endif





















#endif
