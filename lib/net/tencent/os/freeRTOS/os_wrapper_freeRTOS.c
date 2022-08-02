#include "os_wrapper.h"
#include "os/os_api.h"
#include "system/timer.h"


#define USE_MALLOC_SETTING	1	//1:信号量,互斥锁用动态分配创建   0:静态创建,SDK断网后需处理释放(未完善)

long os_wrapper_get_forever_time()
{
    return 0;
}



/***************************信号量*********************************/
#if USE_MALLOC_SETTING

void *os_wrapper_create_signal_mutex(int init_count)
{
    OS_SEM *sem = zalloc(sizeof(OS_SEM));
    if (sem) {
        os_sem_create(sem, init_count);
    }
    return (void *)sem;
}


void os_wrapper_delete_signal_mutex(void **mutex)  	//信号量释放函数
{

    if (!*mutex) {
        return;
    }
    if (os_sem_valid((OS_SEM *)*mutex)) {

        os_wrapper_post_signal((OS_SEM *)*mutex);	//删除前先post

        os_sem_del((OS_SEM *)*mutex, 1);
        free(*mutex);
        *mutex = NULL;
    }
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

void os_wrapper_delete_signal_mutex(void **mutex)  	//信号量释放函数
{
    if (!*mutex) {
        return;
    }
    if (os_sem_valid((OS_SEM *)*mutex)) {

        os_wrapper_post_signal((OS_SEM *)*mutex);	//删除前先post

        os_sem_del((OS_SEM *)*mutex, 1);
        *mutex = NULL;
    }
}

#endif

bool os_wrapper_wait_signal(void *mutex, long time_ms)
{
    if (!mutex) {
        return false;
    }
    if (!os_sem_valid((OS_SEM *)mutex)) {
        return false;
    }
    return (os_sem_pend((OS_SEM *)mutex, time_ms) == 0);
}

void os_wrapper_post_signal(void *mutex)
{
    if (!mutex) {
        return;
    }
    if (!os_sem_valid((OS_SEM *)mutex)) {
        return;
    }
    os_sem_post((OS_SEM *)mutex);
}



/***************************互斥锁*********************************/

#if USE_MALLOC_SETTING

void *os_wrapper_create_locker_mutex()
{
    OS_MUTEX *mutex = zalloc(sizeof(OS_MUTEX));
    if (mutex) {
        os_mutex_create(mutex);
    }
    return (void *)mutex;
}

void os_wrapper_del_locker_mutex(void **mutex)
{
    if (!*mutex) {
        return;
    }
    if (os_mutex_valid((OS_MUTEX *)*mutex)) {

        os_wrapper_unlock_mutex((OS_MUTEX *)*mutex);	//删除前先post

        os_mutex_del((OS_MUTEX *)*mutex, 0);
        free(*mutex);
        *mutex = NULL;
    }
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

void os_wrapper_del_locker_mutex(void **mutex)
{
    if (!*mutex) {
        return;
    }
    if (os_mutex_valid((OS_MUTEX *)*mutex)) {

        os_wrapper_unlock_mutex((OS_MUTEX *)*mutex);	//删除前先post

        os_mutex_del((OS_MUTEX *)*mutex, 0);
        *mutex = NULL;
    }
}
#endif

bool os_wrapper_lock_mutex(void *mutex, long time_ms)
{
    if (!mutex) {
        return false;
    }
    if (!os_mutex_valid((OS_MUTEX *)mutex)) {
        return false;
    }
    return (os_mutex_pend((OS_MUTEX *)mutex, time_ms) == 0);
}

void os_wrapper_unlock_mutex(void *mutex)
{
    if (!mutex) {
        return;
    }
    if (!os_mutex_valid((OS_MUTEX *)mutex)) {
        return;
    }
    os_mutex_post((OS_MUTEX *)mutex);
}



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

    int *thread_handle = (int *)malloc(sizeof(int));
    memset(thread_handle, 0, sizeof(int));
    if (0 != thread_fork(name, prior + 20, stack_depth, 0, thread_handle, thread_func, param)) {
        free(thread_handle);
        return NULL;
    }
    return (void *)thread_handle;

}

void os_wrapper_thread_delete(void **thread_handle)
{
    if (*thread_handle) {
        thread_kill(*thread_handle, KILL_WAIT);
        free(*thread_handle);
        *thread_handle = NULL;
    }
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



