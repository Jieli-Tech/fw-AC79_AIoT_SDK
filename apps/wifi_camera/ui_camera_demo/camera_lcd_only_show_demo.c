#include "app_config.h"
#ifdef CONFIG_UI_ENABLE

#include "storage_device.h"
#include "os/os_api.h"
#include "generic/lbuf.h"
#include "fs/fs.h"
#include "yuv_soft_scalling.h"
#include "yuv_to_rgb.h"
#include "video_rec.h"

/****************************本文件使用lbuff的yuv接收队列实现提高显示帧率***********************************/
//本文件例子：函数根据LCD屏幕驱动的分辨率自动完成对应缩放推屏显示，不支持配合UI合成！！！

extern int lcd_get_color_format_rgb24(void);
extern void lcd_show_frame_to_dev(u8 *buf, u32 len);//该接口显示数据直接推送数据到LCD设备接口，不分数据格式，慎用！
extern void lcd_get_width_height(int *width, int *height);
extern void user_lcd_init(void);
extern void get_yuv_init(void (*cb)(u8 *data, u32 len, int width, int height));

#define YUV_BUFF_FPS		3 //yuv缓存区域帧数
#define YUV_FPS_DBG			1 //1:打开yuv到rgb到屏显帧率测试
#define USE_CUT_TO_LCD      1 //1:使用裁剪推数据到屏丢视角  0使用缩放不丢视角  裁剪要比缩放帧数高
#define YUV2RGB_TASK_NAME	"yuv2rgb"

struct yuv_buffer {
    u32 len;
    u8 data[0];
};
struct yuv2rgb {
    void *lbuf;
    void *yuv_buf;
    void *rgb_buf;
    u16 width;
    u16 height;
    u32 yuv_buf_size;
    u8 kill;
    u8 init;
    u8 index;
};
static struct yuv2rgb yuv2rgb_info = {0};
#define __this (&yuv2rgb_info)


