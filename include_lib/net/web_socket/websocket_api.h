
#ifndef WEBSOCKET_API_H
#define WEBSOCKET_API_H


#include "websocket_define.h"
#include "websocket_base64.h"
#include "websocket_sha_1.h"
#include "websocket_intlib.h"
#include "websocket_api.h"
#include "string.h"

#include "mbedtls/mbedtls_config.h"
//#include "mbedtls/platform.h"
#include "mbedtls/net.h"
//#include "mbedtls/debug.h"
//#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "generic/typedef.h"
#include "mbedtls/certs.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"


#define websockets_sleep  msleep


enum {
    NO_MSG = 0,
    ERCV_DATA_MSG,
    CLIENT_SEND_DATA_MSG,
    CLIENT_RECV_DATA_MSG,
    SERVER_SEND_DATA_MSG,
    SERVER_ERCV_DATA_MSG,
    CLIENT_PING_MSG,
    CLIENT_PONG_MSG,
    SERVER_PING_MSG,
    SERVER_PONG_MSG,

    RECV_TIME_OUT_MSG,
    DISCONNECT_MSG,
    CONNECT_RST_MSG,

    MAX_MSG = 32,
};

// websocket根据data[0]判别数据包类型    比如0x81 = 0x80 | 0x1 为一个txt类型数据包
typedef enum {
    WCT_SEQ     = 0x00,
    WCT_TXTDATA = 0x01,      // 0x1：标识一个txt类型数据包
    WCT_BINDATA = 0x02,      // 0x2：标识一个bin类型数据包
    WCT_DISCONN = 0x08,      // 0x8：标识一个断开连接类型数据包
    WCT_PING    = 0x09,     // 0x8：标识一个断开连接类型数据包
    WCT_PONG    = 0x0a,     // 0xA：表示一个pong类型数据包
    WCT_FIN     = 0x80,     //fin
    WCT_END     = 0x10,
    WCT_CLOSE_OK   = 0xaa,
    WCT_INIT    = 0xff,
} WS_CMD_Type;

typedef struct websockets_mbedtls {
    /*client*/
    mbedtls_net_context server_fd;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt cacert;
    char ssl_fd;

    /*server*/
    u8 sll_ip_addr[16];
    mbedtls_net_context client_fd;//add client fd
    mbedtls_x509_crt srvcert;
    mbedtls_pk_context pkey;

    /* CA */
    char *mbedtls_ca_buf;
    int mbedtls_ca_size;
} WEBSOCKETS_MBTLS_INFO;

struct websocket_req_head {
    u8 medthod[4];
    u8 file[1024];
    u8 host[32];
    u8 version[8];
};

typedef struct websocket_struct {
    void *sk_fd;
    void *lst_fd;
    int ping_thread_id;
    int recv_thread_id;
    char websocket_mode;
    char websocket_recvsub;
    u8 websocket_data_type;
    u8 send_data_use_seq;
    u16 port;
    struct sockaddr_in servaddr;
    struct sockaddr_in clientaddr;
    u8 *ip_or_url;
    const char *origin_str;
    u8 ip_addr[16];
    u8 key[32];
    u8 msg[MAX_MSG];
    u8 msg_write;
    u8 msg_read;
    u8 *recv_buf;
    u64 recv_len;
    u32 recv_time_out;
    u32 payload_data_len;
    u32 payload_data_continue;
    struct websocket_req_head req_head;
    struct websockets_mbedtls websockets_mbtls_info;
    u16 websocket_valid;
    int (*_init)(struct websocket_struct *websocket_info);
    void (*_exit)(struct websocket_struct *websocket_info);
    int (*_handshack)(struct websocket_struct *websocket_info);
    void (*_heart_thread)(void *param);
    void (*_recv_thread)(void *param);
    int (*_recv)(struct websocket_struct *websocket_info);
    int (*_send)(struct websocket_struct *websocket_info, u8 *buf, int len, char type);
    void (*_recv_cb)(u8 *buf, u32 len, u8 type);
    int (*_exit_notify)(struct websocket_struct *websocket_info);
} WEBSOCKET_INFO;

typedef enum {
    NOT_ESTABLISHED = 0x00,
    ESTABLISHED = 0x01,
    INVALID_ESTABLISHED = 0x02,
} WS_STATUS;

extern int atoi(const char *__nptr);

void websocket_msg_fifo(struct websocket_struct *websockets_info, u8 msg_value);
u8 websocket_msg_get(struct websocket_struct *websockets_info);
void websocket_msg_clear(struct websocket_struct *websockets_info);
void websockets_send_data_set_seq_packet(struct websocket_struct *websockets_info, char seq_en);
int websockets_socket_send(struct websocket_struct *websockets_info, u8 *buf, int len, char type);//通用数据发送，type自定义指定类型

/******************websockets**************************************/
int  websockets_pong_heart_beat(struct websocket_struct *websockets_info, u8 *buf, char index);
int  websockets_ping_heart_beat(struct websocket_struct *websockets_info, u8 *buf, char index);
void websockets_client_socket_heart_thread(void *param);
void websockets_client_socket_recv_thread(void *param);
void websockets_client_socket_exit(struct websocket_struct *websocket_info);
int  websockets_client_socket_send(struct websocket_struct *websocket_info, u8 *buf, int len, char type);
int  websockets_client_socket_recv(struct websocket_struct *websocket_info);
int  webcockets_client_socket_handshack(struct websocket_struct *websocket_info);
int  websockets_client_socket_init(struct websocket_struct *websocket_info);
int websockets_client_notify_disconnet_to_server(struct websocket_struct *websockets_info);//客户端往服务器发送disconnect消息

void websockets_serv_socket_heart_thread(void *param);
int  websockets_serv_socket_recv(struct websocket_struct *websockets_info);
int  websockets_serv_socket_send(struct websocket_struct *websockets_info, u8 *buf, int len, char type);
void websockets_serv_socket_exit(struct websocket_struct *websockets_info);
int  websockets_serv_socket_hanshack(struct websocket_struct *websockets_info);
int  websockets_serv_socket_init(struct websocket_struct *websockets_info);

#endif

