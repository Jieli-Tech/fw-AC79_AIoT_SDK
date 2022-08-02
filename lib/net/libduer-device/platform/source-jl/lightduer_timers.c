// Copyright (2017) Baidu Inc. All rights reserveed.
//
// File: lightduer_timers.c
// Auth: Zhang leliang (zhangleliang @baidu.com)
// Desc: Provide the timer APIs.

#include "lightduer_timers.h"


#include "typedef.h"
#include "net_timer.h"

#include "lightduer_connagent.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"

#define DUER_TIMER_DELAY	(100)

#define assert ASSERT

typedef struct _duer_timers_s {
    int 	            _timer;
    int                 _type;
    duer_timer_callback _callback;
    void               *_param;
} duer_timers_t;

static void duer_timer_callback_wrapper(void *param)
{
    assert(param);

    // Which timer expired?
    duer_timers_t *timer = (duer_timers_t *)param;

    if (timer != NULL && timer->_callback != NULL) {
        timer->_callback(timer->_param);
    }
}

duer_timer_handler duer_timer_acquire(duer_timer_callback callback, void *param, int type)
{
    duer_timers_t *handle = NULL;
    int rs = DUER_ERR_FAILED;
    int status;
    DUER_LOGD("duer_timer_acquire, type:%d", type);
    do {
        handle = (duer_timers_t *)DUER_MALLOC(sizeof(duer_timers_t));
        if (handle == NULL) {
            DUER_LOGE("Memory Overflow!!!");
            break;
        }

        handle->_callback = callback;
        handle->_param = param;
        handle->_type = type;
        handle->_timer = -1;

        rs = DUER_OK;
    } while (0);

    if (rs < DUER_OK) {
        duer_timer_release(handle);
        handle = NULL;
    }

    return handle;
}

int duer_timer_start(duer_timer_handler handle, size_t delay)
{
    duer_timers_t *timer = (duer_timers_t *)handle;
    int rs = DUER_ERR_FAILED;

    do {
        if (timer == NULL) {
            rs = DUER_ERR_INVALID_PARAMETER;
            break;
        }

        if (timer->_type == DUER_TIMER_PERIODIC) {
            timer->_timer = net_timer_add(timer, duer_timer_callback_wrapper, delay, 1);
        } else if (timer->_type == DUER_TIMER_ONCE) {
            timer->_timer = net_timer_add(timer, duer_timer_callback_wrapper, delay, 0);
        }

        rs = DUER_OK;
    } while (0);

    return rs;
}

int duer_timer_is_valid(duer_timer_handler handle)
{
    duer_timers_t *timer = (duer_timers_t *)handle;
    return (timer != NULL);
}

int duer_timer_stop(duer_timer_handler handle)
{
    duer_timers_t *timer = (duer_timers_t *)handle;
    if (timer != NULL) {
        if (timer->_timer != -1) {
            net_timer_del(timer->_timer);
            timer->_timer = -1;
        }
    }
    return 0;
}

void duer_timer_release(duer_timer_handler handle)
{
    duer_timers_t *timer = (duer_timers_t *)handle;
    if (timer != NULL) {
        duer_timer_stop(timer);
        DUER_FREE(timer);
    }
}
