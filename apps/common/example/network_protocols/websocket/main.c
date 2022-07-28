#include "web_socket/websocket_api.h"
#include "wifi/wifi_connect.h"
#include "system/includes.h"
#ifdef WEBSOCKET_MEMHEAP_DEBUG
#include "mem_leak_test.h"
#endif

#include "app_config.h"

#ifdef USE_WEBSOCKET_TEST
/**** websoket "ws://"  websokets "wss://"  *****/

#define OBJ_URL 	"ws://82.157.123.54:9010/ajaxchattest" //远程服务器测试
//#define OBJ_URL 	"ws://121.40.165.18:8800"
//#define OBJ_URL     "ws://www.example.com/socketserver"
//#define OBJ_URL 	"wss://localhost:8888"                  //本地服务器测试
//#define OBJ_URL 	"wss://172.16.23.28:8888"                  //本地服务器测试


static void websockets_callback(u8 *buf, u32 len, u8 type)
{
    printf("wbs recv msg : %s\n", buf);
}

/*******************************************************************************
*   Websocket Client api
*******************************************************************************/
static void websockets_client_reg(struct websocket_struct *websockets_info, char mode)
{
    memset(websockets_info, 0, sizeof(struct websocket_struct));
    websockets_info->_init           = websockets_client_socket_init;
    websockets_info->_exit           = websockets_client_socket_exit;
    websockets_info->_handshack      = webcockets_client_socket_handshack;
    websockets_info->_send           = websockets_client_socket_send;
    websockets_info->_recv_thread    = websockets_client_socket_recv_thread;
    websockets_info->_heart_thread   = websockets_client_socket_heart_thread;
    websockets_info->_recv_cb        = websockets_callback;
    websockets_info->_recv           = NULL;

    websockets_info->websocket_mode  = mode;
}

static int websockets_client_init(struct websocket_struct *websockets_info, u8 *url, const char *origin_str)
{
    websockets_info->ip_or_url = url;
    websockets_info->origin_str = origin_str;
    websockets_info->recv_time_out = 1000;
    return websockets_info->_init(websockets_info);
}

static int websockets_client_handshack(struct websocket_struct *websockets_info)
{
    return websockets_info->_handshack(websockets_info);
}

static int websockets_client_send(struct websocket_struct *websockets_info, u8 *buf, int len, char type)
{
    //SSL加密时一次发送数据不能超过16K，用户需要自己分包
    return websockets_info->_send(websockets_info, buf, len, type);
}

static void websockets_client_exit(struct websocket_struct *websockets_info)
{
    websockets_info->_exit(websockets_info);
}

/*******************************************************************************
*   Websocket Client.c
*   Just one example for test
*******************************************************************************/
static void websockets_client_main_thread(void *priv)
{
    int err;
    char mode = WEBSOCKET_MODE;
#ifdef WIN32
    void *heart_thread_hdl = NULL;
    void *recv_thread_hdl = NULL;
#endif
    u8 url[256] = OBJ_URL;
    const char *input_str = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const char *origin_str = "http://coolaf.com";

    printf(" . ----------------- Client Websocket ------------------\r\n");

#ifdef WIN32
    printf(" . Please input the mode for websocket : 1 . websocket , 2 . websockets (ssl)\r\n");
    scanf("%d", &mode);
    if (mode == 2) {
        mode = WEBSOCKETS_MODE;
        printf(" . Please input the URL , for example: wss://localhost:8888\r\n");
    } else {
        mode = WEBSOCKETS_MODE;
        printf(" . Please input the URL , for example: ws://localhost:8888\r\n");
    }

    scanf("%d", &url);
#endif

    /* 0 . malloc buffer */
    struct websocket_struct *websockets_info = malloc(sizeof(struct websocket_struct));
    if (!websockets_info) {
        return;
    }
    /* 1 . register */
    websockets_client_reg(websockets_info, mode);

    /* 2 . init */
    err = websockets_client_init(websockets_info, url, origin_str);
    if (FALSE == err) {
        printf("  . ! Cilent websocket init error !!!\r\n");
        goto exit_ws;
    }

    /* 3 . hanshack */
    err = websockets_client_handshack(websockets_info);
    if (FALSE == err) {
        printf("  . ! Handshake error !!!\r\n");
        goto exit_ws;
    }
    printf(" . Handshake success \r\n");

    /* 4 . CreateThread */
#ifdef WIN32
    heart_thread_hdl = CreateThread(NULL, 0, websockets_info->_heart_thread, websockets_info, 0, &websockets_info->ping_thread_id);
    recv_thread_hdl = CreateThread(NULL, 0, websockets_info->_recv_thread,  websockets_info, 0, &websockets_info->ping_thread_id);
#else
    thread_fork("websocket_client_heart", 19, 512, 0,
                &websockets_info->ping_thread_id,
                websockets_info->_heart_thread,
                websockets_info);
    thread_fork("websocket_client_recv", 18, 512, 0,
                &websockets_info->recv_thread_id,
                websockets_info->_recv_thread,
                websockets_info);
#endif
    websockets_sleep(1000);

    /* 5 . recv or send data */
    while (1) {
        err = websockets_client_send(websockets_info, (u8 *)input_str, strlen(input_str), WCT_TXTDATA);
        if (FALSE == err) {
            printf("  . ! send err !!!\r\n");
            goto exit_ws;
        }
        printf("  . Send massage : %s \r\n", input_str);
        websockets_sleep(3000);
#ifdef WIN32
        printf(" . Please input the send massage\r\n");
        memset(input_str, 0, sizeof(input_str));
        scanf("%s", input_str);
#endif
    }

exit_ws:
    /* 6 . exit */
#ifdef WIN32
    TerminateThread(heart_thread_hdl, 0);
    TerminateThread(recv_thread_hdl, 0);
#else
    thread_kill(&websockets_info->ping_thread_id, KILL_REQ);
    thread_kill(&websockets_info->recv_thread_id, KILL_REQ);
#endif
    websockets_client_exit(websockets_info);
    free(websockets_info);
}

static void websocket_client_thread_create(void)
{
    thread_fork("websockets_client_main", 15, 512 * 3, 0, 0, websockets_client_main_thread, NULL);
}

static void http_websocket_start(void *priv)
{
    enum wifi_sta_connect_state state;
    while (1) {
        printf("Connecting to the network...\n");
        state = wifi_get_sta_connect_state();
        if (WIFI_STA_NETWORK_STACK_DHCP_SUCC == state) {
            printf("Network connection is successful!\n");
            break;
        }
        os_time_dly(500);
    }

    websocket_client_thread_create();
}

//应用程序入口,需要运行在STA模式下
void c_main(void *priv)
{
    if (thread_fork("http_websocket_start", 10, 512, 0, NULL, http_websocket_start, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}

late_initcall(c_main);

#endif//USE_WEBSOCKET_TEST
