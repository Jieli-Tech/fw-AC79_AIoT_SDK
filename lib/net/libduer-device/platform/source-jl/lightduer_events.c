// Copyright (2017) Baidu Inc. All rights reserveed.
//
// File: lightduer_events.c
// Auth: Zhang Leliang (zhangleliang@baidu.com)
// Desc: Light duer events looper.


#include "lightduer_events.h"

#include <sys/time.h>
#include "os/os_api.h"

#include "lightduer_connagent.h"
#include "lightduer_lib.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"
#include "lightduer_priority_conf.h"
#include "lightduer_timestamp.h"

typedef struct _duer_queue_message_s {
    duer_events_func              _callback;
    int                           _what;
    void                         *_data;
    struct _duer_queue_message_s *_pre;
    struct _duer_queue_message_s *_next;
} duer_eq_message;

typedef struct _duer_events_struct {
    int  _magic;
    int  _task;
    int  _mutex;
    OS_SEM	_sem;
    duer_eq_message *_head;
    duer_eq_message *_tail;
    char            *_name;
    bool             _shutdown;
} duer_events_t;

static unsigned long duer_events_timestamp()
{
    return duer_timestamp();
}

static void duer_events_task(void *context)
{
    duer_events_t *events = (duer_events_t *)(context);
    unsigned long long runtime = 0;
    duer_eq_message *message;
    DUER_LOGI("duer_events_task_go: %s", events->_name);

    while (1) {
        /* pthread_mutex_lock(&events->_mutex); */
        os_mutex_pend((OS_MUTEX *)events->_mutex, 0);
        while (events->_tail == NULL && !events->_shutdown) {
            /* pthread_mutex_unlock(&events->_mutex); */
            os_mutex_post((OS_MUTEX *)events->_mutex);
            os_sem_pend(&events->_sem, 0);
            /* pthread_mutex_lock(&events->_mutex); */
            os_mutex_pend((OS_MUTEX *)events->_mutex, 0);
        }

        if (events->_shutdown) {
            DUER_LOGI("Duer Os event task[%s] shutdown ", events->_name);
            /* pthread_mutex_unlock(&events->_mutex); */
            os_mutex_post((OS_MUTEX *)events->_mutex);
            return;
        }
        message = events->_tail;
        events->_tail = events->_tail->_pre;
        if (events->_tail == NULL) {
            events->_head = NULL;
        }
        /* pthread_mutex_unlock(&events->_mutex); */
        os_mutex_post((OS_MUTEX *)events->_mutex);

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
            DUER_LOGE("[%s] Alloc the events resource failed!", name);
            break;
        }

        DUER_MEMSET(events, 0, sizeof(duer_events_t));

        events->_name = (char *)DUER_MALLOC(DUER_STRLEN(name) + 1);
        if (events->_name) {
            DUER_MEMCPY(events->_name, name, DUER_STRLEN(name) + 1);
        }

        /* pthread_mutex_init(&events->_mutex, NULL); */
        /*****************************************************/
        OS_MUTEX *pmutex;
        pmutex = (OS_MUTEX *)malloc(sizeof(OS_MUTEX));
        if (pmutex == NULL) {
            puts("duer_events_create err");
            events->_mutex = 0;
        }

        os_mutex_create(pmutex);
        events->_mutex = (int)pmutex;
        /*****************************************************/
        os_sem_create(&events->_sem, 0);
        events->_head = NULL;
        events->_tail = NULL;
        events->_magic = 0x3DC65FA4;

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

        if (events->_magic != 0x3DC65FA4) {
            DUER_LOGE("events->_magic != 0x3DC65FA4 !");
            return rs;
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
        message->_pre = NULL;

        /* pthread_mutex_lock(&events->_mutex); */
        os_mutex_pend((OS_MUTEX *)events->_mutex, 0);

        if (events->_magic != 0x3DC65FA4) {
            DUER_LOGE("events->_magic != 0x3DC65FA4 !!");
            DUER_FREE(message);
            /* pthread_mutex_unlock(&events->_mutex); */
            os_mutex_post((OS_MUTEX *)events->_mutex);
            return rs;
        }
        if (events->_head) {
            events->_head->_pre = message;
        }
        message->_next = events->_head;
        events->_head = message;
        if (events->_tail == NULL) {
            events->_tail = message;
        }
        /* pthread_mutex_unlock(&events->_mutex); */
        os_mutex_post((OS_MUTEX *)events->_mutex);
        if (events->_tail == message) {
            os_sem_set(&events->_sem, 0);
            os_sem_post(&events->_sem);
        }

        rs = DUER_OK;
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
        } else {
            DUER_LOGI("already destroy!");
            return;
        }

        os_sem_post(&events->_sem);

        //DUER_LOGI("name:%s, id:0x%x, &id:0x%x,  cur id : 0x%x  \n", events->_name, events->_task, &events->_task, get_cur_thread_pid());

        thread_kill(&events->_task, KILL_WAIT);

        /* pthread_mutex_lock(&events->_mutex); */
        os_mutex_pend((OS_MUTEX *)events->_mutex, 0);
        events->_magic = 0;
        while (events->_head) {
            duer_eq_message *message = events->_head;
            events->_head = events->_head->_next;
            DUER_FREE(message);
        }
        /* pthread_mutex_unlock(&events->_mutex); */
        os_mutex_post((OS_MUTEX *)events->_mutex);
        /* pthread_mutex_destroy(&events->_mutex); */
        /********************************************/
        os_mutex_del((OS_MUTEX *)(events->_mutex), OS_DEL_ALWAYS);
        if (events->_mutex) {
            free((void *)(events->_mutex));
            events->_mutex = 0;
        }
        /********************************************/
        os_sem_del(&events->_sem, 1);

        if (events->_name) {
            DUER_FREE(events->_name);
            events->_name = NULL;
        }

        DUER_FREE(events);
    }
}
