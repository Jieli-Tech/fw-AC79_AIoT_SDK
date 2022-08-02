#ifndef __CDP_H__
#define __CDP_H__

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip.h"

#define USE_CDP			0xAABBCCDD
#define USE_CTP			0xDDCCBBAA
/*
命令格式:
|CDP:|2字节topic长度|topic|4字节topic内容长度|topic内容|

若没有topic内容则不填写|topic内容|字段,
*/

#define CDP_PREFIX "CTP:"
#define CDP_PREFIX_LEN 4
#define CDP_TOPIC_LEN   2
#define CDP_TOPIC_CONTENT_LEN   4

#define CDP_KEEP_ALIVE_TOPIC        "CTP_KEEP_ALIVE"
#define CDP_KEEP_ALIVE_TOPIC_LEN    strlen(CDP_KEEP_ALIVE_TOPIC)

#define CDP_KEEP_ALIVE_DEFAULT_TIMEOUT  (60*1000)

//CDP保留TOPIC, 库内部使用
#define CDP_RESERVED_TOPIC   "CTP_RESERVED_TOPIC"

enum cdp_srv_msg_type {
    CDP_SRV_SET_RECV_THREAD_PRIO_STKSIZE,
    CDP_SRV_CLI_PREV_LINK_NOT_CLOSE,
    CDP_SRV_CLI_KEEP_ALIVE_TO,
    CDP_SRV_RECV_KEEP_ALIVE_MSG,
    CDP_SRV_CLI_DISCONNECT,
    CDP_SRV_CLI_CONNECTED,
    CDP_SRV_RECV_MSG,
    CDP_SRV_RECV_MSG_SLICE,
    CDP_SRV_RECV_MSG_WITHOUT_LOGIN,
    CDP_SRV_RECV_LOGIN_MSG,
};

#define MAX_RECV_TOPIC_LEN                  (1*1460)
#define MAX_RECV_TOPIC_CONTENT_LEN_SLICE    (1*1460)


int cdp_srv_init(u16_t port, int (*cb_func)(void *cli, enum cdp_srv_msg_type type, char *topic, char *content, void *priv), void *priv);
int cdp_srv_uninit(void);
int cdp_srv_send(void *_cli, char *topic, char *content);
void cdp_srv_set_thread_payload_max_len(u32 max_topic_len, u32 max_content_slice_len);
u32 cdp_srv_get_cli_cnt(void);
void cdp_srv_set_thread_payload_max_len(u32 max_topic_len, u32 max_content_slice_len);
int cdp_srv_keep_alive_en(const char *recv, const char *send, const char *parm);
void cdp_keep_alive_find_dhwaddr_disconnect(struct eth_addr *dhwaddr);
struct sockaddr_in *cdp_srv_get_cli_addr(void *cli);
void cdp_srv_disconnect_all_cli(void);
void cdp_srv_disconnect_cli(void  *cli_hdl);
void cdp_srv_free_cli(void *cli_hdl);
#endif

