/*
total head 24 bytes

   4       1       1       2      4      4      4      4
| conv |  cmd  |  frg  |  wnd  |  ts  |  sn  | una |  len |

*/

//=====================================================================
//
// KCP - A Better ARQ Protocol Implementation
// skywind3000 (at) gmail.com, 2010-2011
//
// Features:
// + Average RTT reduce 30% - 40% vs traditional ARQ like tcp.
// + Maximum RTT reduce three times vs tcp.
// + Lightweight, distributed as a single source file.
//
//=====================================================================
#ifndef __IKCP_H__
#define __IKCP_H__

#include <stdarg.h>

#define IKCP_HEAD_SIZE  (24)

//=====================================================================
// QUEUE DEFINITION
//=====================================================================
#ifndef __IQUEUE_DEF__
#define __IQUEUE_DEF__

struct IQUEUEHEAD {
    struct IQUEUEHEAD *next, *prev;
};

typedef struct IQUEUEHEAD iqueue_head;


//---------------------------------------------------------------------
// queue init
//---------------------------------------------------------------------
#define IQUEUE_HEAD_INIT(name) { &(name), &(name) }
#define IQUEUE_HEAD(name) \
	struct IQUEUEHEAD name = IQUEUE_HEAD_INIT(name)

#define IQUEUE_INIT(ptr) ( \
	(ptr)->next = (ptr), (ptr)->prev = (ptr))

#define IOFFSETOF(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define ICONTAINEROF(ptr, type, member) ( \
		(type*)( ((char*)((type*)ptr)) - IOFFSETOF(type, member)) )

#define IQUEUE_ENTRY(ptr, type, member) ICONTAINEROF(ptr, type, member)


//---------------------------------------------------------------------
// queue operation
//---------------------------------------------------------------------
#define IQUEUE_ADD(node, head) ( \
	(node)->prev = (head), (node)->next = (head)->next, \
	(head)->next->prev = (node), (head)->next = (node))

#define IQUEUE_ADD_TAIL(node, head) ( \
	(node)->prev = (head)->prev, (node)->next = (head), \
	(head)->prev->next = (node), (head)->prev = (node))

#define IQUEUE_DEL_BETWEEN(p, n) ((n)->prev = (p), (p)->next = (n))

#define IQUEUE_DEL(entry) (\
	(entry)->next->prev = (entry)->prev, \
	(entry)->prev->next = (entry)->next, \
	(entry)->next = 0, (entry)->prev = 0)

#define IQUEUE_DEL_INIT(entry) do { \
	IQUEUE_DEL(entry); IQUEUE_INIT(entry); } while (0)

#define IQUEUE_IS_EMPTY(entry) ((entry) == (entry)->next)

#define iqueue_init		IQUEUE_INIT
#define iqueue_entry	IQUEUE_ENTRY
#define iqueue_add		IQUEUE_ADD
#define iqueue_add_tail	IQUEUE_ADD_TAIL
#define iqueue_del		IQUEUE_DEL
#define iqueue_del_init	IQUEUE_DEL_INIT
#define iqueue_is_empty IQUEUE_IS_EMPTY

#define IQUEUE_FOREACH(iterator, head, TYPE, MEMBER) \
	for ((iterator) = iqueue_entry((head)->next, TYPE, MEMBER); \
		&((iterator)->MEMBER) != (head); \
		(iterator) = iqueue_entry((iterator)->MEMBER.next, TYPE, MEMBER))

#define iqueue_foreach(iterator, head, TYPE, MEMBER) \
	IQUEUE_FOREACH(iterator, head, TYPE, MEMBER)

#define iqueue_foreach_entry(pos, head) \
	for( (pos) = (head)->next; (pos) != (head) ; (pos) = (pos)->next )


#define __iqueue_splice(list, head) do {	\
		iqueue_head *first = (list)->next, *last = (list)->prev; \
		iqueue_head *at = (head)->next; \
		(first)->prev = (head), (head)->next = (first);		\
		(last)->next = (at), (at)->prev = (last); }	while (0)

