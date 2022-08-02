/***********************************消息队列测试说明********************************************
 *说明：
 * 	  设置queue_task任务和task3_task任务为同等优先级，queue_task任务阻塞等到消息，当收到task3_task
 *	任务的消息后，向task3_task发送接收到消息通知。
 *********************************************************************************************/

#include "os_test.h"

#define QUEUE_TASK_PRIO 10        //任务优先级大小
#define QUEUE_STACK_SIZE 512      //任务堆栈大小
#define QUEUE_QUEUE_SIZE 256      //消息队列大小，注意任务间需要通过消息队列通讯时必须分配足够的消息队列大小，否则接收不到消息。
static int queue_pid;

#define TASK3_TASK_PRIO 10
#define TASK3_STACK_SIZE 512
#define TASK3_QUEUE_SIZE 256
static int task3_pid;

//消息类型
enum {
    MESSAGE_1     = 0x01,
    MESSAGE_2     = 0x02,
    MESSAGE_3     = 0x03,
};

enum {
    GET_MESSAGE_1     = 0x01,
    GET_MESSAGE_2     = 0x02,
    GET_MESSAGE_3     = 0x03,
};

static char *message1 = "Message1 received!";
static char *message2 = "Message2 received!";
static char *message3 = "Message3 received!";

//等待消息队列任务
static void queue_task(void *priv)
{
    int argc = 2;  //消息个数
    int argv[4];
    int msg[32];   //接收消息队列buf
    int err;
    char *ptr = NULL;

    //获取当前线程
    printf("queue_task in (%s) task\n", os_current_task());

    while (1) {
        //阻塞等待消息
        err = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (err != OS_TASKQ || msg[0] != Q_USER) {
            continue;
        }

        switch (msg[1]) {
        case MESSAGE_1:
            printf("get MESSAGE_1, data : %d\n", msg[2]);
            ptr = message1;
            break;

        case MESSAGE_2:
            printf("get MESSAGE_2, data : %d\n", msg[2]);
            ptr = message2;
            break;

        case MESSAGE_3:
            printf("get MESSAGE_3, data : %d\n", msg[2]);
            ptr = message3;
            break;

        default:
            break;
        }

        argv[0] = msg[1];
        argv[1] = (int)ptr;

_retry1:
        //向任务task3_task发送接收到消息通知
        err = os_taskq_post_type("task3_task", Q_USER, argc, argv);
        //err = os_taskq_post("task3_task", argc, argv[0], argv[1]);//采用该接口发送信息时可变参数个数不能超过8个
        if (err == OS_Q_FULL) {
            os_time_dly(10);
            goto _retry1;
        }
    }
}

static void task3_task(void *priv)
{
    int err;
    int argc = 2;  //消息个数
    int argv[4];
    int msg[32];   //接收消息队列buf
    char *ptr = NULL;

    //获取当前线程
    printf("task3_task in (%s) task\n", os_current_task());

    //发送的消息内容
    argv[0] = MESSAGE_1;
    argv[1] = 12;

    while (1) {
        //向任务queue_task发送消息
        err = os_taskq_post_type("queue_task", Q_USER, argc, argv);
        //err = os_taskq_post("queue_task", argc, argv[0], argv[1]);//采用该接口发送信息时可变参数个数不能超过8个
        if (err == OS_Q_FULL) {
            os_time_dly(10);
            continue;
        }

        os_time_dly(20);

_retry2:
        //等待queue_task任务回复
        err = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (err != OS_TASKQ || msg[0] != Q_USER) {
            goto _retry2;
        }

        switch (msg[1]) {
        case GET_MESSAGE_1:
            ptr = (char *)msg[2];
            printf("%s\n", ptr);

            argv[0] = MESSAGE_2;
            argv[1] = 10;
            break;

        case GET_MESSAGE_2:
            ptr = (char *)msg[2];
            printf("%s\n", ptr);

            argv[0] = MESSAGE_3;
            argv[1] = 11;
            break;

        case GET_MESSAGE_3:
            ptr = (char *)msg[2];
            printf("%s\n", ptr);

            argv[0] = MESSAGE_1;
            argv[1] = 12;
            break;

        default:
            break;
        }

        os_time_dly(100);
    }
}

//队列测试
void queue_test(void)
{
    if (thread_fork("queue_task", QUEUE_TASK_PRIO, 	QUEUE_STACK_SIZE, QUEUE_QUEUE_SIZE, &queue_pid, queue_task, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }

    //创建任务2
    if (thread_fork("task3_task", TASK3_TASK_PRIO, TASK3_STACK_SIZE, TASK3_QUEUE_SIZE, &task3_pid, task3_task, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}
