#define SYS_ARCH_GLOBALS

/* lwIP includes. */
#include "lwip/sys.h"
#include "lwip/debug.h"
#include "lwip/def.h"
#include "sys_arch.h"
#include "os/os_api.h"
#include "os/os_api.h"
#include <string.h>
#define LWIP_DEBUG_SEM_CNT 0
#define LWIP_DEBUG_MBOX_CNT 0
#define LWIP_DEBUG_MBOX_POST_CNT 0

#if LWIP_DEBUG_SEM_CNT
static int debug_sem_cnt = 0;
#endif
#if LWIP_DEBUG_MBOX_POST_CNT
static int debug_mbox_post_cnt = 0;
#endif
#if LWIP_DEBUG_MBOX_CNT
static int debug_mboxs_cnt = 0;
#endif

extern volatile unsigned long jiffies;
/*----------------------------------------------------------------------------*/
/*                      VARIABLES                                             */
/*----------------------------------------------------------------------------*/
const void *const pvNullPointer = (mem_ptr_t *)0xffffffff;

#define  LWIP_TASK_PRIO                                     16
#define  LWIP_STK_SIZE	                                   	800
//static OS_STK LWIP_TASK_STK[LWIP_STK_SIZE] __attribute__ ((aligned (32)));

/*-----------------------------------------------------------------------------------*/


#ifdef LIWP_USE_BT /*蓝牙SDK不提供os_q api*/
int os_q_create_static(OS_QUEUE *pevent, void *storage, QS size)
{
    xQueueCreateStatic(size, sizeof(void *), storage, pevent);
    return 0 ;
}

int os_q_del(OS_QUEUE *pevent, u8 opt)
{
    vQueueDelete(pevent);
    return 0 ;
}
int os_q_create(OS_QUEUE *pevent, QS size)
{
    u8 *pucQueueStorage =  zalloc(size * sizeof(void *));
    if (pucQueueStorage == NULL) {
        return -1;
    }
    xQueueCreateStatic(size, sizeof(void *), pucQueueStorage, pevent);
    return 0 ;
}

int os_q_del(OS_QUEUE *pevent, u8 opt)
{
    vQueueDelete(pevent);
    free(pevent->pvDummy1[0]);
    pevent->pvDummy1[0] = NULL;
    return 0 ;
}

int os_q_flush(OS_QUEUE *pevent)
{
    int err ;
    err = xQueueReset(pevent);
    return err == pdTRUE ? 0 : -EFAULT;
}

int os_q_pend(OS_QUEUE *pevent, int timeout, void *msg)
{
    int err;
    if (cpu_in_irq()) {
        return OS_ERR_PEND_ISR;
    }
    if (timeout == 0) {
        timeout = portMAX_DELAY;
    }
    err = xQueueReceive(pevent, msg, timeout);
    return err == pdTRUE ? 0 : OS_TIMEOUT;
}

int os_q_post(OS_QUEUE *pevent, void *msg)
{
    int err;
    int pmsg = (int)msg;
    if (cpu_in_irq()) {
        err = xQueueSendToBackFromISR(pevent, &pmsg, 0);
        /* taskYIELD(); */
        vPortYield();
    } else {
        err = xQueueSendToBack(pevent, &pmsg, 0);
    }
    return err == pdTRUE ? 0 : OS_Q_FULL;
}

int  os_q_query(OS_QUEUE *pevent)
{
    int err;
    if (cpu_in_irq()) {
        err = uxQueueMessagesWaitingFromISR(pevent);
    } else {
        err = uxQueueMessagesWaiting(pevent);
    }
    return err ;
}

int os_q_valid(OS_QUEUE *pevent)
{
    /* int type = ucQueueGetQueueType(pevent); */
    int type  = ((OS_QUEUE *) pevent)->ucDummy9;
    return type == queueQUEUE_TYPE_BASE;
}
#endif








err_t sys_mutex_new(sys_mutex_t *mutex)
{
    s32_t	ucErr;
    ucErr = os_mutex_create(mutex);
    if (ucErr != OS_ERR_NONE) {
        LWIP_ASSERT("os_mutex_create ", ucErr == OS_ERR_NONE);
    }
    return 0;
}

void sys_mutex_lock(sys_mutex_t *mutex)
{
    s32_t	ucErr;
    ucErr = os_mutex_pend(mutex, 0);
    if (ucErr != OS_ERR_NONE) {
        LWIP_ASSERT("os_mutex_pend ", ucErr == OS_ERR_NONE);
    }
}