#define iqueue_splice(list, head) do { \
	if (!iqueue_is_empty(list)) __iqueue_splice(list, head); } while (0)

#define iqueue_splice_init(list, head) do {	\
	iqueue_splice(list, head);	iqueue_init(list); } while (0)

#endif


//---------------------------------------------------------------------
// WORD ORDER
//---------------------------------------------------------------------
#ifndef IWORDS_BIG_ENDIAN
#define IWORDS_BIG_ENDIAN  0
#endif



//=====================================================================
// SEGMENT
//=====================================================================
struct IKCPSEG {
    struct IQUEUEHEAD node;
    unsigned int conv;
    unsigned int cmd;
    unsigned int frg;
    unsigned int wnd;
    unsigned int ts;
    unsigned int sn;
    unsigned int una;
    unsigned int len;
    unsigned int resendts;
    unsigned int rto;
    unsigned int fastack;
    unsigned int xmit;
    char data[1];
};


//---------------------------------------------------------------------
// IKCPCB
//---------------------------------------------------------------------
struct IKCPCB {
    unsigned int conv, mtu, agg_mtu, mss, state;
    unsigned int snd_una, snd_nxt, rcv_nxt;
    unsigned int ts_recent, ts_lastack, ssthresh;
    int rx_rttval, rx_srtt, rx_rto, rx_minrto;
    unsigned int snd_wnd, rcv_wnd, rmt_wnd, cwnd, probe;
    unsigned int current, interval, ts_flush, xmit;
    unsigned int nrcv_buf, nsnd_buf;
    unsigned int nrcv_que, nsnd_que;
    unsigned int nodelay, updated;
    unsigned int ts_probe, probe_wait;
    unsigned int dead_link, incr;
    struct IQUEUEHEAD snd_queue;
    struct IQUEUEHEAD rcv_queue;
    struct IQUEUEHEAD snd_buf;
    struct IQUEUEHEAD rcv_buf;
    unsigned int *acklist;
    unsigned int ackcount;
    unsigned int ackblock;
    void *user;
    char *buffer;
    int fastresend;
    int nocwnd, stream;
    int logmask;
    int (*output)(const char *buf, int len, struct IKCPCB *kcp, void *user);
    void (*writelog)(struct IKCPCB *kcp, const char *fmt, va_list va, void *user);
    int send_block;
    int recv_block;
    int (*send_sem_post)(void *send_sem);
    int (*send_sem_pend)(void *send_sem, int timeout);
    int (*send_sem_del)(void *send_sem);
    void *send_sem;
    int (*recv_sem_post)(void *recv_sem);
    int (*recv_sem_pend)(void *recv_sem, int timeout);
    int send_sem_pend_timeout;
    int recv_sem_pend_timeout;
    int (*recv_sem_del)(void *recv_sem);
    void *recv_sem;
    int (*mutex_lock)(void *mutex_lock);
    void (*mutex_unlock)(void *mutex_lock);
    void (*mutex_del)(void *mutex_lock);
    void *mutex;
};


typedef struct IKCPCB ikcpcb;

#define IKCP_LOG_OUTPUT			1
#define IKCP_LOG_INPUT			2
#define IKCP_LOG_SEND			4
#define IKCP_LOG_RECV			8
#define IKCP_LOG_IN_DATA		16
#define IKCP_LOG_IN_ACK			32
#define IKCP_LOG_IN_PROBE		64
#define IKCP_LOG_IN_WINS		128
#define IKCP_LOG_OUT_DATA		256
#define IKCP_LOG_OUT_ACK		512
#define IKCP_LOG_OUT_PROBE		1024
#define IKCP_LOG_OUT_WINS		2048

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------
// interface
//---------------------------------------------------------------------

// create a new kcp control object, 'conv' must equal in two endpoint
// from the same connection. 'user' will be passed to the output callback
// output callback can be setup like this: 'kcp->output = my_udp_output'
ikcpcb *ikcp_create(unsigned int conv, void *user);

