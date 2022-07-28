#include "system/includes.h"
#include "server/video_server.h"
#include "server/video_engine_server.h"
#include "net_video_rec.h"
#include "strm_video_rec.h"
#include "action.h"
#include "app_config.h"
#include "asm/debug.h"
#include "asm/osd.h"
#include "app_database.h"
#include "storage_device.h"
#include "server/ctp_server.h"
#include "os/os_api.h"
#include "net_server.h"
#include "server/net_server.h"
#include "stream_protocol.h"
#include "event/device_event.h"
#include "event/net_event.h"
#include "event/key_event.h"

#include "net/config_network.h"

extern int pthread_mutex_unlock(int *mutex);
/******************************************/
/*tiny code*/
extern int db_select(const char *name);
extern int db_update(const char *name, u32 value);
/******************************************/

void net_video_server_init(void);
static int net_rt_video1_open(struct intent *it);
static int net_rt_video0_open(struct intent *it);
static int net_video_rec_start(u8 mark);
static int net_video_rec_stop(u8 close);
static int net_video_rec0_ch2_stop(u8 close);
static int net_video_rec1_ch2_stop(u8 close);
static int net_video_rec1_stop(u8 close);
static int net_video_rec_take_photo(void (*callback)(char *buffer, int len));
static int net_video_rec_get_abr(u32 width);
static int net_video_rec_alloc_buf(void);
static void net_video_rec_free_buf(void);

#define AUDIO_VOLUME	100
#define NET_VIDEO_REC_SERVER_NAME	"net_video_server"
#define TAKE_PHOTO_MEM_LEN			40 * 1024

#define	NET_VIDEO_ERR(x)	{if(x) printf("######## %s %d err !!!! \n\n\n",__func__,__LINE__);}

static int net_rt_video0_stop(struct intent *it);
static int net_rt_video1_stop(struct intent *it);
static int net_video_rec_control(void *_run_cmd);

static const unsigned char net_osd_format_buf[] = "yyyy-nn-dd hh:mm:ss";

static int net_vd_msg[2];

static const u16 net_rec_pix_w[] = {1280, 640};
static const u16 net_rec_pix_h[] = {720,  480};

static struct net_video_hdl net_rec_handler = {0};
static struct strm_video_hdl *fv_rec_handler = NULL;
static struct video_rec_hdl *rec_handler = NULL;

#define __this_net  (&net_rec_handler)
#define __this_strm	(fv_rec_handler)
#define __this 	(rec_handler)

static OS_MUTEX net_vdrec_mutex;

#ifndef CONFIG_VIDEO0_ENABLE
#undef NET_VREC0_FBUF_SIZE
#define NET_VREC0_FBUF_SIZE 0
#endif
#ifndef CONFIG_VIDEO1_ENABLE
#undef NET_VREC1_FBUF_SIZE
#define NET_VREC1_FBUF_SIZE   0
#endif

#if NET_VREC0_FBUF_SIZE
#if (defined CONFIG_NET_TCP_ENABLE || defined CONFIG_NET_UDP_ENABLE || defined CONFIG_VIDEO_720P)
//内部sram只用静态buff
#define USE_STATIC_BUFF	1// 0 malloc , 1 static
#else
#define USE_STATIC_BUFF	0// 0 malloc , 1 static
#endif
#endif

#if (defined CONFIG_IPERF_ENABLE) && (defined CONFIG_NO_SDRAM_ENABLE) //无sdram开iperf测试内存需要130K，去掉静态留出内存
#undef USE_STATIC_BUFF
#define USE_STATIC_BUFF 0
#endif

#ifdef CONFIG_VIDEO_720P
#define NET_ISC_SBUF_SIZE     	(CONFIG_VIDEO_IMAGE_W*64*3/2)/*720P内部ISC BUFF采用内部ram*/
#else
#define NET_ISC_SBUF_SIZE     	(CONFIG_VIDEO_IMAGE_W*32*3/2)/*720P内部ISC BUFF采用内部ram*/
#endif

#ifdef CONFIG_VIDEO_720P
#define CAP_IMG_SIZE (150*1024)
#else
#define CAP_IMG_SIZE (40*1024)
#endif

#if USE_STATIC_BUFF
u8 net_video_buf[NET_VREC0_FBUF_SIZE] ALIGNE(4);
u8 net_audio_buf[NET_AUDIO_BUF_SIZE] ALIGNE(4);
u8 net_isc_sbuf[NET_ISC_SBUF_SIZE] SEC_USED(.sram) ALIGNE(32);//必须32对齐
#endif

/*
 *start user net video rec , 必须在app_config.h配置宏CONFIG_NET_USR_ENABLE 和NET_USR_PATH
 */
int user_net_video_rec_open(char forward)
{
    int ret;
    u8 open = 2;//char type : 0 audio , 1 video , 2 audio and video
    struct intent it = {0};
    struct rt_stream_app_info info = {0};

    info.width = CONFIG_VIDEO_IMAGE_W;
    info.height = CONFIG_VIDEO_IMAGE_H;

    //帧率 app_config.h : NET_VIDEO_REC_FPS0/NET_VIDEO_REC_FPS1

    it.data = &open;
    it.exdata = (u32)&info;

    if (__this_net->is_open) {
        return 0;
    }
    __this_net->is_open = TRUE;
    if (forward) {
        ret = net_rt_video0_open(&it);
        if (ret) {
            __this_net->is_open = FALSE;
        }
    }
    return ret;
}

int user_net_video_rec_close(char forward)
{
    int ret = 0;
    struct intent it = {0};
    u8 close = 2;//char type : 0 audio , 1 video , 2 audio and video

    it.data = &close;
    if (!__this_net->is_open) {
        return 0;
    }
    if (forward) {
        ret = net_rt_video0_stop(&it);
    }
    __this_net->is_open = FALSE;
    return ret;
}

void user_net_video_rec_take_photo_cb(char *buf, int len)//必须打开user_net_video_rec_open()
{
    if (buf && len) {
        printf("take photo success \n");
        put_buf(buf, 32);
        /*
        //目录1写卡例子
        FILE *fd = fopen(CAMERA0_CAP_PATH"IMG_***.jpg","w+");
        if (fd) {
        	fwrite(fd,buf,len);
        	fclose(fd);
        }
        */
    }
}
//qua : 0 240p, 1 480p, 2 720p
int user_net_video_rec_take_photo(int qua, void (*callback)(char *buf, int len))//必须打开user_net_video_rec_open()
{
    db_update("qua", qua);
    net_video_rec_take_photo(callback);
    return 0;
}
//example 720P: user_net_video_rec_take_photo(2, user_net_video_rec_take_photo_cb);
int user_net_video_rec_take_photo_test(void)
{
    user_net_video_rec_take_photo(2, user_net_video_rec_take_photo_cb);
    return 0;
}
/*
 *end of user net video rec
 */

