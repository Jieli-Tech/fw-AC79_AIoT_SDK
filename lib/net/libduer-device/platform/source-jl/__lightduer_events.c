// Copyright (2017) Baidu Inc. All rights reserveed.
//
// File: lightduer_events.c
// Auth: Zhang Leliang (zhangleliang@baidu.com)
// Desc: Light duer events looper.


#include "lightduer_events.h"

#include <sys/time.h>
#include "os/os_compat.h"

#include "lightduer_connagent.h"
#include "lightduer_lib.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"
#include "lightduer_priority_conf.h"

typedef struct _duer_queue_message_s {
    duer_events_func              _callback;
    int                           _what;
    void                         *_data;
    //struct _duer_queue_message_s *_pre;
    //struct _duer_queue_message_s *_next;
} duer_eq_message;

typedef struct _duer_events_struct {
    int  _task;
    //int  _mutex;
    //duer_eq_message *_head;
    //duer_eq_message *_tail;
    char            *_name;
    bool             _shutdown;
} duer_events_t;

extern unsigned long long timer_get_ms(void);

static unsigned long long duer_events_timestamp()
{
    return timer_get_ms();
}

static void duer_events_task(void *context)
{
    duer_events_t *events = (duer_events_t *)(context);
    unsigned long long runtime = 0;
    duer_eq_message *message;
    int err = 0;
    int msg[32];

    while (1) {
        /* pthread_mutex_lock(&events->_mutex); */
        /* while (events->_tail == NULL && !events->_shutdown) { */
        /* //pthread_cond_wait(&events->_cond, &events->_mutex); */
        /* } */
        /* if (events->_shutdown) { */
        /* pthread_mutex_unlock(&events->_mutex); */
        /* //pthread_exit(0); */
        /* } */
        /* message = events->_tail; */
        /* events->_tail = events->_tail->_pre; */
        /* if (events->_tail == NULL) { */
        /* events->_head = NULL; */
        /* } */
        /* pthread_mutex_unlock(&events->_mutex); */
        err = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (events->_shutdown) {
            break;
        }
        if (err == OS_TASKQ) {
            message = (duer_eq_message *)msg[1];
        } else {
            continue;
        }
        if (message) {
            if (message->_callback) {
                runtime = duer_events_timestamp();
                //DUER_LOGD("[%s] <== event begin = %p", events->_name, message->_callback);
                message->_callback(message->_what, message->_data);
                runtime = duer_events_timestamp() - runtime;
                if (runtime > 50) {
                    //DUER_LOGW("[%s] <== event end = %p, timespent = %ld, message:%p", events->_name, message->_callback, runtime, message);
                }
            }
            DUER_FREE(message);
            message = NULL;
        } else {
            DUER_LOGE("function:%s, get_msg_fifo error!!! line:%d\n", __FUNCTION__, __LINE__);
        }
    }
}

duer_events_handler duer_events_create(const char *name, size_t stack_size, size_t queue_length)
{
    duer_events_t *events = NULL;
    int rs = DUER_ERR_FAILED;
    int status = 0;
    DUER_LOGD("duer_events_create: %s", name);

    do {
        if (name == NULL) {
            name = "default";
        }

        events = (duer_events_t *)DUER_MALLOC(sizeof(duer_events_t));
        if (!events) {
            DUER_LOGE("[%s] Alloc the events resource failed!", events->_name);
            break;
        }

        DUER_MEMSET(events, 0, sizeof(duer_events_t));

        events->_name = (char *)DUER_MALLOC(DUER_STRLEN(name) + 1);
        if (events->_name) {
            DUER_MEMCPY(events->_name, name, DUER_STRLEN(name) + 1);
        }

        //events->_head = NULL;
        //events->_tail = NULL;

        status = thread_fork(name, duer_priority_get(duer_priority_get_task_id(name)), stack_size, queue_length, &events->_task, duer_events_task, events);
        if (status) {
            DUER_LOGE("pthread_create failed! status:%d", status);
            break;
        }

        rs = DUER_OK;
    } while (0);

    if (rs != DUER_OK) {
        duer_events_destroy(events);
        events = NULL;
    }

    return events;
}

int duer_events_call(duer_events_handler handler, duer_events_func func, int what, void *object)
{
    int rs = DUER_ERR_FAILED;
    duer_events_t *events = (duer_events_t *)handler;

    do {
        if (events == NULL || events->_name == NULL) {
            break;
        }
        DUER_LOGD("duer_events_call, name:%s, what:%d, obj:%p", events->_name, what, object);
        duer_eq_message *message = DUER_MALLOC(sizeof(duer_eq_message));
        if (message == NULL) {
            DUER_LOGE("malloc duer_eq_message fail!!");
            break;
        }

        message->_callback = func;
        message->_what = what;
        message->_data = object;
        //message->_pre = NULL;

        os_taskq_post(events->_name, 1, (int *)message) ;
        rs = DUER_OK;

        /* pthread_mutex_lock(&events->_mutex); */
        /* if (events->_head) { */
        /* events->_head->_pre = message; */
        /* } */
        /* message->_next = events->_head; */
        /* events->_head = message; */
        /* if (events->_tail == NULL) { */
        /* events->_tail = message; */
        /* } */
        /* pthread_mutex_unlock(&events->_mutex); */
        /* if (events->_tail == message) { */
        /* //pthread_cond_signal(&events->_cond); */
        /* } */

    } while (0);

    if (rs < 0 && events != NULL && events->_name != NULL) {
        DUER_LOGE("[%s] Queue send failed!", events->_name);
    }

    return rs;
}

void duer_events_destroy(duer_events_handler handler)
{
    duer_events_t *events = (duer_events_t *)handler;

    if (events) {
        if (!events->_shutdown) {
            events->_shutdown = true;
            os_taskq_post(events->_name, 1, 0) ;
        } else {
            DUER_LOGI("already destroy!");
            return;
        }

        thread_kill(&events->_task, KILL_WAIT);

        //msleep(1000);// wait for the thread exit
        /* pthread_mutex_lock(&events->_mutex); */
        /* while (events->_head) { */
        /* duer_eq_message *message = events->_head; */
        /* events->_head = events->_head->_next; */
        /* DUER_FREE(message); */
        /* } */
        /* pthread_mutex_unlock(&events->_mutex); */
        /* pthread_mutex_destroy(&events->_mutex); */

        if (events->_name) {
            DUER_FREE(events->_name);
            events->_name = NULL;
        }

        DUER_FREE(events);
    }
}
