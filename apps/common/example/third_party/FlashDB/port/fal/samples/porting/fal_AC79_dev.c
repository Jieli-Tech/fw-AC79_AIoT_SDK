#include "system/includes.h"
#include "fs/fs.h"
#include "asm/sfc_norflash_api.h"

#define USER_FLASH_SPACE_PATH "mnt/sdfile/app/exif"

static u32 flashDB_addr;

int AC79_onchip_norflash_init(void)
{
    u32 addr;
    FILE *profile_fp = fopen(USER_FLASH_SPACE_PATH, "r");
    if (profile_fp == NULL) {
        puts("__flashDB_mount ERROR!!!\r\n");
        return -1;
    }
    struct vfs_attr file_attr;
    fget_attrs(profile_fp, &file_attr);
    addr = sdfile_cpu_addr2flash_addr(file_attr.sclust);
    fclose(profile_fp);
    flashDB_addr = addr;

    printf("__flashDB_mount_addr = 0x%x, size = 0x%x \r\n", addr, file_attr.fsize);
    return 0;
}

int AC79_onchip_norflash_read(long offset, u8 *buf, u32 size)
{
    return norflash_read(NULL, buf, size, flashDB_addr + offset);
}

int AC79_onchip_norflash_write(long offset, u8 *buf, u32 size)
{
    return norflash_write(NULL, buf, size, flashDB_addr + offset);
}

int AC79_onchip_norflash_erase(long offset, u32 size)
{
    long addr = 0;
    u32	 erase_addr, erase_block = 0, i = 0;

    addr = flashDB_addr + offset;

    if (size % (4 * 1024)) {
        erase_block = (size / (4 * 1024)) + 1;
    } else {
        erase_block = (size / (4 * 1024));
    }

    for (i = 1, erase_addr = addr; i <= erase_block; i++, erase_addr += 4 * 1024) {
        norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, erase_addr);
    }

    return size;
}