int net_video_rec_get_list_vframe(void)
{
    return __this_net->fbuf_fcnt;
}
void net_video_rec_pkg_get_video_in_frame(char *fbuf, u32 frame_size)
{
    __this_net->fbuf_fcnt++;
    __this_net->fbuf_ffil += frame_size;
}
void net_video_rec_pkg_get_video_out_frame(char *fbuf, u32 frame_size)
{
    if (__this_net->fbuf_fcnt) {
        __this_net->fbuf_fcnt--;
    }
    if (__this_net->fbuf_ffil) {
        __this_net->fbuf_ffil -= frame_size;
    }
}
static void net_video_handler_init(void)
{
    if (!fv_rec_handler) {
        fv_rec_handler = (struct strm_video_hdl *)get_strm_video_rec_handler();
    }
    if (!rec_handler) {
        rec_handler  = (struct video_rec_hdl *)get_video_rec_handler();
    }
}

void *get_net_video_rec_handler(void)
{
    return (void *)&net_rec_handler;
}

void *get_video_rec0_handler(void)
{
    if (!__this) {
        return NULL;
    }
    return (void *)__this->video_rec0;
}

void *get_video_rec1_handler(void)
{
    if (!__this) {
        return NULL;
    }
#ifdef CONFIG_VIDEO1_ENABLE
    return (void *)__this->video_rec1;
#endif
#ifdef CONFIG_VIDEO2_ENABLE
    return (void *)__this->video_rec2;
#endif
    return NULL;
}

int net_video_rec_state(void)//获取录像状态，返回值大于0（1280/640）成功，失败 0
{
    int res = db_select("res");
    res = (res > 1 || res < 0) ? 1 : res;
    if (__this && __this->state == VIDREC_STA_START) {
        return net_rec_pix_w[res];
    }
    return 0;
}
int net_video_rec_uvc_online(void)
{
    return dev_online("uvc");
}
int net_pkg_get_video_size(int *width, int *height)
{
    u8 id = __this_net->video_id ? 1 : 0;
    *width = __this_net->net_videoreq[id].rec.width;
    *height = __this_net->net_videoreq[id].rec.height;
    return 0;
}
int net_video_buff_set_frame_cnt(void)
{
#ifdef NET_VIDEO_BUFF_FRAME_CNT
    return NET_VIDEO_BUFF_FRAME_CNT;
#else
    return 0;
#endif
}
u8 net_video_rec_get_drop_fp(void)
{
#ifdef NET_VIDEO_REC_DROP_REAl_FP
    return NET_VIDEO_REC_DROP_REAl_FP;
#else
    return 0;
#endif
}
u8 net_video_rec_get_lose_fram(void)
{
#ifdef NET_VIDEO_REC_LOSE_FRAME_CNT
    return NET_VIDEO_REC_LOSE_FRAME_CNT;
#else
    return 0;
#endif
}
u8 net_video_rec_get_send_fram(void)
{
#ifdef NET_VIDEO_REC_SEND_FRAME_CNT
    return  NET_VIDEO_REC_SEND_FRAME_CNT;
#else
    return 0;
#endif
}
int net_video_rec_get_fps(void)
{
    int ret;
    int fp = 15;
    ret = net_video_rec_state();
    if (ret) {
        fp = VIDEO_REC_FPS;
    } else {
        if (!__this_net->video_id) {
            fp = NET_VIDEO_REC_FPS0;
        } else {
            fp = NET_VIDEO_REC_FPS1;
        }
    }
    ret = fp ? fp : 20;
    return ret;
}

void net_video_rec_post_msg(const char *msg, ...)
{
#ifdef CONFIG_UI_ENABLE
    /*
    union uireq req;
    va_list argptr;

    va_start(argptr, msg);

    if (__this->ui) {
        req.msg.receiver = ID_WINDOW_VIDEO_REC;
        req.msg.msg = msg;
        req.msg.exdata = argptr;

        server_request(__this->ui, UI_REQ_MSG, &req);
    }

    va_end(argptr);
    */
#endif

}
void net_video_rec_fmt_notify(void)
{
    char buf[32];
#if defined CONFIG_ENABLE_VLIST
    extern void FILE_DELETE(char *__fname, u8 create_file);
    FILE_DELETE(NULL, __this_net->is_open);
    sprintf(buf, "frm:1");
    CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "FORMAT", "NOTIFY", buf);
#endif
}

char *video_rec_finish_get_name(FILE *fd, int index, u8 is_emf)  //index ：video0前视0，video1则1，video2则2 , is_emf 紧急文件
{
    static char path[64] ALIGNE(32) = {0};
    u8 name[64];
    u8 *dir;
    int err;

#ifdef CONFIG_ENABLE_VLIST
    memset(path, 0, sizeof(path));
    err = fget_name(fd, name, 64);
    /*printf("finish_get_name: %s \n", name);*/
    if (err <= 0) {
        return NULL;
    }
    if (index < 0) {
        strcpy(path, name);
        return path;
    }
    switch (index) {
    case 0:
        dir = CONFIG_REC_PATH_0;
        break;
    case 1:
        dir = CONFIG_REC_PATH_1;
        break;
    case 2:
        dir = CONFIG_REC_PATH_2;
        break;
    default:
        return NULL;
    }
#ifdef CONFIG_EMR_DIR_ENABLE
    if (is_emf) {
        sprintf(path, "%s%s%s", dir, CONFIG_EMR_REC_DIR, name);
    } else
#endif
    {
        sprintf(path, "%s%s", dir, name);
    }

    return  path;
#else
    return NULL;
#endif
}

int video_rec_finish_notify(char *path)
{
    int err = 0;

    if (__this_net->video_rec_err) {
        __this_net->video_rec_err = FALSE;
        return 0;
    }
    os_mutex_pend(&net_vdrec_mutex, 0);
#ifdef CONFIG_ENABLE_VLIST
    FILE_LIST_ADD(0, (const char *)path, __this_net->is_open);
#endif
    os_mutex_post(&net_vdrec_mutex);
    return err;
}
int video_rec_delect_notify(FILE *fd, int id)
{
    int err = 0;
    if (__this_net->video_rec_err) {
        __this_net->video_rec_err = FALSE;
        return 0;
    }
#ifdef CONFIG_ENABLE_VLIST
    char *delect_path;
    os_mutex_pend(&net_vdrec_mutex, 0);
    char *path = video_rec_finish_get_name(fd, id, 0);
    if (path == NULL) {
        os_mutex_post(&net_vdrec_mutex);
        return -1;
    }
    FILE_DELETE((const char *)path, __this_net->is_open);
    os_mutex_post(&net_vdrec_mutex);
#endif
    return err;
}

int video_rec_err_notify(const char *method)
{
    int err = 0;
    char *err_path;

    os_mutex_pend(&net_vdrec_mutex, 0);
    if (method && !strcmp((const char *)method, "VIDEO_REC_ERR")) {
        __this_net->video_rec_err = TRUE;
    }
    os_mutex_post(&net_vdrec_mutex);
    return err;
}
int video_rec_state_notify(void)
{
    int err = 0;
    net_vd_msg[0] = NET_VIDREC_STATE_NOTIFY;
    err = os_taskq_post_msg(NET_VIDEO_REC_SERVER_NAME, 1, (int)net_vd_msg);
    return err;
}

