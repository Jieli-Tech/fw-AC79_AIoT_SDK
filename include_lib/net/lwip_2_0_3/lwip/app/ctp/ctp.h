#ifndef __CTP_H__
#define __CTP_H__

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip.h"

#define USE_CTPS 0
#define USE_CDP			0xAABBCCDD
#define USE_CTP			0xDDCCBBAA

/*
命令格式:
|CTP:|2字节topic长度|topic|4字节topic内容长度|topic内容|

若没有topic内容则不填写|topic内容|字段,
*/

#define CTP_PREFIX "CTP:"
#define CTP_PREFIX_LEN 4
#define CTP_TOPIC_LEN   2
#define CTP_TOPIC_CONTENT_LEN   4

#define CTP_KEEP_ALIVE_TOPIC        "CTP_KEEP_ALIVE"
#define CTP_KEEP_ALIVE_TOPIC_LEN    strlen(CTP_KEEP_ALIVE_TOPIC)

#define CTP_LOGIN_TOPIC             "CTP_LOGIN"
#define CTP_LOGIN_TOPIC_LEN         strlen(CTP_LOGIN_TOPIC)

#define CTP_KEEP_ALIVE_DEFAULT_TIMEOUT  (60*1000)

//CTP保留TOPIC, 库内部使用
#define CTP_RESERVED_TOPIC   "CTP_RESERVED_TOPIC"

enum ctp_srv_msg_type {
    CTP_SRV_SET_RECV_THREAD_PRIO_STKSIZE,
    CTP_SRV_CLI_PREV_LINK_NOT_CLOSE,
    CTP_SRV_CLI_KEEP_ALIVE_TO,
    CTP_SRV_RECV_KEEP_ALIVE_MSG,
    CTP_SRV_CLI_DISCONNECT,
    CTP_SRV_CLI_CONNECTED,
    CTP_SRV_RECV_MSG,
    CTP_SRV_RECV_MSG_SLICE,
    CTP_SRV_RECV_MSG_WITHOUT_LOGIN,
    CTP_SRV_RECV_LOGIN_MSG,
};

enum ctp_cli_msg_type {
    CTP_CLI_CONNECT_SUCC,
    CTP_CLI_CONNECT_FAIL,
    CTP_CLI_SEND_TO,
    CTP_CLI_RECV_TO,
    CTP_CLI_DISCONNECT,
    CTP_CLI_RECV_MSG,
};



#define MAX_RECV_TOPIC_LEN                  (1*1460)
#define MAX_RECV_TOPIC_CONTENT_LEN_SLICE    (1*1460)

//srv
char *ctp_get_pram(u32 parm_num, char *parm_list);

int ctp_srv_init(u16_t port, int (*cb_func)(void *cli, enum ctp_srv_msg_type type, char *topic, char *content, void *priv), void *priv);
int ctp_srv_keep_alive_en(const char *recv, const char *send, const char *parm);
int ctp_srv_login_en(const char *login_recv, const char *login_send);
void ctp_srv_set_keep_alive_timeout(u32 timeout_sec);
int ctp_srv_get_keep_alive_timeout();
void ctp_srv_set_thread_prio_stksize(u32 prio, u32 stk_size);
void ctp_srv_set_thread_payload_max_len(u32 max_topic_len, u32 max_content_slice_len);
void ctp_srv_uninit(void);
int ctp_srv_send(void *cli, char *topic, char *content);
int ctp_srv_send_ext(void *cli, char *user_pkt, u32 pkt_len);
void ctp_srv_disconnect_cli(void  *cli_hdl);
void ctp_srv_disconnect_all_cli(void);
u32 ctp_srv_get_cli_cnt(void);
int ctp_sock_set_send_timeout(void *cli, u32 millsec);
int ctp_sock_set_recv_timeout(void *cli, u32 millsec);
struct sockaddr_in *ctp_srv_get_cli_addr(void *cli);

int cdp_srv_send(void *cli, char *topic, char *content);

void ctp_srv_free_cli(void *cli_hdl);
struct sockaddr_in *ctp_srv_get_first_cli(void);
void ctp_keep_alive_find_dhwaddr_disconnect(struct eth_addr *dhwaddr);




//cli

int ctp_cli_init(void);
void ctp_cli_uninit(void);

void *ctp_cli_reg(u16_t id, struct sockaddr_in *dest_addr, int (*cb_func)(void *hdl, enum ctp_cli_msg_type type, const char *topic, const char *parm_list, void *priv), void *priv);


void ctp_cli_unreg(void *handle);


int ctp_cli_send(void *handle, const char *topic, const char *content);

struct sockaddr_in *ctp_cli_get_hdl_addr(void *handle);
#endif

