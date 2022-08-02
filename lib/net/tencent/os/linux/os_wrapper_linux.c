#include "os_wrapper.h"
#include <semaphore.h>
#include <pthread.h>
#include <sys/sysinfo.h>

long os_wrapper_get_forever_time()
{
    return -1;
}

/***************************信号量*********************************/
void *os_wrapper_create_signal_mutex(int init_count)
{
    sem_t *s = TVS_MALLOC(sizeof(sem_t));
    memset(s, 0, sizeof(sem_t));

    sem_init(s, 0, init_count);

    return s;
}

bool os_wrapper_wait_signal(void *sem, long msecs)
{
    if (msecs == -1) {
        return sem_wait(sem) == 0;
    }

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long secs = msecs / 1000;
    msecs = msecs % 1000;

    long add = 0;
    msecs = msecs * 1000 * 1000 + ts.tv_nsec;
    add = msecs / (1000 * 1000 * 1000);
    ts.tv_sec += (add + secs);
    ts.tv_nsec = msecs % (1000 * 1000 * 1000);

    return sem_timedwait(sem, &ts) == 0;
}

void os_wrapper_post_signal(void *sem)
{
    sem_post(sem);
}

/***************************互斥锁*********************************/
void *os_wrapper_create_locker_mutex()
{
    pthread_mutex_t *lock = TVS_MALLOC(sizeof(pthread_mutex_t));
    pthread_mutex_t init_value = PTHREAD_MUTEX_INITIALIZER;
    memcpy(lock, &init_value, sizeof(pthread_mutex_t));
    return lock;
}

bool os_wrapper_lock_mutex(void *mutex, long msecs)
{
    if (msecs == -1) {
        return pthread_mutex_lock(mutex) == 0;
    }

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long secs = msecs / 1000;
    msecs = msecs % 1000;

    long add = 0;
    msecs = msecs * 1000 * 1000 + ts.tv_nsec;
    add = msecs / (1000 * 1000 * 1000);
    ts.tv_sec += (add + secs);
    ts.tv_nsec = msecs % (1000 * 1000 * 1000);

    return pthread_mutex_timedlock(mutex, &ts) == 0;
}

void os_wrapper_unlock_mutex(void *mutex)
{
    pthread_mutex_unlock(mutex);
}

void os_wrapper_sleep(long time_ms)
{
    usleep(time_ms * 1000);
}

void os_wrapper_thread_delete(void **thread_handle)
{

}

void *os_wrapper_start_thread(void *thread_func, void *param, const char *name, int prior, int stack_depth)
{
    pthread_t thread_handle = NULL;

    pthread_create(&thread_handle, NULL, thread_func, param);

    return thread_handle;
}

static long get_sys_runtime(int type)
{
    struct timespec times = {0, 0};
    long time;

    clock_gettime(CLOCK_MONOTONIC, &times);

    if (1 == type) {
        time = times.tv_sec;
    } else {
        time = times.tv_sec * 1000 + times.tv_nsec / 1000000;
    }

    return time;
}


long os_wrapper_get_time_ms()
{
    return get_sys_runtime(2);
}

void os_wrapper_start_timer(void **handle, void *func, int time_ms, bool repeat)
{

}

void os_wrapper_stop_timer(void *handle)
{

}