void sys_mutex_unlock(sys_mutex_t *mutex)
{
    s32_t	ucErr;
    ucErr = os_mutex_post(mutex);
    if (ucErr != OS_ERR_NONE) {
        LWIP_ASSERT("os_mutex_post ", ucErr == OS_ERR_NONE);
    }
}

void sys_mutex_free(sys_mutex_t *mutex)
{
    s32_t	ucErr;
    ucErr = os_mutex_del(mutex, 1);
    if (ucErr != OS_ERR_NONE) {
        LWIP_ASSERT("os_mutex_del ", ucErr == OS_ERR_NONE);
    }
}

/*
  Creates and returns a new semaphore. The "count" argument specifies
  the initial state of the semaphore. TBD finish and test
*/
err_t
sys_sem_new(sys_sem_t *sem, u8_t count)
{
    s32_t	ucErr;

#if LWIP_DEBUG_SEM_CNT
    printf("+debug_sem_cnt %d\r\n", ++debug_sem_cnt);
#endif

    ucErr = os_sem_create(sem, count);
    if (ucErr != OS_ERR_NONE) {
        LWIP_ASSERT("os_sem_create ", ucErr == OS_ERR_NONE);
        return -1;
    }
    return 0;
}

/*-----------------------------------------------------------------------------------*/
/*
  Signals a semaphore
*/
void
sys_sem_signal(sys_sem_t *sem)
{
    s32_t	ucErr;
    ucErr = os_sem_post(sem);
    LWIP_ASSERT("OSSemPost ", ucErr == OS_ERR_NONE);
}

/*-----------------------------------------------------------------------------------*/
/*
  Deallocates a semaphore
*/
void
sys_sem_free(sys_sem_t *sem)
{
    s32_t     ucErr;
#if LWIP_DEBUG_SEM_CNT
    printf("-debug_sem_cnt %d\r\n", --debug_sem_cnt);
#endif

    ucErr = os_sem_del(sem, OS_DEL_ALWAYS);
    LWIP_ASSERT("OSSemDel ", ucErr == OS_ERR_NONE);
}

int sys_sem_valid(sys_sem_t *sem)
{
    if (os_sem_valid(sem)) {
        return 1;
    } else {
        return 0;
    }
}

/** Set a semaphore invalid so that sys_sem_valid returns 0 */
void sys_sem_set_invalid(sys_sem_t *sem)
{
    if (sys_sem_valid(sem)) {
        sys_sem_free(sem);
    }
}

/*-----------------------------------------------------------------------------------*/
/*
  Blocks the thread while waiting for the semaphore to be
  signaled. If the "timeout" argument is non-zero, the thread should
  only be blocked for the specified time (measured in
  milliseconds).

  If the timeout argument is non-zero, the return value is the number of
  milliseconds spent waiting for the semaphore to be signaled. If the
  semaphore wasn't signaled within the specified time, the return value is
  SYS_ARCH_TIMEOUT. If the thread didn't have to wait for the semaphore
  (i.e., it was already signaled), the function may return zero.

  Notice that lwIP implements a function with a similar name,
  sys_sem_wait(), that uses the sys_arch_sem_wait() function.
*/
u32_t
sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
    s32_t ucErr;
    u32_t ucos_timeout, timeout_new;

    // timeout 单位以ms计转换为ucos_timeout 单位以TICK计
    if (timeout != 0) {
        ucos_timeout = (timeout * OS_TICKS_PER_SEC) / 1000; // convert to timetick
        if (ucos_timeout < 1) {
            ucos_timeout = 1;
        } else if (ucos_timeout > 65536) { // uC/OS only support u16_t pend
            ucos_timeout = 65535;    // 最多等待TICK数 这是uC/OS所能 处理的最大延时
        }
    } else {
        ucos_timeout = 0;
    }

    timeout = OSGetTime(); // 记录起始时间

    ucErr = os_sem_pend(sem, ucos_timeout);

    if (ucErr == OS_TIMEOUT) {
//        printf("sys_arch_sem_wait TO = 0x%x\n",ucErr);
        timeout = SYS_ARCH_TIMEOUT;	// only when timeout!
    } else {
        //LWIP_ASSERT( "OSSemPend ", ucErr == OS_NO_ERR );
        //for pbuf_free, may be called from an ISR

        timeout_new = OSGetTime(); // 记录终止时间
        if (timeout_new >= timeout) {
            timeout_new = timeout_new - timeout;
        } else {
            timeout_new = 0xffffffff - timeout + timeout_new;
        }

        timeout = (timeout_new * 1000 / OS_TICKS_PER_SEC + 1); //convert to milisecond 为什么加1？
    }

    return timeout;
}

/*-----------------------------------------------------------------------------------*/
/*
  Creates an empty mailbox.
*/

