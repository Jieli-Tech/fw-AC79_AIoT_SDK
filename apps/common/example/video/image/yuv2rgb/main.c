#include "app_config.h"
#include "system/includes.h"
#include "device/device.h"
#include "yuv_to_rgb.h"
#include "fs/fs.h"

static void yuv2rgb_test(void)
{
    FILE *fd = NULL;
    int len;
    int w = 320;//YUV420数据分辨率宽
    int h = 240;//YUV420数据分辨率高
    void *yuv = NULL;
    void *rgb = NULL;
    int ret;

    extern int storage_device_ready(void);
    while (!storage_device_ready()) {//等待sd文件系统挂载完成
        os_time_dly(2);
    }

    //1.从SD卡打开yuv文件数据到内存中
    fd = fopen(CONFIG_ROOT_PATH"test.yuv", "rb");
    if (fd == NULL) {
        printf("no file\n");
        goto exit;
    }
    len = flen(fd);//读取YUV数据长度
    yuv = malloc(len);
    if (!yuv) {
        printf("yuv buf malloc err \n");
        goto exit;
    }
    if (fread(yuv, 1, len, fd) != len) {//读取YUV数据
        printf("read file len err \n");
        goto exit;
    }
    fclose(fd);
    fd = NULL;

    //2.申请缩放的内存
    rgb = malloc(w * h * 3);//RGB24数据量: 分辨率*3
    if (!rgb) {
        printf("rgb buf malloc err \n");
        goto exit;
    }

    //3.YUV转RGB24
    yuv420p_quto_rgb24(yuv, rgb, w, h);
    fd = fopen(CONFIG_ROOT_PATH"test_rgb24.rgb", "w+");//创建文件
    if (fd) {
        fwrite(rgb, 1, w * h * 3, fd);//rgb24 : w * h * 2
        fclose(fd);
        fd = NULL;
        printf("YUV2RGB24 write file ok\n");
    }

    //4.YUV转RGB565
    yuv420p_quto_rgb565(yuv, rgb, w, h, 1);//转RGB565-BE大端模式
    fd = fopen(CONFIG_ROOT_PATH"test_rgb565.rgb", "w+");//创建文件
    if (fd) {
        fwrite(rgb, 1, w * h * 2, fd);//rgb565 : w * h * 2
        fclose(fd);
        fd = NULL;
        printf("YUV2RGB565 write file ok\n");
    }

    //5.RGB565转YUV
    fd = fopen(CONFIG_ROOT_PATH"test_rgb565.rgb", "rb");//读取文件
    if (fd) {
        fread(rgb, 1, w * h * 2, fd);//rgb565 : w * h * 2
        fclose(fd);
        fd = NULL;

        //RGB565转YUV
        rgb565_to_yuv420p(rgb, yuv, w, h, 1);//RGB565-BE大端模式转YUV420P

        fd = fopen(CONFIG_ROOT_PATH"test_rgb2yuv.yuv", "w+");//创建文件
        if (fd) {
            fwrite(yuv, 1, w * h * 3 / 2, fd);//yuv420p : w * h * 3 / 2
            fclose(fd);
            fd = NULL;
            printf("RGB2YUV write file ok\n");
        }
    }

exit:
    if (rgb) {
        free(rgb);
        rgb = NULL;
    }
    if (yuv) {
        free(yuv);
        yuv = NULL;
    }
    if (fd) {
        fclose(fd);
        fd = NULL;
    }
    while (1) {//等待被杀（注意：不能自杀）
        os_time_dly(10);
    }
}
static int c_main(void)
{
    os_task_create(yuv2rgb_test, NULL, 12, 1000, 0, "yuv2rgb_test");
    return 0;
}
late_initcall(c_main);
