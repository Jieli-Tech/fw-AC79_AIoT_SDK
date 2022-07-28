#include "system/timer.h"
#include "os/os_api.h"
#include "vt_bk.h"

#define WANXUE_0S_INFO 0
#define WANXUE_0S_DBUG 0
#define WANXUE_0S_WARN 0
#define WANXUE_0S_EROR 0

#if WANXUE_0S_INFO
#define wanxue_os_info(fmt, ...) printf("\e[0;32m[VT_0S_INFO] [%s %d] : "fmt"\e[0m\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define wanxue_os_info(fmt, ...)
#endif

#if WANXUE_0S_DBUG
#define wanxue_os_debug(fmt, ...) printf("\e[0m[VT_0S_DBUG] [%s %d] : "fmt"\e[0m\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define wanxue_os_debug(fmt, ...)
#endif

#if WANXUE_0S_WARN
#define wanxue_os_warn(fmt, ...) printf("\e[0;33m[VT_0S_WARN] [%s %d] : "fmt"\e[0m\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define wanxue_os_warn(fmt, ...)
#endif

#if WANXUE_0S_EROR
#define wanxue_os_error(fmt, ...) printf("\e[0;31m[VT_0S_EROR] [%s %d] : "fmt"\e[0m\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define wanxue_os_error(fmt, ...)
#endif


/////////////////// mutex /////////////////////
static int wanxue_mutex_getlock(void *mutex)
{
    /* return pthread_mutex_lock((int *)&mutex); */
    return os_mutex_pend((OS_MUTEX *)mutex, 0);
}

static int wanxue_mutex_putlock(void *mutex)
{
    /* return pthread_mutex_unlock((int *)&mutex); */
    return os_mutex_post((OS_MUTEX *)mutex);
}

static void *wanxue_create_mutex(uint8_t process_shared)
{
    /* int mutex = 0; */
    /* pthread_mutex_init(&mutex, NULL); */
    /*************************************************/
    OS_MUTEX *mutex;
    mutex = (OS_MUTEX *)malloc(sizeof(OS_MUTEX));
    if (mutex == NULL) {
        return NULL;
    }

    os_mutex_create(mutex);
    /*************************************************/

    return (void *)mutex;
}

static int wanxue_delete_mutex(void *mutex)
{
    /* return pthread_mutex_destroy((int *)&mutex); */
    /*************************************************/
    os_mutex_del((OS_MUTEX *)mutex, OS_DEL_ALWAYS);
    if (mutex) {
        free((void *)mutex);
        mutex = NULL;
    }
    /*************************************************/

    return 0;
}


////////////////// task延时函数 //////////////////
static void wanxue_task_delay(int time_ms)
{
    extern void msleep(unsigned int ms);
    msleep(time_ms);
}


////////////////// message queue ////////////////////
static void *wanuxue_create_message_queue(int queue_size, int item_length)//ok
{
    OS_QUEUE *s_wanxue_msg_queue = NULL;

    if (NULL == (s_wanxue_msg_queue = (OS_QUEUE *)zalloc(sizeof(OS_QUEUE)))) {
        return NULL;
    }

    ASSERT(item_length == sizeof(void *));

    if (0 != os_q_create(s_wanxue_msg_queue, queue_size)) {
        free(s_wanxue_msg_queue);
        return NULL;
    }

    return (void *)s_wanxue_msg_queue;
}

static int wanxue_send_message(void *queue, const void *data, int wait_time_ms, uint8_t send_front)//ok
{
    int ret = -1;
    OS_QUEUE *s_wanxue_msg_queue = (OS_QUEUE *)queue;

    while (1) {
        if (send_front == 0) {
            ret = os_q_post_to_back(s_wanxue_msg_queue, (void *)data, 0);
        } else if (send_front == 1) {
            ret = os_q_post_to_front(s_wanxue_msg_queue, (void *)data, 0);
        }

        if (OS_Q_FULL != ret) {
            break;
        }
        msleep(wait_time_ms);
    }

    return ret;
}

static int wanxue_recv_message(void *queue, void *data, int wait_time_ms, uint8_t peek_msg)//ok
{
    int ret = -1;
    OS_QUEUE *s_wanxue_msg_queue = (OS_QUEUE *)queue;

    if (peek_msg == 0) {
        ret = os_q_recv(s_wanxue_msg_queue, data, wait_time_ms);
    } else if (peek_msg == 1) {
        ret = os_q_peek(s_wanxue_msg_queue, data, wait_time_ms);
    }

    return ret;
}

static int wanxue_get_message_size(void *queue)//ok
{
    return os_q_query((OS_QUEUE *)queue);  //返回消息队列里的消息数目、消息队列为空时返回0
}

static int wanxue_destory_msgqueue(void *queue)//ok
{
    int ret = os_q_del((OS_QUEUE *)queue, 0);
    free(queue);
    return ret;
}


////////////////// timer ///////////////////////
typedef struct {
    unsigned short id;
    unsigned char oneshort;
    vt_timer_cb cb;
    int msec;
} OS_Timer_t;

