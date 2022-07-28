#include "os_test.h"

#define TASK1_TASK_PRIO 10     //任务优先级
#define TASK1_STACK_SIZE 512   //任务栈大小
#define TASK1_QUEUE_SIZE 0     //消息队列大小
static int task1_pid = 0;      //任务pid

#define TASK2_TASK_PRIO 10
#define TASK2_STACK_SIZE 512
#define TASK2_QUEUE_SIZE 0
static int task2_pid = 0;

static OS_MUTEX mutex;
static int value = 0;

char *str1 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
char *str2 = "abcdefghijklmnopqrstuvwxyz";

#if 0
/***********************************互斥量测试说明********************************************
 *说明：
 * 	  设置task1_task任务和task2_task任务为同等优先级，获取到互斥量的任务拥有该互斥量的使用权，
 *其他任务想获取该互斥量必须等获取到互斥量的任务完成其工作并释放该互斥量才行。
 *********************************************************************************************/
static void task1_task(void *priv)
{
    char *str = (char *)str1;

    os_mutex_pend(&mutex, 0);

    while (*str != '\0') {
        printf("%c\n", *str);
        str++;
    }
    puts("\n");

    os_mutex_post(&mutex);
}

static void task2_task(void *priv)
{
    char *str = (char *)str2;

    os_mutex_pend(&mutex, 0);

    while (*str != '\0') {
        printf("%c\n", *str);
        str++;
    }
    puts("\n");

    os_mutex_post(&mutex);
}
#else
/***********************************互斥量测试说明********************************************
 *说明：
 * 	  设置task1_task任务和task2_task任务为同等优先级，某个任务在执行期间对value的值进行修改
 *  只有获取到互斥量的任务能获得value的使用权，其他任务需要等待该互斥量释放并获取到互斥量才
 * 	能对value进行修改, 从而可以通过互斥量来处理共享资源，防止多线程重入。
 *********************************************************************************************/
static void task1_task(void *priv)
{
    //获取当前线程
    printf("task1_task in (%s) task\n", os_current_task());
    printf("task1_task pid : 0x%x\n", task1_pid);

    while (1) {
        os_mutex_pend(&mutex, 0);
        value = 100;
        printf("==========>Task1 gets the mutex!");

        for (int i = 0; i < 8; i++) {
            printf("task1 is running...\n");
            printf("value : (%d)\n", value);
            os_time_dly(20);
        }

        printf("==========>Task1 post the mutex!");
        os_mutex_post(&mutex);
        os_time_dly(100);
    }
}

void task2_task(void *priv)
{
    //获取当前线程
    printf("task2_task in (%s) task\n", os_current_task());
    printf("task2_task pid : 0x%x\n", task2_pid);

    while (1) {
        os_mutex_pend(&mutex, 0);
        value = 200;
        printf("==========>Task2 gets the mutex!");
        for (int i = 0; i < 8; i++) {
            printf("task2 is running...\n");
            printf("value : (%d)\n", value);
            os_time_dly(20);
        }

        printf("==========>Task2 post the mutex!");
        os_mutex_post(&mutex);
        os_time_dly(100);
    }
}
#endif

void mutex_test(void)
{
    os_mutex_create(&mutex);

    //创建任务1
    if (thread_fork("task1_task", TASK1_TASK_PRIO, TASK1_STACK_SIZE, TASK1_QUEUE_SIZE, &task1_pid, task1_task, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }

    //创建任务2
    if (thread_fork("task2_task", TASK1_TASK_PRIO, TASK1_STACK_SIZE, TASK1_QUEUE_SIZE, &task2_pid, task2_task, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}
