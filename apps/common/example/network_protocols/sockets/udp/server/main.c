#include "sock_api/sock_api.h"
#include "os/os_api.h"
#include "app_config.h"
#include "system/includes.h"
#include "wifi/wifi_connect.h"
#include "lwip.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#ifdef USE_UDP_SERVER_TEST
static void *socket_fd = 0;
static int recv_pid = 0;
#define SERVER_UDP_PORT 32769

//功  能：udp_server接收线程，用于接收udp_client的数据
//参  数: 无
//返回值：无
static void udp_recv_handler(void)
{
    struct sockaddr_in remote_addr = {0};
    socklen_t len = sizeof(remote_addr);
    int recv_len = 0;
    u8 recv_buf[50] = {0};

    for (;;) {

        //接收udp_client的数据，并获取udp_client的地址信息
        recv_len = sock_recvfrom(socket_fd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&remote_addr, &len);
        printf("\n recv_len is %d  \n", recv_len);

        //将接收到的数据发送至udp_client
        sock_sendto(socket_fd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr_in));

        memset(recv_buf, 0, sizeof(recv_buf));
    }
}

//功  能：搭建udp_server
//参  数: int port：端口号
//返回值：返回0，成功；返回-1，失败
static int udp_server_init(int port)
{
    struct sockaddr_in local_addr = {0};

    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(port);

    socket_fd = sock_reg(AF_INET, SOCK_DGRAM, 0, NULL, NULL);
    if (socket_fd == NULL) {
        printf("%s build socket fail\n",  __FILE__);
        return -1;
    }

    if (sock_bind(socket_fd, (struct sockaddr *)&local_addr, sizeof(struct sockaddr))) {
        printf("%s sock_bind fail\n", __FILE__);
        return -1;
    }

    //创建线程，用于接收tcp_client的数据
    if (thread_fork("udp_recv_handler", 25, 512, 0, &recv_pid, udp_recv_handler, NULL) != OS_NO_ERR) {
        printf("%s thread fork fail\n", __FILE__);
        return -1;
    }

    return 0;
}

//功  能：注销udp_server
//参  数: 无
//返回值：无
static void udp_server_exit(void)
{
    thread_kill(&recv_pid, KILL_WAIT);
    sock_unreg(socket_fd);
}

static void udp_server_start(void *priv)
{
    int err;
    enum wifi_sta_connect_state state;

    while (1) {
        state = wifi_get_sta_connect_state();
        printf("Connecting to the network...\n");
        if (WIFI_STA_NETWORK_STACK_DHCP_SUCC == state) {
            printf("Network connection is successful!\n");
            break;
        }

        os_time_dly(1000);
    }

    err = udp_server_init(SERVER_UDP_PORT);
    if (err == -1) {
        printf("udp_server_init faile\n");
        udp_server_exit();
    }
}

//应用程序入口,需要运行在STA模式下
void c_main(void *priv)
{
    if (thread_fork("udp_server_start", 10, 512, 0, NULL, udp_server_start, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}

late_initcall(c_main);
#endif//USE_UDP_SERVER_TEST
