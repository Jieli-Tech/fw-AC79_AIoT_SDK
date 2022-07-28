#include "app_config.h"
#include "device/device.h"//u8
#include "lcd_config.h"//LCD_h
#include "system/includes.h"//late_initcall
#include "yuv_soft_scalling.h"//YUV420p_Soft_Scaling
#include "lcd_te_driver.h"//set_lcd_show_data_mode
#include "get_yuv_data.h"//get_yuv_init

#include "device/device.h"//u8
#include "storage_device.h"//SD
#include "server/video_dec_server.h"//fopen
#include "system/includes.h"//GPIO
#include "lcd_drive.h"
#include "sys_common.h"
#include "yuv_soft_scalling.h"
#include "asm/jpeg_codec.h"
#include "lcd_te_driver.h"
#include "get_yuv_data.h"
#include "lcd_config.h"

#ifdef USE_CAMERA_DVP_SHOW_TO_LCD_DEMO

#define CAMERA_YUV420_BUF_LEN   (CONFIG_VIDEO_IMAGE_W * CONFIG_VIDEO_IMAGE_W * 3 / 2)

OS_SEM yuv_soft_sem;

static struct lbuf_test_head {
    u8 data[0];
};

static void Calculation_frame(void)
{
    static u32 tstart = 0, tdiff = 0;
    static u8 fps_cnt = 0;

    fps_cnt++ ;

    if (!tstart) {
        tstart = timer_get_ms();
    } else {
        tdiff = timer_get_ms() - tstart;

        if (tdiff >= 1000) {
            printf("\n [get_yuv]fps_count = %d\n", fps_cnt *  1000 / tdiff);
            tstart = 0;
            fps_cnt = 0;
        }
    }
}

static void Calculation_frame1(void)
{
    static u32 tstart = 0, tdiff = 0;
    static u8 fps_cnt = 0;

    fps_cnt++ ;

    if (!tstart) {
        tstart = timer_get_ms();
    } else {
        tdiff = timer_get_ms() - tstart;

        if (tdiff >= 1000) {
            printf("\n [yuv_soft]fps_count = %d\n", fps_cnt *  1000 / tdiff);
            tstart = 0;
            fps_cnt = 0;
        }
    }
}

static void *lbuf_ptr = NULL;
struct lbuff_head *yuv_lbuf_hdl = NULL;
static struct lbuff_head *lib_system_lbuf_test_init(u32 buf_size)
{
    struct lbuff_head *lbuf_handle = NULL;
    lbuf_ptr = malloc(buf_size);

    if (lbuf_ptr == NULL) {
        printf("lbuf malloc buf err");
        return NULL;
    }

    //lbuf初始化:
    lbuf_handle = lbuf_init(lbuf_ptr, buf_size, 4, sizeof(struct lbuf_test_head));

    return lbuf_handle;
}

int isc_log_en()//如果定义该函数丢帧信息屏蔽
{
    return 0;
}
/******数据流程******************/
/******yuv回调出数据后转为对应屏幕大小YUV交给下一个线程处理***********/
/******这样显示和yuv资源占用差不多才能同步均匀*************************************/
static void get_yuv(u8 *yuv_buf, u32 len, int yuv_in_w, int yuv_in_h)//YUV数据回调线程
{
    Calculation_frame();
    struct lbuf_test_head *wbuf = NULL;

    if (lbuf_free_space(yuv_lbuf_hdl) < (CAMERA_YUV420_BUF_LEN)) { //查询LBUF空闲数据块是否有足够长度
        printf("%s >>>note lbuf lbuf_free_space fail have = %dk\r\n", __func__, lbuf_free_space(yuv_lbuf_hdl) / 1024);
        return ;
    }
    wbuf = (struct lbuf_test_head *)lbuf_alloc(yuv_lbuf_hdl, CAMERA_YUV420_BUF_LEN); //lbuf内申请一块空间
    if (wbuf != NULL) {
        memcpy(wbuf->data, yuv_buf, CAMERA_YUV420_BUF_LEN);
        lbuf_push(wbuf, BIT(0));//把数据块推送更新到lbuf的通道0
        os_sem_post(&yuv_soft_sem);
    } else {
        printf("%s >>>lbuf no buf\r\n", __func__);
        return ;
    }
}

static void yuv_soft_task(void)
{
    struct lbuf_test_head *rbuf = NULL;
    while (1) {
        os_sem_pend(&yuv_soft_sem, 0);
        Calculation_frame1();
        if (lbuf_empty(yuv_lbuf_hdl)) {//查询LBUF内是否有数据帧
            printf("%s >>>note lbuf lbuf_empty fail\r\n", __func__);
        } else {
            rbuf = (struct lbuf_test_head *)lbuf_pop(yuv_lbuf_hdl, BIT(0));//从lbuf的通道0读取数据块
            if (rbuf == NULL) {
                printf("%s >>>note lbuf rbuf == NULL\r\n", __func__);
            } else {
                YUV420p_Soft_Scaling(rbuf->data, NULL, CONFIG_VIDEO_IMAGE_W, CONFIG_VIDEO_IMAGE_H, LCD_W, LCD_H);
                lcd_show_frame(rbuf->data, LCD_YUV420_DATA_SIZE); //这里输出的是对应屏幕大小的YUV数据 发送到TE线程处理数据
                lbuf_free(rbuf);
            }
        }
    }
}

static void camera_to_lcd_fps_task(void)
{
    static struct lcd_device *lcd_dev;
    /******ui_lcd_一起初始化数据***********/
    user_ui_lcd_init();

    /*****默认为UI模式*****/
    set_lcd_show_data_mode(camera);

    //主要该方法用于优化显示流程需要自行将 get_yuv_data.c 123行设置为0使用单BUF用于增快数据获取速度
    /******YUV数据回调初始化**********/
    get_yuv_init(get_yuv);
}

static int camera_to_lcd_fps_task_init(void)
{
    puts("camera_to_lcd_fps_task_init \n\n");
    os_sem_create(&yuv_soft_sem, 0);

    yuv_lbuf_hdl = lib_system_lbuf_test_init(CAMERA_YUV420_BUF_LEN * 2 + 128);
    thread_fork("yuv_soft_task", 9, 1024, 32, 0, yuv_soft_task, NULL);
    thread_fork("camera_to_lcd_fps_task", 22, 1024, 32, 0, camera_to_lcd_fps_task, NULL);
    return 0;
}
late_initcall(camera_to_lcd_fps_task_init);

#endif
