#include "system/includes.h"
#include "server/video_server.h"
#include "app_config.h"
#include "asm/debug.h"
#include "asm/osd.h"
#include "asm/isc.h"
#include "os/os_api.h"
#include "camera.h"
#include "video_buf_config.h"
#include "stream_protocol.h"
#include "rt_stream_pkg.h"

/***********************************USER VIDEO*******************************************
 1、调用user_video_rec0_open,打开摄像头成功后，会回调到video_rt_usr_init
 2、数据回调：user_video_frame_callback函数
    注意：该函数不能进行延时太长操作，否则丢帧，数据回调用在发送数据或者拷贝数据
 3、调用user_video_rec0_close关闭摄像头
****************************************************************************************/


#ifdef CONFIG_USR_VIDEO_ENABLE

struct user_video_hdl {
    u8 state;
    u8 page_turning;
    u8 req_send;
    u8 *user_isc_buf;
    u8 *user_video_buf;
    u8 *user_audio_buf;
    struct server *user_video_rec;
};

static struct user_video_hdl user_handler = {0};
#define __this_user  (&user_handler)

#define USER_ISC_SBUF_SIZE     		(CONFIG_VIDEO_IMAGE_W*32*3/2)
#define USER_VIDEO_SBUF_SIZE   		USER_VREC0_FBUF_SIZE

#define USER_ISC_STATIC_BUFF_ENABLE		1	//1使用静态内部ram

#if USER_ISC_STATIC_BUFF_ENABLE
static u8 user_isc_sbuf[USER_ISC_SBUF_SIZE] SEC_USED(.sram) ALIGNE(32);//必须32对齐
#endif
#endif

#ifdef CONFIG_OSD_ENABLE
static const unsigned char user_osd_format_buf[] = "yyyy-nn-dd hh:mm:ss";//时间OSD
#endif

void set_video_rt_cb(u32(*cb)(void *buf, u32 len, u8 type));

/*
 *码率控制，根据具体分辨率设置:1000 - 6000
 */
int user_video_rec_get_abr(u32 width)
{
    if (width <= 640) {
        return 2800;
    } else {
        return 2000;
    }
}
/*
 *音视频数据回调
 */
u32 user_video_frame_callback(void *data, u32 len, u8 type)
{
    if (type == JPEG_TYPE_VIDEO) { //jpeg视频
        putchar('V');
        /*put_buf(data, len);*/
    } else if (type == PCM_TYPE_AUDIO) { //PCM音频
        putchar('A');
        /*put_buf(data, len);*/
    }
    return 0;
}
/*
 *用户VIDEO打开
 */
