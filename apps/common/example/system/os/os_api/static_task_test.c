/***********************************静态任务创建步骤********************************************
 *1.创建使用  thread_fork 接口的 静态任务堆栈
 *  (1)在app_main.c中创建静态任务堆栈:
 *      #define STATIC_TASK_CREATE_STK_SIZE 256
 *      #define STATIC_TASK_CREATE_Q_SIZE 0
 *      #static u8 test_stk_q[sizeof(struct thread_parm) + STATIC_TASK_CREATE_STK_SIZE * 4 + \
 *      					 (STATIC_TASK_CREATE_Q_SIZE ? (sizeof(struct task_queue) + APP_CORE_Q_SIZE) : 0 )] ALIGNE(4);
 *
 *  (2)在app_main.c中将静态任务加入任务列表const struct task_info task_info_table[]：
 *      {任务名,       	任务优先级,      任务堆栈大小,   任务消息队列大小,  任务堆栈},
 *      {"task_create_static_test", STATIC_TASK_CREATE_PRIO, STATIC_TASK_CREATE_STK_SIZE, STATIC_TASK_CREATE_Q_SIZE, test_stk_q},
 *********************************************************************************************/
#include "os_test.h"
#define STATIC_TASK_CREATE_STK_SIZE 256
#define STATIC_TASK_CREATE_Q_SIZE 0
#define STATIC_TASK_CREATE_PRIO 12

static int task_pid;
static int kill_req = 0;

static void task_create_static_test(void *p)
{
    //获取当前线程
    printf("task_create_static_test in (%s) task\n", os_current_task());

    while (1) {
        printf("Use static task stack to create task test!");
        os_time_dly(50);

        if (kill_req) {
            //退出任务前必须释放占用的资源
            os_time_dly(1000);
            break;
        }
    }
}

void static_task_test(void)
{
    thread_fork("task_create_static_test", STATIC_TASK_CREATE_PRIO, STATIC_TASK_CREATE_STK_SIZE, STATIC_TASK_CREATE_Q_SIZE, &task_pid, task_create_static_test, NULL);

    os_time_dly(50);

    puts("Exiting the task_create_static_test thread!\n");

    //退出线程请求
    kill_req = 1;

    //阻塞等待线程退出
    thread_kill(&task_pid, KILL_WAIT);
    puts("Exit the task_create_static_test thread successfully!");
}