int video_rec_start_notify(void)
{
    return net_video_rec_control(0);//启动录像之前需要关闭实时流
}
int video_rec_all_stop_notify(void)
{
    int err = 0;
    net_vd_msg[0] = NET_VIDREC_STA_STOP;
    err = os_taskq_post_msg(NET_VIDEO_REC_SERVER_NAME, 1, (int)net_vd_msg);
    return err;
}
void net_video_rec_status_notify(void)
{
    char buf[128];
    u8 sta = 0;
    if (__this && __this->state == VIDREC_STA_START) {
        sta = 1;
    }
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "status:%d", sta);
    CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "VIDEO_CTRL", "NOTIFY", buf);
}
int net_video_rec_event_notify(void)
{
    char buf[128];
    int res = db_select("res2");
    res = res > 0 ? res : 0;
#ifdef CONFIG_VIDEO1_ENABLE
#ifdef CONFIG_SPI_VIDEO_ENABLE
    extern int spi_camera_width_get(void);
    extern int spi_camera_height_get(void);
    sprintf(buf, "status:1,h:%d,w:%d,fps:%d,rate:%d,format:1", spi_camera_height_get(), spi_camera_width_get(), 15, net_video_rec_get_audio_rate());
    CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "PULL_VIDEO_STATUS", "NOTIFY", buf);
    return 0;
#endif
    if (dev_online("video1"))
#else
#ifdef CONFIG_VIDEO2_ENABLE
    if (dev_online("uvc"))
#else
    if (0)
#endif
#endif
    {
        switch (res) {
        case VIDEO_RES_1080P:
            sprintf(buf, "status:1,h:%d,w:%d,fps:%d,rate:%d,format:1", 720, 1280, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
            break;
        case VIDEO_RES_720P:
            sprintf(buf, "status:1,h:%d,w:%d,fps:%d,rate:%d,format:1", 480, 640, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
            break;
        case VIDEO_RES_VGA:
            sprintf(buf, "status:1,h:%d,w:%d,fps:%d,rate:%d,format:1", 480, 640, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
            break;
        default:
            sprintf(buf, "status:1,h:%d,w:%d,fps:%d,rate:%d,format:1", 480, 640, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
            break;
        }
        CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "PULL_VIDEO_STATUS", "NOTIFY", buf);
    } else {
        strcpy(buf, "status:0");
        CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "PULL_VIDEO_STATUS", "NOTIFY", buf);
    }
    return 0;
}
int net_video_rec_event_stop(void)
{
    net_video_rec_stop(1);
    return 0;
}
int net_video_rec_event_start(void)
{
    net_rt_video0_open(NULL);
    return 0;
}
int video_rec_sd_event_ctp_notify(char state)
{
    char buf[128];
    printf("~~~ : %s , %d\n\n", __func__, state);

#if defined CONFIG_ENABLE_VLIST
    if (!state) {
        FILE_REMOVE_ALL();
    }
#endif
    sprintf(buf, "online:%d", state);
    CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "SD_STATUS", "NOTIFY", buf);

    if (state) {
        u32 space = 0;
        struct vfs_partition *part = NULL;
        if (storage_device_ready() == 0) {
            msleep(200);
            if (storage_device_ready()) {
                goto sd_scan;
            }
            printf("---%s , storage_device_not_ready !!!!\n\n", __func__);
            CTP_CMD_COMBINED(NULL, CTP_SD_OFFLINE, "TF_CAP", "NOTIFY", CTP_SD_OFFLINE_MSG);
        } else {
sd_scan:
            part = fget_partition(CONFIG_ROOT_PATH);
            fget_free_space(CONFIG_ROOT_PATH, &space);
            sprintf(buf, "left:%d,total:%d", space / 1024, part->total_size / 1024);
            CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "TF_CAP", "NOTIFY", buf);
        }
    }

    return 0;
}

//NET USE API
static void video_rec_get_app_status(struct intent *it)
{
    it->data = (const char *)__this;
    it->exdata = (u32)__this_net;
}
static void ctp_cmd_notity(const char *path)
{
#if defined CONFIG_ENABLE_VLIST
    FILE_LIST_ADD(0, path, __this_net->is_open);
#endif
}

int net_video_rec_get_audio_rate()
{
    return VIDEO_REC_AUDIO_SAMPLE_RATE;
}

int video_rec_cap_photo(char *buf, int len)
{
    u32 *flen;
    u8 *cap_img;
    if (__this_net->cap_image) {
        cap_img = &__this_net->cap_image;
    } else {
        cap_img = &__this_strm->cap_image;
    }
    if (*cap_img && __this && __this->cap_buf) {
        *cap_img = FALSE;
        flen = __this->cap_buf;
        memcpy(__this->cap_buf + 4, buf, len);
        *flen = len;
    }
    return 0;
}
static int net_video_rec_take_photo(void (*callback)(char *buffer, int len))
{
    struct server *server = NULL;
    union video_req req = {0};
    char *path;
    char buf[48];
    char name_buf[20];
    int err;

    if (!(__this_net->net_state == VIDREC_STA_START || __this_net->net_state_ch2 == VIDREC_STA_START)) {
        printf("waring :net video not open or video record running \n");
        goto error;
    }

    server = __this_net->net_video_rec;
    req.icap.quality = VIDEO_MID_Q;
    req.icap.buf_size = CAP_IMG_SIZE;
    req.icap.buf = malloc(CAP_IMG_SIZE);
    if (!req.icap.buf) {
        goto error;
    }
    req.rec.text_osd = NULL;
    req.rec.graph_osd = NULL;
    req.icap.text_label = NULL;
    req.icap.file_name = name_buf;
    req.icap.path = CAMERA0_CAP_PATH"IMG_****.jpg";
    path = CAMERA0_CAP_PATH;
    err = server_request(server, VIDEO_REQ_IMAGE_CAPTURE, &req);
    if (err != 0) {
        puts("\n\n\ntake photo err\n\n\n");
        goto error;
    }
    sprintf(buf, "%s%s", path, req.icap.file_name);
    printf("%s\n\n", buf);
    CTP_CMD_COMBINED(NULL, CTP_REQUEST, "PHOTO_CTRL", "NOTIFY", CTP_REQUEST_MSG);
    if (req.icap.buf) {
        free(req.icap.buf);
    }
    return 0;

error:
    if (req.icap.buf) {
        free(req.icap.buf);
    }
    return -EINVAL;
}



extern int atoi(const char *);
static void rt_stream_cmd_analysis(u8 chl,  u32 add)
{
    char *key;
    char *value;
    struct rt_stream_app_info *info = (struct rt_stream_app_info *)add;

    info->width = CONFIG_VIDEO_IMAGE_W;
    info->height = CONFIG_VIDEO_IMAGE_H;

    __this_net->net_videoreq[chl].rec.width = info->width;
    __this_net->net_videoreq[chl].rec.height = info->height;
    __this_net->net_videoreq[chl].rec.format = info->type - NET_VIDEO_FMT_AVI;
    __this_net->net_videoreq[chl].rec.format = 0;
    __this_net->net_videoreq[chl].rec.fps = net_video_rec_get_fps();
    __this_net->net_videoreq[chl].rec.real_fps = net_video_rec_get_fps();
    __this_net->priv = info->priv;
    printf(">>>>>>>> ch %d , info->type : %d ,w:%d , h:%d\n\n", chl, info->type, info->width, info->height);

}

