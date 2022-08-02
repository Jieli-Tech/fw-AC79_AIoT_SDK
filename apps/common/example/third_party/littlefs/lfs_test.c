#include "system/includes.h"
#include "lfs.h"

static lfs_t *lfs_hdl;

// entry point
void lfs_test(void)
{
    int ret;
    lfs_file_t file;

    lfs_hdl = lfs_dev_mount();
    if (lfs_hdl == NULL) {
        printf("lfs_test lfs_dev_mount fail! \r\n");
    }

    // read current count
    u32 boot_count = 0;
    ret = lfs_file_open(lfs_hdl, &file, "boot_count", LFS_O_RDWR | LFS_O_CREAT);
    if (ret < 0) {
        printf("lfs_test lfs_file_open fail! \r\n");
    }

    lfs_file_read(lfs_hdl, &file, &boot_count, sizeof(boot_count));

    // update boot count
    boot_count += 1;
    lfs_file_rewind(lfs_hdl, &file);
    lfs_file_write(lfs_hdl, &file, &boot_count, sizeof(boot_count));

    // remember the storage is not updated until the file is closed successfully
    lfs_file_close(lfs_hdl, &file);

    // print the boot count
    printf("lfs_test boot_count: %d\n", boot_count);

    //reboot the device
}
