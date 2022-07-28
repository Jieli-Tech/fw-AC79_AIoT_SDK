#include "rtp/rtp_h264.h"
#include "sys_common.h"
#include "system/spinlock.h"
#include "os/os_api.h"
#include "server/rt_stream_pkg.h"
#include "app_config.h"
#include "lwip.h"
#include "lwip/sockets.h"
#include "sock_api/sock_api.h"
#include "os/os_api.h"
#include "stream_core.h"








/*
 UDP端口:2224
 UDP 每片数据包 格式:
|1byte 类型 |  1byte 保留 | 2byte  payload长度 | 4byte 序号 |  4byte  帧大小 |  4byte 偏移  | 4byte 时间戳  |  payload  |
*/
#if __SDRAM_SIZE__ >= (8 * 1024 * 1024)
#define UDP_SEND_BUF_SIZE  (44*1472)
#else
#define UDP_SEND_BUF_SIZE  (16*1472)
#endif



#define VIDEO_FRQ_60HZ  60
#define VIDEO_FRQ_30HZ  30
#define VIDEO_FRQ_15HZ  15


#define MAX_PAYLOAD (1472-sizeof(struct frm_head))//最后4字节属于h264流的
static u32 video_timestramp  = 0;
static u32 audio_timestramp  = 0;
static u32 seq = 1;
static u32 aux_frame_cnt = 0;

extern int atoi(const char *__nptr);
static int  path_analyze(struct rt_stream_info *info, const *path)
{

    char *tmp = NULL;
    char *tmp2 = NULL;
    char ip[15] = {0};
    u16 port = 0;
    tmp = strstr(path, "rtp://");
    if (!tmp) {
        return -1;
    }

    tmp += strlen("rtp://");

    tmp2 = strchr(tmp, ':');
    printf("tmp=%s  len=%d\n", tmp, tmp2 - tmp);
    strncpy(ip, tmp, tmp2 - tmp);
    port = atoi(tmp2 + 1);

    printf("remote ip:%s  port:%d\n", ip, port);


    info->addr.sin_family = AF_INET;
    info->addr.sin_addr.s_addr = inet_addr(ip);
    info->addr.sin_port = htons(port);
    return 0;

}


static void *get_sock_handler(int (*cb_func)(enum sock_api_msg_type type, void *priv), void *priv)
{
    void *fd = NULL;


    if (0) { //条件链表
        //add P2P code
    } else {

        fd = sock_reg(AF_INET, SOCK_DGRAM, 0, cb_func, priv);

        if (fd == NULL) {
            printf("%s %d->Error in socket()\n", __func__, __LINE__);
            return NULL;
        }

        u32 millsec = 100;
        sock_setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const void *)&millsec, sizeof(millsec));
        return fd;
    }
}

static struct rt_stream_info *net_rt_vpkg_open(const char *path, const char *mode)
{

    int flags = 0;
    int err;
    struct rt_stream_info *info = (struct rt_stream_info *)calloc(1, sizeof(struct rt_stream_info));

    if (info == NULL) {
        printf("%s %d->Error in malloc()\n", __func__, __LINE__);
        return NULL;
    }

    info->udp_send_buf = (char *)malloc(UDP_SEND_BUF_SIZE);

    if (info->udp_send_buf == NULL) {
        printf("%s %d->Error in malloc()\n", __func__, __LINE__);
        free(info);
        return NULL;
    }

    if (path_analyze(info, path)) {

        printf("%s %d->Error in path_analyze\n", __func__, __LINE__);
        free(info);
        return NULL;
    }

    info->fd = get_sock_handler(NULL, NULL);
    if (!info->fd) {
        printf("%s %d->Error get_sock_handler\n", __func__, __LINE__);
        free(info);
        return NULL;
    }


    return info;
err1:
    free(info->udp_send_buf);
    free(info);
    return NULL;
}

static int net_rt_vpkg_close(struct rt_stream_info *info)
{
    video_timestramp = 0;
    audio_timestramp = 0;
    aux_frame_cnt = 0;
    //os_sem_del(&mutex,OS_DEL_ALWAYS);
    sock_unreg(info->fd);
    free(info->udp_send_buf);
    free(info);

    return 0;
}


static u32 old_times = 0;
static u32 new_times = 0;

static int net_rt_send_frame(struct rt_stream_info *info, char *buffer, size_t len, u8 type)
{
    struct rt_stream_cli_info *cli = NULL;
    struct list_head *pos = NULL;
    int ret;
    int find = 0;
    char *tmp = NULL;
    int sps_data_len = 0;
    int pps_data_len = 0;

    tmp = buffer + 4;
    if (*tmp == 0x67) {

        memcpy(&sps_data_len, buffer, 4);
        sps_data_len = htonl(sps_data_len);

        memcpy(&pps_data_len, buffer + 4 + sps_data_len, 4);
        pps_data_len = htonl(pps_data_len);
        ret = rtp_h264_send_frame(info, buffer + 4, sps_data_len, 30);

        if (ret < 0) {
            puts("rt_stream_sent error!\n");
            return -1;
        }

        ret = rtp_h264_send_frame(info, buffer + sps_data_len + 4 + 4, pps_data_len, 30);

        if (ret < 0) {
            puts("rt_stream_sent error!\n");
            return -1;
        }



        ret = rtp_h264_send_frame(info, buffer + sps_data_len + pps_data_len + 12, len - sps_data_len - pps_data_len - 12, 30);

        if (ret < 0) {
            puts("rt_stream_sent error!\n");
            return -1;
        }

    } else {
        ret = rtp_h264_send_frame(info, buffer + 4, len - 4, 30);

        if (ret < 0) {
            puts("rt_stream_sent error!\n");
            return -1;
        }


    }
    return len;
}

REGISTER_NET_VIDEO_STREAM_SUDDEV(rtp_video_stream_sub) = {
    .name = "rtp",
    .open = net_rt_vpkg_open,
    .write = net_rt_send_frame,
    .close = net_rt_vpkg_close,
};


