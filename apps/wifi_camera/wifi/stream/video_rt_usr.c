#include "server/rt_stream_pkg.h"
#include "stream_core.h"
#include "sock_api/sock_api.h"
#include "app_config.h"

/**************************************************************************************************************
1、修改自定义路径，app_config.h打开CONFIG_NET_UDP_ENABLE，改写NET_USR_PATH的自定义路径
2、参照user_video_rec.c两个打开和关闭函数
3、当打开video_rt_usr_init()函数返回非NULL，打开完成之后，有数据会调用到video_rt_usr_send()函数，
	type的类型是JPEG_TYPE_VIDEO则为视频帧，PCM_TYPE_AUDIO则为音频。
**************************************************************************************************************/
static u32(*video_rt_cb)(void *, u8 *, u32);
static void *cb_priv;

void set_video_rt_cb(u32(*cb)(void *, u8 *, u32), void *priv)
{
    video_rt_cb = cb;
    cb_priv = priv;
}

static void *video_rt_usr_init(const char *path, const char *mode)
{
    //连接服务器操作
    puts("video_rt_usr_init\n");

    return calloc(1, sizeof(struct rt_stream_info));
}

static int video_rt_usr_send(void *hdr, void *data, u32 len, u8 type)
{
    struct rt_stream_info *r_info = (struct rt_stream_info *)hdr;

    if (video_rt_cb) {
        video_rt_cb(cb_priv, data, len);
    } else {
        if (type == H264_TYPE_VIDEO || type == JPEG_TYPE_VIDEO) {
            putchar('V');
        } else {
            putchar('A');
        }
    }
    return len;
}

static int video_rt_usr_uninit(void *hdr)
{
    puts("video_rt_usr_uninit\n\n\n\n\n\n");
    struct rt_stream_info *r_info = (struct rt_stream_info *)hdr;
    free(r_info);
    return 0;
}

REGISTER_NET_VIDEO_STREAM_SUDDEV(usr_video_stream_sub) = {
    .name = "usr",
    .open =  video_rt_usr_init,
    .write = video_rt_usr_send,
    .close = video_rt_usr_uninit,
};

