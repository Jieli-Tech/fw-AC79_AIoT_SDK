#ifndef __MSSDP_H__
#define __MSSDP_H__

#include "typedef.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

/*
宣告端格式:"MSSDP_SEARCH MSG"
应答端格式:"MSSDP_NOTIFY MSG"
*/

enum mssdp_recv_msg_type {
    MSSDP_SEARCH_MSG,
    MSSDP_NOTIFY_MSG,
    MSSDP_USER_MSG,
    MSSDP_BEFORE_SEND_SEARCH_MSG,
    MSSDP_BEFORE_SEND_NOTIFY_MSG,
};

int mssdp_set_notify_msg(const char *notify_msg, u32_t notify_time);
int mssdp_set_search_msg(const char *search_msg, u32_t search_time);
int mssdp_init(const char *search_prefix, const char *notify_prefix, const char *user_prefix, u16_t port, void (*recv_func)(u32 dest_ipaddr, enum mssdp_recv_msg_type type, char *buf, void *priv), void *priv);
void mssdp_uninit(void);
int mssdp_search(void);
int mssdp_notify(struct sockaddr_in *si);
int mssdp_send_msg(struct sockaddr_in *si, char *buf, u32_t buf_len);
void get_mssdp_info(const char **notify_prefix, int *socket, u16_t *port);

#endif
