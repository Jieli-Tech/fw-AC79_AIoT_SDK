#include "system/includes.h"
#include "server/audio_server.h"
#include "fs/fs.h"
#include "storage_device.h"
#include "os/os_api.h"
#include "app_config.h"
#include "malloc.h"
#include <string.h>


#if 1
#define log_info(x, ...)    printf("\n\n[audio_test]###" x " ", ## __VA_ARGS__)
#else
#define log_info(...)
#endif


#define DATA_BUF_SIZE   (8*1024)


struct net_audio_hdl {
    struct server *dec_server;  //解码服务
    cbuffer_t data_cbuf;        //音频数据cycle buffer
    char *recv_buf;             //音频数据buffer
    char *ptr;					//
    u32	ptr_len;				//
    u32 recv_size;				//数据缓存buffer
    u32 total_size;             //音频文件总长度
    u32 play_size;              //当前播放进度
    OS_SEM r_sem;               //读音频数据信号量
    OS_SEM w_sem;               //写音频数据信号量
    u8 run_flag;                //播放标志
    u8 dec_ready_flag;          //解码器就绪标志位
    u8 dec_volume;              //播放音量
};

int close_audio_dec(void *priv)
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


    return 0;
}


static int net_audio_vfs_fread(void *priv, void *data, u32 len)
{
    struct net_audio_hdl *hdl = (struct net_audio_hdl *)priv;

    int rlen = 0;

    rlen = cbuf_read(&hdl->data_cbuf, data, len);

    /*printf("rlen=%d", rlen);*/
    if (rlen) {

        return  len;
    } else if (rlen == -2) {
        return -2 ;
    } else if (rlen < 0) {
        return 0;
    }

    if (rlen < (int)len) {
        rlen = len;
    }

    return rlen ;
}

static int net_audio_vfs_flen(void *priv)
{
    return 0;
}

static int net_audio_vfs_seek(void *priv, u32 offset, int orig)
{
    return 0;
}

static const struct audio_vfs_ops net_audio_vfs_ops = {
    .fread  = net_audio_vfs_fread,
    .flen   = net_audio_vfs_flen,
    .fseek  = net_audio_vfs_seek,
};

int open_audio_dec(void *priv)
{
    union audio_req req = {0};
    struct net_audio_hdl *hdl = (struct net_audio_hdl *)priv;

    memset(&req, 0, sizeof(union audio_req));


    req.dec.output_buf      = NULL;
    req.dec.volume          = hdl->dec_volume;
    req.dec.output_buf_len  = 8 * 1024;
    req.dec.channel         = 1;
    req.dec.sample_rate     = 8000;
    req.dec.priority        = 1;
    req.dec.orig_sr         = 0;
    req.dec.vfs_ops         = &net_audio_vfs_ops;
    req.dec.file            = (FILE *)hdl;
    req.dec.dec_type 		= "pcm";
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
static int net_audio_buf_write(void *priv, char *data, u32 len)
{
    struct net_audio_hdl *hdl = (struct net_audio_hdl *)priv;
    int wlen = 0;
    u32 *buf = data;

    wlen = cbuf_write(&hdl->data_cbuf, buf, len);

    /*printf("wlen=%d", wlen);*/
    if (wlen) {
        if (!hdl->dec_ready_flag) {
            hdl->dec_ready_flag = 1;
            open_audio_dec(hdl);
        }
    }

    return wlen;
}
void *open_audio_server(void)
{
    struct net_audio_hdl *hdl = (struct net_audio_hdl *)calloc(1, sizeof(struct net_audio_hdl));

    if (hdl == NULL) {
        return NULL;
    }

    hdl->recv_buf = (char *)calloc(1, DATA_BUF_SIZE);

    if (hdl->recv_buf == NULL) {
        return NULL;
    }

    //初始化cycle buffer
    cbuf_init(&hdl->data_cbuf, hdl->recv_buf, DATA_BUF_SIZE);

    /*os_sem_create(&hdl->r_sem, 0);*/
    /*os_sem_create(&hdl->w_sem, 0);*/

    hdl->dec_volume = 90;
    hdl->dec_ready_flag = 0;
    hdl->dec_server = server_open("audio_server", "dec");

    if (!hdl->dec_server) {
        log_info("audio server open fail.\n");
        return NULL;
    } else {
        log_info("open success");
    }

    return hdl;
}
void rx_play_voice(void *priv, void *buf, u32 len)
{
    struct net_audio_hdl *hdl = (struct net_audio_hdl *)priv;

    net_audio_buf_write(hdl, buf, len);
}
