/********************************测试例程说明************************************
 *  功能说明：
 *      通过HTTP协议，播放网络音频文件，并将音频文件存储至指定路径（可选）。
 *
 *  使用说明：
 *      确保网络连接成功、存储设备就绪，指定网络音频路径URL、存储路径PATH，
 *      调用 net_audio_play_test()函数播放网络音频，即可开始下载。
 * ******************************************************************************/
#include "system/includes.h"
#include "http/http_cli.h"
#include "server/audio_server.h"
#include "fs/fs.h"
#include <stdlib.h>
#include "storage_device.h"
#include "device/device.h"//u8
#include "server/video_dec_server.h"//fopen
#include "system/includes.h"//GPIO
#include "asm/port_waked_up.h"
#include "asm/gpio.h"
#include "app_config.h"
#include "wifi/wifi_connect.h"

#ifdef USE_WIFI_MUSIC_PLAY

/*#define WIFI_MUSIC_PATH    "http://music.163.com/song/media/outer/url?id=66282.mp3"*/
#define WIFI_MUSIC_PATH    "https://test02.jieliapp.com/tmp/2021/12/31/music_test.mp3"

#if 1
#define log_info(x, ...)    printf("\n\n[audio_test]###" x " ", ## __VA_ARGS__)
#else
#define log_info(...)
#endif

/* #define SAVE_AUDIO_FILE */
#define DATA_BUF_SIZE   (50*1024)
#define RECV_BUF_SIZE   (512)

#define SHUTDOWN_CMD    (1000)

struct net_audio_hdl {
    char *url;                  //音频链接
    char *path;                 //文件存放路径
    FILE *fp;                   //文件指针
    struct server *dec_server;  //解码服务
    u8 *data_buf;               //音频数据buffer
    cbuffer_t data_cbuf;        //音频数据cycle buffer
    u32 recv_size;              //已接收音频数据长度
    char *recv_buf;             //数据缓存buffer
    u32 total_size;             //音频文件总长度
    u32 play_size;              //当前播放进度
    OS_SEM r_sem;               //读音频数据信号量
    OS_SEM w_sem;               //写音频数据信号量
    u8 run_flag;                //播放标志
    u8 dec_ready_flag;          //解码器就绪标志位
    u8 dec_status;              //解码器运行标志位
    u8 dec_volume;              //播放音量
    int recv_task_pid;          //recv_task句柄
    int connect_flag;           //网络连接标志
    httpcli_ctx *ctx;           //HTTP请求参数
    const struct net_download_ops *download_ops; //http操作集
};


struct play_parm {
    char *url;                  //音频链接
    char *path;                 //文件存放路径
    u8 volume;                  //播放音量
};


static int net_audio_buf_write(void *priv, char *data, u32 len);
static int net_audio_vfs_fread(void *priv, void *data, u32 len);
static int net_audio_vfs_flen(void *priv);
static int net_audio_vfs_seek(void *priv, u32 offset, int orig);
static void dec_server_event_handler(void *priv, int argc, int *argv);
static int open_audio_dec(void *priv);
static int close_audio_dec(void *priv);
static void recv_net_audio_data_task(void *priv);
static s32 httpin_user_cb(void *_ctx, void *buf, u32 size, void *priv, httpin_status status);
static void net_audio_play_task(void *priv);


//将音频数据写入data_cbuf
static int net_audio_buf_write(void *priv, char *data, u32 len)
{
    struct net_audio_hdl *hdl = (struct net_audio_hdl *)priv;
    u32 wlen = 0;

    while (hdl->run_flag) {
        wlen = cbuf_write(&hdl->data_cbuf, data, len);
        if (wlen) {
            os_sem_set(&hdl->w_sem, 0);
            os_sem_post(&hdl->r_sem);
            /*putchar('('); */

            hdl->recv_size += len;

            //音频下载成功，或缓存成功，打开解码器，开始播放
            if ((hdl->recv_size >= hdl->total_size || hdl->recv_size >= 40 * 1024) && hdl->dec_status == 0) {
                log_info(">>>>>>>>>>>audio dec open!\n");
                open_audio_dec(hdl);
                hdl->dec_status = 1;
            }
            break;
        }

        log_info(">>>>>write pend......\n");
        os_sem_pend(&hdl->w_sem, 0);
    }

    return wlen;
}


