#include "app_config.h"
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


/* 开关打印提示 */
#if 1
#define log_info(x, ...)    printf("\n[camera_to_lcd_test]>>>>>>>>>>>>>>>>>>>###" x " \n", ## __VA_ARGS__)
#else
#define log_info(...)
#endif

#define in_w 		640
#define in_h 		480
#define yuv_out_w   2000
#define yuv_out_h   2000
#define led_w       320
#define led_h       240
#define yuv_y_out_w 672
#define yuv_y_out_h 384


/********保存YUV数据到SD卡中************/
static void save_YUV_date_ontime(u8 *buf, u32 len)
{
    if (storage_device_ready()) {
        char file_name[64];//定义路径存储
        snprintf(file_name, 64, CONFIG_ROOT_PATH"YUV/YUV***.bin");
        FILE *fd = fopen(file_name, "wb+");
        fwrite(buf, 1, len, fd);
        fclose(fd);
        log_info("YUV save ok name=YUV\r\n");
    }
}
/********读取JPG数据并保存在SD卡中该数据经过缩放保存大小1280*720********/
static void get_JPG_save_to_SD(char *yuv_img_buf)
{
    struct jpeg_encode_req req = {0};
    u32 buf_len =  yuv_out_w * yuv_out_h * 3 / 2 ;
    u8 *yuv_out_buf =  malloc(buf_len);
    u8 err = 0;

    if (!yuv_out_buf) {
        log_info("jpeg buf not enough err!!!\n");
        return;
    }

    YUV420p_Soft_Scaling(yuv_img_buf, yuv_out_buf, in_w, in_h, yuv_out_w, yuv_out_h);
    //q >=2 , size  = w*h*q*0.32
    req.q = 13;	//jepg编码质量(范围0-13),0最小 质量越好,下面的编码buf需要适当增大
    req.format = JPG_SAMP_FMT_YUV420;
    req.data.buf = yuv_out_buf;
    req.data.len = buf_len;
    req.width = yuv_out_w ;
    req.height =  yuv_out_h ;
    req.y = yuv_out_buf;
    req.u = req.y + (yuv_out_w * yuv_out_h);
    req.v = req.u + (yuv_out_w * yuv_out_h) / 4;
    err = jpeg_encode_one_image(&req);//获取一张JPG图片

    if (!err) {
        save_jpg_ontime(req.data.buf, buf_len); //保存图片到SD卡
        free(yuv_out_buf);
    } else {
        log_info("get_jpg err!!!\n");
        free(yuv_out_buf);
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

    get_JPG_save_to_SD(yuv_buf);//将YUV数据保存在SD卡*/
}
void camera_to_lcd_init(void)
{
    get_yuv_init(get_yuv);//GET_YUV该函数会在后台以摄像头出图帧数被回调
}

void camera_to_lcd_uninit(void)
{
    void get_yuv_uninit(void);
    get_yuv_uninit();
}

static int get_jpg_test_task(void)//更多有关摄像头应用请参考WIFI_CANERA中UI_CAMERA文件夹 以及 wifi_story
{
    camera_to_lcd_init();//需要包含gei_yuv_data.c
}

int get_jpg_test(void)
{
    return thread_fork("get_jpg_test_task", 18, 512, 0, NULL, get_jpg_test_task, NULL);
}
