#ifndef   _KAL_H_
#define   _KAL_H_

#include "../../../system/os/os_api.h"

typedef enum {
    KAL_ERR_NONE,
    KAL_ERR_NULL_PTR,
    KAL_ERR_OVF,
    KAL_ERR_RSRC,
    KAL_ERR_OS,
    KAL_ERR_INVALID_ARG,
    KAL_ERR_MEM_ALLOC,
    KAL_ERR_ISR,
    KAL_ERR_CREATE,
    KAL_ERR_TIMEOUT,
    KAL_ERR_ABORT,
    KAL_ERR_LOCK_OWNER,
    KAL_ERR_WOULD_BLOCK,
} KAL_ERR;


#define KAL_Init(...)

typedef u32 KAL_TICK;
typedef OS_MUTEX KAL_LOCK_HANDLE;
typedef OS_SEM KAL_SEM_HANDLE;
typedef OS_QUEUE KAL_Q_HANDLE;

#define KAL_FeatureQuery(...) DEF_YES

#define KAL_TIMEOUT_INFINITE 0
#define KAL_LockCreate(a,b)  *b=os_mutex_create(&(a))
#define KAL_LockAcquire(a,b,c,d) *d=os_mutex_pend(&(a),c)
#define KAL_LockRelease(a,b) *b=os_mutex_post(&(a))

// extern void msleep(int);
#define KAL_TickRate 100
#define KAL_Dly(a) msleep(a)
#define KAL_DlyTick(a,b) msleep(a)
#define KAL_TickGet(a) os_time_get()

#define KAL_SemCreate(a,b)  *b = os_sem_create(&(a),0)


#define KAL_SemDel(a,b)     *b = os_sem_del(&(a),OS_DEL_ALWAYS)
#define KAL_SemPend(a,b,c,d) *d=os_sem_pend(&(a),c)
#define KAL_SemPendAbort(a,b) *b=os_sem_pend(&(a),-1)
#define KAL_SemSet(a,b,c) *c=os_sem_set(&(a),b)
#define KAL_SemPost(a,b,c) *c=os_sem_post(&(a))



#define KAL_QCreate(a,b,c,d) *d=os_q_create(&(a),b)
#define KAL_QPend(a,b,c,d) *d=os_q_pend(&(a),0,&c)
#define KAL_QPost(a,b,c,d) *d=os_q_post(&(a),b)


#define KAL_TaskCreate(a,b,c,d,e,f,g)  *g = os_task_create(b,c,d,e,f,a)


#endif