static int net_audio_vfs_fread(void *priv, void *data, u32 len)
{
    struct net_audio_hdl *hdl = (struct net_audio_hdl *)priv;
    u32 rlen = 0;

    while (hdl->run_flag && hdl->dec_ready_flag) {
        if (hdl->total_size - hdl->play_size < 512) {
            len = hdl->total_size - hdl->play_size;
        }

        rlen = cbuf_read(&hdl->data_cbuf, data, len);
        if (rlen) {
            hdl->play_size += rlen;

            //发送写信号
            os_sem_set(&hdl->r_sem, 0);
            os_sem_post(&hdl->w_sem);
            /* putchar(')'); */

            if (hdl->play_size >= hdl->total_size) {
                hdl->run_flag = 0;
                hdl->dec_ready_flag = 0;
                log_info("net audio play complete.\n");
                log_info("play size = %d.\n", hdl->play_size);
            }
            break;
        }

        /* log_info(">>>>>read pend......\n"); */
        os_sem_pend(&hdl->r_sem, 0);
    }

    return ((hdl->play_size >= hdl->total_size) ? 0 : len);
}


static int net_audio_vfs_flen(void *priv)
{
    struct net_audio_hdl *hdl = (struct net_audio_hdl *)priv;
    return hdl->total_size;
}


static int net_audio_vfs_seek(void *priv, u32 offset, int orig)
{
    struct net_audio_hdl *hdl = (struct net_audio_hdl *)priv;

    if (offset > 0) {
        hdl->dec_ready_flag = 1; //解码器开始偏移数据后，再开始播放，避免音频播放开端吞字
    }
    return 0;
}


static const struct audio_vfs_ops net_audio_vfs_ops = {
    .fread  = net_audio_vfs_fread,
    .flen   = net_audio_vfs_flen,
    .fseek  = net_audio_vfs_seek,
};


static void dec_server_event_handler(void *priv, int argc, int *argv)
{
    struct net_audio_hdl *hdl = (struct net_audio_hdl *)priv;
    int msg = 0;

    switch (argv[0]) {
    case AUDIO_SERVER_EVENT_ERR:
        log_info("AUDIO_SERVER_EVENT_ERR\n");
        break;

    case AUDIO_SERVER_EVENT_END:
        log_info("AUDIO_SERVER_EVENT_END\n");
        msg = SHUTDOWN_CMD;
        os_taskq_post("net_audio_play_task", 1, msg);

        break;

    case AUDIO_SERVER_EVENT_CURR_TIME:
        log_d("play_time: %d\n", argv[1]);
        break;

    default:
        break;
    }
}


static int open_audio_dec(void *priv)
{
    union audio_req req = {0};
    struct net_audio_hdl *hdl = (struct net_audio_hdl *)priv;

    req.dec.cmd             = AUDIO_DEC_OPEN;
    req.dec.volume          = hdl->dec_volume;
    req.dec.output_buf      = NULL;
    req.dec.output_buf_len  = 12 * 1024;
    req.dec.channel         = 0;
    req.dec.sample_rate     = 0;
    req.dec.priority        = 1;
    req.dec.vfs_ops         = &net_audio_vfs_ops;
    req.dec.file            = (FILE *)hdl;
    req.dec.dec_type 		= "mp3";
    req.dec.sample_source   = "dac";

    //打开解码器
    if (server_request(hdl->dec_server, AUDIO_REQ_DEC, &req) != 0) {
        return -1;
    }

    //开始解码
    req.dec.cmd = AUDIO_DEC_START;
    if (server_request(hdl->dec_server, AUDIO_REQ_DEC, &req) != 0) {
        return -1;
    }

    return 0;
}


static int close_audio_dec(void *priv)
{
    union audio_req req = {0};
    struct net_audio_hdl *hdl = (struct net_audio_hdl *)priv;

    req.dec.cmd = AUDIO_DEC_STOP;
    if (hdl->dec_server) {

        //关闭解码器
        log_info("stop dec.\n");
        server_request(hdl->dec_server, AUDIO_REQ_DEC, &req);

        //关闭解码服务
        log_info("close audio_server.\n");
        server_close(hdl->dec_server);
        hdl->dec_server = NULL;
    }

    //关闭文件
#ifdef SAVE_AUDIO_FILE
    if (!hdl->fp) {
        fclose(hdl->fp);
        fdelete(hdl->fp);
    }
#endif

    return 0;
}