err_t
sys_mbox_new(sys_mbox_t *mbox, int size)
{
    // prarmeter "size" can be ignored in your implementation.
    s32_t       ucErr;

#if LWIP_DEBUG_MBOX_CNT
    printf("+debug_mboxs_cnt %d\r\n", ++debug_mboxs_cnt);
#endif
    ucErr = os_q_create(&mbox->pQ, size);
    /*ucErr = os_q_create_static(&mbox->pQ, mbox->pvQEntries, sizeof(mbox->pvQEntries)/sizeof(mbox->pvQEntries[0]));*/

    LWIP_ASSERT("OSQCreate ", ucErr == OS_ERR_NONE);

    if (ucErr == OS_ERR_NONE) {
        return 0;
    }
    return -1;
}

void
sys_mbox_free(sys_mbox_t *mbox)
{
    s32_t     ucErr;
    sys_mbox_t m_box = *mbox;
#if LWIP_DEBUG_MBOX_CNT
    printf("-debug_mboxs_cnt %d\r\n", --debug_mboxs_cnt);
#endif

    LWIP_ASSERT("sys_mbox_free ", mbox != (void *)0);

//    ucErr = os_q_flush(&mbox->pQ);
//    LWIP_ASSERT( "os_q_flush ", ucErr == OS_ERR_NONE );

    /* ucErr = os_q_del_static(&mbox->pQ, OS_DEL_ALWAYS); */
    ucErr = os_q_del(&mbox->pQ, OS_DEL_ALWAYS);
    LWIP_ASSERT("OSQDel ", ucErr == OS_ERR_NONE);
}

/*-----------------------------------------------------------------------------------*/
/*
  Posts the "msg" to the mailbox.
*/
void
sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
    s32_t     ucErr;
    u8_t i = 0;

#if LWIP_DEBUG_MBOX_POST_CNT
    printf("+debug_mbox_post_cnt %d\r\n", ++debug_mbox_post_cnt);
#endif
    if (msg == NULL) {
        msg = (void *)&pvNullPointer;
    }
    /* try 10 times */
    while (i < 10) {
        ucErr = os_q_post(&mbox->pQ, msg);
        if (ucErr == OS_ERR_NONE) {
            break;
        } else {
#ifdef CONFIG_FREE_RTOS_ENABLE
            /*printf("sys_mbox_trypost ucErr pointer = 0x%x, curr_item_length = %lu, item_length = %lu, each_item_size = %lu\n", (unsigned int)&mbox->pQ, mbox->pQ.uxDummy4[0], mbox->pQ.uxDummy4[1], mbox->pQ.uxDummy4[2]);*/
#else
            /*printf("sys_mbox_post ucErr = 0x%x,size= %d\n", (unsigned int)&mbox->pQ, ((OS_Q *)mbox->pQ.OSEventPtr)->c);*/
#endif
        }
        i++;
        os_time_dly(10);
    }
    LWIP_ASSERT("sys_mbox_post error!\n", i != 10);
}

/*-----------------------------------------------------------------------------------*/
/*
  Try to post the "msg" to the mailbox.
*/
err_t
sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
    s32_t     ucErr;
#if LWIP_DEBUG_MBOX_POST_CNT
    printf("+debug_mbox_post_cnt %d\r\n", ++debug_mbox_post_cnt);
#endif

    if (msg == NULL) {
        msg = (void *)&pvNullPointer;
    }
    ucErr = os_q_post(&mbox->pQ, msg);
    if (ucErr != OS_ERR_NONE) {
#ifdef CONFIG_FREE_RTOS_ENABLE
        /*printf("sys_mbox_trypost ucErr pointer = 0x%x, curr_item_length = %lu, item_length = %lu, each_item_size = %lu\n", (unsigned int)&mbox->pQ, mbox->pQ.uxDummy4[0], mbox->pQ.uxDummy4[1], mbox->pQ.uxDummy4[2]);*/
#else
        /*printf("sys_mbox_post ucErr = 0x%x,size= %d\n", (unsigned int)&mbox->pQ, ((OS_Q *)mbox->pQ.OSEventPtr)->c);*/
#endif
        return ERR_MEM;
    }
    return ERR_OK;
}