static void *wanuxe_create_timer(vt_timer_cb cb, int period_ms, uint8_t one_short)
{
    OS_Timer_t *s_wanuxe_timer = NULL;

    if (NULL == (s_wanuxe_timer = (OS_Timer_t *)malloc(sizeof(OS_Timer_t)))) {
        return NULL;
    }

    memset(s_wanuxe_timer, 0, sizeof(OS_Timer_t));

    s_wanuxe_timer->cb = cb;

    if (period_ms > 0 && period_ms < 10) {
        period_ms = 10;
    }

    s_wanuxe_timer->msec = period_ms;
    s_wanuxe_timer->oneshort = one_short;

    return (void *)s_wanuxe_timer;
}

static int wanxue_start_timer(void *timer)
{
    OS_Timer_t *s_wanuxe_timer = (OS_Timer_t *)timer;
    if (s_wanuxe_timer->oneshort) {
        s_wanuxe_timer->id = sys_timeout_add_to_task("sys_timer", timer, s_wanuxe_timer->cb, s_wanuxe_timer->msec);
    } else {
        s_wanuxe_timer->id = sys_timer_add_to_task("sys_timer", timer, s_wanuxe_timer->cb, s_wanuxe_timer->msec);
    }

    return 0;
}

static int wanxue_stop_timer(void *timer)
{
    OS_Timer_t *s_wanuxe_timer = (OS_Timer_t *)timer;

    if (s_wanuxe_timer->id) {
        sys_timeout_del(s_wanuxe_timer->id);
        s_wanuxe_timer->id = 0;
    }

    return 0;
}

static int wanxue_delete_timer(void *timer)
{
    OS_Timer_t *s_wanuxe_timer = (OS_Timer_t *)timer;
    wanxue_stop_timer(timer);
    free(s_wanuxe_timer);

    return 0;
}


////////////////////// semaphone event ///////////////////////
static void *wanxue_create_semaphone(uint8_t process_shared, int max_count, int init_count)
{
    int sem = 0;
    sema_init(&sem, init_count);
    return (void *)sem;
}

static int wanxe_post_semaphone(void *semaphone)
{
    return sema_post((int *)&semaphone);
}

static int wanxue_wait_semaphone(void *semaphone, int time_ms)
{
    if (time_ms == -1) {
        time_ms = 0;
    }
    return sem_pend((int *)&semaphone, (time_ms + 9) / 10);
}

static int wanxue_destory_semaphone(void *semaphone)
{
    return sem_del((int *)&semaphone);
}


//////////////////////cond event ///////////////////////
static void *wanxue_create_event(uint8_t process_shared)
{
    OS_SEM *psem = (OS_SEM *)calloc(1, sizeof(OS_SEM));
    if (psem == NULL) {
        return NULL;
    }

    int os_sem_binary_create(OS_SEM * p_sem);
    os_sem_binary_create(psem);

exit:
    return (void *)psem;
}

static int wanxue_post_event(void *event)
{
    return sema_post((int *)&event);
}

static int wanxue_wait_event(void *event, void *mutex, int time_ms)
{
    if (time_ms == -1) {
        time_ms = 0;
    }

    return sem_pend((int *)&event, (time_ms + 9) / 10);
}

static int wanxue_destory_event(void *event)
{
    return sem_del((int *)&event);
}


/////////////////memory///////////////////
static void *wanxue_malloc(unsigned long size)
{
    return malloc(size);
}

static void wanxue_free(void *ptr)
{
    free(ptr);
}

static void *wanxue_calloc(unsigned long nmemb, unsigned long size)
{
    return calloc(nmemb, size);
}

static void *wanxue_realloc(void *ptr, unsigned long size)
{
    return realloc(ptr, size);
}

static unsigned long getTickCount(void)
{
    extern u32 timer_get_ms(void);
    return timer_get_ms();
}

const vt_os_ops_t linux_ops_t = {
    .create_event = wanxue_create_event,
    .post_event = wanxue_post_event,
    .wait_event = wanxue_wait_event,
    .destory_event = wanxue_destory_event,
    .create_semaphone = wanxue_create_semaphone,
    .post_semaphone = wanxe_post_semaphone,
    .wait_semaphone = wanxue_wait_semaphone,
    .destory_semaphone = wanxue_destory_semaphone,
    .create_mutex = wanxue_create_mutex,
    .mutex_lock = wanxue_mutex_getlock,
    .mutex_unlock = wanxue_mutex_putlock,
    .destory_mutex = wanxue_delete_mutex,
    .create_timer = wanuxe_create_timer,
    .start_timer = wanxue_start_timer,
    .stop_timer = wanxue_stop_timer,
    .delete_timer = wanxue_delete_timer,
    .create_message_queue = wanuxue_create_message_queue,
    .send_message = wanxue_send_message,
    .recv_message = wanxue_recv_message,
    .get_message_size = wanxue_get_message_size,
    .destory_msgqueue = wanxue_destory_msgqueue,
    .task_delay  = wanxue_task_delay,
    .vt_plat_malloc = wanxue_malloc,
    .vt_plat_free = wanxue_free,
    .vt_plat_calloc = wanxue_calloc,
    .vt_plat_realloc = wanxue_realloc,
    .vt_alg_malloc = wanxue_malloc,
    .vt_alg_free = wanxue_free,
    .vt_alg_realloc = wanxue_realloc,
    .get_timestamp = getTickCount,
};