static void shutdown_audio_play(void *priv)
{
    struct net_audio_hdl *hdl = (struct net_audio_hdl *)priv;

    //关闭音频
    close_audio_dec(hdl);


    //关闭数据接收线程
    if (hdl->recv_task_pid) {
        log_info("kill recv task");
        thread_kill(&hdl->recv_task_pid, KILL_FORCE);
    }

    //关闭http
    if (hdl->connect_flag) {
        log_info("close http.\n");
        hdl->download_ops->close(hdl->ctx);
        hdl->connect_flag = 0;
    }

    //关闭播放线程
    /* if (hdl->play_task_pid) { */
    /*     thread_kill(hdl->play_task_pid, KILL_WAIT); */
    /* } */

    //释放内存
    log_info("free memory start.\n");

    if (hdl->ctx) {
        free(hdl->ctx);
    }

    if (hdl->data_buf) {
        free(hdl->data_buf);
    }

    if (hdl->recv_buf) {
        free(hdl->recv_buf);
    }

    if (hdl->url) {
        free(hdl->url);
    }

    if (hdl->path) {
        free(hdl->path);
    }

    if (hdl) {
        memset(hdl, 0, sizeof(struct net_audio_hdl));
        free(hdl);
    }

    log_info("free memory stop.\n");
}


static void recv_net_audio_data_task(void *priv)
{
    struct net_audio_hdl *hdl = (struct net_audio_hdl *)priv;
    int ret = 0;


_reconnect_:
    //发起连接请求，建立socket连接
    ret = hdl->download_ops->init(hdl->ctx);
    if (ret != HERROR_OK) {
        if (hdl->ctx->req_exit_flag == 0) {
            os_time_dly(50);
            goto _reconnect_;
        }
    } else {
        hdl->connect_flag = 1;
    }

    while (hdl->ctx->req_exit_flag == 0) {
        //读取数据
        ret = hdl->download_ops->read(hdl->ctx, hdl->recv_buf, sizeof(hdl->recv_buf));
        if (hdl->ctx->req_exit_flag) {
            goto _out_;
        }

        if (ret < 0) {  //读取数据失败
            log_info("recv fail.\n");
        } else {
            net_audio_buf_write(hdl, hdl->recv_buf, ret);
            /*log_info(">>>>>>>>>>>>>>>>>11net_audio_buf_write\n");*/

#ifdef SAVE_AUDIO_FILE
            //将音频数据存储到指定路径
            if (fwrite(hdl->fp, buf, size) <= 0) {
                log_info("data storge fail.\n");
            }
#endif
        }

        if (hdl->recv_size >= hdl->total_size) {
            break;
        }
    }

_out_:
    //关闭网络连接
    hdl->download_ops->close(hdl->ctx);
    hdl->connect_flag = 0;
}


static s32 httpin_user_cb(void *_ctx, void *buf, u32 size, void *priv, httpin_status status)
{
    u32 wlen = 0;
    httpcli_ctx *ctx = (httpcli_ctx *)_ctx;
    struct net_audio_hdl *hdl = (struct net_audio_hdl *)priv;

    if (status == HTTPIN_HEADER) {
        if (ctx->content_length) {
            hdl->run_flag = 1;
            hdl->total_size = ctx->content_length;
#ifdef SAVE_AUDIO_FILE
            //打开文件存储路径
            hdl->fp = fopen(hdl->path, "w+");
            if (hdl->fp == NULL) {
                log_info("storge path open error.\n");
                return -1;
            }
#endif
            log_info("HTTPIN_HEADER content_length = %d\n", ctx->content_length);
        }

        if (ctx->content_type[0]) {
            log_info("HTTPIN_HEADER content_type = %s\n", ctx->content_type);
        }
    }

    return 0;
}


