#include "app_config.h"
#include "system/includes.h"
#include "device/device.h"
#include "fs/fs.h"

#ifdef USE_SD_TEST_DEMO

static void sd_fs_test(void)
{
    extern int storage_device_ready(void);

    while (!storage_device_ready()) {//等待sd文件系统挂载完成
        os_time_dly(2);
    }

    //读写测试
    u8 *test_data = "sd test 123456";
    u8 read_buf[32];
    int len;

    //1.创建文件
    FILE *fd = fopen(CONFIG_ROOT_PATH"test.txt", "w+");
    if (fd) {
        fwrite(test_data, 1, strlen(test_data), fd);
        fclose(fd);
        printf("write file : %s \n", read_buf);
    }

    //2.读取文件
    fd = fopen(CONFIG_ROOT_PATH"test.txt", "r");
    if (fd) {
        len = flen(fd);//获取整个文件大小
        fread(read_buf, 1, len, fd);
        fclose(fd);
        printf("read file : %s \n", read_buf);
    }
    while (1) {
        os_time_dly(2);
    }

}
static int c_main(void)
{
    os_task_create(sd_fs_test, NULL, 12, 1000, 0, "sd_fs_test");
    return 0;
}
late_initcall(c_main);
#endif
