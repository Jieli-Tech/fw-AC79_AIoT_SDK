/********************************测试例程说明************************************
 *  功能说明：
 *      通过HTTP协议，先获取服务器上最新的固件版本信息（版本信息中一般包含有版本号、固件下载地址URL等信息），
 * 通过获取到的最新版本号和当前系统版本进行比较，如果已经是最新版本则退出升级，否则提取最新固件URL，获取最新固件进行升级。
 * 通过该方法可以实现设备开机版本自动检测、批量设备自检升级。
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
#include "http/http_cli.h"
#include "json_c/json_object.h"
#include "json_c/json_tokener.h"
#include "syscfg_id.h"

#ifdef USE_HTTP_UPGRADE_DEMO2
/** 固件版本信息URL*/
#define OTA_VERSION_URL "https://profile.jieliapp.com/license/v1/fileupdate/check2?auth_key=Jk0D2fufdROhKX6q&proj_code=jieli_wl80_test"

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

enum {
    OTA_NO_ERROR = 0,
    OTA_GET_SAME_VERSION = -80,
    OTA_GET_VERSION_FAILURE = -81,
};

//设备版本描述信息长度
#define VERSION_INTRO_LEN 64

//设备版本号长度
#define VERSION_LEN 16

/* 设备当前版本号 */
static char g_currentVersion[VERSION_LEN] = "v1.0.0-001";  //应当填写烧录时的版本号

/* 保存从服务器上获取到的最新版本信息 */
static char g_latestVersion[VERSION_LEN] = {0};

/** 保存固件升级的URL*/
#define OTA_URL_LEN 512
static char http_ota_url[OTA_URL_LEN] = {0};

/** 获取版本信息接收buf大小*/
#define HTTP_RECV_BUF_SIZE 1 * 1024

//需要在sysfg_id.h配置CFG_UPDATE_VERSION_INFO
//开机时获取固件版本号
static void get_fw_version(void)
{
    int ret;
    ret = syscfg_read(CFG_UPDATE_VERSION_INFO, (u8 *)g_currentVersion, VERSION_LEN);
    if (ret < 0) {
        //读取版本失败，使用默认版本号
        ret = syscfg_write(CFG_UPDATE_VERSION_INFO, (u8 *)g_currentVersion, VERSION_LEN);
        if (ret < 0) {
            printf("write version error!");
        } else {
            printf("No version number, initial version number(%s)\n", g_currentVersion);
        }
    } else {
        puts("\n*************************************************\n");
        printf("        *Current Firmware Version (%s)*        \n", g_currentVersion);
        puts("\n*************************************************\n");
    }
}

/** 升级成功后更新固件版本号*/
static void renew_fw_version(void)
{
    int ret;

    ret = syscfg_write(CFG_UPDATE_VERSION_INFO, (u8 *)g_latestVersion, VERSION_LEN);
    if (ret < 0) {
        printf("renew_fw_version err!/n");
    }
}

static int get_current_version(void)
{
    /* 获取设备当前版本号到全局变量g_currentVersion */
    get_fw_version();
    return 0;
}

