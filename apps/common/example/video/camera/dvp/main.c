#include "app_config.h"
#include "device/device.h"//u8
#include "lcd_config.h"//LCD_h
#include "system/includes.h"//late_initcall
#include "yuv_soft_scalling.h"//YUV420p_Soft_Scaling
#include "lcd_te_driver.h"//set_lcd_show_data_mode
#include "get_yuv_data.h"//get_yuv_init

#ifdef USE_CAMERA_DVP_SHOW_TO_LCD_DEMO

int isc_log_en()//如果定义该函数丢帧信息屏蔽
{
    return 0;
}


#define FACE_DETECT_DEMO
#ifdef FACE_DETECT_DEMO
#include "child_face_detect.h"
#include "yuv_soft_scalling.h"
face_detect_f e;
static u32 time1 = 0;
static u32 time2 = 0;
#endif



/******数据流程******************/
/******yuv回调出数据后转为对应屏幕大小YUV交给下一个线程处理***********/
/******这样显示和yuv资源占用差不多才能同步均匀*************************************/
static void get_yuv(u8 *yuv_buf, u32 len, int yuv_in_w, int yuv_in_h)//YUV数据回调线程
{
    /*******将YUV输出数据转成屏幕大小的YUV*********************/
    /*YUV420p_Cut(yuv_buf, yuv_in_w, yuv_in_h, yuv_buf, len, 480, LCD_W+480, 240, LCD_H+240);//裁剪取屏大小数据*/
    YUV420p_Soft_Scaling(yuv_buf, NULL, yuv_in_w, yuv_in_h, LCD_W, LCD_H);

#ifdef FACE_DETECT_DEMO
    frame fp = {0};
    time1 = timer_get_ms();
    printf("during:%d ms\n", time1 - time2);
    time2 = time1;
    fp.w = LCD_W;
    fp.h = LCD_H;
    fp.c = 1;
    fp.pixel = yuv_buf;
    int num_box = face_detect_process(&fp, &e, e.confidence_, e.bounding_box, e.alignment_temp);
    printf("num_box=%d \n", num_box);
    if (num_box) {
        printf("x1:%d  x2:%d  y1:%d  y2:%d\n", e.bounding_box[0].x1, e.bounding_box[0].x2, e.bounding_box[0].y1, e.bounding_box[0].y2);
    }
#endif


    lcd_show_frame(yuv_buf, LCD_YUV420_DATA_SIZE); //这里输出的是对应屏幕大小的YUV数据 发送到TE线程处理数据
}

static void camera_to_lcd_fps_task(void)
{
    static struct lcd_device *lcd_dev;
    /******ui_lcd_一起初始化数据***********/
    user_ui_lcd_init();

#ifdef FACE_DETECT_DEMO
    int fast_m = 1;
    int num_r = 6;
    int num_o = 3;
    int thresh[3] = {0, -30, -20};
    int smile_thresh = 0;
    face_detect_init(fast_m, num_r, num_o, thresh, smile_thresh, &e);
#endif




    /*****默认为UI模式*****/
    set_lcd_show_data_mode(camera);

    /******YUV数据回调初始化**********/
    get_yuv_init(get_yuv);
}

static int camera_to_lcd_fps_task_init(void)
{
    puts("camera_to_lcd_fps_task_init \n\n");
    return thread_fork("camera_to_lcd_fps_task", 11, 512, 32, 0, camera_to_lcd_fps_task, NULL);
}
late_initcall(camera_to_lcd_fps_task_init);

#endif