/*-----------------------------------------------------------------------------------*/
/*
  Blocks the thread until a message arrives in the mailbox, but does
  not block the thread longer than "timeout" milliseconds (similar to
  the sys_arch_sem_wait() function). The "msg" argument is a result
  parameter that is set by the function (i.e., by doing "*msg =
  ptr"). The "msg" parameter maybe NULL to indicate that the message
  should be dropped.

  The return values are the same as for the sys_arch_sem_wait() function:
  Number of milliseconds spent waiting or SYS_ARCH_TIMEOUT if there was a
  timeout.

  Note that a function with a similar name, sys_mbox_fetch(), is
  implemented by lwIP.
*/
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
    s32_t	ucErr;
    u32_t	ucos_timeout, timeout_new;
    void	*temp;

    if (timeout != 0) {
        ucos_timeout = (timeout * OS_TICKS_PER_SEC) / 1000; //convert to timetick

        if (ucos_timeout < 1) {
            ucos_timeout = 1;
        } else if (ucos_timeout > 65535) {	//ucOS only support u16_t timeout
            ucos_timeout = 65535;
        }
    } else {
        ucos_timeout = 0;
    }

    timeout = OSGetTime();
    ucErr = os_q_pend(&mbox->pQ, ucos_timeout, &temp);
    if (msg != NULL) {
        if (temp == (void *)&pvNullPointer) {
            *msg = NULL;
        } else {
            *msg = temp;
        }
    }

    if (ucErr == OS_TIMEOUT) {
        timeout = SYS_ARCH_TIMEOUT;
        //puts("OSQPend timeout\n");
    } else {
#if LWIP_DEBUG_MBOX_POST_CNT
        printf("-debug_mbox_post_cnt %d\r\n", --debug_mbox_post_cnt);
#endif
        //if(ucErr != OS_NO_ERR)  printf("sys_arch_mbox_fetch ucErr = %d\n",ucErr);
        LWIP_ASSERT("OSQPend ", ucErr == OS_NO_ERR);
        timeout_new = OSGetTime();
        if (timeout_new > timeout) {
            timeout_new = timeout_new - timeout;
        } else {
            timeout_new = 0xffffffff - timeout + timeout_new;
        }

        timeout = timeout_new * 1000 / OS_TICKS_PER_SEC + 1; //convert to milisecond
    }

    return timeout;

}

/**
  * Check if an mbox is valid/allocated:
  * @param sys_mbox_t *mbox pointer mail box
  * @return 1 for valid, 0 for invalid
  */
int sys_mbox_valid(sys_mbox_t *mbox)
{
    if (os_q_valid(&mbox->pQ)) {
        return 1;
    } else {
        return 0;
    }
}
/**
  * Set an mbox invalid so that sys_mbox_valid returns 0
  */
void sys_mbox_set_invalid(sys_mbox_t *mbox)
{

    if (sys_mbox_valid(mbox)) {
        sys_mbox_free(mbox);
    }
}

/*-----------------------------------------------------------------------------------*/
/*
  Initialize sys arch
*/

void
sys_init(void)
{
}

/*-----------------------------------------------------------------------------------*/
/*
  Starts a new thread with priority "prio" that will begin its execution in the
  function "thread()". The "arg" argument will be passed as an argument to the
  thread() function. The id of the new thread is returned. Both the id and
  the priority are system dependent.
*/

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
    /* static u8 lwip_tcb_stk_q[sizeof(StaticTask_t) + LWIP_STK_SIZE * 4] ALIGNE(4); */
    /* return 	os_task_create_static(thread, arg, LWIP_TASK_PRIO, LWIP_STK_SIZE, 0, (s8 *)name, lwip_tcb_stk_q); */
    return thread_fork(name, LWIP_TASK_PRIO, LWIP_STK_SIZE, 0, NULL, thread, arg);
}

#include "system/spinlock.h"
static spinlock_t spinlock;

__attribute__((always_inline))
void lwip_spin_init(void)
{
    spin_lock_init(&spinlock);

}

__attribute__((always_inline))
void lwip_spin_lock(void)
{
    spin_lock(&spinlock);

}

__attribute__((always_inline))
void lwip_spin_unlock(void)
{
    spin_unlock(&spinlock);
}

#if 0
static OS_MUTEX    lwip_sys_arch_protect_mutex;
__attribute__((always_inline))
int enter_lwip_sys_arch_protect(void)
{
    return os_mutex_pend(&lwip_sys_arch_protect_mutex, 0);
}
__attribute__((always_inline))
int exit_lwip_sys_arch_protect(void)
{
    return os_mutex_post(&lwip_sys_arch_protect_mutex);
}

__attribute__((always_inline))
int init_lwip_sys_arch_protect(void)
{
    int err;
    err = os_mutex_create(&lwip_sys_arch_protect_mutex);
    if (err != OS_ERR_NONE) {

        printf("create init_lwip_sys_arch_protect mutex err =%d \r\n", err) ;
        return -1 ;
    }
    return 0 ;
}
#endif