static void net_video_rec_set_bitrate(unsigned int bits_rate)
{
    union video_req req = {0};

    req.rec.channel = __this_net->channel;

    req.rec.state = VIDEO_STATE_RESET_BITS_RATE;
    req.rec.abr_kbps = bits_rate;

    server_request(__this_net->net_video_rec, VIDEO_REQ_REC, &req);
}

/*码率控制，根据具体分辨率设置*/
static int net_video_rec_get_abr(u32 width)
{
    /*视频码率kbps使用说明:
     码率：一帧图片以K字节为单位大小 * 帧率 * 8，比如：一帧图片为30KB，帧率为20帧，则码率为30*20*8=4800
     VGA图片大小说明：低等质量(小于20K)，中等质量(20K-40K)，高质量(大于40K，极限70K)
     720P图片大小说明：低等质量(小于50K)，中等质量(50k-100K)，高质量(大于100K，极限150K)
    */

    if (width <= 640) {
        return 1000;
    } else {
        return 1000;
    }
}

static void net_rec_dev_server_event_handler(void *priv, int argc, int *argv)
{
    int mark = 2;
    struct intent it;
    /*
     *该回调函数会在录像过程中，写卡出错被当前录像APP调用，例如录像过程中突然拔卡
     */
    switch (argv[0]) {
    case VIDEO_SERVER_UVM_ERR:
        log_e("APP_UVM_DEAL_ERR\n");
        break;
    case VIDEO_SERVER_PKG_ERR:
        log_e("video_server_pkg_err\n");
        if (__this && __this->state == VIDREC_STA_START) {
            net_video_rec_control(0);
        }
        break;
    case VIDEO_SERVER_PKG_END:
        if (db_select("cyc") > 0) {
            /*video_rec_savefile((int)priv);*/
        } else {
            net_video_rec_control(0);
        }
        break;
    case VIDEO_SERVER_PKG_NET_ERR:

        init_intent(&it);
        it.data = &mark;
        if (__this_net->net_video0_vrt_on) {
            net_rt_video0_stop(&it);
            __this_net->is_open = FALSE;
        } else if (__this_net->net_video1_vrt_on) {
#ifdef CONFIG_SPI_VIDEO_ENABLE
            extern int spi_video_task_kill(void *priv);
            int err = spi_video_task_kill(__this_net->priv);
            if (!err) {
                __this_net->net_video1_vrt_on = 0;
            }
#endif
        }
        break;
    default :
        break;
    }
}

static int net_video_rec0_start()
{
    int err;
    union video_req req = {0};
    struct video_text_osd text_osd = {0};
    struct video_graph_osd graph_osd;
    u16 max_one_line_strnum;
    char buf[128];

    if (!__this_net->net_video_rec) {
#ifdef CONFIG_UVC_VIDEO2_ENABLE
        __this_net->net_video_rec = server_open("video_server", "video2.1");
#else
        __this_net->net_video_rec = server_open("video_server", "video0.1");
#endif
        if (!__this_net->net_video_rec) {
            return VREC_ERR_V0_SERVER_OPEN;
        }

        server_register_event_handler(__this_net->net_video_rec, (void *)0, net_rec_dev_server_event_handler);
    }

    __this_net->video_id = 0;
#ifdef CONFIG_UVC_VIDEO2_ENABLE
    req.rec.uvc_id = __this_net->uvc_id;
#endif
#ifdef CONFIG_VIDEO_REC_PPBUF_MODE
    req.rec.bfmode = VIDEO_PPBUF_MODE;
#endif
#ifdef  CONFIG_VIDEO_SPEC_DOUBLE_REC_MODE
    req.rec.wl80_spec_mode = VIDEO_WL80_SPEC_DOUBLE_REC_MODE;
#endif
#ifdef CONFIG_UI_ENABLE
    req.rec.picture_mode = 1;//整帧图片模式
#endif
#if USE_STATIC_BUFF
    req.rec.isc_sbuf = net_isc_sbuf;
    req.rec.sbuf_size = NET_ISC_SBUF_SIZE;
#else
    req.rec.isc_sbuf = NULL;
    req.rec.sbuf_size = 0;
#endif
    req.rec.block_done_cb = NULL;
    req.rec.camera_type = VIDEO_CAMERA_NORMAL;
    req.rec.channel = __this_net->channel = 1;
    req.rec.width 	= __this_net->net_videoreq[0].rec.width;
    req.rec.height 	= __this_net->net_videoreq[0].rec.height;
    printf(">>>>>>width=%d    height=%d\n\n\n\n", __this_net->net_videoreq[0].rec.width, __this_net->net_videoreq[0].rec.height);
    req.rec.state 	= VIDEO_STATE_START;
    req.rec.fpath 	= CONFIG_REC_PATH_0;

    if (__this_net->net_videoreq[0].rec.format == 1) {
        puts("in h264\n\n");
        req.rec.format 	= NET_VIDEO_FMT_MOV;
    } else {
        req.rec.format 	= NET_VIDEO_FMT_AVI;
    }
    if (__this_net->net_videoreq[0].rec.format == 1) {
        req.rec.fname    = "vid_***.mov";
    } else {
        req.rec.fname    = "vid_***.avi";
    }

    req.rec.quality = VIDEO_LOW_Q;
    req.rec.fps = 0;
    req.rec.real_fps = net_video_rec_get_fps();
    req.rec.net_par.net_audrt_onoff  = __this_net->net_video0_art_on;
    req.rec.net_par.net_vidrt_onoff = __this_net->net_video0_vrt_on;
    req.rec.audio.sample_rate = VIDEO_REC_AUDIO_SAMPLE_RATE;
    req.rec.audio.channel 	= 1;
    req.rec.audio.channel_bit_map = 0;
    req.rec.audio.volume    = AUDIO_VOLUME;
    req.rec.audio.buf = __this_net->audio_buf;
    req.rec.audio.buf_len = NET_AUDIO_BUF_SIZE;
    req.rec.pkg_mute.aud_mute = !db_select("mic");

    /*
    *码率，I帧和P帧比例，必须是偶数（当录MOV的时候才有效）,
    *roio_xy :值表示宏块坐标， [6:0]左边x坐标 ，[14:8]右边x坐标，[22:16]上边y坐标，[30:24]下边y坐标,写0表示1个宏块有效
    * roio_ratio : 区域比例系数
    */
    req.rec.abr_kbps = net_video_rec_get_abr(req.rec.width);
    req.rec.IP_interval = 0;
    /*感兴趣区域为下方 中间 2/6 * 4/6 区域，可以调整
    	感兴趣区域qp 为其他区域的 70% ，可以调整
    */
    req.rec.roi.roio_xy = (req.rec.height * 5 / 6 / 16) << 24 | (req.rec.height * 3 / 6 / 16) << 16 | (req.rec.width * 5 / 6 / 16) << 8 | (req.rec.width) * 1 / 6 / 16;
    req.rec.roi.roi1_xy = (req.rec.height * 11 / 12 / 16) << 24 | (req.rec.height * 4 / 12 / 16) << 16 | (req.rec.width * 11 / 12 / 16) << 8 | (req.rec.width) * 1 / 12 / 16;
    req.rec.roi.roi2_xy = 0;
    req.rec.roi.roi3_xy = (1 << 24) | (0 << 16) | ((req.rec.width / 16) << 8) | 0;
    req.rec.roi.roio_ratio = 256 * 70 / 100 ;
    req.rec.roi.roio_ratio1 = 256 * 90 / 100;
    req.rec.roi.roio_ratio2 = 0;
    req.rec.roi.roio_ratio3 = 256 * 80 / 100;

#ifdef CONFIG_OSD_ENABLE
    text_osd.font_w = OSD_DEFAULT_WIDTH;//必须16对齐
    text_osd.font_h = OSD_DEFAULT_HEIGHT;//必须16对齐
    text_osd.text_format = net_osd_format_buf;
    text_osd.x = (req.rec.width - text_osd.font_w * strlen(text_osd.text_format) + 15) / 16 * 16;
    text_osd.y = (req.rec.height - text_osd.font_h + 15) / 16 * 16;
    text_osd.osd_yuv = 0xe20095;
    text_osd.font_matrix_table = NULL;
    text_osd.font_matrix_base = NULL;
    text_osd.font_matrix_len = 0;
    if (db_select("dat") > 0) {
        req.rec.text_osd = &text_osd;
    }
#endif

    /*
    *慢动作倍数(与延时摄影互斥,无音频); 延时录像的间隔ms(与慢动作互斥,无音频)
    */
    req.rec.slow_motion = 0;
    req.rec.tlp_time = 0;//db_select("gap");
    req.rec.buf = __this_net->net_v0_fbuf;
    req.rec.buf_len = NET_VREC0_FBUF_SIZE;

    struct sockaddr_in *addr = ctp_srv_get_cli_addr(__this_net->priv);
    if (!addr) {
        addr = cdp_srv_get_cli_addr(__this_net->priv);
    }
#if (defined CONFIG_NET_UDP_ENABLE)
    sprintf(req.rec.net_par.netpath, "udp://%s:%d"
            , inet_ntoa(addr->sin_addr.s_addr)
            , _FORWARD_PORT);
#elif (defined CONFIG_NET_TCP_ENABLE)
    sprintf(req.rec.net_par.netpath, "tcp://%s:%d"
            , inet_ntoa(addr->sin_addr.s_addr)
            , _FORWARD_PORT);
#elif (defined CONFIG_USR_VIDEO_ENABLE)
    sprintf(req.rec.net_par.netpath, "usr://%s", CONFIG_USR_PATH);
#elif (defined CONFIG_NET_TUTK_ENABLE)
    sprintf(req.rec.net_par.netpath, "tutk://%s", " 192.168.1.1");
#endif

    err = server_request(__this_net->net_video_rec, VIDEO_REQ_REC, &req);

    if (err != 0) {
        puts("\n\n\nstart rec err\n\n\n");
        return VREC_ERR_V0_REQ_START;
    }
    return 0;
}

