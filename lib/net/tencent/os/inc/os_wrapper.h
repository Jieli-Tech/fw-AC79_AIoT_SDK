
/**
* @file  os_wrapper.h
* @brief TVS SDK OS抽象层封装
* @date  2019-5-10
* 为了适配不同的操作系统，将SDK所需要的系统函数抽象成单独一层，不同的OS有不同的实现
*/
#ifndef __OS_WRAPPER_H_JIL212FAFD__
#define __OS_WRAPPER_H_JIL212FAFD__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/**
 * @brief 配置OS Wrapper，是否使用默认的malloc/realloc/calloc/free等内存管理函数
 *
 * 如果对应OS不推荐使用默认的malloc/realloc/calloc/free等内存管理函数，需要在编译脚本中
 * 加入-DTVS_CONFIG_OS_USE_DEFAULT_MALLOC=0，并实现os_wrapper_malloc等函。
 *
 * @param size 需要分配的内存块的大小，单位为字节。
 * @return 成功则返回分配的内存块地址；失败则返回NULL
 */
#ifndef TVS_CONFIG_OS_USE_DEFAULT_MALLOC
#define TVS_CONFIG_OS_USE_DEFAULT_MALLOC  1
#endif

#if (TVS_CONFIG_OS_USE_DEFAULT_MALLOC)

#if 0
void *os_wrapper_malloc(int size, const char *file, int line);
void *os_wrapper_realloc(void *rmem, int newsize, const char *file, int line);

void os_wrapper_free(void *p);

void *os_wrapper_calloc(int nmemb, int size, const char *file, int line);


#define TVS_MALLOC(size) os_wrapper_malloc(size, __FILE__, __LINE__)
#define TVS_CALLOC(rmem, size) os_wrapper_calloc(rmem, size, __FILE__, __LINE__)
#define TVS_REALLOC(rmem, newsize) os_wrapper_realloc(rmem, newsize, __FILE__, __LINE__)
#define TVS_FREE os_wrapper_free

void os_wrapper_print_mem();

#else
#define TVS_MALLOC malloc
#define TVS_CALLOC calloc
#define TVS_REALLOC realloc
#define TVS_FREE free

#endif

#else
/**
 * @brief 分配内存块
 *
 * @param size 需要分配的内存块的大小，单位为字节。
 * @return 成功则返回分配的内存块地址；失败则返回NULL
 */
void *os_wrapper_malloc(int size);

/**
 * @brief 重新分配内存块
 *
 * @param rmem	 指向已分配的内存块
 * @param newsize 重新分配的内存大小
 * @return 成功则返回分配的内存块地址；失败则返回NULL
 */
void *os_wrapper_realloc(void *rmem, int newsize);

/**
 * @brief 分配多内存块
 *
 * @param nmemb	 分配的空间对象的计数
 * @param size 分配的空间对象的大小
 * @return 成功则指向指向第一个内存块地址的指针，并且所有分配的内存块都被初始化成零；如果失败则返回 NULL。
 */
void *os_wrapper_calloc(int nmemb, int size);

/**
 * @brief 释放内存块
 *
 * @param size 待释放的内存块指针
 * @return void
 */
void os_wrapper_free(void *p);

#define TVS_MALLOC os_wrapper_malloc
#define TVS_CALLOC os_wrapper_calloc
#define TVS_REALLOC os_wrapper_realloc
#define TVS_FREE os_wrapper_free
#endif

/**
 * @brief 在信号量等待信号，或者互斥量加锁的时候，如果要持续等待到获得信号或者加锁成功为止，需要调用此函数，并将结果作为参数传入对应函数中；
 * 例如：os_wrapper_wait_signal(mutex, os_wrapper_get_forever_time());
 *
 * @param
 * @return 二值信号量的句柄
 */
long os_wrapper_get_forever_time();

/**
 * @brief 创建二值信号量
 *
 * @param init_count 二值信号量的初始值，取值0或者1
 * @return 二值信号量的句柄
 */
void *os_wrapper_create_signal_mutex(int init_count);

/**
 * @brief 二值信号量等待信号
 *
 * @param mutex 二值信号量的句柄
 * @paran time_ms 等待时间，如果要永远等待下去，需要传入os_wrapper_get_forever_time的返回值
 * @return void
 */
bool os_wrapper_wait_signal(void *mutex, long time_ms);

/**
 * @brief 二值信号量释放信号
 *
 * @param mutex 二值信号量的句柄
 * @return void
 */
void os_wrapper_post_signal(void *mutex);

void os_wrapper_delete_signal(void *mutex);

/**
 * @brief 创建互斥量，一般用于保护公共变量，处理线程安全问题
 *
 * @param
 * @return 互斥量的句柄
 */
void *os_wrapper_create_locker_mutex();

/**
 * @brief 互斥量加锁
 *
 * @param mutex 目标互斥量的句柄
 * @paran time_ms 等待时间，如果要永远等待下去，需要传入os_wrapper_get_forever_time的返回值
 * @return 为true代表加锁成功，为false代表失败或者超时
 */
bool os_wrapper_lock_mutex(void *mutex, long time_ms);

/**
 * @brief 互斥量解锁
 *
 * @param mutex 目标互斥量
 * @return void
 */
void os_wrapper_unlock_mutex(void *mutex);

void os_wrapper_delete_mutex(void *mutex);

/**
 * @brief 睡眠
 *
 * @param time_ms 毫秒数
 * @return void
 */
void os_wrapper_sleep(long time_ms);

/**
 * @brief 此函数一般用于freeRTOS, 在task末尾调用vTaskDelete(NULL),其他OS一般用不到
 *
 * @param thread_handle 线程句柄
 * @return void
 */
void os_wrapper_thread_delete(void **thread_handle);

/**
 * @brief 启动线程
 *
 * @param thread_func 线程函数，void ()(void*)
 * @param param 线程参数
 * @param name 线程名称
 * @param prior 线程优先级，数值越大优先级越高
 * @param stack_depth 线程栈深度，注意，单位为字（4bytes）
 * @return 线程的句柄，NULL代表启动线程失败
 */
void *os_wrapper_start_thread(void *thread_func, void *param, const char *name, int prior, int stack_depth);

/**
 * @brief 获取从开机到当前时刻的持续时间
 *
 * @param
 * @return 从开机到当前时刻的持续时间，单位为毫秒
 */
long os_wrapper_get_time_ms();

/**
 * @brief 启动定时器
 *
 * @param handle 出参，timer的句柄
 * @param func 定时器触发时执行的函数
 * @param time_ms 定时间隔，单位为毫秒
 * @param repeat true代表定时器将重复触发，false代表只触发一次
 * @return void
 */
void os_wrapper_start_timer(void **handle, void *func, int time_ms, bool repeat);

/**
 * @brief 停止定时器
 *
 * @param handle timer的句柄
 * @return void
 */
void os_wrapper_stop_timer(void *handle);

#ifdef __cplusplus
}
#endif

#endif
