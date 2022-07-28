/**
* @file  tvs_threads.c
* @brief TVS SDK Thread封装
* @date  2019-5-10
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tvs_threads.h"
#include "os_wrapper.h"

#define TVS_LOG_DEBUG  0
#include "tvs_log.h"

static bool g_use_test_locker_mode = false;

/**********************************************************************************/
/*
TVS_HTREADS
*/
/**********************************************************************************/

struct tvs_thread_handle {
    void *thread_handle;          /*!< 线程句柄 */
    thread_func func;             /*!< 线程主函数 */
    thread_end_func end_func;     /*!< 线程结束前调用的回调函数 */
    void *locker_mutex;            /*!< 用于保护多线程使用的公共资源 */
    void *runner_mutex;    /*!< 用于等待线程执行完毕 */
    bool loop_running;      /*!< 用于控制线程执行的开关变量 */
    bool start;             /*!< 判断线程是否停止 */
    void *param;            /*!< 线程参数 */
};

void *tvs_thread_get_param(tvs_thread_handle_t *thread)
{
    if (NULL == thread) {
        return NULL;
    }
    return thread->param;
}

void *tvs_thread_get_task_handle(tvs_thread_handle_t *thread)
{
    if (NULL == thread) {
        return NULL;
    }
    return (void *)thread->thread_handle;
}

void tvs_thread_lock(tvs_thread_handle_t *thread)
{
    if (thread == NULL) {
        return;
    }

    os_wrapper_lock_mutex(thread->locker_mutex, os_wrapper_get_forever_time());
}

void tvs_thread_unlock(tvs_thread_handle_t *thread)
{
    if (thread == NULL) {
        return;
    }

    os_wrapper_unlock_mutex(thread->locker_mutex);
}

static void tvs_thread_function(void *param)
{
    tvs_thread_handle_t *thread = (tvs_thread_handle_t *)param;

    thread->func(thread);
    tvs_thread_lock(thread);
    thread->loop_running = false;
    thread->thread_handle = NULL;
    thread->start = false;
    tvs_thread_unlock(thread);
    if (thread->end_func != NULL) {
        // 执行end_fuc
        thread->end_func(thread);
    }
    if (thread->param != NULL) {
        // 由于在start_prepare阶段拷贝了一份，所以需要释放param占用的内存
        TVS_FREE(thread->param);
        thread->param = NULL;
    }
    // 解锁，
    os_wrapper_unlock_mutex(thread->runner_mutex);
    /* os_wrapper_delete_mutex(thread->runner_mutex); */
    /* os_wrapper_delete_mutex(thread->locker_mutex); */
    os_wrapper_thread_delete(NULL);
}

tvs_thread_handle_t *tvs_thread_new(thread_func func, thread_end_func end_func)
{
    tvs_thread_handle_t *thread = TVS_MALLOC(sizeof(tvs_thread_handle_t));
    if (NULL == thread) {
        return NULL;
    }
    memset(thread, 0, sizeof(tvs_thread_handle_t));

    thread->runner_mutex = os_wrapper_create_locker_mutex();
    thread->locker_mutex = os_wrapper_create_locker_mutex();
    thread->func = func;
    thread->end_func = end_func;

    return thread;
}

void tvs_thread_start_now(tvs_thread_handle_t *thread, const char *name, int prior, int stack_depth)
{
    // 开始执行线程
    thread->thread_handle = os_wrapper_start_thread(tvs_thread_function, thread, name, prior, stack_depth);

    if (thread->thread_handle == NULL) {
        // 线程创建失败，需要解锁
        os_wrapper_unlock_mutex(thread->runner_mutex);
    }
}

// 准备启动线程
void tvs_thread_start_prepare(tvs_thread_handle_t *thread, void *param, int param_size)
{
    if (NULL == thread) {
        return;
    }

    // 加锁，为了避免同一个线程的两个实例并发，如果一个实例正在运行，那么另一个实例会因此等待
    // 如果其他线程调用了join，也会因此等待
    os_wrapper_lock_mutex(thread->runner_mutex, os_wrapper_get_forever_time());

    tvs_thread_lock(thread);
    thread->start = true;
    if (thread->param != NULL) {
        TVS_FREE(thread->param);
        thread->param = NULL;
    }
    if (NULL != param && param_size > 0) {
        // param需要复制一份
        thread->param = TVS_MALLOC(param_size);
        if (NULL != thread->param) {
            memcpy(thread->param, param, param_size);
        }
    }
    thread->loop_running = true;
    tvs_thread_unlock(thread);
}

// 线程是否停止
bool tvs_thread_is_stop(tvs_thread_handle_t *thread)
{
    bool stop = false;
    if (NULL == thread) {
        return true;
    }
    tvs_thread_lock(thread);
    stop = !thread->start;
    tvs_thread_unlock(thread);
    return stop;
}

// 停止线程中的循环体
void tvs_thread_try_stop(tvs_thread_handle_t *thread)
{
    if (thread == NULL) {
        return;
    }

    if (tvs_thread_is_stop(thread)) {
        return;
    }

    // try stop thread
    tvs_thread_lock(thread);
    thread->loop_running = false;
    tvs_thread_unlock(thread);
}

// 判断线程体是否可以循环执行
bool tvs_thread_can_loop(tvs_thread_handle_t *thread)
{
    if (NULL == thread) {
        return false;
    }

    bool can_loop = false;
    tvs_thread_lock(thread);
    can_loop = thread->loop_running;
    tvs_thread_unlock(thread);
    return can_loop;
}

// 阻塞等待线程执行完毕
bool tvs_thread_join(tvs_thread_handle_t *thread, int block_ms)
{
    if (thread == NULL) {
        return true;
    }

    if (tvs_thread_is_stop(thread)) {
        return true;
    }

    if (!os_wrapper_lock_mutex(thread->runner_mutex, block_ms)) {
        // 加锁失败的情况
        return false;
    }

    // 加锁成功，代表等待的目标线程已经执行完毕，则解锁
    os_wrapper_unlock_mutex(thread->runner_mutex);

    return true;
}

void tvs_thread_release(tvs_thread_handle_t **p_handle)
{
    if (NULL == p_handle || NULL == *p_handle) {
        return;
    }

    TVS_FREE(*p_handle);
    *p_handle = NULL;
}

void tvs_thread_use_locker_test_mode(bool mode)
{
    g_use_test_locker_mode = mode;
}

bool tvs_do_lock(const char *file, int line, const char *last_file, int last_line, void *mutex)
{
    if (g_use_test_locker_mode) {
        bool lock = os_wrapper_lock_mutex(mutex, 10 * 1000);
        if (!lock) {
            // 如果10秒内都没有解锁，打印日志
            int i = 10;
            while (i > 0) {
                printf("!!!!!WANNING, lock failed!, file %s:%d, lock by %s:%d\n",
                       file, line, last_file == NULL ? "" : last_file, last_line);
                i--;
            }
        } else {
            return lock;
        }
    }
    return os_wrapper_lock_mutex(mutex, os_wrapper_get_forever_time());
}

