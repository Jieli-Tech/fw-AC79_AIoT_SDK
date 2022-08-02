#include "os_test.h"

static OS_SEM psem;

static void thread_pend_sem(void *priv)
{
    //获取当前线程
    printf("thread_pend_sem in (%s) task\n", os_current_task());

    while (1) {
        puts(">>>>>>>[0]thread_pend_sem : wait sem!");

        os_sem_pend(&psem, 0); //获取信号量

        puts(">>>>>>>[2]thread_pend_sem : get sem!");
    }
}

static void thread_post_sem(void *priv)
{
    //获取当前线程
    printf("thread_post_sem in (%s) task\n", os_current_task());

    while (1) {
        os_time_dly(1000);  //延时模拟任务执行一段时间后释放信号量

        os_sem_post(&psem); //释放信号量

        puts(">>>>>>>[1]thread_post_sem : post sem!");
    }
}

void sem_test(void)
{
    os_sem_create(&psem, 0);

    if (thread_fork("thread_pend_sem", 10, 	256, 0, NULL, thread_pend_sem, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }

    if (thread_fork("thread_post_sem", 10, 256, 0, NULL, thread_post_sem, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }

}