static int net_video_rec0_stop(u8 close)
{
    union video_req req = {0};
    int err = 0;
    printf("\nnet video rec0 stop\n");
    __this_net->net_state = VIDREC_STA_STOPING;
    if (__this && __this->state == VIDREC_STA_START) {
        err = stream_protocol_task_kill();
        __this_net->net_video_rec = NULL;
    } else {
        req.rec.channel = 1;
        req.rec.state = VIDEO_STATE_STOP;
        err = server_request(__this_net->net_video_rec, VIDEO_REQ_REC, &req);

        if (err != 0) {
            printf("\nstop rec err 0x%x\n", err);
            return VREC_ERR_V0_REQ_STOP;
        }

        if (close) {
            if (__this_net->net_video_rec) {
                server_close(__this_net->net_video_rec);
                __this_net->net_video_rec = NULL;
            }
        }
    }
    __this_net->net_state = VIDREC_STA_STOP;
    __this_net->video_id = 0;
    return 0;
}

static void net_video_rec_free_buf(void)
{
#if (USE_STATIC_BUFF == 0)
    if (__this_net->net_v0_fbuf) {
        free(__this_net->net_v0_fbuf);
        __this_net->net_v0_fbuf = NULL;
    }
    if (__this_net->audio_buf) {
        free(__this_net->audio_buf);
        __this_net->audio_buf = NULL;
    }
#else
    __this_net->net_v0_fbuf = NULL;
    __this_net->audio_buf = NULL;
#endif
}
static int net_video_rec_alloc_buf(void)
{
    if (!__this_net->net_v0_fbuf && NET_VREC0_FBUF_SIZE) {
#if (USE_STATIC_BUFF == 0)
        __this_net->net_v0_fbuf = malloc(NET_VREC0_FBUF_SIZE);
#else
        __this_net->net_v0_fbuf = net_video_buf;
#endif
        if (!__this_net->net_v0_fbuf) {
            puts("malloc v0_buf err\n\n");
            return -ENOMEM;
        }
    }
    if (!__this_net->audio_buf && VIDEO_REC_AUDIO_SAMPLE_RATE && NET_AUDIO_BUF_SIZE) {
#if (USE_STATIC_BUFF == 0)
        __this_net->audio_buf = malloc(NET_AUDIO_BUF_SIZE);
#else
        __this_net->audio_buf = net_audio_buf;
#endif
        if (!__this_net->audio_buf) {
            free(__this_net->net_v0_fbuf);
            __this_net->net_v0_fbuf = NULL;
            return -ENOMEM;
        }
    }
    return 0;
}

char *get_net_video_isc_buf(int *buf_size)
{
#if (USE_STATIC_BUFF == 0)
    *buf_size = 0;
    return 0;
#else
    *buf_size = NET_ISC_SBUF_SIZE;
    return net_isc_sbuf;
#endif
}
char *get_net_video_rec0_video_buf(int *buf_size)
{
    *buf_size = NET_VREC0_FBUF_SIZE;
#if (USE_STATIC_BUFF == 0)
    return __this_net->net_v0_fbuf;
#else
    return net_video_buf;
#endif
}
char *get_net_video_rec1_video_buf(int *buf_size)
{
    *buf_size = 0;
    return NULL;
}
char *get_net_video_rec2_video_buf(int *buf_size)
{
    *buf_size = 0;
    return NULL;
}
char *get_net_video_rec_audio_buf(int *buf_size)
{
    *buf_size = NET_AUDIO_BUF_SIZE;
#if (USE_STATIC_BUFF == 0)
    return __this_net->audio_buf;
#else
    return net_audio_buf;
#endif
}
static int net_video0_rec_start(struct intent *it)
{
    int err = 0;
#ifdef CONFIG_VIDEO0_ENABLE
    err = net_video_rec_alloc_buf();
    if (err) {
        return err;
    }
    if (__this_net->net_video0_art_on || __this_net->net_video0_vrt_on) {
        __this_net->net_state = VIDREC_STA_STARTING;
        err = net_video_rec0_start();

        if (err) {
            goto __start_err0;
        }
        if (__this && __this->state != VIDREC_STA_START) {
            __this_net->videoram_mark = 1;
        } else {
            __this_net->videoram_mark = 0;
        }
        __this_net->net_state = VIDREC_STA_START;
    }

    return 0;

__start_err0:
    puts("\nstart0 err\n");
    net_video_rec0_stop(0);

    if (err) {
        printf("\nstart wrong0 %x\n", err);
        //while(1);
    }

#endif
    return err;
}

