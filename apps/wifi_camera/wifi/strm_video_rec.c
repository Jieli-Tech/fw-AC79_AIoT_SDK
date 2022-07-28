#include "strm_video_rec.h"
#include "asm/osd.h"
#include "app_config.h"


static struct strm_video_hdl fv_rec_handler;

#define AUDIO_VOLUME	64

#define __this_strm 	(&fv_rec_handler)

#define STRM_USE_NET_VIDEO_BUFF	1 //当使用共用视频buf时:1

extern int net_video_rec_get_uvc_id(void);
extern int net_video_rec_state(void);
extern void *get_video_rec0_handler(void);
extern void *get_video_rec1_handler(void);
static int fv_video_rec_start(void);
static int fv_video_rec_stop(void);
static int fv_video_rec_close(char close);
extern int db_select(const char *name);

static const u16 strm_rec_pix_w[] = {1280, 640};
static const u16 strm_rec_pix_h[] = {720,  480};
static const unsigned char net_osd_format_buf[] = "yyyy-nn-dd hh:mm:ss";

static struct fenice_source_info s_info = {
    .type = STRM_SOURCE_VIDEO0,//摄像头类型
    .width = 640,//分辨率
    .height = 480,
    .fps = STRM_VIDEO_REC_FPS0,//帧率
    .sample_rate = 0,//采样率，默认配置为0
    .channel_num = 1,//通道数
};

/*码率控制，根据具体分辨率设置*/
static int strm_video_rec_get_abr(u32 width)
{
    if (width <= 384) {
        return 1000;
    } else if (width <= 640) {
        return 2000;//2000;
    } else if (width <= 1280) {
        return 2500;
        /* return 10000; */
    } else if (width <= 1920) {
        return 3000;
    } else {
        return 3000;
    }
}
int strm_video_rec_get_fps(void)
{
    return 25;
}
int strm_video_rec_get_list_vframe(void)
{
    return __this_strm->fbuf_fcnt;
}
void strm_video_rec_pkg_get_video_in_frame(char *fbuf, u32 frame_size)
{
    __this_strm->fbuf_fcnt++;
    __this_strm->fbuf_ffil += frame_size;
}
void strm_video_rec_pkg_get_video_out_frame(char *fbuf, u32 frame_size)
{
    if (__this_strm->fbuf_fcnt) {
        __this_strm->fbuf_fcnt--;
    }
    if (__this_strm->fbuf_ffil) {
        __this_strm->fbuf_ffil -= frame_size;
    }
}
int strm_video_buff_set_frame_cnt(void)
{
#ifdef STRM_VIDEO_BUFF_FRAME_CNT
    return STRM_VIDEO_BUFF_FRAME_CNT;
#else
    return 0;
#endif
}
int strm_video_rec_get_drop_fp(void)
{
#ifdef STRM_VIDEO_REC_DROP_REAl_FP
    return STRM_VIDEO_REC_DROP_REAl_FP;
#else
    return 0;
#endif
}
int strm_video_rec_get_state(void)
{
    return 0;
}

static int fv_video_rec_open(void)
{
    int ret;
    int id = 0;
    int buf_size[] = {VREC0_FBUF_SIZE, VREC1_FBUF_SIZE, VREC2_FBUF_SIZE};
    int net_buf_size[] = {NET_VREC0_FBUF_SIZE, NET_VREC1_FBUF_SIZE};

    if (__this_strm->state == NET_VIDREC_STA_START) {
        printf("video opened \n\n");
        return 0;
    }

    if (s_info.type == STRM_SOURCE_VIDEO1) {
#ifdef CONFIG_VIDEO1_ENABLE
        id = 1;
#endif
#ifdef CONFIG_VIDEO2_ENABLE
        id = 2;
#endif
    }
#if STRM_USE_NET_VIDEO_BUFF
    //当视频buf使用静态buf时，可以打开
    extern u8 net_video_buf[NET_VREC0_FBUF_SIZE];
    extern u8 net_audio_buf[NET_AUDIO_BUF_SIZE] ;

    __this_strm->video_buf_size = NET_VREC0_FBUF_SIZE;
    __this_strm->video_buf = net_video_buf;
    __this_strm->audio_buf_size = NET_AUDIO_BUF_SIZE;
    __this_strm->audio_buf = net_audio_buf;
#else
    __this_strm->video_buf_size = net_buf_size[id ? 1 : 0];
    if (!__this_strm->video_buf) {
        __this_strm->video_buf = malloc(__this_strm->video_buf_size);
        if (!__this_strm->video_buf) {
            printf("malloc fv_v0_buf err\n");
            return -1;
        }
    }
    __this_strm->audio_buf_size = NET_AUDIO_BUF_SIZE;
    if (!__this_strm->audio_buf) {
        __this_strm->audio_buf = malloc(__this_strm->audio_buf_size);
        if (!__this_strm->audio_buf) {
            printf("malloc fv_audio_buf err\n");
            free(__this_strm->video_buf);
            return -ENOMEM;
        }
    }
#endif
    ret = fv_video_rec_start();
    return ret;
}