int user_video_rec0_open(void)
{
    int err = 0;
#ifdef CONFIG_USR_VIDEO_ENABLE
    union video_req req = {0};
    char path[48];

    if (__this_user->state) {
        printf("user already open \n");
        return 0;
    }
    if (!__this_user->user_video_buf) {
        __this_user->user_video_buf = malloc(USER_VIDEO_SBUF_SIZE);
        if (!__this_user->user_video_buf) {
            puts("no mem \n");
            return -1;
        }
    }
    if (!__this_user->user_audio_buf && VIDEO_REC_AUDIO_SAMPLE_RATE) {
        __this_user->user_audio_buf = malloc(USER_AUDIO_BUF_SIZE);
        if (!__this_user->user_audio_buf) {
            puts("no mem \n");
            return -1;
        }
    }
#if USER_ISC_STATIC_BUFF_ENABLE
    __this_user->user_isc_buf = user_isc_sbuf;
#else
    if (!__this_user->user_isc_buf) {
        __this_user->user_isc_buf = malloc(USER_ISC_SBUF_SIZE);
        if (!__this_user->user_isc_buf) {
            puts("no mem \n");
            goto __exit;
        }
    }
#endif

    if (!__this_user->user_video_rec) {
        __this_user->user_video_rec = server_open("video_server", "video0.1");
        if (!__this_user->user_video_rec) {
            goto __exit;
        }
    }

#ifdef CONFIG_VIDEO_REC_PPBUF_MODE
    req.rec.bfmode = VIDEO_PPBUF_MODE;
#endif
#ifdef  CONFIG_VIDEO_SPEC_DOUBLE_REC_MODE
    req.rec.wl80_spec_mode = VIDEO_WL80_SPEC_DOUBLE_REC_MODE;
#endif
    req.rec.picture_mode = 0;//非绘本模式
    req.rec.isc_sbuf = __this_user->user_isc_buf;
    req.rec.sbuf_size = USER_ISC_SBUF_SIZE;
    req.rec.camera_type = VIDEO_CAMERA_NORMAL;
    req.rec.channel = 1;
    req.rec.width 	= CONFIG_VIDEO_IMAGE_W;
    req.rec.height 	= CONFIG_VIDEO_IMAGE_H;
    req.rec.state 	= VIDEO_STATE_START;
    req.rec.format 	= USER_VIDEO_FMT_AVI;
    req.rec.quality = VIDEO_LOW_Q;//VIDEO_MID_Q;
    req.rec.fps = 0;
    req.rec.real_fps = 15;//帧率

    //需要音频：请写audio.sample_rate和audio.buf、audio.buf_len
    req.rec.audio.sample_rate = VIDEO_REC_AUDIO_SAMPLE_RATE;//8000 音频采样率
    req.rec.audio.channel = 1;//选择硬件1个MIC数
    req.rec.audio.channel_bit_map = 0;//选择第几个MIC，0则是版籍配置文件默认,//LADC_CH_MIC1_P_N
    req.rec.audio.volume = 100;//音频增益0-100
    req.rec.audio.buf = __this_user->user_audio_buf;//音频BUFF
    req.rec.audio.buf_len = USER_AUDIO_BUF_SIZE;//音频BUFF长度

    req.rec.abr_kbps = user_video_rec_get_abr(req.rec.width);//JPEG图片码率
    req.rec.buf = __this_user->user_video_buf;
    req.rec.buf_len = USER_VIDEO_SBUF_SIZE;
    req.rec.block_done_cb = 0;

#ifdef CONFIG_OSD_ENABLE
    struct video_text_osd text_osd = {0};
    text_osd.font_w = OSD_DEFAULT_WIDTH;//必须16对齐
    text_osd.font_h = OSD_DEFAULT_HEIGHT;//必须16对齐
    text_osd.text_format = user_osd_format_buf;
    text_osd.x = (req.rec.width - text_osd.font_w * strlen(text_osd.text_format) + 15) / 16 * 16;
    text_osd.y = (req.rec.height - text_osd.font_h + 15) / 16 * 16;
    text_osd.osd_yuv = 0xe20095;
    req.rec.text_osd = &text_osd;
#endif
#ifdef CONFIG_USR_VIDEO_ENABLE
    sprintf(path, "usr://%s", CONFIG_USR_PATH);
#else
    sprintf(path, "usr://%s", "192.168.1.1");
#endif

    err = server_request(__this_user->user_video_rec, VIDEO_REQ_REC, &req);
    if (err != 0) {
        puts("user start rec err\n");
        goto __exit;
    }
    req.rec.packet_cb = stream_packet_cb;//注册数据包回调函数进行协议转发
    err = server_request(__this_user->user_video_rec, VIDEO_REQ_SET_PACKET_CALLBACK, &req);
    if (err != 0) {
        puts("stream_packet_cb set err\n");
        goto __exit;
    }
#if 1 /*需要加上video_rt_usr.c文件则可以创建协议转发任务*/
    err = stream_protocol_task_create(path, NULL);//协议转发任务创建
    if (err != 0) {
        puts("stream_packet_cb set err\n");
        goto __exit;
    }
    set_video_rt_cb(user_video_frame_callback);
#endif
    __this_user->state = true;
    printf("user video rec open ok\n");

    return err;

__exit:
    if (__this_user->user_video_rec) {
        memset(&req, 0, sizeof(req));
        req.rec.channel = 1;
        req.rec.state = VIDEO_STATE_STOP;
        server_request(__this_user->user_video_rec, VIDEO_REQ_REC, &req);
        server_close(__this_user->user_video_rec);
        __this_user->user_video_rec = NULL;
    }
    if (__this_user->user_video_buf) {
        free(__this_user->user_video_buf);
        __this_user->user_video_buf = NULL;
    }
    if (__this_user->user_audio_buf) {
        free(__this_user->user_audio_buf);
        __this_user->user_audio_buf = NULL;
    }
#if !USER_ISC_STATIC_BUFF_ENABLE
    if (__this_user->user_isc_buf) {
        free(__this_user->user_isc_buf);
        __this_user->user_isc_buf = NULL;
    }
#endif
#endif

    return err;
}

/*
 *用户VIDEO关闭
 */
int user_video_rec0_close(void)
{
    int err = 0;
#ifdef CONFIG_USR_VIDEO_ENABLE
    union video_req req = {0};
    if (__this_user->user_video_rec) {
        req.rec.channel = 1;
        req.rec.state = VIDEO_STATE_STOP;
        err = server_request(__this_user->user_video_rec, VIDEO_REQ_REC, &req);
        if (err != 0) {
            printf("\nstop rec err 0x%x\n", err);
            return -1;
        }
        server_close(__this_user->user_video_rec);
#if 1 /*需要加上video_rt_usr.c文件则可删除协议转发任务*/
        stream_protocol_task_kill();
#endif
        __this_user->user_video_rec = NULL;
        __this_user->state = false;
        printf("user video rec close ok\n");
    }
    if (__this_user->user_video_buf) {
        free(__this_user->user_video_buf);
        __this_user->user_video_buf = NULL;
    }
    if (__this_user->user_audio_buf) {
        free(__this_user->user_audio_buf);
        __this_user->user_audio_buf = NULL;
    }
#if !USER_ISC_STATIC_BUFF_ENABLE
    if (__this_user->user_isc_buf) {
        free(__this_user->user_isc_buf);
        __this_user->user_isc_buf = NULL;
    }
#endif
#endif
    return 0;
}



