#include "sock_api/sock_api.h"
#include "app_config.h"
#include "system/includes.h"
#include "wifi/wifi_connect.h"
#include "lwip.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#ifdef USE_TCP_CLIENT_TEST
#define CLIENT_TCP_PORT 0      			//客户端端口号
static void *sock = NULL;
#define SERVER_TCP_IP "192.168.10.102"   //服务端ip
#define SERVER_TCP_PORT 32768           //服务端端口号

static int tcp_client_init(const char *server_ip, const int server_port)
{
    //struct sockaddr_in local;
    struct sockaddr_in dest;

    //创建socket
    sock = sock_reg(AF_INET, SOCK_STREAM, 0, NULL, NULL);
    if (sock == NULL) {
        printf("sock_reg fail.\n");
        return -1;
    }

    //绑定本地地址信息
//    local.sin_addr.s_addr = INADDR_ANY;
//    local.sin_port = htons(CLIENT_TCP_PORT);
//    local.sin_family = AF_INET;
//    if (0 != sock_bind(sock, (struct sockaddr *)&local, sizeof(struct sockaddr_in))) {
//        sock_unreg(sock);
//        printf("sock_bind fail.\n");
//        return -1;
//    }

    //连接指定地址服务端
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = inet_addr(server_ip);
    dest.sin_port = htons(server_port);
    if (0 != sock_connect(sock, (struct sockaddr *)&dest, sizeof(struct sockaddr_in))) {
        printf("sock_connect fail.\n");
        sock_unreg(sock);
        return -1;
    }

    return 0;
}

//发送数据
static int tcp_send_data(const void *sock_hdl, const void *buf, const u32 len)
{
    return sock_send(sock_hdl, buf, len, 0);
}

//接收数据
static int tcp_recv_data(const void *sock_hdl, void *buf, u32 len)
{
    return sock_recvfrom(sock_hdl, buf, len, 0, NULL, NULL);
}

//tcp client任务
static void tcp_client_task(void *priv)
{
    int  err;
    char *payload = "Please send me some data!";
    char recv_buf[1024];
    int  recv_len;
    int  send_len;

    err = tcp_client_init(SERVER_TCP_IP, SERVER_TCP_PORT);
    if (err) {
        printf("tcp_client_init err!");
        return;
    }

    send_len = tcp_send_data(sock, payload, strlen(payload));
    if (send_len == -1) {
        printf("sock_sendto err!");
        sock_unreg(sock);
        return;
    }

    for (;;) {
        recv_len = tcp_recv_data(sock, recv_buf, sizeof(recv_buf));
        if ((recv_len != -1) && (recv_len != 0)) {
            recv_buf[recv_len] = '\0';
            printf("Received %d bytes, data: %s\n\r", recv_len, recv_buf);
            tcp_send_data(sock, "Data received successfully!", strlen("Data received successfully!"));
        } else {
            printf("sock_recvfrom err!");
            break;
        }
    }

    if (sock) {
        sock_unreg(sock);
        sock = NULL;
    }

}

static void tcp_client_start(void *priv)
{
    enum wifi_sta_connect_state state;

    while (1) {
        printf("Connecting to the network...\n");
        state = wifi_get_sta_connect_state() ;
        if (WIFI_STA_NETWORK_STACK_DHCP_SUCC == state) {
            printf("Network connection is successful!\n");
            break;
        }
        os_time_dly(1000);
    }

    if (thread_fork("tcp_client_task", 10, 1024, 0, NULL, tcp_client_task, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}

//应用程序入口,需要运行在STA模式下
void c_main(void *priv)
{

    if (thread_fork("tcp_client_start", 10, 512, 0, NULL, tcp_client_start, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}

late_initcall(c_main);
#endif //USE_TCP_CLIENT_TEST
