#ifndef TASK_SCHEDULE_H
#define TASK_SCHEDULE_H


#include "typedef.h"
#include "os/os_api.h"

//low -> high
static u8 TASK_PRIO_IDLE            = tskIDLE_PRIORITY;              //always exist
//Not time slice
#define TASK_DEBUG_IDLE             IO_DEBUG_TOGGLE(C, 0)

static u8 TASK_PRIO_CLI             = configMAX_PRIORITIES - 3;
static const char *TASK_NAME_CLI    = "CLI";
static u32 TASK_STACK_SIZE_CLI      = 128;
#define TASK_DEBUG_CLI              IO_DEBUG_TOGGLE(C, 2)

static u8 TASK_PRIO_INPUT           = configMAX_PRIORITIES - 2;
static const char *TASK_NAME_INPUT  = "Input Task";
static u32 TASK_STACK_SIZE_INPUT    = 128;
#define TASK_DEBUG_INPUT            IO_DEBUG_TOGGLE(C, 2)

static u8 TASK_PRIO_BTSTACK         = configMAX_PRIORITIES - 1;
static const char *TASK_NAME_BTSTACK = "btstack";
static u32 TASK_STACK_SIZE_BTSTACK  = 0x300;
#define TASK_DEBUG_BTSTACK()        IO_DEBUG_TOGGLE(C, 2)
extern struct thread_owner          bt_task_thread;

static u8 TASK_PRIO_USER            = configMAX_PRIORITIES - 2;
static const char *TASK_NAME_USER   = "btstack";
static u32 TASK_STACK_SIZE_USER     = 128;

#endif  /*  TASK_SCHEDULE_H */

