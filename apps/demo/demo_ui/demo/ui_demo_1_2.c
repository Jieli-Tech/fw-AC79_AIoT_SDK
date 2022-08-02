#include "app_config.h"
#include "device/device.h"//u8
#include "server/video_dec_server.h"//fopen
#include "system/includes.h"//GPIO
#include "lcd_drive.h"
#include "lcd_config.h"
#include "sys_common.h"
#include "yuv_soft_scalling.h"
#include "asm/jpeg_codec.h"
#include "lcd_te_driver.h"
#include "ename.h"
#include "get_yuv_data.h"

#include "ename.h"
#include "ui/ui.h"
#include "ui_api.h"
#include "ui/includes.h"
#include "font/font_all.h"
#include "font/font_textout.h"
#include "font/language_list.h"


#ifdef UI_DEMO_1_2

int isc_log_en()//如果定义该函数丢帧信息屏蔽
{
    return 0;
}
static void Calculation_frame(void)
{
    struct unumber timer;
    static u32 tstart = 0, tdiff = 0;
    static u8 fps_cnt = 0;

    fps_cnt++ ;

    if (!tstart) {
        tstart = timer_get_ms();
    } else {
        tdiff = timer_get_ms() - tstart;

        if (tdiff >= 1000) {
            printf("\n [MSG]fps_count = %d\n", fps_cnt *  1000 / tdiff);
            timer.numbs = 2;
            timer.type = TYPE_NUM;
            timer.number[0] = fps_cnt *  1000 / tdiff;
            ui_number_update_by_id(BASEFORM_80, &timer);

            tstart = 0;
            fps_cnt = 0;
        }
    }
}
static void save_YUV_date_ontime(u8 *buf, u32 len)
{
    if (storage_device_ready()) {
        char file_name[64];//定义路径存储
        snprintf(file_name, 64, CONFIG_ROOT_PATH"YUV/YUV***.bin");
        FILE *fd = fopen(file_name, "wb+");
        fwrite(buf, 1, len, fd);
        fclose(fd);
        /*log_info("YUV save ok name=YUV\r\n");*/
    }
}
/******数据流程******************/
/******最优数据流程 yuv回调出数据后转为对应屏幕大小YUV交给下一个线程处理***********/
/******这样显示和yuv资源占用差不多才能同步均匀*************************************/
/******同一先线程处理所有事情速度响应跟不上无法达到理想帧数************************/
static void get_yuv(u8 *yuv_buf, u32 len, yuv_in_w, yuv_in_h)//YUV数据回调线程
{
    /*save_YUV_date_ontime(yuv_buf,len);//将YUV数据保存在SD卡*/
    /*******将YUV输出数据转成屏幕大小的YUV*********************/
    YUV420p_Soft_Scaling(yuv_buf, NULL, yuv_in_w, yuv_in_h, LCD_W, LCD_H);
    lcd_show_frame(yuv_buf, LCD_YUV420_DATA_SIZE); //这里输出的是对应屏幕大小的YUV数据 发送到TE线程处理数据
    Calculation_frame();
}

static void camera_to_lcd_fps_task(void)
{
    static struct lcd_device *lcd_dev;
    /******ui_lcd_一起初始化***********/
    user_ui_lcd_init();

    /******开机图片********************/
    ui_show_main(PAGE_2); //开机图片
    os_time_dly(100);
    ui_hide_curr_main(); //关闭上一个画面

    ui_show_main(PAGE_7); //开机图片
    static char str1[] = "摄像头帧数测试";
    static char str2[] = "fps";
    ui_show_main(PAGE_7); //开机图片
    ui_text_set_textu_by_id(BASEFORM_78, str1, strlen(str1), FONT_DEFAULT | FONT_SHOW_MULTI_LINE);
    ui_text_set_textu_by_id(BASEFORM_79, str2, strlen(str2), FONT_DEFAULT | FONT_SHOW_MULTI_LINE);

    set_lcd_show_data_mode(ui_camera);
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