static int net_video_rec_start(u8 mark)
{
    int err;
    if (!__this_net->is_open) {
        return 0;
    }

    puts("start net rec\n");
    err = net_video_rec_alloc_buf();
    if (err) {
        return err;
    }

#ifdef CONFIG_VIDEO0_ENABLE

    printf("\n art %d, vrt %d\n", __this_net->net_video0_art_on, __this_net->net_video0_vrt_on);

    if ((__this_net->net_video0_art_on || __this_net->net_video0_vrt_on)
        && (__this_net->net_state != VIDREC_STA_START)) {
        puts("\nnet video rec0 start \n");
        err = net_video_rec0_start();
        if (err) {
            goto __start_err0;
        }
        __this_net->net_state = VIDREC_STA_START;
    }

#endif

    return 0;

#ifdef CONFIG_VIDEO0_ENABLE
__start_err0:
    puts("\nstart0 err\n");
    net_video_rec0_stop(0);

    if (err) {
        printf("\nstart wrong0 %x\n", err);
    }

#endif

    return -EFAULT;
}


static int net_video0_rec_stop(u8 close)
{
    int err;
#ifdef CONFIG_VIDEO0_ENABLE
    __this_net->net_state = VIDREC_STA_STOPING;
    err = net_video_rec0_stop(close);

    if (err) {
        puts("\n net stop0 err\n");
    }

    __this_net->net_state = VIDREC_STA_STOP;
#endif
    return err;
}

static int net_video_rec_stop(u8 close)
{
    if (!__this_net->is_open) {
        return 0;
    }

    int err = 0;

#ifdef CONFIG_VIDEO0_ENABLE
    puts("\n net_video_rec_stop. 0.. \n");
#if !(defined CONFIG_VIDEO1_ENABLE || defined CONFIG_VIDEO2_ENABLE)
    if (__this_net->videoram_mark != 1 && close == 0) {
        puts("\n video ram mark\n");
        return 0;
    }
#endif

    if (__this_net->net_state == VIDREC_STA_START) {
        __this_net->net_state = VIDREC_STA_STOPING;
        err = net_video_rec0_stop(close);

        if (err) {
            puts("\n net stop0 err\n");
        }
        __this_net->net_state = VIDREC_STA_STOP;
        printf("-----%s state stop \n", __func__);
    }

#endif

    return err;
}

static int  net_rt_video0_open(struct intent *it)
{
    int ret;
    if (!__this_net->is_open) {
        return 0;
    }
    puts("\nnet rt video0 open \n");
#ifdef CONFIG_VIDEO0_ENABLE
    if (it) {
        u8 mark = *((u8 *)it->data);

        if (mark == 0) {
            __this_net->net_video0_art_on = 1;
            __this_net->net_video0_vrt_on = 0 ;
        } else if (mark == 1) {
            __this_net->net_video0_vrt_on = 1;
            __this_net->net_video0_art_on = 0;
        } else {
            __this_net->net_video0_art_on = 1;
            __this_net->net_video0_vrt_on = 1;
        }
        rt_stream_cmd_analysis(0, it->exdata);
    } else {
        if (__this_net->net_video0_art_on == 0 && __this_net->net_video0_vrt_on == 0) {
            puts("\n rt not open\n");
            return 0;
        }
    }
    if (__this_net->net_state == VIDREC_STA_START) {
        puts("\n net rt is on \n");
        return 0;
    }
    ret = net_video0_rec_start(it);
    return ret;
#else
    return 0;
#endif

}


static int  net_rt_video0_stop(struct intent *it)
{
    int ret = 0;
    puts("\nnet rt video0 stop \n");
    if (!__this_net->is_open) {
        return 0;
    }

#ifdef CONFIG_VIDEO0_ENABLE
    u8 mark = 2;
    if (it) {
        mark = *((u8 *)it->data);
    }

    if (mark == 0) {
        __this_net->net_video0_art_on = 0;
    } else if (mark == 1) {
        __this_net->net_video0_vrt_on = 0;
    } else {
        __this_net->net_video0_art_on = 0;
        __this_net->net_video0_vrt_on = 0;
    }

    if (__this_net->net_state == VIDREC_STA_START) {
        ret = net_video0_rec_stop(1);
    }
#endif
    return ret;
}

void  net_video_rec0_close()
{
    if (__this_net->net_video_rec) {
        server_close(__this_net->net_video_rec);
        __this_net->net_video_rec = NULL;
    }

}

static int net_video_rec_close()
{
#ifdef CONFIG_VIDEO0_ENABLE
    net_video_rec0_close();
#endif
    return 0;
}

/*
 * 录像app的录像控制入口, 根据当前状态调用相应的函数
 */
static int net_video_rec_control(void *_run_cmd)
{
    int err = 0;
    u32 clust;
    int run_cmd = (int)_run_cmd;
    struct vfs_partition *part;
    if (storage_device_ready() == 0) {
        if (!dev_online(SDX_DEV)) {
            net_video_rec_post_msg("noCard");
        } else {
            net_video_rec_post_msg("fsErr");
        }
        CTP_CMD_COMBINED(NULL, CTP_SDCARD, "VIDEO_CTRL", "NOTIFY", CTP_SDCARD_MSG);
        return 0;
    } else {
        net_video_handler_init();
        part = fget_partition(CONFIG_ROOT_PATH);
        if (!part) {
            printf("err in fget_partition");
            return 0;
        }
        if (__this) {
            __this->total_size = part->total_size;
        }
        if (part->clust_size < 32 || (part->fs_attr & F_ATTR_RO)) {
            net_video_rec_post_msg("fsErr");
            CTP_CMD_COMBINED(NULL, CTP_SDCARD, "VIDEO_CTRL", "NOTIFY", CTP_SDCARD_MSG);
            return 0;
        }
    }
    if (__this) {
        switch (__this->state) {
        case VIDREC_STA_IDLE:
        case VIDREC_STA_STOP:
            if (run_cmd) {
                break;
            }
            __this_net->video_rec_err = FALSE;//用在录像IMC打不开情况下
            printf("--NET_VIDEO_START\n");
            /*NET_VIDEO_ERR(strm_video_rec_close(1));*/
            NET_VIDEO_ERR(net_video_rec_stop(1));
            __this_net->fbuf_fcnt = 0;
            __this_net->fbuf_ffil = 0;
            err = video_rec_control_start();
            if (err == 0) {
                /* net_video_rec_post_msg("onREC"); */
                if (__this->gsen_lock == 1) {
                    net_video_rec_post_msg("lockREC");
                }
            }
            NET_VIDEO_ERR(err);

            /*NET_VIDEO_ERR(strm_video_rec_open());*/
            NET_VIDEO_ERR(net_rt_video0_open(NULL));
            /*NET_VIDEO_ERR(net_rt_video1_open(NULL));*/

            net_video_rec_status_notify();
            printf("--NET_VIDEO_OPEN OK\n");

            break;
        case VIDREC_STA_START:
            if (run_cmd == 0) {
                printf("--NET_VIDEO_STOP\n");
                /*NET_VIDEO_ERR(strm_video_rec_close(1));*/
                NET_VIDEO_ERR(net_video_rec_stop(1));
                __this_net->fbuf_fcnt = 0;
                __this_net->fbuf_ffil = 0;

                err = video_rec_control_doing();
                NET_VIDEO_ERR(err);

                /*NET_VIDEO_ERR(strm_video_rec_open());*/
                NET_VIDEO_ERR(net_video_rec_start(1));
                printf("--NET_VIDEO_OPEN OK\n");
            }
            net_video_rec_status_notify();
            break;
        default:
            puts("\nvrec forbid\n");
            err = 1;
            break;
        }
    }

    return err;
}
/*
 *录像的状态机,进入录像app后就是跑这里
 */
