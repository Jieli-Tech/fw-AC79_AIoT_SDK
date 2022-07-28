#include "app_config.h"
#include "system/includes.h"
#include "device/device.h"
#include "yuv_soft_scalling.h"
#include "fs/fs.h"

static void yuv_scalling_test(void)
{
    FILE *fd = NULL;
    int len;
    int w = 320;//YUV420数据分辨率宽
    int h = 240;//YUV420数据分辨率高
    int scal_w = 168;//480;//YUV420缩放后的宽
    int scal_h = 128;//320;//YUV420缩放后的高
    void *yuv = NULL;
    void *yuv_scanl = NULL;
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
    yuv_scanl = malloc(scal_w * scal_h * 3 / 2);//YUV420大小：分辨率*1.5
    if (!yuv_scanl) {
        printf("yuv_scanl buf malloc err \n");
        goto exit;
    }

    //3.缩放YUV
    ret = YUV420p_Soft_Scaling(yuv, yuv_scanl, w, h, scal_w, scal_h);//返回值为缩放后的数据量大小
    if (ret) {
        fd = fopen(CONFIG_ROOT_PATH"test_scal.yuv", "w+");//创建文件
        if (fd) {
            fwrite(yuv_scanl, 1, ret, fd);
            fclose(fd);
            fd = NULL;
            printf("YUV420p_Soft write file ok\n");
        }
    }

exit:
    if (yuv_scanl) {
        free(yuv_scanl);
        yuv_scanl = NULL;
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
    os_task_create(yuv_scalling_test, NULL, 12, 1000, 0, "yuv_scalling_test");
    return 0;
}
late_initcall(c_main);
