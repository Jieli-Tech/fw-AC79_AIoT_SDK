#include "os/pthread.h"
#include "system/init.h"
#include "app_config.h"

#ifdef USE_PTHREAD_API_TEST

//选择其中一种进行测试
/* #define POSIX_MUTEX_TEST */
/* #define POSIX_SEM_TEST */
#define POSIX_QUEUE_TEST

#ifdef POSIX_MUTEX_TEST
static pthread_mutex_t mutex;
static pthread_t p1, p2;

extern void os_time_dly(int tick);

static void *func1(void *arg)
{
    char *str = (char *)arg;

    //获取互斥锁
    pthread_mutex_lock(&mutex);

    while (*str != '\0') {
        printf("%c\n", *str);
        str++;
    }
    puts("\n");

    //释放互斥锁
    pthread_mutex_unlock(&mutex);

    return (void *)0;
}

static void *func2(void *arg)
{
    char *str = (char *)arg;

    //获取互斥锁
    pthread_mutex_lock(&mutex);

    while (*str != '\0') {
        printf("%c\n", *str);
        str++;
    }
    puts("\n");

    //释放互斥锁
    pthread_mutex_unlock(&mutex);

    return (void *)0;
}

static void posix_mutex_test(void)
{
    char *str1 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char *str2 = "abcdefghijklmnopqrstuvwxyz";
    int err;

    pthread_mutex_init(&mutex, NULL);

    if ((err = pthread_create(&p1, NULL, func1, str1)) != 0) {
        printf("[0]pthread_create fail\n");
    }

    if ((err = pthread_create(&p2, NULL, func2, str2)) != 0) {
        printf("[1]pthread_create fail\n");
    }

    /* pthread_join(p1, NULL); */
    /* pthread_join(p2, NULL); */
}
#endif

#ifdef POSIX_QUEUE_TEST
static mqd_t mqID;

//发送线程
static void *thread_send(void *args)
{
    char msg[] = "This is a message for test!";
    mqd_t *sendID = (mqd_t *)args;

    while (1) {
        if (mq_send(*sendID, msg, sizeof(msg), 0) < 0) {
            printf("send msg err!\n");
            //终止当前线程
            pthread_exit(NULL);
        }

        os_time_dly(100);
    }

    return (void *)0;
}

//接收线程
static void *thread_rec(void *args)
{
    char buf[128] ;
    mqd_t *recID = (mqd_t *)args;

    while (1) {
        if (mq_receive(*recID, buf, sizeof(buf), NULL) < 0) {
            printf("rec msg err!\r\n");

            //终止当前线程
            pthread_exit(NULL);
        }

        printf("Message received :(%s)\n", buf);
    }

    return (void *)0;
}

static void posix_queue_test(void)
{
    pthread_t send;
    pthread_t rec;
    struct mq_attr mqAttr;
    pthread_attr_t attr = {0};
    struct sched_param param;
    int ret;

    //创建消息队列
    mqID = mq_open("/mQueue_test", O_RDWR | O_CREAT, 0666, NULL); //队列消息名字需要符合POSIX IPC的名字规则
    if (mqID < 0) {
        printf("mq_open err!!!\r\n");
        return;
    }

    //获取消息队列属性
    if (mq_getattr(mqID, &mqAttr) < 0) {
        printf("mq_getattr err!!!\r\n");
        return;
    }

    printf("mq_flags:%d\r\n", mqAttr.mq_flags);
    printf("mq_maxmsg:%d\r\n", mqAttr.mq_maxmsg);
    printf("mq_msgsize:%d\r\n", mqAttr.mq_msgsize);
    printf("mq_curmsgs:%d\r\n", mqAttr.mq_curmsgs);

    //初始化线程属性变量
    pthread_attr_init(&attr);

    //设置优先级
    pthread_attr_getschedparam(&attr, &param);
    param.sched_priority = 10;

    //设置优先级
    pthread_attr_setschedparam(&attr, &param);

    //采用默认优先级时，设置为NULL
    ret = pthread_create(&send, &attr, thread_send, &mqID);
    //ret = pthread_create(&send, NULL,thread_send, &mqID);
    if (ret) {
        printf("pthread_create send error!!!\r\n");
    }

    //修改优先级
    /* pthread_setschedparam(send, SCHED_RR, &param); */

    pthread_create(&rec, NULL, thread_rec, &mqID);
    if (ret) {
        printf("pthread_create send error!!!\r\n");
    }

    //修改优先级
    /* pthread_setschedparam(rec, SCHED_RR, &param); */

    /* pthread_join(send, NULL); //阻塞等待线程结束 */

    /* pthread_join(rec, NULL); //阻塞等待线程结束 */

}
#endif

#ifdef POSIX_SEM_TEST
static sem_t psem;
static pthread_t waitSem;
static pthread_t postSem;

static void *thread_wait_sem(void *args)
{
    while (1) {
        puts(">>>>>>>[0]thread_wait_sem : wait sem!");
        sem_wait(&psem);
        puts(">>>>>>>[2]thread_wait_sem : get sem!");
    }
    return (void *)0;
}

static void *thread_post_sem(void *args)
{
    while (1) {
        os_time_dly(1000);
        sem_post(&psem);
        puts(">>>>>>>[1]thread_post_sem : post sem!");
    }

    return (void *)0;
}

static void posix_sem_test(void)
{
    int ret;

    ret = sem_init(&psem, 0, 0);
    if (ret) {
        puts("sem_init err!!!");
    }

    pthread_create(&waitSem, NULL, thread_wait_sem, NULL);

    pthread_create(&postSem, NULL, thread_post_sem, NULL);

    os_time_dly(50);

}
#endif

int c_main(void)
{
#ifdef POSIX_MUTEX_TEST
    posix_mutex_test();
#endif

#ifdef POSIX_SEM_TEST
    posix_sem_test();
#endif

#ifdef POSIX_QUEUE_TEST
    posix_queue_test();
#endif
    return 0;
}

late_initcall(c_main);

#endif//USE_PTHREAD_API_TEST
