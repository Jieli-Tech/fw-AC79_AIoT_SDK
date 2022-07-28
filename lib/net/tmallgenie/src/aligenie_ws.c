#include "aligenie_ws.h"
#include "aligenie_libs.h"
#include "aligenie_os.h"
#include "web_socket/websocket_api.h"
#include "sdk.h"

//静态存放
struct ws_info {
    struct websocket_struct websockets_info;
    AG_WS_CONNECT_INFO_T *info;
    u32 state;
};
static struct ws_info ff;
#define __this (&ff)


static OS_MUTEX websockets_op_mutex;
static void websockets_disconnect(struct websocket_struct *websockets_info);

static void websockets_callback(u8 *buf, u32 len, u8 type)
{
    bool is_fin = type & 0x80;
    u8 data_type = type & 0x0f;
    switch (data_type) {
    case WCT_SEQ:
        if (!is_fin) {
            __this->info->callbacks->cb_on_recv_bin(buf, len, AG_WS_BIN_DATA_CONTINUE);
        } else {
            __this->info->callbacks->cb_on_recv_bin(buf, len, AG_WS_BIN_DATA_FINISH);
        }
        break;
    case WCT_TXTDATA:
        __this->info->callbacks->cb_on_recv_text((char *)buf, len);
        break;
    case WCT_BINDATA:
        __this->info->callbacks->cb_on_recv_bin(buf, len, AG_WS_BIN_DATA_START);
        break;
    case WCT_DISCONN:
        break;
    case WCT_PING:
        break;
    case WCT_PONG:
        break;
    case WCT_END:
        break;
    default:
        break;
    }
}

static int websockets_client_exit_notify(struct websocket_struct *websockets_info)
{
    if (__this->state != AG_WS_STATUS_DISCONNECTED) {
        __this->state = AG_WS_STATUS_DISCONNECTED;
        websockets_disconnect(&__this->websockets_info);
        os_taskq_post("ali_app_task", 1, ALI_WS_DISCONNECT_MSG);
    }

    return 0;
}

static void websockets_client_reg(struct websocket_struct *websockets_info, char mode)
{
    memset(websockets_info, 0, sizeof(struct websocket_struct));
    websockets_info->_init           = websockets_client_socket_init;
    websockets_info->_exit           = websockets_client_socket_exit;
    websockets_info->_handshack      = webcockets_client_socket_handshack;
    websockets_info->_send           = websockets_socket_send;
    websockets_info->_recv_thread    = websockets_client_socket_recv_thread;
    websockets_info->_heart_thread   = websockets_client_socket_heart_thread;
    websockets_info->_recv_cb        = websockets_callback;
    websockets_info->_recv           = NULL;
    websockets_info->websocket_mode  = mode;
    websockets_info->_exit_notify    = websockets_client_exit_notify;
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
    int ret  = -1;
    os_mutex_pend(&websockets_op_mutex, 0);
    if (websockets_info->websocket_valid == ESTABLISHED) {
        //SSL加密时一次发送数据不能超过16K，用户需要自己分包
        ret = websockets_info->_send(websockets_info, buf, len, type);
    }
    os_mutex_post(&websockets_op_mutex);
    if (ret <= 0) {
        websockets_disconnect(websockets_info);
    }
    return ret;
}

static void websockets_client_exit(struct websocket_struct *websockets_info)
{
    websockets_info->_exit(websockets_info);
}

static int websockets_connect(void)
{
    int ret;
    char mode = WEBSOCKETS_MODE;
    struct websocket_struct *websockets_info = &__this->websockets_info;

    char *url = calloc(1460, 1);
    if (!url) {
        return -1;
    }

    char src_buf[256] = {0};
    sprintf(url, "wss://%s:%d%s", __this->info->server, __this->info->port, __this->info->path);

    printf("url=>%s\n", url);

    os_mutex_pend(&websockets_op_mutex, 0);
    if (websockets_info->websocket_valid == ESTABLISHED) {
        ret = -1;
        goto exit;
    }
    if (websockets_info->websocket_valid == INVALID_ESTABLISHED) {
        websockets_disconnect(websockets_info);
    }
    /* 1 . register */
    websockets_client_reg(websockets_info, mode);
    /* 2 . init */
    ret = websockets_client_init(websockets_info, (unsigned char *)url, NULL);
    if (FALSE == ret) {
        printf("  . ! Cilent websocket init error !!!\r\n");
        ret = -1;
        goto exit;
    }
    /* 3 . hanshack */
    ret = websockets_client_handshack(websockets_info);
    if (FALSE == ret) {
        ret = -1;
        printf("  . ! Handshake error !!!\r\n");
        goto exit;
    }
    printf(" . Handshake success \r\n");
    /* 4 . CreateThread */
    snprintf((char *)src_buf, sizeof(src_buf), "websocket_client_heart_%d", (u16)(random32(0) & 0xFFFF));
    thread_fork((const char *)src_buf, 19, 1024, 0,
                &websockets_info->ping_thread_id,
                websockets_info->_heart_thread,
                websockets_info);
    snprintf((char *)src_buf, sizeof(src_buf), "websocket_client_recv_%d", (u16)(random32(0) & 0xFFFF));
    thread_fork((const char *)src_buf, 18, 1024, 0,
                &websockets_info->recv_thread_id,
                websockets_info->_recv_thread,
                websockets_info);
    ret = 0;

exit:
    os_mutex_post(&websockets_op_mutex);
    free(url);
    return ret;
}

