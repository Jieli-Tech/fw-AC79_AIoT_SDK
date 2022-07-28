#ifndef WEBSOCKET_SSL_H
#define WEBSOCKET_SSL_H

#include "websocket_define.h"
#include "websocket_api.h"

int websockets_mbtls_write(void *sk_fd, u8 *buf, u64 *len, char type); //数据发送

/**********client********************/
void websockets_mbtls_client_set_ca(struct websockets_mbedtls *wbsk_mbtls_info, char *ca_buf, int ca_size);
int  websockets_mbtls_client_init(struct websockets_mbedtls *wbsk_mbtls_info);
void websocket_mbedtls_client_set_timeout(struct websockets_mbedtls *wbsk_mbtls_info, int send_to_ms, int recv_to_ms);
int  websockets_mbtls_client_connect(struct websockets_mbedtls *wbsk_mbtls_info, u8 *host, int port);
int  websockets_mbtls_client_handshack(struct websockets_mbedtls *wbsk_mbtls_info);
int  websockets_mbtls_client_write(struct websockets_mbedtls *wbsk_mbtls_info, u8 *buf, u64 *len, char type, char seq_en);
int  websockets_mbtls_client_read(struct websockets_mbedtls *wbsk_mbtls_info, u8 *buf, int len);
void websockets_mbtls_client_close(struct websockets_mbedtls *wbsk_mbtls_info);
void websockets_mbtls_client_exit(struct websockets_mbedtls *wbsk_mbtls_info);

/************server****************/
void websocket_mbedtls_serv_init(struct websockets_mbedtls *wbsk_mbtls_info);
int  websocket_mbedtls_serv_bind(struct websockets_mbedtls *wbsk_mbtls_info, int port);
void websocket_mbedtls_serv_set_timeout(struct websockets_mbedtls *wbsk_mbtls_info, int send_to_ms, int recv_to_ms);
int  websocket_mbedtls_serv_accept(struct websockets_mbedtls *wbsk_mbtls_info);
int  websockets_mbtls_serv_write(struct websockets_mbedtls *wbsk_mbtls_info, u8 *buf, u64 *len, char type, char seq_en);
int  websockets_mbtls_serv_read(struct websockets_mbedtls *wbsk_mbtls_info, u8 *buf, int len);
void websockets_mbtls_serv_close(struct websockets_mbedtls *wbsk_mbtls_info);
void websockets_mbtls_serv_exit(struct websockets_mbedtls *wbsk_mbtls_info);

#endif


