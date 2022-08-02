/********************************测试例程说明************************************
 *  功能说明：
 *      通过HTTP协议，下载URL指定的网络文件存储至指定路径，并具有断点重载的功能。
 *
 *  使用说明：
 *      确保网络连接成功、存储设备就绪，指定下载路径URL、存储路径PATH，
 *      调用 net_file_download_test()函数建立下载任务，即可开始下载。
 * ******************************************************************************/

#include "system/includes.h"
#include "os/os_api.h"
#include "http/http_cli.h"
#include "fs/fs.h"
#include <stdlib.h>
#include "storage_device.h"
#include "app_config.h"
#include "system/includes.h"
#include "wifi/wifi_connect.h"

#ifdef USE_HTTP_DOWNLOAD_TEST

#if 1
#define log_info    printf
#else
#define log_info
#endif

#define MAX_RECONNECT_CNT       (10)    //最大重连次数
#define DEFAULT_RECV_BUF_SIZE   (4096)   //数据缓存BUF大小

#define HTTP_DOWNLOAD_URL "http://test02.jieliapp.com/file/2020/08/07/e3643b55-13df-4f2d-b3f0-fe68e39a55ef.bfu"
#define HTTP_STORAGE_PATH CONFIG_STORAGE_PATH"/C/CC/demo.bfu"

struct download_hdl {
    char *url;                    //下载链接
    char *path;                   //文件存放路径
    FILE *fp;                     //文件指针
    u32 file_size;                //需要下载的文件大小
    char *recv_buf;               //数据缓存BUF
    u32 recv_buf_size;            //数据缓存BUF大小
    u32 download_len;             //已下载的数据长度
    u32 reconnect_download_len;   //重连后已下载的数据长度
    u32 reconnect_cnt;            //重连次数
    u32 reconnecting;             //当前连接是否为重连
    httpcli_ctx ctx;              //http请求参数
    const struct net_download_ops *download_ops; //http操作集
};

struct download_parm {
    char *url;  //下载链接
    char *path; //存放路径
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

        //打开文件存储路径
        hdl->fp = fopen(hdl->path, "w+");
        if (hdl->fp == NULL) {
            log_info("storge path open error.\n");
            return -1;
        }
    }
    return 0;
}


static void download_task(void *priv)
{
    struct download_hdl *hdl = priv;
    int ret = 0;
    int offset = 0;
    int downloaded = 0;
    int new = 0;

_reconnect_:
    //发起连接请求，建立socket连接
    ret = hdl->download_ops->init(&hdl->ctx);
    if (ret != HERROR_OK) {
        if (hdl->ctx.req_exit_flag == 0) {
            os_time_dly(50);
            goto _reconnect_;
        }
    }

    printf("downloaded >>> 00%%\n");
    while (hdl->ctx.req_exit_flag == 0) {
        //读取数据
        ret = hdl->download_ops->read(&hdl->ctx, hdl->recv_buf, hdl->recv_buf_size);

        if (hdl->ctx.req_exit_flag) {
            goto _out_;
        }

        if (ret < 0) {  //读取数据失败
            log_info("recv fail.\n");
            //保存重定向后的URL
            if (hdl->ctx.redirection_url) {
                free(hdl->url);
                hdl->url = NULL;
                hdl->url = calloc(1, strlen(hdl->ctx.redirection_url) + 1);
                if (hdl->url == NULL) {
                    log_info("reconnect calloc for url fail.\n");
                    goto _out_;
                }
                strcpy(hdl->url, hdl->ctx.redirection_url);
                hdl->ctx.url = hdl->url;
            }

            //关闭网络连接
            hdl->download_ops->close(&hdl->ctx);
            //记录断点位置，重新发起下载请求
            if (hdl->reconnect_cnt < MAX_RECONNECT_CNT) {
                hdl->ctx.lowRange = hdl->download_len;
                hdl->ctx.highRange = 0;
                hdl->reconnect_cnt++;
                hdl->reconnecting = 1;
                goto _reconnect_;
            } else {
                log_info("download reconnect upto max count.\n");
                goto _out_;
            }
        } else {    //读取数据成功，将数据写入指定路径
            //服务器不支持带range字段的http请求
            if (hdl->reconnecting && (hdl->ctx.support_range == 0)) {
                if (hdl->reconnect_download_len + ret > hdl->download_len) {
                    offset = hdl->download_len - hdl->reconnect_download_len;
                    //printf("downloaded : %d%\n", (hdl->download_len * 100) / hdl->file_size);
                    if (hdl->fp) {
                        if (fwrite(hdl->recv_buf + offset, 1, ret - offset, hdl->fp) <= 0) {
                            log_info("data storge fail.\n");
                            goto _out_;
                        }
                    }
                } else {
                    hdl->reconnect_download_len += ret;
                }
            } else {
                //服务器支持带range字段的http请求
                hdl->reconnecting = 0;

                if (fwrite(hdl->recv_buf, 1, ret, hdl->fp) <= 0) {
                    log_info("data storge fail.\n");
                    goto _out_;
                }

                downloaded = (hdl->download_len * 100) / hdl->file_size;
                hdl->download_len += ret;
                new = (hdl->download_len * 100) / hdl->file_size;
                if (downloaded != new) {
                    printf("downloaded >>> %02d %%\n", new);
                }
            }
        }

        //下载完成
        if (hdl->download_len >= hdl->file_size) {
            printf("downloaded >>> 100%%\n");
            log_info("download success.\n");
            break;
        }
    }

_out_:
    //关闭网络连接
    hdl->download_ops->close(&hdl->ctx);

    //释放内存
    if (hdl->fp) {
        fclose(hdl->fp);
    }
    free(hdl->url);
    free(hdl->path);
    free(hdl->recv_buf);
    memset(hdl, 0, sizeof(struct download_hdl));
    free(hdl);
}