static void websockets_disconnect(struct websocket_struct *websockets_info)
{
    int ret;

    if (websockets_info->websocket_valid == NOT_ESTABLISHED) {
        return;
    }

retry_again:
    puts("\nwebsockets_disconnect enter\n");

    ret = os_mutex_pend(&websockets_op_mutex, 10);
    if (ret == OS_NO_ERR) {
        if (websockets_info->websocket_valid != NOT_ESTABLISHED) {
            /* 6 . exit */
            puts("[1]");
            websockets_info->websocket_valid = INVALID_ESTABLISHED;
            if (websockets_info->recv_thread_id) {
                thread_kill(&websockets_info->recv_thread_id, KILL_WAIT);
            }
            puts("[2]");
            if (websockets_info->ping_thread_id) {
                thread_kill(&websockets_info->ping_thread_id, KILL_WAIT);
            }
            puts("[3]");
            websockets_client_exit(websockets_info);
            websockets_info->websocket_valid = NOT_ESTABLISHED;
            puts("[4]");
        }
        os_mutex_post(&websockets_op_mutex);
    } else {
        if (websockets_info->websocket_valid != NOT_ESTABLISHED) {
            log_e("\n %s %d\n", __func__, __LINE__);
            goto retry_again;
        }
    }

    puts("\nwebsockets_disconnect exit\n");
}

/**********************************************************************/

int32_t ag_ws_connect(AG_WS_CONNECT_INFO_T *info)
{
    int ret = 0;

    __this->state = AG_WS_STATUS_CONNECTING;
    __this->info = info;

    printf("info->server:%s\n", info->server);
    printf("info->port:%d\n", info->port);
    printf("info->schema:%s\n", info->schema); //加密方式 wss  ws
    printf("info->path:%s\n", info->path);

    os_mutex_create(&websockets_op_mutex);

    ret = websockets_connect();

    if (ret < 0) {
        __this->state = AG_WS_STATUS_DISCONNECTED;
        os_taskq_post("ali_app_task", 1, ALI_WS_DISCONNECT_MSG);
    } else {
        __this->state = AG_WS_STATUS_CONNECTED;
        os_taskq_post("ali_app_task", 1, ALI_WS_CONNECT_MSG);
    }

    return ret == 0 ? AG_WS_RET_OK : AG_WS_RET_ERROR;
}

void ag_ws_on_connect()
{
    if (__this->info) {
        ag_os_task_mdelay(500);
        __this->info->callbacks->cb_on_connect();
    }
}

void ag_ws_on_disconnect()
{
    if (__this->info) {
        ag_os_task_mdelay(500);
        __this->info->callbacks->cb_on_disconnect();
    }
}

int32_t ag_ws_disconnect()
{
    __this->state = AG_WS_STATUS_DISCONNECTED;
    websockets_disconnect(&__this->websockets_info);
    os_taskq_post("ali_app_task", 1, ALI_WS_DISCONNECT_MSG);
    /* os_mutex_del(&websockets_op_mutex, 0); */
    return AG_WS_RET_OK;
}

AG_WS_STATUS_E ag_ws_get_connection_status()
{
    return __this->state;
}

int32_t ag_ws_send_text(char *text, uint32_t len)
{
    int ret = websockets_client_send(&__this->websockets_info, (u8 *)text, len, WCT_TXTDATA | (char)0x80);
    return ret > 0 ? AG_WS_RET_OK : AG_WS_RET_ERROR;
}

int32_t ag_ws_send_binary(void *data, uint32_t len, AG_WS_BIN_DATA_TYPE_T type)
{
    u8 _type = 0;

    if (type == AG_WS_BIN_DATA_START) {
        _type = WCT_BINDATA;
    } else if (type == AG_WS_BIN_DATA_CONTINUE) {
        _type = WCT_SEQ;
    } else if (type == AG_WS_BIN_DATA_FINISH) {
        _type = WCT_SEQ | 0x80;
    }

    int ret = websockets_client_send(&__this->websockets_info, data, len, _type);
    return ret > 0 ? AG_WS_RET_OK : AG_WS_RET_ERROR;
}