// release kcp control object
void ikcp_release(ikcpcb *kcp);

// set output callback, which will be invoked by kcp
void ikcp_setoutput(ikcpcb *kcp, int (*output)(const char *buf, int len,
                    ikcpcb *kcp, void *user));

// user/upper level recv: returns size, returns below zero for EAGAIN
int ikcp_recv(ikcpcb *kcp, char *buffer, int len);

// user/upper level send, returns below zero for error
int ikcp_send(ikcpcb *kcp, const char *buffer, int len);

// update state (call it repeatedly, every 10ms-100ms), or you can ask
// ikcp_check when to call it again (without ikcp_input/_send calling).
// 'current' - current timestamp in millisec.
void ikcp_update(ikcpcb *kcp, unsigned int (*iclock)(void));

// Determine when should you invoke ikcp_update:
// returns when you should invoke ikcp_update in millisec, if there
// is no ikcp_input/_send calling. you can call ikcp_update in that
// time, instead of call update repeatly.
// Important to reduce unnacessary ikcp_update invoking. use it to
// schedule ikcp_update (eg. implementing an epoll-like mechanism,
// or optimize ikcp_update when handling massive kcp connections)
unsigned int ikcp_check(const ikcpcb *kcp, unsigned int (*iclock)(void));

// when you received a low level packet (eg. UDP packet), call it
int ikcp_input(ikcpcb *kcp, const char *data, long size);

// flush pending data
void ikcp_flush(ikcpcb *kcp, unsigned int (*iclock)(void));

// check the size of next message in the recv queue
int ikcp_peeksize(const ikcpcb *kcp);

// change MTU size, default is 1400
int ikcp_setmtu(ikcpcb *kcp, int mtu, int agg_mtu);

// set maximum window size: sndwnd=32, rcvwnd=32 by default
int ikcp_wndsize(ikcpcb *kcp, int sndwnd, int rcvwnd);

// get how many packet is waiting to be sent
int ikcp_waitsnd(const ikcpcb *kcp);

// get how many packet is waiting to be recv
unsigned int ikcp_get_nrcv_que(ikcpcb *kcp);

// fastest: ikcp_nodelay(kcp, 1, 20, 2, 1)
// nodelay: 0:disable(default), 1:enable
// interval: internal update timer interval in millisec, default is 100ms
// resend: 0:disable fast resend(default), 1:enable fast resend
// nc: 0:normal congestion control(default), 1:disable congestion control
int ikcp_nodelay(ikcpcb *kcp, int nodelay, int interval, int resend, int nc);

// setup allocator
void ikcp_allocator(void *(*new_malloc)(unsigned int len), void (*new_free)(void *p));

// read conv
unsigned int ikcp_getconv(const void *ptr);
// read len
unsigned int ikcp_getlen(const void *ptr);

void ikcp_set_user(ikcpcb *kcp, void *user);

void ikcp_set_writelog(ikcpcb *kcp, void (*writelog)(struct IKCPCB *kcp, const char *fmt, va_list va, void *user), int logmask);

void ikcp_set_send_block(ikcpcb *kcp, int block, int (*send_sem_post)(void *priv), int (*send_sem_pend)(void *priv, int timeout), int (*send_sem_del)(void *priv), int timeout, void *priv);
void ikcp_set_recv_block(ikcpcb *kcp, int block, int (*recv_sem_post)(void *priv), int (*recv_sem_pend)(void *priv, int timeout), int (*recv_sem_del)(void *priv), int timeout, void *priv);
void ikcp_set_mutex_lock_func(ikcpcb *kcp, int (*mutex_lock)(void *priv), void (*mutex_unlock)(void *priv), void (*mutex_del)(void *priv), void *priv);

void ikcp_set_iclock(ikcpcb *kcp, unsigned int (*iclock)(void));

int ikcp_dead_link(ikcpcb *kcp);

#ifdef __cplusplus
}
#endif

#endif