//本函数根据LCD屏幕驱动的分辨率自动完成对应缩放推屏显示，不支持配合UI合成！！！
//使用本函数前，先移植LCD驱动才能显示！！！
static void camera_show_lcd(u8 *buf, u32 size, int width, int height)
{
    int src_w = width;
    int src_h = height;
    int out_w;
    int out_h;
    int out_size;
    char rgb24 = 0;
    u32 msg[2];
    struct yuv_buffer *yuv = NULL;

    if (!__this->init || __this->kill) {
        return ;
    }
    rgb24 = lcd_get_color_format_rgb24();
    lcd_get_width_height(&out_w, &out_h);
    if (out_w <= 0 || out_h <= 0) {
        printf("lcd_get_width_height err \n");
        return ;
    }
    if (!__this->yuv_buf) {
        __this->width = out_w;
        __this->height = out_h;
        __this->yuv_buf_size = out_w * out_h * 3 / 2 * YUV_BUFF_FPS + 256;
        __this->yuv_buf = malloc(__this->yuv_buf_size);
        if (!__this->yuv_buf) {
            printf("no mem size = %d \n", __this->yuv_buf_size);
            return ;
        }
        __this->lbuf = lbuf_init(__this->yuv_buf, __this->yuv_buf_size, 4, sizeof(struct yuv_buffer));
        yuv = lbuf_alloc(__this->lbuf, out_w * out_h * 3 / 2);
    } else {
        if ((out_w * out_h * 3 / 2) < lbuf_free_space(__this->lbuf)) {
            yuv = lbuf_alloc(__this->lbuf, out_w * out_h * 3 / 2);
            if (yuv) {
                yuv->len = out_w * out_h * 3 / 2;
            }
        }
    }
    if (!yuv) {
        return;
    }
#if USE_CUT_TO_LCD
    YUV420p_Cut(buf, src_w, src_h, yuv->data, yuv->len, 0, out_w, 0, out_h);//裁剪取屏大小数据
#else
    YUV420p_Soft_Scaling(buf, yuv->data, src_w, src_h, out_w, out_h);//缩放到LCD指定的宽高
#endif
    lbuf_push((void *)yuv, BIT(__this->index));
    os_taskq_post_type(YUV2RGB_TASK_NAME, Q_MSG, 0, NULL);
}
static void yuv2rgb_task(void)
{
    u32 time_1 = 0, time_2 = 0;
    u32 cnt = 0;
    u32 out_size;
    char rgb24;
    int res;
    u32 msg[8];

    __this->yuv_buf = NULL;
    __this->rgb_buf = NULL;
    __this->init = true;
    __this->index = 0;
    while (!__this->yuv_buf && !__this->kill) {
        os_time_dly(1);
    }
    if (__this->kill) {
        goto exit;
    }
    rgb24 = lcd_get_color_format_rgb24();
    __this->rgb_buf = malloc(__this->width * __this->height * (rgb24 ? 3 : 2));
    out_size = __this->width * __this->height * (rgb24 ? 3 : 2);
    if (!__this->rgb_buf) {
        printf("err no mem in %s \n", __func__);
        goto exit;
    }
    printf("yuv2rgb_task init\n");
    while (1) {
#if YUV_FPS_DBG
        time_1 = timer_get_ms();
        if (time_1 - time_2 > 1000) {
            time_2 = timer_get_ms();
            printf("lcd cnt = %d \n", cnt);
            cnt = 0;
        }
#endif
        res = os_task_pend("taskq", msg, ARRAY_SIZE(msg));
        if (__this->kill) {
            goto exit;
        }
        switch (res) {
        case OS_TASKQ:
            switch (msg[0]) {
            case Q_MSG:
                ;
                struct yuv_buffer *yuv = lbuf_pop(__this->lbuf, BIT(__this->index));
                if (yuv) {
                    if (rgb24) {
                        yuv420p_quto_rgb24(yuv->data, __this->rgb_buf, __this->width, __this->height);//YUV转RGB
                    } else {
                        yuv420p_quto_rgb565(yuv->data, __this->rgb_buf, __this->width, __this->height, 1);//YUV转RGB
                    }
                    /**************推屏显示******************/
                    lcd_show_frame_to_dev(__this->rgb_buf, out_size);//显示一帧摄像头数据

                    /****************************************/
                    lbuf_free(yuv);
                    cnt++;
                }
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
    }
exit:
    __this->init = 0;
    if (__this->yuv_buf) {
        free(__this->yuv_buf);
        __this->yuv_buf = NULL;
    }
    if (__this->rgb_buf) {
        free(__this->rgb_buf);
        __this->rgb_buf = NULL;
    }
    __this->kill = 0;
}
void yuv2rgb_task_kill(void)
{
    if (__this->init) {
        __this->kill = true;
        os_taskq_post_type(YUV2RGB_TASK_NAME, Q_MSG, 0, NULL);
        while (__this->kill) {
            os_time_dly(1);
        }
        printf("yuv2rgb_task kill ok\n");
    }
}

#ifdef CONFIG_UVC_VIDEO2_ENABLE
extern int jpeg2yuv_open(void);
extern void jpeg2yuv_yuv_callback_register(void (*cb)(u8 *data, u32 len, int width, int height));
extern int jpeg2yuv_jpeg_frame_write(u8 *buf, u32 len);
extern void jpeg2yuv_close(void);
extern int user_net_video_rec_open(char forward);
extern int user_net_video_rec_close(char forward);
extern void set_video_rt_cb(u32(*cb)(void *, u8 *, u32), void *priv);

static void jpeg_cb(void *hdr, void *data, u32 len)
{
#define JPEG_HEAD 0xE0FFD8FF
    u32 *head = (u32 *)data;
    if (*head == JPEG_HEAD) {
        //video
        jpeg2yuv_jpeg_frame_write((u8 *)data, len);
    } else {
        //audio
    }
}
static void uvc_video_jpeg2yuv_task(void)
{
    int ret;
    //0.等待UVC上线
start:
    while (!dev_online("uvc")) {
        os_time_dly(5);
    }
    //1.打开jpeg解码YUV
    ret = jpeg2yuv_open();
    if (ret) {
        return;
    }
    //2.注册YUV数据回调函数
    jpeg2yuv_yuv_callback_register(camera_show_lcd);
    //3.打开UVC实时流
    ret = user_net_video_rec_open(1);
    if (ret) {
        jpeg2yuv_close();
        return;
    }
    //4.注册jpeg数据回调函数
    set_video_rt_cb(jpeg_cb, NULL);

#if	TCFG_LCD_USB_SHOW_COLLEAGUE
    void set_tcp_uvc_rt_cb(u32(*cb)(void *priv, void *data, u32 len));
    void set_udp_uvc_rt_cb(u32(*cb)(void *priv, void *data, u32 len));
    set_udp_uvc_rt_cb(jpeg_cb); //wifi udp
    set_tcp_uvc_rt_cb(jpeg_cb); //wifi tcp
#endif
    //5.检查是否掉线
    while (dev_online("uvc")) {
        os_time_dly(5);
    }
    //6.关闭UVC实时流
    user_net_video_rec_close(1);
    //7.关闭jpeg解码YUV
    jpeg2yuv_close();
    goto start;
    //8.删除任务
    yuv2rgb_task_kill();
}
static void uvc_video_init(void)
{
    int thread_fork(const char *thread_name, int prio, int stk_size, u32 qsize, int *pid, void (*func)(void *), void *parm);
    thread_fork("uvc_video_jpeg2yuv_task", 15, 1024, 0, 0, uvc_video_jpeg2yuv_task, NULL);
}
#endif

//外部使用初始化lcd驱动，注册YUV回调，创建任务发送数据
void camera_to_lcd_test(void)
{
    user_lcd_init();
#ifdef CONFIG_UVC_VIDEO2_ENABLE
    uvc_video_init();
#else
    get_yuv_init(camera_show_lcd);
#endif
    thread_fork(YUV2RGB_TASK_NAME, 16, 1024 * 1, 128, 0, yuv2rgb_task, NULL);
}
#endif