static int net_video_rec_state_machine(struct application *app, enum app_state state, struct intent *it)
{
    int err = 0;
    int len;
    char buf[128];

    switch (state) {
    case APP_STA_START:
        if (!it) {
            break;
        }
        switch (it->action) {
        case ACTION_VIDEO_REC_MAIN:
            net_video_server_init();
            break;
        case ACTION_VIDEO_TAKE_PHOTO:
            /*printf("----ACTION_VIDEO_TAKE_PHOTO----\n\n");*/
            net_video_rec_take_photo(NULL);
            break;
        case ACTION_VIDEO_REC_CONCTRL:
            printf("----ACTION_VIDEO_REC_CONCTRL-----\n\n");
            err = net_video_rec_control(0);
            break;

        case ACTION_VIDEO_REC_GET_APP_STATUS:
            printf("----ACTION_VIDEO_REC_GET_APP_STATUS-----\n\n");
            video_rec_get_app_status(it);
            break;

        case ACTION_VIDEO_REC_GET_PATH:
            /*printf("----ACTION_VIDEO_REC_GET_PATHL-----\n\n");*/
            break;
        case ACTION_VIDEO0_OPEN_RT_STREAM:
            printf("----ACTION_VIDEO0_OPEN_RT_STREAM-----\n\n");
            __this_net->is_open = TRUE;
            __this_net->fbuf_fcnt = 0;
            __this_net->fbuf_ffil = 0;
#ifdef CONFIG_SPI_VIDEO_ENABLE
            extern int spi_video_wait_done(void);
            spi_video_wait_done();
#endif
            err = net_rt_video0_open(it);
            sprintf(buf, "format:%d,w:%d,h:%d,fps:%d,rate:%d"
                    , __this_net->net_videoreq[0].rec.format
                    , __this_net->net_videoreq[0].rec.width
                    , __this_net->net_videoreq[0].rec.height
                    , __this_net->net_videoreq[0].rec.real_fps
                    , VIDEO_REC_AUDIO_SAMPLE_RATE);
            printf("<<<<<<< : %s\n\n\n\n\n", buf);
            if (err) {
                printf("ACTION_VIDEO0_OPEN_RT_STREAM err!!!\n\n");
                CTP_CMD_COMBINED(NULL, CTP_RT_OPEN_FAIL, "OPEN_RT_STREAM", "NOTIFY", CTP_RT_OPEN_FAIL_MSG);
            } else {
                if (CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "OPEN_RT_STREAM", "NOTIFY", buf)) {
                    CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "OPEN_RT_STREAM", "NOTIFY", buf);
                }
                printf("CTP NOTIFY VIDEO0 OK\n\n");
            }
            break;

        case ACTION_VIDEO1_OPEN_RT_STREAM:
            printf("----ACTION_VIDEO1_OPEN_RT_STREAM----\n\n");
            __this_net->fbuf_fcnt = 0;
            __this_net->fbuf_ffil = 0;

#ifdef CONFIG_SPI_VIDEO_ENABLE
            extern int spi_video_task_create(void *priv);
            extern int spi_camera_width_get(void);
            extern int spi_camera_height_get(void);
            extern int spi_video_wait_done(void);
            spi_video_wait_done();
            if (it) {
                rt_stream_cmd_analysis(1, it->exdata);
                err = spi_video_task_create(__this_net->priv);
                if (!err) {
                    __this_net->net_video1_vrt_on = 1;
                    __this_net->net_videoreq[1].rec.width = spi_camera_width_get();
                    __this_net->net_videoreq[1].rec.height = spi_camera_height_get();
                    __this_net->net_videoreq[1].rec.real_fps = 15;
                }
            }
#endif
            sprintf(buf, "format:%d,w:%d,h:%d,fps:%d,rate:%d"
                    , __this_net->net_videoreq[1].rec.format
                    , __this_net->net_videoreq[1].rec.width
                    , __this_net->net_videoreq[1].rec.height
                    , __this_net->net_videoreq[1].rec.real_fps
                    , VIDEO_REC_AUDIO_SAMPLE_RATE);
            printf("<<<<<<< : %s\n\n\n\n\n", buf);
            if (err) {
                printf("ACTION_VIDEO1_OPEN_RT_STREAM err!!!\n\n");
                CTP_CMD_COMBINED(NULL, CTP_RT_OPEN_FAIL, "OPEN_PULL_RT_STREAM", "NOTIFY", CTP_RT_OPEN_FAIL_MSG);
            } else {
                CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "OPEN_PULL_RT_STREAM", "NOTIFY", buf);
                printf("CTP NOTIFY VIDEO1 OK\n\n");
            }
            break;

        case ACTION_VIDEO0_CLOSE_RT_STREAM:
            printf("---ACTION_VIDEO0_CLOSE_RT_STREAM---\n\n");
#ifdef CONFIG_SPI_VIDEO_ENABLE
            extern int spi_video_wait_done(void);
            spi_video_wait_done();
#endif
            net_rt_video0_stop(it);
            if (err) {
                printf("ACTION_VIDEO_CLOE_RT_STREAM err!!!\n\n");
                strcpy(buf, "status:0");
            } else {
                strcpy(buf, "status:1");
            }
            CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "CLOSE_RT_STREAM", "NOTIFY", buf);
            printf("CTP NOTIFY VIDEO0 OK\n\n");
            __this_net->is_open = FALSE;
            __this_net->fbuf_fcnt = 0;
            __this_net->fbuf_ffil = 0;
            break;

        case ACTION_VIDEO1_CLOSE_RT_STREAM:
            printf("---ACTION_VIDEO1_CLOSE_RT_STREAM---\n\n");
#ifdef CONFIG_SPI_VIDEO_ENABLE
            extern int spi_video_task_kill(void *priv);
            extern int spi_video_wait_done(void);
            spi_video_wait_done();
            err = spi_video_task_kill(__this_net->priv);
            if (!err) {
                __this_net->net_video1_vrt_on = 0;
            }
#endif
            if (err) {
                printf("ACTION_VIDE1_CLOE_RT_STREAM err!!!\n\n");
                strcpy(buf, "status:0");
            } else {
                strcpy(buf, "status:1");
            }

            CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "CLOSE_PULL_RT_STREAM", "NOTIFY", buf);
            printf("CTP NOTIFY VIDEO1 OK\n\n");
            __this_net->is_open = FALSE;
            __this_net->fbuf_fcnt = 0;
            __this_net->fbuf_ffil = 0;
            break;
        case ACTION_VIDEO_CYC_SAVEFILE:
#if 0
            video_cyc_file(0);
#if defined CONFIG_VIDEO1_ENABLE
            video_cyc_file(1);
