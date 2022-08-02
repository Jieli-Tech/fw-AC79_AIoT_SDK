#include "app_config.h"
#include "system/includes.h"
#include "event/device_event.h"
#include "fs/fs.h"

#ifdef USE_USB_UDISK_HOST_TEST_DEMO
void filename_test(void)
{
    u8 *test_data = "filename test123"; //写入文件数据
    u8 read_buf[32];//读取数据buf
    int len;//文件大小
    char path[256] = "storage/udisk1/C/";//路径，高速USB口，开源板需要自己接出来。


    //创建并写文件

    FILE *fd = fopen(CONFIG_UDISK_ROOT_PATH"test/test.txt", "w+");

    if (fd) {
        fwrite(test_data, 1, strlen(test_data), fd);//写入文件数据
        fclose(fd);//关闭文件
        printf("write file : %s \n", read_buf);
    }

    //读文件
    fd = fopen(CONFIG_UDISK_ROOT_PATH"test/test.txt", "r");
    if (fd) {
        len = flen(fd);//获取整个文件大小
        fread(read_buf, 1, len, fd);//读取文件
        fclose(fd);//关闭文件
        printf("read file : %s \n", read_buf);
    }
}


late_initcall(filename_test);
#endif
