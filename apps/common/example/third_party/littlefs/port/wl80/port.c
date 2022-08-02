#include "system/includes.h"
#include "fs/fs.h"
#include "asm/sfc_norflash_api.h"
#include "lfs.h"

// variables used by the filesystem
static lfs_t lfs;

static int lfs_flash_block_read(const struct lfs_config *c, lfs_block_t block,
                                lfs_off_t off, void *buffer, lfs_size_t size)
{
    /*printf("lfs_read,block=%d,off=%d,size=%d\r\n",block,off,size);*/
    norflash_read(NULL, buffer, size, (u32)c->context + block * c->block_size + off);
    return 0;
}

static int lfs_flash_block_prog(const struct lfs_config *c, lfs_block_t block,
                                lfs_off_t off, const void *buffer, lfs_size_t size)
{
    /*printf("lfs_prog,block=%d,off=%d,size=%d\r\n",block,off,size);*/
    norflash_write(NULL, (void *)buffer, size, (u32)c->context + block * c->block_size + off);
    return 0;
}

static int lfs_flash_block_erase(const struct lfs_config *c, lfs_block_t block)
{
    /*printf("lfs_flash_erase=%d\r\n",block);*/
    norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, (u32)c->context + block * c->block_size);
    return 0;
}
static int lfs_flash_block_sync(const struct lfs_config *c)
{
    return 0;
}

// configuration of the filesystem is provided by this struct

static struct lfs_config cfg = {
    // block device operations
    .read  = lfs_flash_block_read,
    .prog  = lfs_flash_block_prog,
    .erase = lfs_flash_block_erase,
    .sync  = lfs_flash_block_sync,

    // block device configuration
    .read_size = 1,
    .prog_size = 256,
    .block_size = 4096,
    .cache_size = 4096,
    .block_cycles = 500,

};

lfs_t *lfs_dev_mount(void)
{
    u32 lfs_addr, lfs_space;
    FILE *profile_fp = fopen("mnt/sdfile/app/exif", "r");
    if (profile_fp == NULL) {
        puts("lfs_mount ERROR!!!\r\n");
        return NULL;
    }
    struct vfs_attr file_attr;
    fget_attrs(profile_fp, &file_attr);
    lfs_addr = sdfile_cpu_addr2flash_addr(file_attr.sclust);
    lfs_space = file_attr.fsize;
    fclose(profile_fp);
    printf("LFS_addr = 0x%x, size = 0x%x \r\n", lfs_addr, lfs_space);

    // mount the filesystem
    cfg.context = (void *)lfs_addr;
    cfg.block_count = lfs_space / cfg.block_size;
    cfg.lookahead_size = (cfg.block_count / 8) + (((cfg.block_count / 8) & 7) ? ((8) - ((cfg.block_count / 8) & 7)) : 0);

    if (lfs_mount(&lfs, &cfg)) {
        // reformat if we can't mount the filesystem
        // this should only happen on the first boot
        lfs_format(&lfs, &cfg);
        if (lfs_mount(&lfs, &cfg)) {
            puts("lfs_mount fail! \r\n");
            return NULL;
        }

    }

    return &lfs;
}
