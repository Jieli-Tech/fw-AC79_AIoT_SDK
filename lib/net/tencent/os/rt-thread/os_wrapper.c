#include "os_wrapper.h"
#include <rtthread.h>

#define THREAD_TIMESLICE     5

#if !(TVS_CONFIG_OS_USE_DEFAULT_MALLOC)

void *os_wrapper_malloc(int size)
{
    return rt_malloc(size);
}

void *os_wrapper_realloc(void *pv, int size)
{
    return rt_realloc(pv, size);
}

void *os_wrapper_calloc(int nmemb, int size)
{
    return rt_calloc(nmemb, size);
}

void os_wrapper_free(void *p)
{
    return rt_free(p);
}

#endif

long os_wrapper_get_forever_time()
{
    return RT_WAITING_FOREVER;
}

/***************************信号量*********************************/
void *os_wrapper_create_signal_mutex(int init_count)
{
    return rt_sem_create("tvs", init_count, RT_IPC_FLAG_FIFO);
}

bool os_wrapper_wait_signal(void *mutex, long time_ms)
{
    return rt_sem_take(mutex, time_ms) == RT_EOK;
}

void os_wrapper_post_signal(void *mutex)
{
    rt_sem_release(mutex);
}

/***************************互斥锁*********************************/
void *os_wrapper_create_locker_mutex()
{
    return rt_mutex_create("tvs", RT_IPC_FLAG_FIFO);
}

bool os_wrapper_lock_mutex(void *mutex, long time_ms)
{
    return rt_mutex_take(mutex, time_ms) == RT_EOK;
}

void os_wrapper_unlock_mutex(void *mutex)
{
    rt_mutex_release(mutex);
}

void os_wrapper_sleep(long time_ms)
{
    rt_thread_sleep(time_ms);
}

void os_wrapper_thread_delete(void **thread_handle)
{
    // rt-thread无需实现此函数
}

void *os_wrapper_start_thread(void *thread_func, void *param, const char *name, int prior, int stack_depth)
{
    rt_thread_t tid = RT_NULL;

    tid = rt_thread_create(name, thread_func, param, stack_depth * 4, prior + 20, THREAD_TIMESLICE);

    if (tid != RT_NULL) {
        rt_thread_startup(tid);
    }

    return tid;
}

long os_wrapper_get_time_ms()
{
    return rt_tick_get();
}

void os_wrapper_start_timer(void **handle, void *func, int time_ms, bool repeat)
{
    if (handle == NULL) {
        return;
    }

    if (*handle != NULL) {
        rt_timer_stop(*handle);
        rt_timer_delete(*handle);
    }

    rt_uint8_t flag = repeat ? RT_TIMER_FLAG_PERIODIC : RT_TIMER_FLAG_ONE_SHOT;

    *handle = rt_timer_create("t", func, RT_NULL, time_ms, flag);

    if (*handle != RT_NULL) {
        rt_timer_start(*handle);
    }
}

void os_wrapper_stop_timer(void *handle)
{
    if (handle == NULL) {
        return;
    }

    rt_timer_stop(handle);
}

