#ifndef __UDTP_H__
#define __UDTP_H__

#define UDTP_NONE (0)
#define UDTP_WRITE (1<<0)
#define UDTP_READ  (1<<1)

enum udtp_msg_type {
    UDTP_SET_THREAD_PRIO_STKSIZE,
    UDTP_SEND_DATA,
    UDTP_BEFORE_RECV,
    UDTP_RECV_DATA,
    UDTP_HDL_CLOSE,
};

int udtp_init(void);
void udtp_uninit(void);
void *udtp_reg(struct sockaddr_in *dst_addr, u16 listen_port, int (*cb_func)(void *handle, struct sockaddr_in *dst_addr, enum udtp_msg_type type, u8 *buf, u32 len, void *priv),  void *priv, int udtp_mode);
void udtp_unreg(void *handle);
int udtp_send(void *handle);
int udtp_send_buf(void *handle, char *buf, u32 len, int flag);
void udtp_set_send_thread_prio_stksize(void *handle, u32 prio, u32 stk_size);
void udtp_set_recv_thread_prio_stksize(void *handle, u32 prio, u32 stk_size);
void udtp_set_recvbuf(void *handle, u8 *recvbuf, u32 recvbuf_len);
void udtp_set_recv_timeout(void *handle, u32 millsec);
int udtp_recv_timeout(void *handle);
int udtp_recv(void *handle, u8 *buf, u32 len, int flag, struct sockaddr_in *dest_addr);

#endif