static int create_download_task(void *priv)
{
    struct download_hdl *hdl = NULL;
    struct download_parm *parm = (struct download_parm *)priv;

    //申请堆内存
    hdl = (struct download_hdl *)calloc(1, sizeof(struct download_hdl));
    if (hdl == NULL) {
        return -ENOMEM;
    }

    hdl->download_ops = &http_ops;
    hdl->recv_buf_size = DEFAULT_RECV_BUF_SIZE;

    hdl->url = (char *)calloc(1, strlen(parm->url) + 1);
    if (hdl->url == NULL) {
        return -ENOMEM;
    }

    hdl->path = (char *)calloc(1, strlen(parm->path) + 1);
    if (hdl->path == NULL) {
        return -ENOMEM;
    }

    hdl->recv_buf = (char *)calloc(1, hdl->recv_buf_size);
    if (hdl->recv_buf == NULL) {
        return -ENOMEM;
    }

    strcpy(hdl->url, parm->url);
    strcpy(hdl->path, parm->path);

    //http请求参数赋值
    hdl->ctx.url = hdl->url;
    hdl->ctx.cb = http_user_cb;
    hdl->ctx.priv = hdl;
    hdl->ctx.connection = "close";
    hdl->ctx.timeout_millsec = 10000;

    //创建下载线程
    return thread_fork("download_task", 20, 1024, 0, NULL, download_task, hdl);
}


static int net_file_download_test(char *url, char *path)
{
    struct download_parm parm = {0};

    if (url == NULL || url[0] == '\0') {
        return -EINVAL;
    }

    if (path == NULL || path[0] == '\0') {
        return -EINVAL;
    }

    //存储设备未就绪，直接退出
    if (!storage_device_ready()) {
        log_info("storge dev not ready.\n");
        return -1;
    }

    parm.url = url;
    parm.path = path;

    return create_download_task(&parm);
}

static void http_download_start(void *priv)
{
    int err;
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

    net_file_download_test(HTTP_DOWNLOAD_URL, HTTP_STORAGE_PATH);
}

//应用程序入口,需要运行在STA模式下
void c_main(void *priv)
{
    if (thread_fork("http_download_start", 10, 512, 0, NULL, http_download_start, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}

late_initcall(c_main);
#endif//USE_HTTP_DOWNLOAD_TEST
