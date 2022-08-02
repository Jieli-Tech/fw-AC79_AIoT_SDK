#include "fs/fs.h"
#include "spiflash.h"
#include "app_config.h"
#include "asm/sfc_norflash_api.h"

#ifdef TCFG_EXTFLASH_ENABLE

#define EXTFLASH_BLOCK_SIZE (512)
#define MAX_READ_SIZE       (63 * 1024)


static u32 EXTFLASH_SIZE = 0;
static struct extflash {
    u32 ext_size;
    u32 ext_addr;
} extflash_info = {0};


static u8 extflash_info_init(void)
{
    struct vfs_attr file_attr;
    FILE *profile_fp;

    if (extflash_info.ext_size) {
        return 1;
    }

    profile_fp = fopen("mnt/sdfile/EXT_RESERVED/extflash", "r");
    if (!profile_fp) {
        printf("get_extflash_addr ERROR!!!\r\n");
        return 0;
    }
    fget_attrs(profile_fp, &file_attr);
    extflash_info.ext_addr = sdfile_cpu_addr2flash_addr(file_attr.sclust);
    extflash_info.ext_size = file_attr.fsize;
    fclose(profile_fp);

    printf("get_extflash_addr = 0x%x, size = 0x%x \r\n", extflash_info.ext_addr, extflash_info.ext_size);
    ASSERT(extflash_info.ext_size, "%s\n", "The space of extflash is zero\n");
    return 1;
}


static int extflash_dev_init(const struct dev_node *node, void *arg)
{
    return norflash_init(node, arg);
}


static int extflash_dev_open(const char *name, struct device **device, void *arg)
{
    extflash_info_init();
    return norflash_open(name, device, arg);
}


static int extflash_dev_read(struct device *device, void *buf, u32 len, u32 offset)
{
    u8 *data;
    int err = 0;
    u32 addr_offset, addr, size;

    data = (u8 *)buf;

    addr_offset = extflash_info.ext_addr / EXTFLASH_BLOCK_SIZE * EXTFLASH_BLOCK_SIZE;
    addr  = offset + (addr_offset / EXTFLASH_BLOCK_SIZE); //偏移
    addr *= EXTFLASH_BLOCK_SIZE;
    size  = len * EXTFLASH_BLOCK_SIZE;

    if (size <= MAX_READ_SIZE) {
        err = norflash_origin_read(data, addr, size);
    } else {
        for (int i = 1; i <= (size / MAX_READ_SIZE); i++) {
            err  += norflash_origin_read(data, addr, MAX_READ_SIZE);
            data += MAX_READ_SIZE;
            addr += MAX_READ_SIZE;
        }

        if (size % MAX_READ_SIZE) {
            err += norflash_origin_read(data, addr, size % MAX_READ_SIZE);
        }
    }

    return (err / EXTFLASH_BLOCK_SIZE) ; //返回扇区个数
}


static int extflash_dev_write(struct device *device, void *buf, u32 len, u32 offset)
{
    u32 align_size = 4096;
    u8 *data = NULL, *temp = NULL;
    u32 addr_offset, begin, end, addr, size, w_size, remain_size;

    addr_offset = extflash_info.ext_addr / EXTFLASH_BLOCK_SIZE * EXTFLASH_BLOCK_SIZE;
    addr  = offset + (addr_offset / EXTFLASH_BLOCK_SIZE); //偏移
    addr *= EXTFLASH_BLOCK_SIZE;
    size  = len * EXTFLASH_BLOCK_SIZE;

    data  = (u8 *)buf;
    begin = addr / align_size;
    end   = (addr + size) / align_size;
    temp  = malloc(align_size);
    if (!temp) {
        return 0;
    }

    w_size = 0;
    remain_size = align_size - (addr % align_size);
    norflash_origin_read(temp, begin * align_size, align_size);
    if (size <= remain_size) {
        memcpy(temp + (addr % align_size), data, size);
        norflash_erase(IOCTL_ERASE_SECTOR, begin * align_size);
        norflash_write(NULL, temp, align_size, begin * align_size);
    } else {
        memcpy(temp + (addr % align_size), data, remain_size);
        norflash_erase(IOCTL_ERASE_SECTOR, begin * align_size);
        norflash_write(NULL, temp, align_size, begin * align_size);
        w_size += remain_size;

        for (int i = begin + 1; i < end; i++) {
            norflash_erase(IOCTL_ERASE_SECTOR, (begin + i) * align_size);
            norflash_write(NULL, data + w_size, align_size, (begin + i) * align_size);
            w_size += align_size;
        }

        if (begin != end && size > w_size) {
            norflash_origin_read(temp, end * align_size, align_size);
            memcpy(temp, data + w_size, size - w_size);
            norflash_erase(IOCTL_ERASE_SECTOR, end * align_size);
            norflash_write(NULL, temp, align_size, end * align_size);
        }
    }
    free(temp);

    return len;
}


static int extflash_dev_ioctl(struct device *device, u32 cmd, u32 arg)
{
    int err = 0;
    switch (cmd) {
    case IOCTL_GET_ID:
        break;

    case IOCTL_GET_SECTOR_SIZE:
        *((u32 *)arg) = 256;
        break;

    case IOCTL_GET_BLOCK_SIZE:
        *((u32 *)arg) = EXTFLASH_BLOCK_SIZE;
        break;

    case IOCTL_GET_CAPACITY:
        extflash_info_init();
        ASSERT(extflash_info.ext_size, "%s\n", "The space of extflash is zero\n");
        *((int *)arg) = ((extflash_info.ext_size + EXTFLASH_BLOCK_SIZE - 1) / EXTFLASH_BLOCK_SIZE);
        break;

    case IOCTL_GET_UNIQUE_ID:
        break;

    case IOCTL_ERASE_BLOCK:
        break;

    case IOCTL_ERASE_CHIP:
        break;

    case IOCTL_SET_WRITE_PROTECT:
        break;

    case IOCTL_GET_BLOCK_NUMBER:
        extflash_info_init();
        ASSERT(extflash_info.ext_size, "%s\n", "The space of extflash is zero\n");
        *(u32 *)arg = ((extflash_info.ext_size + EXTFLASH_BLOCK_SIZE - 1) / EXTFLASH_BLOCK_SIZE);
        break;

    case IOCTL_GET_STATUS:
        *(u32 *)arg = 1;
        break;

    default:
        err = -EINVAL;
        break;
    }

    return err;
}


static int extflash_dev_close(struct device *device)
{
    return 0;
}


const struct device_operations extflash_dev_ops = {
    .init 	= extflash_dev_init,
    .open 	= extflash_dev_open,
    .read 	= extflash_dev_read,
    .write	= extflash_dev_write,
    .ioctl 	= extflash_dev_ioctl,
    .close  = extflash_dev_close,
};


void extflash_mount_to_fs_test(void)
{
    FILE *fp;
    u8 buf[] = "extflash wr test";

    if (mount("extflash", "mnt/extflash", "fat", 0, NULL)) {
        printf("extflash mount succ");
        fp = fopen("mnt/extflash/C/test.txt", "w+");
        if (fp) {
            printf("fopen succ\n");
            fwrite(buf, sizeof(buf), 1, fp);
            fclose(fp);

            memset(buf, 0, sizeof(buf));
            fp = fopen("mnt/extflash/C/test.txt", "w+");
            fread(buf, sizeof(buf), 1, fp);
            printf("test.txt : %s\n", buf);
            fclose(fp);
        }
    } else {
        printf("extflash mount failed!!!");
    }

}


#endif


