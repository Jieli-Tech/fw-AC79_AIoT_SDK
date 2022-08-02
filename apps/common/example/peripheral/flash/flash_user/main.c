#include "app_config.h"
#include "system/includes.h"
#include "fs/fs.h"
#include "asm/sfc_norflash_api.h"

#ifdef USE_FLASH_USER_TEST_DEMO

#define USER_FLASH_SPACE_PATH "mnt/sdfile/app/exif"

static u32 user_get_flash_exif_addr(void)
{
    u32 addr;
    FILE *profile_fp = fopen(USER_FLASH_SPACE_PATH, "r");
    if (profile_fp == NULL) {
        puts("user_get_flash_addr ERROR!!!\r\n");
        return 0;
    }
    struct vfs_attr file_attr;
    fget_attrs(profile_fp, &file_attr);
    addr = sdfile_cpu_addr2flash_addr(file_attr.sclust);
    fclose(profile_fp);

    printf("user_get_flash_exif_addr = 0x%x, size = 0x%x \r\n", addr, file_attr.fsize);
    return addr;
}

static int c_main(void)
{
    printf("\r\n\r\n\r\n\r\n\r\n ----------- USER_FLASH_EXIF example run %s-------------\r\n\r\n\r\n\r\n\r\n", __TIME__);

    char buf[256];
    u32 flash_exif_addr = user_get_flash_exif_addr();
    if (flash_exif_addr == 0) {
        return -1;
    }

    puts("USER_FLASH_EXIF ERASE_SECTOR...\r\n");
    norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, flash_exif_addr);

    puts("USER_FLASH_EXIF READ\r\n");
    memset(buf, 0, sizeof(buf));
    norflash_read(NULL, buf, sizeof(buf), flash_exif_addr);
    put_buf(buf, sizeof(buf));

    puts("\r\n USER_FLASH_EXIF WRITE\r\n");
    for (int i = 0; i < sizeof(buf); i++) {
        buf[i] = i;
    }
    norflash_write(NULL, buf, sizeof(buf), flash_exif_addr);

    puts("USER_FLASH_EXIF READ\r\n");
    memset(buf, 0, sizeof(buf));
    norflash_read(NULL, buf, sizeof(buf), flash_exif_addr);
    put_buf(buf, sizeof(buf));

    return 0;
}
late_initcall(c_main);
#endif