#endif
#if defined CONFIG_VIDEO2_ENABLE
            video_cyc_file(2);
#endif
#endif
            strcpy(buf, "status:1");
            CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "VIDEO_CYC_SAVEFILE", "NOTIFY", buf);
            break;
        }
        break;
    }

    return err;
}

static void net_video_rec_ioctl(u32 argv)
{
    char buf[128];
    u32 *pargv = (u32 *)argv;
    u32 type = (u32)pargv[0];
    char *path = (char *)pargv[1];

    /*printf("%s type : %d , %s \n\n", __func__, type, path);*/
    switch (type) {
    case NET_VIDREC_STA_STOP:
        if (__this_net->net_state == VIDREC_STA_START || __this_net->net_state1 == VIDREC_STA_START \
            || __this_net->net_state_ch2 == VIDREC_STA_START || __this_net->net_state1_ch2 == VIDREC_STA_START) {
            net_video_rec_stop(1);
        }
        /*if (__this_strm->state == VIDREC_STA_START || __this_strm->state_ch2 == VIDREC_STA_START) {*/
        /*strm_video_rec_close(1);*/
        /*}*/
        __this_net->is_open = FALSE;
        net_video_rec_free_buf();
        break;
    case NET_VIDREC_STATE_NOTIFY:
        sprintf(buf, "status:%d", ((__this->state == VIDREC_STA_START) ? 1 : 0));
        CTP_CMD_COMBINED(__this_net->priv, CTP_NO_ERR, "VIDEO_CTRL", "NOTIFY", buf);
        break;
    default :
        return ;
    }
}

static int net_video_rec_device_event_handler(struct device_event *dev)
{
    int err;
    struct intent it;
    u8 *tpye;

    if (!ASCII_StrCmp(dev->arg, "usb_host*", 9)) {
        switch (dev->event) {
        case DEVICE_EVENT_IN:
            tpye = (u8 *)dev->value;
            if (strstr(tpye, "uvc")) {
                __this_net->uvc_id = tpye[3] - '0';
            }
            printf("UVC or msd_storage online %s, id=%d\n", tpye, __this_net->uvc_id);
            break;
        case DEVICE_EVENT_OUT:
            tpye = (u8 *)dev->value;
            if (strstr(tpye, "uvc")) {
                __this_net->uvc_id = tpye[3] - '0';
            }
            printf("UVC or msd_storage offline %s, id=%d\n", tpye, __this_net->uvc_id);
            if (__this && __this->state == VIDREC_STA_START) {
                video_rec_control_doing();
            }
            net_video_rec_event_stop();
            break;
        }
    } else if (!ASCII_StrCmp(dev->arg, "sd*", 4)) {
        switch (dev->event) {
        case DEVICE_EVENT_IN:
            video_rec_sd_event_ctp_notify(1);
            break;
        case DEVICE_EVENT_OUT:
            video_rec_sd_event_ctp_notify(0);
            break;
        }
    } else if (!ASCII_StrCmp(dev->arg, "sys_power", 7)) {
        switch (dev->event) {
        case DEVICE_EVENT_POWER_CHARGER_IN:
            puts("---charger in\n\n");
            if ((__this && __this->state == VIDREC_STA_IDLE) ||
                (__this && __this->state == VIDREC_STA_STOP)) {
                video_rec_control_start();
            }
            break;
        case DEVICE_EVENT_POWER_CHARGER_OUT:
            puts("---charger out\n");
            if (__this && __this->state == VIDREC_STA_START) {
                video_rec_control_doing();
            }
            break;
        }
    }
    return 0;
}
static int net_video_key_event_handler(struct key_event *key)
{
    int ret = 0;
    static char state = 1;

    switch (key->action) {
    case KEY_EVENT_CLICK:
        printf("KEY_EVENT_CLICK \n");
        int cmd_video_rec_ctl(char start);
        cmd_video_rec_ctl(state);
        state = !state;
        ret = true;
        break;
    case KEY_EVENT_LONG:
        printf("KEY_EVENT_LONG \n");
        int cmd_take_photo_ctl(char start);
        cmd_take_photo_ctl(1);
        ret = true;
        break;
    default:
        break;
    }
    return ret;
}
int net_video_event_hander(void *e)
{
    struct net_event *event = (struct net_event *)e;
    struct ctp_arg *event_arg = (struct ctp_arg *)event->arg;
    /* struct net_event *net = &event->u.net; */

    switch (event->event) {
    case NET_EVENT_CMD:
        /*printf("IN NET_EVENT_CMD\n\n\n\n");*/
        ctp_cmd_analysis(event_arg->topic, event_arg->content, event_arg->cli);
        if (event_arg->content) {
            free(event_arg->content);
        }
        event_arg->content = NULL;
        if (event_arg) {
            free(event_arg);
        }
        event_arg = NULL;
        return true;
        break;
    case NET_EVENT_DATA:
        /* printf("IN NET_EVENT_DATA\n"); */
        break;
    case NET_EVENT_SMP_CFG_FINISH:
        printf(">>>>>>>>>>>>>>>>>>NET_EVENT_SMP_CFG_FINISH");
        config_network_stop();
        config_network_connect();
        break;
    }
    return false;
}
static void net_video_server_task(void *p)
{
    int res;
    int msg[16];

    if (os_mutex_create(&net_vdrec_mutex) != OS_NO_ERR) {
        printf("net_video_server_task , os_mutex_create err !!!\n\n");
        return;
    }
    net_video_handler_init();
    printf("net_video_server_task running\n\n");

    while (1) {

        res = os_task_pend("taskq", msg, ARRAY_SIZE(msg));
        if (os_task_del_req(OS_TASK_SELF) == OS_TASK_DEL_REQ) {
            os_task_del_res(OS_TASK_SELF);
        }

        switch (res) {
        case OS_TASKQ:
            switch (msg[0]) {
            case Q_EVENT:
                break;
            case Q_MSG:
                net_video_rec_ioctl((u32)msg[1]);
                break;
            default:
                break;
            }
            break;
        case OS_TIMER:
            break;
        case OS_TIMEOUT:
            break;
        }
    }
    os_mutex_del(&net_vdrec_mutex, OS_DEL_ALWAYS);
}

void net_video_server_init(void)
{
    task_create(net_video_server_task, 0, "net_video_server");
}


/*录像app的事件总入口*/
static int net_video_rec_event_handler(struct application *app, struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        return net_video_key_event_handler((struct key_event *)event->payload);
    case SYS_DEVICE_EVENT:
        return net_video_rec_device_event_handler((struct device_event *)event->payload);
    /*return video_rec_device_event_action((struct device_event *)event->payload);//录卡时:设备事件和 vidoe_rec公用一个handler，*/
    case SYS_NET_EVENT:
        net_video_event_hander((void *)event->payload);
        return true;
    default:
        return false;
    }
}

static const struct application_operation net_video_rec_ops = {
    .state_machine  = net_video_rec_state_machine,
    .event_handler 	= net_video_rec_event_handler,
};

REGISTER_APPLICATION(app_video_rec) = {
    .name 	= "net_video_rec",
    .ops 	= &net_video_rec_ops,
    .state  = APP_STA_DESTROY,
};