static void net_audio_play_task(void *priv)
{
    struct play_parm *parm = (struct play_parm *)priv;
    struct net_audio_hdl *hdl = NULL;
    int msg[32] = {0};
    int res = 0;

    //申请堆内存
    hdl = (struct net_audio_hdl *)calloc(1, sizeof(struct net_audio_hdl));
    if (hdl == NULL) {
        return;
    }

    hdl->ctx = (httpcli_ctx *)calloc(1, sizeof(httpcli_ctx));
    if (hdl->ctx == NULL) {
        return;
    }

    hdl->data_buf = (char *)calloc(1, DATA_BUF_SIZE);
    if (hdl->data_buf == NULL) {
        return;
    }

    hdl->recv_buf = (char *)calloc(1, RECV_BUF_SIZE);
    if (hdl->recv_buf == NULL) {
        return;
    }

    hdl->url = (char *)calloc(1, strlen(parm->url) + 1);
    if (hdl->url == NULL) {
        return;
    }

#ifdef SAVE_AUDIO_FILE
    hdl->path = (char *)calloc(1, strlen(parm->path) + 1);
    if (hdl->url == NULL) {
        return;
    }
    strcpy(hdl->path, parm->path);
#endif
    strcpy(hdl->url, parm->url);

    //初始化cycle buffer
    cbuf_init(&hdl->data_cbuf, hdl->data_buf, DATA_BUF_SIZE);

    os_sem_create(&hdl->r_sem, 0);
    os_sem_create(&hdl->w_sem, 0);


    hdl->dec_volume = parm->volume;
    hdl->download_ops = &http_ops;

    //http请求参数赋值
    hdl->ctx->url = hdl->url;
    hdl->ctx->cb = httpin_user_cb;
    hdl->ctx->priv = hdl;
    hdl->ctx->connection = "close";
    hdl->ctx->timeout_millsec = 10000;

    hdl->dec_server = server_open("audio_server", "dec");
    if (!hdl->dec_server) {
        log_info("audio server open fail.\n");
        return;
    } else {
        log_info("open success");
    }

    log_info(">>>>>>>>%d\n", __LINE__);
    server_register_event_handler(hdl->dec_server, hdl, dec_server_event_handler);
    log_info(">>>>>>>>%d\n", __LINE__);

    thread_fork("recv_net_audio_data_task", 20, 1024 * 4, 0, &hdl->recv_task_pid, recv_net_audio_data_task, hdl);

    for (;;) {
        res = os_task_pend("taskq", msg, ARRAY_SIZE(msg));

        if (msg[1] == SHUTDOWN_CMD) {
            log_info("recv shutdown req.\n");

            shutdown_audio_play(hdl);
            log_info("kill self.\n");
            return;
        }
    }
}


static void shutdown_audio_play_test(void)
{
    int msg = 0;
    msg = SHUTDOWN_CMD;
    os_taskq_post("net_audio_play_task", 1, msg);
}


void net_audio_play_test(char *url, char *path, u8 volume)
{
    struct play_parm parm = {0};

    if (url == NULL || url[0] == '\0') {
        return;
    }

#ifdef SAVE_AUDIO_FILE
    if (path == NULL || path[0] == '\0') {
        return;
    }

    //存储设备未就绪，直接退出
    if (!storage_device_ready()) {
        log_info("storge dev not ready.\n");
        return;
    }
#endif

    parm.url = (char *)calloc(1, strlen(url) + 1);
    if (parm.url == NULL) {
        return;
    }

#ifdef SAVE_AUDIO_FILE
    parm.path = (char *)calloc(1, strlen(path) + 1);
    if (parm.url == NULL) {
        return;
    }
    strcpy(parm.path, path);
#endif
    strcpy(parm.url, url);

    parm.volume = volume;

    thread_fork("net_audio_play_task", 20, 1024, 512, NULL, net_audio_play_task, &parm);

    //强制停止播放测试
    /*Port_Wakeup_Reg(EVENT_PA8, IO_PORTA_08, EDGE_POSITIVE, shutdown_audio_play_test, NULL);*/
}

static void wifi_music_test_init(void *priv)
{
    u8 state;
    while (1) {
        printf("wait the network ok...\n");
        state = wifi_get_sta_connect_state();
        if (WIFI_STA_NETWORK_STACK_DHCP_SUCC == state) {
            printf("Network connection is successful!\n");
            break;
        }
        os_time_dly(200);
    }
    net_audio_play_test(WIFI_MUSIC_PATH, CONFIG_STORAGE_PATH, 80);
}

static int wifi_music_test_task_init(void)
{
    puts("wifi_music_test_task_init \n\n");
    return thread_fork("wifi_music_test_init", 11, 1024, 0, 0, wifi_music_test_init, NULL);
}
late_initcall(wifi_music_test_task_init);

#endif


