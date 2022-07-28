#include "os_wrapper.h"
#include "os/os_api.h"
#include "system/timer.h"


long os_wrapper_get_forever_time()
{
    return 0;
}


#define USE_MALLOC_SETTING	1	//yii: 1:信号量,互斥锁用动态分配创建,SDK工程创建后不作释放处理   0:静态创建,SDK断网后需处理释放(未完善)
/***************************信号量*********************************/
#if USE_MALLOC_SETTING
void *os_wrapper_create_signal_mutex(int init_count)
{
    OS_SEM *sem = calloc(sizeof(OS_SEM), 1);
    if (sem) {
        os_sem_create(sem, init_count);
    }
    return (void *)sem;
}
#else
static int sem_create_cnt = 0, mutex_create_cnt = 0;
static OS_SEM wrapper_sem[30] = {0};
void *os_wrapper_create_signal_mutex(int init_count)
{
    os_sem_create(&wrapper_sem[sem_create_cnt], init_count);
    if (!os_sem_valid(&wrapper_sem[sem_create_cnt])) {
        printf("-------%s------------%d----------sem create failed!!", __func__, __LINE__);
        return NULL;
    }
    printf("---------%s------------%d-----------cnt = %d", __func__, __LINE__, sem_create_cnt);
    sem_create_cnt++;
    return (void *)&wrapper_sem[sem_create_cnt - 1];
}
#endif

bool os_wrapper_wait_signal(void *mutex, long time_ms)
{
    return (os_sem_pend((OS_SEM *)mutex, time_ms) == 0);
}

void os_wrapper_post_signal(void *mutex)
{
    os_sem_post((OS_SEM *)mutex);
}

void os_wrapper_delete_signal(void *mutex)  	//yii：信号量释放函数
{
    os_sem_del((OS_SEM *)mutex, 1);
    free(mutex);
}
#if USE_MALLOC_SETTING
//
#else
//yii:添加sem释放函数
int os_wrapper_del_sem(void)
{
    int i = 0;
    while (os_sem_valid(&wrapper_sem[i])) {
        if (os_sem_del(&wrapper_sem[i], 0)) {
            printf("-----------%s------------%d-----os_sem_del[%d] failed", __func__, __LINE__, i);
            return -1;
        }
        i++;
    }
    return 0;
}
#endif

/***************************互斥锁*********************************/

#if USE_MALLOC_SETTING
void *os_wrapper_create_locker_mutex()
{
    OS_MUTEX *mutex = calloc(sizeof(OS_MUTEX), 1);
    if (mutex) {
        os_mutex_create(mutex);
    }
    return (void *)mutex;
}
#else
static OS_MUTEX wrapper_mutex[30] = {0};
void *os_wrapper_create_locker_mutex()
{
    os_mutex_create(&wrapper_mutex[mutex_create_cnt]);

    if (!os_mutex_valid(&wrapper_mutex[mutex_create_cnt])) {
        printf("-------%s------------%d----------mutex create failed!!", __func__, __LINE__);
        return NULL;
    }
    printf("---------%s------------%d-----------cnt = %d", __func__, __LINE__, mutex_create_cnt);
    mutex_create_cnt++;
    return (void *)&wrapper_mutex[mutex_create_cnt - 1];
}
#endif

bool os_wrapper_lock_mutex(void *mutex, long time_ms)
{
    return (os_mutex_pend((OS_MUTEX *)mutex, time_ms) == 0);
}

void os_wrapper_unlock_mutex(void *mutex)
{
    os_mutex_post((OS_MUTEX *)mutex);
}

void os_wrapper_delete_mutex(void *mutex)  		//yii:互斥锁释放函数
{
    os_mutex_del((OS_MUTEX *)mutex, 1);
    free(mutex);
}
#if USE_MALLOC_SETTING

#else
//yii:添加mutex释放函数
int os_wrapper_del_mutex(void)
{
    int i = 0;
    while (os_mutex_valid(&wrapper_mutex[i])) {
        if (os_mutex_del(&wrapper_mutex[i], 0)) {
            printf("-----------%s------------%d-----os_mutex_del[%d] failed", __func__, __LINE__, i);
            return -1;
        }
        i++;
    }
    return 0;
}
#endif







/**************************延时函数*********************************/

void os_wrapper_sleep(long time_ms)
{
    msleep(time_ms);
}

extern u32 timer_get_ms(void);
long os_wrapper_get_time_ms()
{
    return timer_get_ms();
}


/**************************线程函数*********************************/

void *os_wrapper_start_thread(void *thread_func, void *param, const char *name, int prior, int stack_depth)
{

    if (0 != thread_fork(name, prior + 20, stack_depth, 0, NULL, thread_func, param)) {
        return NULL;
    }
    return (void *) - 1;

}

void os_wrapper_thread_delete(void **thread_handle)
{
//	thread_kill(*thread_handle,KILL_WAIT);
}



/**************************定时器函数*********************************/




void os_wrapper_start_timer(void **handle, void *func, int time_ms, bool repeat)
{
    if (handle == NULL) {
        return;
    }

    if (repeat) {
        *handle = (void *)sys_timer_add_to_task("tc_tvs_task", NULL, func, time_ms);
    } else {
        *handle = (void *)sys_timeout_add_to_task("tc_tvs_task", NULL, func, time_ms);
    }
}

void os_wrapper_stop_timer(void *handle)
{
    if (handle == NULL) {
        return;
    }
    sys_timer_del((int)handle);
}



