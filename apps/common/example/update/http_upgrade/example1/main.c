/********************************测试例程说明************************************
 *  功能说明：
 *      通过HTTP协议，下载URL指定的网络升级文件存储至指定路径，并进行升级固件。
 */

#include "system/includes.h"
#include "os/os_api.h"
#include "http/http_cli.h"
#include "fs/fs.h"
#include <stdlib.h>
#include "storage_device.h"
#include "net_update.h"
#include "app_config.h"
#include "wifi/wifi_connect.h"

#ifdef USE_HTTP_UPGRADE_DEMO1

#define OTA_URL_ADDR "https://test02.jieliapp.com/tmp/2021/12/29/update-ota.ufw"

#define DEFAULT_RECV_BUF_SIZE   (4*1024)   //数据缓存BUF大小
#define MAX_RECONNECT_CNT 10               //最大重连次数

struct download_hdl {
    char *url;                    //下载链接
    u32 file_size;                //需要下载的文件大小
    u8 *recv_buf;                 //数据缓存BUF
    u32 recv_buf_size;            //数据缓存BUF大小
    u32 download_len;             //已下载的数据长度
    u32 reconnect_cnt;             //重连次数
    httpcli_ctx ctx;              //http请求参数
    const struct net_download_ops *download_ops; //http操作集
};

struct download_parm {
    char *url;  //下载链接
};

//http回调函数
static int http_user_cb(void *ctx, void *buf, unsigned int size, void *priv, httpin_status status)
{
    struct download_hdl *hdl = (struct download_hdl *)priv;

    if (status == HTTPIN_HEADER) {
        //第一次连接时，记录文件总长度
        if (hdl->ctx.lowRange == 0) {
            hdl->file_size = hdl->ctx.content_length;
        }

        log_e("http_need_download_file=%d KB", hdl->file_size / 1024);
    }
    return 0;
}

static void download_task(void *priv)
{
    struct download_hdl *hdl = priv;
    void *update_fd = NULL;
    int ret = 0;
    int err = 0;
    char sock_err = 0;

    update_fd = net_fopen(CONFIG_UPGRADE_OTA_FILE_NAME, "w");
    if (!update_fd) {
        log_e("open update_fd error\n");
        goto _out_;
    }

_reconnect_:
    //发起连接请求，建立socket连接
    ret = hdl->download_ops->init(&hdl->ctx);

    if (ret != HERROR_OK) {
        if (hdl->ctx.req_exit_flag == 0) {
            if (hdl->reconnect_cnt < MAX_RECONNECT_CNT) {
                hdl->reconnect_cnt++;
                goto _reconnect_;
            } else {
                log_e("download reconnect upto max count.\n");
                goto _out_;
            }
        }
    }

    while (hdl->ctx.req_exit_flag == 0) {
        ret = hdl->download_ops->read(&hdl->ctx, (char *)hdl->recv_buf, hdl->recv_buf_size);//最大接收为recv_buf_size
        if (ret <  0) {  //读取数据失败
            sock_err = 1;
            goto _out_;
        } else {
            hdl->download_len += ret;
            err = net_fwrite(update_fd, hdl->recv_buf, ret, 0);
            if (err != ret) {
                goto _out_;
            }
        }

        if (hdl->download_len >= hdl->file_size) {
            goto _finish_;
        }
    }

_finish_:
    printf("download success.\n");

    net_fclose(update_fd, sock_err);
    system_reset();

_out_:
    //关闭网络连接
    hdl->download_ops->close(&hdl->ctx);

    net_fclose(update_fd, sock_err);
    free(hdl->url);
    free(hdl->recv_buf);
    memset(hdl, 0, sizeof(struct download_hdl));
    free(hdl);
}

static int http_create_download_task(void)
{
    struct download_parm parm = {0};
    struct download_hdl *hdl = NULL;

    parm.url = OTA_URL_ADDR;

    //申请堆内存
    hdl = (struct download_hdl *)calloc(1, sizeof(struct download_hdl));

    if (hdl == NULL) {
        return -ENOMEM;
    }

    hdl->download_ops = &http_ops;
    hdl->recv_buf_size = DEFAULT_RECV_BUF_SIZE;

    hdl->url = (char *)calloc(1, strlen(parm.url) + 1);

    if (hdl->url == NULL) {
        return -ENOMEM;
    }

    hdl->recv_buf = (u8 *)calloc(1, hdl->recv_buf_size);

    if (hdl->recv_buf == NULL) {
        return -ENOMEM;
    }

    strcpy(hdl->url, parm.url);

    //http请求参数赋值
    hdl->ctx.url = hdl->url;
    hdl->ctx.cb = http_user_cb;
    hdl->ctx.priv = hdl;
    hdl->ctx.connection = "close";

    //创建下载线程
    return thread_fork("download_task", 20,  2 * 1024, 0, NULL, download_task, hdl);
}

static void http_upgrade_start(void *priv)
{
    enum wifi_sta_connect_state state;

    while (1) {
        printf("Connecting to the network...\n");
        state = wifi_get_sta_connect_state();
        if (WIFI_STA_NETWORK_STACK_DHCP_SUCC == state) {
            printf("Network connection is successful!\n");
            break;
        }

        os_time_dly(1000);
    }

    http_create_download_task();
}

//应用程序入口,需要运行在STA模式下
void c_main(void *priv)
{
    if (thread_fork("http_upgrade_start", 10, 512, 0, NULL, http_upgrade_start, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}

late_initcall(c_main);
#endif
