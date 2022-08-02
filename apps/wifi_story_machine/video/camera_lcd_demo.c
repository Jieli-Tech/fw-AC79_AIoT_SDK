#include "app_config.h"

#if defined CONFIG_UI_ENABLE && defined CONFIG_VIDEO_ENABLE

#include "device/device.h"
#include "os/os_api.h"
#include "generic/lbuf.h"
#include "yuv_soft_scalling.h"
#include "yuv_to_rgb.h"
#include "system/sys_time.h"
#include "system/init.h"

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
    int pid;
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
        return;
    }
    rgb24 = lcd_get_color_format_rgb24();
    lcd_get_width_height(&out_w, &out_h);
    if (out_w <= 0 || out_h <= 0) {
        printf("lcd_get_width_height err \n");
        return;
    }
    if (!__this->yuv_buf) {
        __this->width = out_w;
        __this->height = out_h;
        __this->yuv_buf_size = out_w * out_h * 3 / 2 * YUV_BUFF_FPS + 256;
        __this->yuv_buf = malloc(__this->yuv_buf_size);
        if (!__this->yuv_buf) {
            printf("no mem size = %d \n", __this->yuv_buf_size);
            return;
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
    lbuf_push(yuv, BIT(__this->index));
    os_taskq_post_type(YUV2RGB_TASK_NAME, Q_MSG, 0, NULL);
}

static void yuv2rgb_task(void *priv)
{
    u32 time_1 = 0, time_2 = 0;
    u32 cnt = 0;
    u32 out_size;
    char rgb24;
    int res;
    int msg[4];

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
    if (__this->yuv_buf) {
        free(__this->yuv_buf);
        __this->yuv_buf = NULL;
    }
    if (__this->rgb_buf) {
        free(__this->rgb_buf);
        __this->rgb_buf = NULL;
    }
    __this->init = 0;
    __this->kill = 0;
}

static void yuv2rgb_task_kill(void)
{
    if (__this->init) {
        __this->kill = true;
        os_taskq_post_type(YUV2RGB_TASK_NAME, Q_MSG, 0, NULL);
        thread_kill(&__this->pid, KILL_WAIT);
    }
}

#ifdef CONFIG_UVC_VIDEO2_ENABLE
extern int jpeg2yuv_open(void);
extern void jpeg2yuv_yuv_callback_register(void (*cb)(u8 *data, u32 len, int width, int height));
extern int jpeg2yuv_jpeg_frame_write(u8 *buf, u32 len);
extern void jpeg2yuv_close(void);
extern int user_video_rec0_open(int dev);
extern int user_video_rec0_close(void);
extern void set_video_rt_cb(u32(*cb)(void *, u8 *, u32), void *priv);
extern int uvc_host_online(void);

static u32 jpeg_cb(void *hdr, u8 *data, u32 len)
{
#define JPEG_HEAD 0xE0FFD8FF
    u32 *head = (u32 *)data;
    if (*head == JPEG_HEAD) {
        //video
        jpeg2yuv_jpeg_frame_write(data, len);
    } else {
        //audio
    }

    return 0;
}

static void uvc_video_jpeg2yuv_task(void *priv)
{
    int ret;

    os_time_dly(50);

    while (1) {
        //0.等待UVC上线
        while (!dev_online("uvc")) {
            os_time_dly(5);
        }

        //1.打开jpeg解码YUV
        ret = jpeg2yuv_open();
        if (ret) {
            break;
        }

        thread_fork(YUV2RGB_TASK_NAME, 6, 512, 128, &__this->pid, yuv2rgb_task, NULL);

        //2.注册YUV数据回调函数
        jpeg2yuv_yuv_callback_register(camera_show_lcd);

        //3.打开UVC实时流
        ret = user_video_rec0_open(uvc_host_online() ? 3 : 2);
        if (ret) {
            jpeg2yuv_close();
            yuv2rgb_task_kill();
            os_time_dly(100);
            continue;
        }

        //4.注册jpeg数据回调函数
        set_video_rt_cb(jpeg_cb, NULL);

        //5.检查是否掉线
        while (dev_online("uvc")) {
            os_time_dly(5);
        }

        //6.关闭UVC实时流
        user_video_rec0_close();

        //7.关闭jpeg解码YUV
        jpeg2yuv_close();

        //8.删除任务
        yuv2rgb_task_kill();
    }
}

static int uvc_video_init(void)
{
    return thread_fork("uvc_video_jpeg2yuv_task", 4, 512, 0, 0, uvc_video_jpeg2yuv_task, NULL);
}

int uvc_video_jpeg2yuv_start(int usb_id)
{
    //1.打开jpeg解码YUV
    if (jpeg2yuv_open()) {
        return -1;
    }

    thread_fork(YUV2RGB_TASK_NAME, 6, 512, 128, &__this->pid, yuv2rgb_task, NULL);

    //2.注册YUV数据回调函数
    jpeg2yuv_yuv_callback_register(camera_show_lcd);

    //3.打开UVC实时流
    if (user_video_rec0_open(uvc_host_online() ? 3 : 2)) {
        jpeg2yuv_close();
        yuv2rgb_task_kill();
        return -1;
    }

    //4.注册jpeg数据回调函数
    set_video_rt_cb(jpeg_cb, NULL);

    return 0;
}

int uvc_video_jpeg2yuv_stop(void)
{
    user_video_rec0_close();

    //7.关闭jpeg解码YUV
    jpeg2yuv_close();

    //8.删除任务
    yuv2rgb_task_kill();

    return 0;
}
#endif

static int camera_to_lcd_init(void)
{
    user_lcd_init();

#ifdef CONFIG_UVC_VIDEO2_ENABLE
    /* uvc_video_init(); */
#else
    get_yuv_init(camera_show_lcd);
#endif

    return 0;
}
late_initcall(camera_to_lcd_init);

#endif
