#include "os_test.h"
#include "app_config.h"

#ifdef USE_OS_API_TEST

//选择其中一种进行测试
#define OS_MUTEX_TEST  //互斥量测试
/* #define OS_QUEUE_TEST    //消息队列测试 */
/* #define OS_SEM_TEST    //信号量测试 */
//#define TASK_CREATE_STATIC_TEST // 采用静态任务栈创建任务测试
//#define THREAD_CAN_NOT_KILL_TEST //模拟线程无法杀死例子

int c_main(void)
{
#ifdef OS_MUTEX_TEST
    mutex_test();
#endif

#ifdef OS_QUEUE_TEST
    queue_test();
#endif

#ifdef OS_SEM_TEST
    sem_test();
#endif

#ifdef TASK_CREATE_STATIC_TEST
    static_task_test();
#endif

#ifdef THREAD_CAN_NOT_KILL_TEST
    thread_can_not_kill_test();
#endif

    return 0;
}

late_initcall(c_main);

#endif//USE_OS_API_TEST
