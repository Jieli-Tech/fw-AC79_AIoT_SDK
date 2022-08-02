#ifndef __DTP_H__
#define __DTP_H__

#include "lwip/sockets.h"
#include "lwip/netdb.h"

enum dtp_cli_msg_type {
    DTP_CLI_SET_BIND_PORT,
    DTP_CLI_SET_CONNECT_TO,
    DTP_CLI_CONNECT_SUCC,
    DTP_CLI_CONNECT_FAIL,
    DTP_CLI_BIND_FAIL,
    DTP_CLI_SRV_DISCONNECT,
    DTP_CLI_UNREG,
    DTP_CLI_RECV_DATA,
    DTP_CLI_SEND_DATA,
    DTP_CLI_BEFORE_RECV,
    DTP_CLI_SEND_TO,
    DTP_CLI_RECV_TO,
};

enum dtp_srv_msg_type {
    DTP_SRV_SET_MAX_BACKLOG,
    DTP_SRV_CLI_CONNECTED,
    DTP_SRV_CLI_DISCONNECT,
    DTP_SRV_SRV_DISCONNECT,
    DTP_SRV_HDL_CLOSE,
    DTP_SRV_RECV_DATA,
    DTP_SRV_SEND_DATA,
    DTP_SRV_BEFORE_RECV,
    DTP_SRV_SEND_TO,
    DTP_SRV_RECV_TO,
};

#define DTP_NONE (1<<0)
#define DTP_WRITE (1<<1)
#define DTP_READ  (1<<2)
#define DTP_CONNECT_NON_BLOCK  (1<<3)

inline void dtp_cli_set_recvbuf(void *hdl, u8 *recvbuf, u32 recvbuf_len);
inline void dtp_cli_set_recv_wait_all(void *hdl, bool enable);
void *dtp_cli_reg(struct sockaddr_in *dest_addr, int (*cb_func)(void *hdl, enum dtp_cli_msg_type type, char *buf, u32 len, void *priv),  void *priv, int dtp_mode);
void dtp_cli_unreg(void *hdl);
int dtp_cli_send(void *hdl);
int dtp_cli_send_buf(void *hdl, char *buf, u32 len, int flag);
inline void dtp_cli_set_recv_timeout(void *hdl, u32 millsec);
inline int dtp_cli_recv_timeout(void *hdl);
inline int dtp_cli_set_send_timeout(void *hdl, u32 millsec);
inline int dtp_cli_send_timeout(void *hdl);
void dtp_cli_set_recv_flag(void *hdl, int flag);
void dtp_cli_set_send_flag(void *hdl, int flag);
int dtp_cli_recv(void *handle, char *buf, u32 len, int flag);
void ctp_cli_uninit(void);
int dtp_cli_init(void);
void dtp_cli_set_connect_to(void *hdl, int sec);
void dtp_cli_set_connect_interval(void *hdl, int sec);
void dtp_cli_set_local_port(void *hdl, u16 port);
void dtp_cli_set_thread_prio_stksize(u32 prio, u32 stk_size);
struct sockaddr_in *dtp_cli_get_hdl_addr(void *handle);



void *dtp_cli_grp_create(void);
void dtp_cli_grp_del(void *srv_grp);
int dtp_cli_grp_add(void *srv_grp, void *hdl);

int dtp_cli_grp_connect(void *srv_grp, bool wait_complete);
int dtp_cli_grp_send(void *srv_grp);


int dtp_srv_init(void);
void dtp_srv_uninit(void);
void dtp_srv_set_send_thread_prio_stksize(void *hdl, u32 prio, u32 stk_size);
void dtp_srv_set_recv_thread_prio_stksize(void *hdl, u32 prio, u32 stk_size);
void *dtp_srv_reg(u16 port, int (*cb_func)(void *hdl, void *cli, enum dtp_srv_msg_type type, u8 *buf, u32 len, void *priv),  void *priv, int dtp_mode);
void dtp_srv_unreg(void *hdl);
int dtp_srv_send(void *cli);
int dtp_srv_send_buf(void *cli, char *buf, u32 len, int flag);
int dtp_srv_recv(void *cli, char *buf, u32 len, int flag);
inline int dtp_srv_set_recv_timeout(void *cli, u32 millsec);
inline int dtp_srv_recv_timeout(void *cli);
inline int dtp_srv_set_send_timeout(void *cli, u32 millsec);
inline int dtp_srv_send_timeout(void *cli);
void dtp_srv_set_recv_flag(void *hdl, int flag);
void dtp_srv_set_send_flag(void *hdl, int flag);
void dtp_srv_disconnect_cli(void *handle, void *cli);
void dtp_srv_disconnect_all(void *handle);
void dtp_srv_set_recvbuf(void *hdl, u8 *recvbuf, u32 recvbuf_len);
struct sockaddr_in *dtp_srv_get_cli_addr(void *cli);


#endif