static void strm_video_rec_server_event_handler(void *priv, int argc, int *argv)
{
    /*
     *该回调函数会在录像过程中，写卡出错被当前录像APP调用，例如录像过程中突然拔卡
     */
    switch (argv[0]) {
    case VIDEO_SERVER_PKG_NET_ERR:
        fv_video_rec_close(1);
        break;
    default :
        break;
    }
}
static int fv_video_rec_start(void)
{
    int err = 0;
    char name[32] = {0};
    union video_req req = {0};
    struct video_text_osd text_osd;
    u16 max_one_line_strnum;
    u16 osd_line_num;
    u8 id;
    u8 ch;

    __this_strm->fbuf_fcnt = 0;
    __this_strm->fbuf_ffil = 0;

    req.rec.width 	= s_info.width;
    req.rec.height 	= s_info.height;
    req.rec.IP_interval = 0;
    req.rec.three_way_type = 0;
    req.rec.camera_type = VIDEO_CAMERA_NORMAL;
    ch = 1;

    if (s_info.type == STRM_SOURCE_VIDEO0) {
        printf("----ACTION_VIDEO0_OPEN_RT_STREAM-----\n\n");
        puts("start_video_rec0 \n");
        __this_strm->video_id = 0;
        if (!__this_strm->video_rec0) {
            sprintf(name, "video0.%d", 2);
            __this_strm->video_rec0 = server_open("video_server", name);
            if (!__this_strm->video_rec0) {
                return -1;
            }
            server_register_event_handler(__this_strm->video_rec0, NULL, strm_video_rec_server_event_handler);
        }
    } else {
        printf("err unknown video type !!!!!\n\n");
        return -EINVAL;
    }

    __this_strm->width = req.rec.width;
    __this_strm->height = req.rec.height;

#ifdef CONFIG_VIDEO_REC_PPBUF_MODE
    req.rec.bfmode = VIDEO_PPBUF_MODE;
#endif
#ifdef  CONFIG_VIDEO_SPEC_DOUBLE_REC_MODE
    req.rec.wl80_spec_mode = VIDEO_WL80_SPEC_DOUBLE_REC_MODE;
#endif
#if STRM_USE_NET_VIDEO_BUFF
#define NET_ISC_SBUF_SIZE     	(CONFIG_VIDEO_IMAGE_W*32*3/2)/*720P内部ISC BUFF采用内部ram*/
    extern u8 net_isc_sbuf[NET_ISC_SBUF_SIZE];
    req.rec.isc_sbuf = net_isc_sbuf;
    req.rec.sbuf_size = NET_ISC_SBUF_SIZE;
#endif

    req.rec.state 	= VIDEO_STATE_START;

    req.rec.format  = STRM_VIDEO_FMT_AVI;
    req.rec.channel = __this_strm->channel = ch;
    req.rec.quality = VIDEO_LOW_Q;
    req.rec.fps 	= 0;
    req.rec.real_fps    = strm_video_rec_get_fps();
    req.rec.audio.sample_rate = s_info.sample_rate;
    req.rec.audio.channel 	= 1;
    req.rec.audio.volume    = AUDIO_VOLUME;
    req.rec.audio.buf = __this_strm->audio_buf;
    req.rec.audio.buf_len = __this_strm->audio_buf_size;
    req.rec.pkg_mute.aud_mute = !db_select("mic");
    req.rec.abr_kbps = strm_video_rec_get_abr(req.rec.width);
    req.rec.roi.roio_xy = (req.rec.height * 5 / 6 / 16) << 24 | (req.rec.height * 3 / 6 / 16) << 16 | (req.rec.width * 5 / 6 / 16) << 8 | (req.rec.width) * 1 / 6 / 16;
    req.rec.roi.roio_ratio = 256 * 70 / 100 ;
    req.rec.roi.roi1_xy = 0;
    req.rec.roi.roi2_xy = 0;
    req.rec.roi.roi3_xy = (1 << 24) | (0 << 16) | ((req.rec.width / 16) << 8) | 0;
    req.rec.roi.roio_ratio1 = 0;
    req.rec.roi.roio_ratio2 = 0;
    req.rec.roi.roio_ratio3 = 256 * 80 / 100;


    text_osd.font_w = OSD_DEFAULT_WIDTH;//必须16对齐
    text_osd.font_h = OSD_DEFAULT_HEIGHT;//必须16对齐

#if 1
    text_osd.text_format = NULL;//关闭OSD时间水印
#else
    text_osd.text_format = (char *)net_osd_format_buf;
#endif
    text_osd.x = (req.rec.width - text_osd.font_w * strlen(net_osd_format_buf) + 15) / 16 * 16;
    text_osd.y = (req.rec.height - text_osd.font_h + 15) / 16 * 16;
    text_osd.osd_yuv = 0xe20095;

    text_osd.font_matrix_table = NULL;
    text_osd.font_matrix_base = NULL;
    text_osd.font_matrix_len = 0;

    /* if (db_select("dat")) { */
    /* req.rec.text_osd = &text_osd; */
    /* } */

    req.rec.slow_motion = 0;
#ifdef CONFIG_VIDEO1_ENABLE
    req.rec.tlp_time = 0;
#endif
#ifdef CONFIG_VIDEO2_ENABLE
    req.rec.tlp_time = 0;//db_select("gap");
#endif

    if (req.rec.tlp_time && (req.rec.camera_type != VIDEO_CAMERA_UVC)) {
        req.rec.real_fps = 1000 / req.rec.tlp_time;

    }
    if (req.rec.slow_motion || req.rec.tlp_time) {
        req.rec.audio.sample_rate = 0;
        req.rec.audio.channel 	= 0;
        req.rec.audio.volume    = 0;
        req.rec.audio.buf = 0;
        req.rec.audio.buf_len = 0;
    }

    req.rec.buf = __this_strm->video_buf;
    req.rec.buf_len = __this_strm->video_buf_size;
    req.rec.rec_small_pic 	= 0;
    req.rec.cycle_time = 0;

    if (s_info.type == STRM_SOURCE_VIDEO0) {
        err = server_request(__this_strm->video_rec0, VIDEO_REQ_REC, &req);
    }
    if (err != 0) {
        puts("\n\n\nstart rec2 err\n\n\n");
        return -EINVAL;
    }
    __this_strm->state = NET_VIDREC_STA_START;
    return 0;
}

