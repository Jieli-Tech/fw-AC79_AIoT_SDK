#include "os_test.h"

static void kill_thread_test(void *p)
{
    //获取当前线程
    printf("kill_thread_test in (%s) task\n", os_current_task());

    while (1) {
        printf("kill_thread_test is running!\n");
        os_time_dly(50);
    }
}

void thread_can_not_kill_test(void)
{
    int kpid;

    if (thread_fork("kill_thread_test", 10, 256, 0, &kpid, kill_thread_test, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }

    //阻塞等待线程退出
    thread_kill(&kpid, KILL_WAIT);
}
