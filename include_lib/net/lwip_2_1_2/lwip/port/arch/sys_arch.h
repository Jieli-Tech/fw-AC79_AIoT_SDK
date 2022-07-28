#ifndef __ARCH_SYS_ARCH_H__
#define __ARCH_SYS_ARCH_H__

#include "cc.h" //包含cc.h头文件
#include "arch.h"
#include "os/os_api.h"
#include "os/os_cpu.h"
#ifdef SYS_ARCH_GLOBALS
#define SYS_ARCH_EXT
#else
#define SYS_ARCH_EXT extern
#endif


int enter_lwip_sys_arch_protect(void);
int exit_lwip_sys_arch_protect(void);
int init_lwip_sys_arch_protect(void);
void lwip_spin_lock(void);
void lwip_spin_unlock(void);

#define SYS_ARCH_DECL_PROTECT(lev)
#define  SYS_ARCH_PROTECT(lev)    lwip_spin_lock()
#define  SYS_ARCH_UNPROTECT(lev)  lwip_spin_unlock();


/*-----------------macros-----------------------------------------------------*/
//max number of lwip tasks (TCPIP) note LWIP TASK start with 1

#define MAX_QUEUE_ENTRIES (DEFAULT_TCP_RECVMBOX_SIZE)	// the max size of each mailbox

#define sys_arch_mbox_tryfetch(mbox,msg)    sys_arch_mbox_fetch(mbox,msg,1)

/*-----------------type define------------------------------------------------*/
/** struct of LwIP mailbox */
typedef struct {
    OS_QUEUE   pQ;
    // void*       pvQEntries[MAX_QUEUE_ENTRIES];
} TQ_DESCR;
typedef struct {
    OS_QUEUE   pQ;
    // void*       pvQEntries[TCPIP_MBOX_SIZE];
} TQ_DESCR_TCPIP;
/** struct of LwIP mailbox */

typedef OS_SEM sys_sem_t; // type of semiphores
typedef OS_MUTEX sys_mutex_t; // type of mutex
typedef TQ_DESCR sys_mbox_t; // type of tcp udp mailboxes
typedef TQ_DESCR_TCPIP sys_tcpip_mbox_t; // type of  tcpip mailboxes
typedef u32_t sys_thread_t; // type of id of the new thread
typedef u8_t sys_prot_t;

//#define     SYS_MBOX_NULL NULL
#define     SYS_SEM_NULL NULL

#define LWIP_NETCONN_THREAD_SEM_GET(LWIP_NETCONN_THREAD_SEM)   \
int	ucErr;\
sys_sem_t thread_sem;\
ucErr = os_sem_create (&thread_sem,0);\
if(ucErr!=OS_ERR_NONE)\
{\
    LWIP_ASSERT("LWIP_NETCONN_THREAD_SEM_GET ",ucErr == OS_ERR_NONE );\
    LWIP_NETCONN_THREAD_SEM = (sys_sem_t *)NULL;\
}\
LWIP_NETCONN_THREAD_SEM = &thread_sem;\

#define LWIP_NETCONN_THREAD_SEM_ALLOC()
#define LWIP_NETCONN_THREAD_SEM_FREE()

#endif /* __SYS_RTXC_H__ */
