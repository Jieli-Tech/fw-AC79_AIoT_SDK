#include "app_config.h"
#include "system/includes.h"
#include "device/device.h"

static void jpeg_yuv_test(void)
{
    int jpeg_decoder_test(const char *path);
    int jpeg_encode_test(const char *path, int width, int height);
    int jpeg_encode_large_test(const char *path, int width, int height, int out_width, int out_height);
    int storage_device_ready(void);

    while (!storage_device_ready()) {//等待sd文件系统挂载完成
        os_time_dly(2);
    }
    //jpeg_decoder_test(CONFIG_ROOT_PATH"test.jpg");//JPG解码成YUV，YUV再转RGB
    jpeg_encode_test(CONFIG_ROOT_PATH"test.yuv", 320, 240);//YUV编码成JPG
    jpeg_encode_large_test(CONFIG_ROOT_PATH"test.yuv", 320, 240, 1280, 720);//YUV编码成JPG，大分辨率或者小分辨率（自动完成缩放）
    while (1) {
        os_time_dly(10);
    }
}
static int c_main(void)
{
    os_task_create(jpeg_yuv_test, NULL, 12, 1000, 0, "jpeg_yuv_test");
    return 0;
}
late_initcall(c_main);