static int fv_video_rec_stop(void)
{
    union video_req req = {0};
    int err;

    req.rec.channel = __this_strm->channel;  /* video0的sd卡录像为:channel0,所以这里不能在占用channel0 */
    req.rec.state = VIDEO_STATE_STOP;
    if (s_info.type == STRM_SOURCE_VIDEO0) {
        if (__this_strm->video_rec0) {
            printf("----ACTION_VIDEO0_CLOSE_RT_STREAM-----\n\n");
            err = server_request(__this_strm->video_rec0, VIDEO_REQ_REC, &req);
            if (err != 0) {
                printf("ERR:stop video rec0 err 0x%x\n", err);
                return -1;
            }
        }
    }
    __this_strm->fbuf_fcnt = 0;
    __this_strm->fbuf_ffil = 0;
    printf("fv_video_rec_stop ok \n");
    return 0;
}


static int fv_video_rec_close(char close)
{
    int err;

    if (__this_strm->state != NET_VIDREC_STA_START) {
        printf("err : __this_strm->state : %d  \n", __this_strm->state);
        return 0;
    }

    __this_strm->video_id = 0;
    err = fv_video_rec_stop();
    if (err) {
        printf("fv_video_rec_stop err !!!\n");
        return -1;
    }

    if (s_info.type == STRM_SOURCE_VIDEO0) {
        if (__this_strm->video_rec0 && close && __this_strm->state == NET_VIDREC_STA_START) {
            __this_strm->state = NET_VIDREC_STA_STARTING;
            server_close(__this_strm->video_rec0);
            __this_strm->video_rec0 = NULL;
        }
    }
#if !STRM_USE_NET_VIDEO_BUFF
    //当不使用共用视频buf时，必须关闭
    if (__this_strm->video_buf) {
        free(__this_strm->video_buf);
        __this_strm->video_buf = NULL;
    }
    if (__this_strm->audio_buf) {
        free(__this_strm->audio_buf);
        __this_strm->audio_buf = NULL;
    }
#endif

    __this_strm->state = NET_VIDREC_STA_STOP;

    printf("stream_media close \n\n");
    return 0;
}
void fenice_video_rec_open(void)
{
    int ret;

    ret = fv_video_rec_open();
    if (ret) {
        printf("fv_video_rec_open err!!!\n");
    }
}
void fenice_video_rec_close(void)
{
    int ret;
    ret = fv_video_rec_close(1);
    if (ret) {
        printf("fv_video_rec_close err!!!\n");
    }
    //RTSP恢复默认
    s_info.type 		= STRM_SOURCE_VIDEO0;//摄像头类型
    s_info.width 		= 640;//分辨率
    s_info.height 		= 480;
    s_info.fps 			= STRM_VIDEO_REC_FPS0;//帧率
    s_info.sample_rate 	= 0;//采样率，默认配置为0
    s_info.channel_num 	= 1;//通道数
}
int strm_video_rec_open(void)//主要用于录像控制RTSP
{
    int ret = 0;
    if (__this_strm->is_open) {
        ret = fv_video_rec_open();
    }
    return ret;
}
int strm_video_rec_close(u8 close)//主要用于录像控制RTSP
{
    int ret = 0;
    if (__this_strm->is_open) {
        ret = fv_video_rec_close(close);
    }
    return ret;
}
/* 用于开启实时流时,stream_media_server回调 */
int fenice_video_rec_setup(void)
{
    if (!__this_strm->is_open) {//注意：RTSP可能会多次调用open，务必加判断，防止多次open！
        __this_strm->is_open = TRUE;
        fenice_video_rec_open();
    }
    return 0;
}