static int get_latest_version_info(char *latestVersion, int latestVersionLen, char *latestIntro, int latestIntroLen)
{
    int ret = 0;
    int err = -1;
    http_body_obj http_body_buf;
    json_object *parse = NULL, *data = NULL;

    httpcli_ctx *ctx = (httpcli_ctx *)calloc(1, sizeof(httpcli_ctx));
    if (!ctx) {
        printf("calloc faile\n");
        return err;
    }

    memset(&http_body_buf, 0x0, sizeof(http_body_obj));

    http_body_buf.recv_len = 0;
    http_body_buf.buf_len = HTTP_RECV_BUF_SIZE;
    http_body_buf.buf_count = 1;
    http_body_buf.p = (char *) malloc(http_body_buf.buf_len * http_body_buf.buf_count);
    if (!http_body_buf.p) {
        free(ctx);
        return err;
    }

    ctx->url = OTA_VERSION_URL;
    ctx->timeout_millsec = 5000;
    ctx->priv = &http_body_buf;
    ctx->connection = "close";

    ret = httpcli_get(ctx);
    if (ret != HERROR_OK) {
        printf("get_latest_version_info : httpcli_get fail\n");
        goto EXIT;
    } else {
        if (http_body_buf.recv_len > 0) {
            /** 对从服务器收到的json格式的版本信息进行解析，获取其版本号，版本描述信息和升级固件的URL信息*/
            parse = json_tokener_parse(http_body_buf.p);

            if (200 == json_object_get_int(json_object_object_get(parse, "code"))) {
                data = json_object_object_get(parse, "data");
                if (data != NULL) {
                    /** 获取版本信息*/
                    strcpy(latestVersion, json_object_get_string(json_object_object_get(data, "version")));

                    /** 保存从服务器上获取的版本号*/
                    strcpy(g_latestVersion, latestVersion);

                    /** 获取版本描述信息*/
                    strcpy(latestIntro, json_object_get_string(json_object_object_get(data, "explain")));

                    /** 获取升级文件URL*/
                    strcpy(http_ota_url, json_object_get_string(json_object_object_get(data, "url")));

                    puts("\n*************************************************************\n");
                    printf("latestVersion:%s\n", latestVersion);
                    printf("explain:%s\n", latestIntro);
                    printf("update url:%s\n", http_ota_url);
                    puts("\n*************************************************************\n");

                    /** 获取成功*/
                    err = 0;
                }
            }
        }
    }

EXIT:
    httpcli_close(ctx);

    if (http_body_buf.p) {
        free(http_body_buf.p);
        http_body_buf.p = NULL;
    }

    if (ctx) {
        free(ctx);
        ctx = NULL;
    }

    if (parse) {
        json_object_put(parse);
        parse = NULL;
    }

    return err;
}

//获取服务器上最新版本号
int http_ota_get_version(char *version, int *verLen)
{
    if (version == NULL || verLen == NULL) {
        return OTA_GET_VERSION_FAILURE;
    }

    char latestVersion[VERSION_LEN] = {0};
    char latestVerIntro[VERSION_INTRO_LEN] = {0};

    if (get_latest_version_info(latestVersion, VERSION_LEN, latestVerIntro, VERSION_INTRO_LEN) != 0) {
        return OTA_GET_VERSION_FAILURE;
    }

    if (get_current_version() != 0) {
        return OTA_GET_VERSION_FAILURE;
    }

    if (strlen(g_currentVersion) == strlen(latestVersion) &&
        memcmp(g_currentVersion, latestVersion, strlen(latestVersion)) == 0) {
        return OTA_GET_SAME_VERSION;
    } else {
        if (version ==  NULL) {
            return OTA_GET_VERSION_FAILURE;
        }

        memcpy(version, latestVersion, strlen(latestVersion));

        *verLen = strlen(latestVersion);
    }

    return OTA_NO_ERROR;
}

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

    //更新版本号
    renew_fw_version();

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

    parm.url = http_ota_url;

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

    int ret;
    char latestVersion[VERSION_LEN] = {0}; /* 保存获取的最新版本号 */
    int verLen = 0;

    ret = http_ota_get_version(latestVersion, &verLen);
    if (ret == OTA_GET_VERSION_FAILURE) {
        printf("http_ota_get_version err!");
        return;
    }

    if (ret == OTA_GET_SAME_VERSION) {
        printf("The current version is the latest version!");
        return;
    }

    printf("latest version :[%s]\n", latestVersion);

    http_create_download_task();
}

//应用程序入口,需要运行在STA模式下
void c_main(void *priv)
{
    if (thread_fork("http_upgrade_start", 10, 4 * 1024, 0, NULL, http_upgrade_start, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}

late_initcall(c_main);

#endif //USE_HTTP_UPGRADE_DEMO2
