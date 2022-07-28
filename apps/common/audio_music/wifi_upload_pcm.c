#include "sock_api/sock_api.h"
#include "os/os_api.h"

#define TEST_RUN_TIME_SEC	60

static void *server_socket;
static void *client_socket;
static u32 run_sec = 0;

static int wifi_pcm_stream_socket_create(void)
{
    struct sockaddr_in local_ipaddr;
    struct sockaddr_in clientaddr;
    u32 len = sizeof(clientaddr);
    char ipaddr[20];
    if (!server_socket) {
        server_socket = sock_reg(AF_INET, SOCK_STREAM, 0, NULL, NULL);
        if (server_socket == NULL) {
            goto __err;
        }

        if (0 != sock_set_reuseaddr(server_socket)) {
            goto __err;
        }

        extern void Get_IPAddress(char is_wireless, char *ipaddr);
        Get_IPAddress(1, ipaddr);

        local_ipaddr.sin_addr.s_addr = inet_addr(ipaddr);
        local_ipaddr.sin_port = htons(5002);
        local_ipaddr.sin_family = AF_INET;

        if (0 != sock_bind(server_socket, (struct sockaddr *)&local_ipaddr, sizeof(local_ipaddr))) {
            goto __err;
        }

        if (0 != sock_listen(server_socket, 1)) {
            goto __err;
        }
    }

    client_socket = sock_accept(server_socket, (struct sockaddr *)&clientaddr, (socklen_t *)&len, NULL, NULL);
    if (!client_socket) {
        goto __err;
    }

    sock_set_send_timeout(client_socket, 3000);
    run_sec = 0;
    return 0;

__err:
    if (server_socket) {
        sock_unreg(server_socket);
        server_socket = NULL;
    }
    return -1;
}

void wifi_pcm_stream_socket_send(u8 *buf, u32 len)
{
    if (client_socket) {
        sock_send(client_socket, buf, len, 0);
    }
}

void wifi_pcm_stream_socket_close(void)
{
    if (client_socket) {
        sock_unreg(client_socket);
        client_socket = NULL;
    }
#if 0
    if (server_socket) {
        sock_unreg(server_socket);
        server_socket = NULL;
    }
#endif
    run_sec = TEST_RUN_TIME_SEC;
}

static void wifi_pcm_stream_task(void *priv)
{

    while (0 == wifi_pcm_stream_socket_create()) {
        while (1) {
            if (++run_sec > TEST_RUN_TIME_SEC) {
                wifi_pcm_stream_socket_close();
                break;
            } else {
                os_time_dly(100);
            }
        }
    }
}

void wifi_pcm_stream_task_init(void)
{
    if (!server_socket) {
        thread_fork("wifi_pcm_stream", 10, 512, 0, 0, wifi_pcm_stream_task, NULL);
    }
}