/* 用于实时流异常退出时,stream_media_server回调 */
int fenice_video_rec_exit(void)
{
    if (__this_strm->is_open) {//注意：RTSP可能会多次调用close，务必加判断，防止多次close！
        __this_strm->is_open = FALSE;
        fenice_video_rec_close();
    }
    return 0;
}

void *get_strm_video_rec_handler(void)
{
    return (void *)&fv_rec_handler;
}

int fenice_set_media_info(struct fenice_source_info *info)
{
    s_info.type = info->type == 0 ? STRM_SOURCE_VIDEO0 : STRM_SOURCE_VIDEO1;
    s_info.width = info->width;
    s_info.height = info->height;
    __this_strm->width = s_info.width;
    __this_strm->height = s_info.height;
    printf("strm media info : %s , w : %d , h : %d \n"
           , s_info.type == STRM_SOURCE_VIDEO1 ? "video1/2" : "video0"
           , s_info.width
           , s_info.height);
    return 0;
}

int fenice_get_audio_info(struct fenice_source_info *info)
{
    s_info.sample_rate = VIDEO_REC_AUDIO_SAMPLE_RATE;
    info->width = s_info.width;
    info->height = s_info.height;
    info->fps = s_info.fps;
    info->sample_rate = s_info.sample_rate;//rtsp选择打开,将采样率设置为8000
    info->channel_num = s_info.channel_num;
    printf("strm audio info : %d , %d\n", info->sample_rate, info->channel_num);
    return 0;
}

int fenice_get_video_info(struct fenice_source_info *info)
{
    __this_strm->width = s_info.width;
    __this_strm->height = s_info.height;
    info->width = s_info.width;
    info->height = s_info.height;
    info->fps = s_info.fps;
    info->sample_rate = s_info.sample_rate;
    info->channel_num = s_info.channel_num;
    printf("strm video info : %d , %d\n", info->sample_rate, info->channel_num);
    return 0;
}


