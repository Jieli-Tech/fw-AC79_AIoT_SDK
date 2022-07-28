#include "server/rt_stream_pkg.h"
#include "server/stream_core.h"

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

    struct rt_stream_info *r_info = calloc(1, sizeof(struct rt_stream_info));
    if (r_info == NULL) {
        printf("%s malloc fail\n", __FILE__);
        return NULL;
    }

    return (void *)r_info;
}

static int video_rt_usr_send(void *hdr, void *data, u32 len, u8 type)
{
    struct rt_stream_info *r_info = (struct rt_stream_info *)hdr;

    if (video_rt_cb) {
        video_rt_cb(cb_priv, data, len);
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
