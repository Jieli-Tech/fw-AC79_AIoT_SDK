#include "server/rt_stream_pkg.h"
#include "stream_core.h"
#include "sock_api/sock_api.h"

/**************************************************************************************************************
当打开video_rt_usr_init()函数返回非NULL，打开完成之后，有数据会调用到video_rt_usr_send()函数，
	type的类型是JPEG_TYPE_VIDEO则为视频帧，PCM_TYPE_AUDIO则为音频。
**************************************************************************************************************/
static u32(*video_rt_cb)(void *buf, u32 len, u8 type);

void set_video_rt_cb(u32(*cb)(void *buf, u32 len, u8 type))
{
    video_rt_cb = cb;
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
        video_rt_cb(data, len, type);
    }

    return len;
}

static int video_rt_usr_uninit(void *hdr)
{
    puts("video_rt_usr_uninit\n");
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

