/*
 * Tencent is pleased to support the open source community by making IoT Hub
 available.
 * Copyright (C) 2018-2020 THL A29 Limited, a Tencent company. All rights
 reserved.

 * Licensed under the MIT License (the "License"); you may not use this file
 except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT

 * Unless required by applicable law or agreed to in writing, software
 distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 KIND,
 * either express or implied. See the License for the specific language
 governing permissions and
 * limitations under the License.
 *
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

//#include "freertos/FreeRTOS.h"
//#include "freertos/semphr.h"
//#include "freertos/task.h"
#include "qcloud_iot_export_error.h"
#include "qcloud_iot_import.h"

#include "os/os_api.h"


/**************************************消息队列*****************************/
void *HAL_QueueCreate(unsigned long queue_length, unsigned long queue_item_size)
{
    OS_QUEUE *queue_handle = (OS_QUEUE *)calloc(1, sizeof(OS_QUEUE));
    if (NULL == queue_handle) {
        return NULL;
    }
    if (os_q_create(queue_handle, queue_length * queue_item_size)) {
        return NULL;
    }
    return (void *)queue_handle;

}

void HAL_QueueDestory(void *queue_handle)
{
    if (queue_handle) {
        os_q_del((OS_QUEUE *)queue_handle, OS_DEL_ALWAYS);
        free(queue_handle);
    }
}

uint32_t HAL_QueueReset(void *queue_handle)
{
    if (!os_q_flush((OS_QUEUE *)queue_handle)) {
        return QCLOUD_RET_SUCCESS;
    } else {
        return QCLOUD_ERR_FAILURE;
    }
}

unsigned long HAL_QueueItemWaitingCount(void *queue_handle)
{
    return os_q_query((OS_QUEUE *)queue_handle);
}

unsigned long HAL_QueueItemPop(void *queue_handle, void *const item_buffer, uint32_t wait_timeout)
{
    if (!os_q_recv((OS_QUEUE *)queue_handle, item_buffer, wait_timeout)) {
        return QCLOUD_RET_SUCCESS;
    } else {
        return QCLOUD_ERR_FAILURE;
    }
}

unsigned long HAL_QueueItemPush(void *queue_handle, void *const item_buffer, uint32_t wait_timeout)
{
    if (!os_q_pend((OS_QUEUE *)queue_handle, wait_timeout, item_buffer)) {
        return QCLOUD_RET_SUCCESS;
    } else {
        return QCLOUD_ERR_FAILURE;
    }
}

void HAL_SleepMs(_IN_ uint32_t ms)
{
    unsigned int t;

    t = ms / 10 + ((ms % 10) > 5 ? 1 : 0);

    os_time_dly(t == 0 ? 1 : t);
}

void HAL_Printf(_IN_ const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

int HAL_Snprintf(_IN_ char *str, const int len, const char *fmt, ...)
{
    va_list args;
    int     rc;

    va_start(args, fmt);
    rc = vsnprintf(str, len, fmt, args);
    va_end(args);

    return rc;
}

int HAL_Vsnprintf(_IN_ char *str, _IN_ const int len, _IN_ const char *format, va_list ap)
{
    return vsnprintf(str, len, format, ap);
}

void *HAL_Malloc(_IN_ uint32_t size)
{
    return malloc(size);
}

void HAL_Free(_IN_ void *ptr)
{
    if (ptr) {
        free(ptr);
        ptr = NULL;
    }
}

void *HAL_Calloc(size_t nelements, size_t elementSize)
{
    return calloc(nelements, elementSize);
}

void *HAL_MutexCreate(void)
{
    OS_MUTEX *mutex = (OS_MUTEX *)calloc(1, sizeof(OS_MUTEX));
    if (mutex) {
        os_mutex_create(mutex);
    }
    return (void *)mutex;
}

void HAL_MutexDestroy(_IN_ void *mutex)
{
    if (mutex) {
        os_mutex_del((OS_MUTEX *)mutex, OS_DEL_ALWAYS);
        free(mutex);
    }
}

void HAL_MutexLock(_IN_ void *mutex)
{
    if (mutex) {
        os_mutex_pend((OS_MUTEX *)mutex, 0);
    }
}

int HAL_MutexTryLock(_IN_ void *mutex)
{
    if (mutex) {
        int ret = os_mutex_pend((OS_MUTEX *)mutex, 100);
        if (ret != 0 && ret != OS_TIMEOUT) {
            HAL_Printf("%s: HAL_MutexTryLock failed\n", __FUNCTION__);
            return -1;
        }
    }

    return -1;
}

void HAL_MutexUnlock(_IN_ void *mutex)
{
    if (mutex) {
        os_mutex_post((OS_MUTEX *)mutex);
    }
}

int HAL_TaskCreate(void (*func)(void *), char *thread_name, int stk_size, int *parm, int prio, void *pxCreatedTask)
{
    return thread_fork(thread_name, prio, stk_size, 0, NULL, func, parm);
}

int HAL_TaskDelete(void *param)
{
    return 0;
}

#ifdef MULTITHREAD_ENABLED

// platform-dependant thread routine/entry function
static void _HAL_thread_func_wrapper_(void *ptr)
{
    ThreadParams *params = (ThreadParams *)ptr;

    params->thread_func(params->user_arg);
}

// platform-dependant thread create function
int HAL_ThreadCreate(ThreadParams *params)
{
    if (params == NULL) {
        return QCLOUD_ERR_INVAL;
    }

    if (params->thread_name == NULL) {
        HAL_Printf("thread name is required for FreeRTOS platform!\n");
        return QCLOUD_ERR_INVAL;
    }

    return thread_fork(params->thread_name, params->priority + 20, params->stack_size / 4, 0, (int *)&params->thread_id, _HAL_thread_func_wrapper_, params);
}

#endif
