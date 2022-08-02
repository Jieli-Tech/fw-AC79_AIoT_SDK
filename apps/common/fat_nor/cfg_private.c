#include "includes.h"
#include "boot.h"
#include "asm/sfc_norflash_api.h"
#include "cfg_private.h"

extern u32 sdfile_cpu_addr2flash_addr(u32 offset);

FILE *cfg_private_open(const char *path, const char *mode)
{
    return fopen(path, mode);
}

int cfg_private_read(FILE *file, void *buf, u32 len)
{
    return fread(file, buf, len);
}

void cfg_private_erase(u32 addr, u32 len)
{
    u32 erase_total_size = len;
    u32 erase_addr = addr;
    u32 erase_size = 4096;
    u32 erase_cmd = IOCTL_ERASE_SECTOR;
    //flash不同支持的最小擦除单位不同(page/sector)
    //boot_info.vm.align == 1: 最小擦除单位page;
    //boot_info.vm.align != 1: 最小擦除单位sector;
    if (boot_info.vm.align == 1) {
        erase_size = 256;
        erase_cmd = IOCTL_ERASE_PAGE;
    }
    while (erase_total_size) {
        //擦除区域操作
        norflash_ioctl(NULL, erase_cmd, erase_addr);
        erase_addr += erase_size;
        erase_total_size -= erase_size;
    }
}

int cfg_private_check(char *buf, int len)
{
    for (int i = 0; i < len; i++) {
        if (buf[i] != 0xff) {
            return 0;
        }
    }
    return 1;
}

int cfg_private_write(FILE *file, void *buf, u32 len)
{
    int align_size = 4096;
    if (boot_info.vm.align == 1) {
        align_size = 256;
    }
    struct vfs_attr attrs = {0};
    fget_attrs(file, &attrs);
    attrs.sclust = sdfile_cpu_addr2flash_addr(attrs.sclust);
    u32 fptr = fpos(file);
    r_printf(">>>[test]:addr = %d, fsize = %d, fptr = %d, w_len = %d\n", attrs.sclust, attrs.fsize, fptr, len);
    if (len + fptr > attrs.fsize) {
        r_printf(">>>[test]:error, write over!!!!!!!!\n");
        return -1;
    }
    char *buf_temp = (char *)malloc(align_size);
    int res = len;

    for (int wlen = 0; len > 0;) {
        u32 align_addr = (attrs.sclust + fptr) / align_size * align_size;
        u32 w_pos = attrs.sclust + fptr - align_addr;
        wlen = align_size - w_pos;
        if (wlen > len) {
            wlen = len;
        }
        norflash_read(NULL, (void *)buf_temp, align_size, align_addr);
        if (0 == cfg_private_check(buf_temp, align_size)) {
            cfg_private_erase(align_addr, align_size);
        }
        memcpy(buf_temp + w_pos, buf, wlen);
        norflash_write(NULL, (void *)buf_temp, align_size, align_addr);
        fptr += wlen;
        len -= wlen;
    }
__exit:
    free(buf_temp);
    fseek(file, fptr, SEEK_SET);
    return res;
}

int cfg_private_delete_file(FILE *file)
{
    int align_size = 4096;
    if (boot_info.vm.align == 1) {
        align_size = 256;
    }
    struct vfs_attr attrs = {0};
    fget_attrs(file, &attrs);
    attrs.sclust = sdfile_cpu_addr2flash_addr(attrs.sclust);
    int len = attrs.fsize;
    char *buf_temp = (char *)malloc(align_size);
    u32 fptr = 0;

    for (int wlen = 0; len > 0;) {
        u32 align_addr = (attrs.sclust + fptr) / align_size * align_size;
        u32 w_pos = attrs.sclust + fptr - align_addr;
        wlen = align_size - w_pos;
        if (wlen > len) {
            wlen = len;
        }
        norflash_read(NULL, (void *)buf_temp, align_size, align_addr);
        if (0 == cfg_private_check(buf_temp, align_size)) {
            cfg_private_erase(align_addr, align_size);
        }
        memset(buf_temp + w_pos, 0xff, wlen);
        norflash_write(NULL, (void *)buf_temp, align_size, align_addr);
        fptr += wlen;
        len -= wlen;
    }
    free(buf_temp);
    fclose(file);
    return 0;
}

int cfg_private_close(FILE *file)
{
    return fclose(file);
}

int cfg_private_seek(FILE *file, int offset, int fromwhere)
{
    return fseek(file, offset, fromwhere);
}

void cfg_private_test_demo(void)
{
#include "cfg_private.h"
#define N 256
    char test_buf[N] = {0};
    char r_buf[512] = {0};
    for (int i = 0; i < N; i++) {
        test_buf[i] = i & 0xff;
    }

    char path[64] = "mnt/sdfile/app/FATFSI/cfg/eq_cfg_hw.bin";
    FILE *fp = cfg_private_open(path, "w+");
    if (!fp) {
        r_printf(">>>[test]:open fail!!!!!\n");
        while (1);
    }
    /* cfg_private_delete_file(fp); */
    /* fp = cfg_private_open(path, "w+"); */
    cfg_private_read(fp, r_buf, 512);
    put_buf(r_buf, 512);
    cfg_private_seek(fp, 0, SEEK_SET);

    cfg_private_write(fp, test_buf, N);
    cfg_private_close(fp);
    fp = cfg_private_open(path, "r");
    cfg_private_read(fp, r_buf, 512);
    put_buf(r_buf, 512);
    y_printf("\n >>>[test]:func = %s,line= %d\n", __FUNCTION__, __LINE__);

    FILE *file = cfg_private_open("mnt/sdfile/app/FATFSI", "w+");
    if (!file) {
        r_printf(">>>[test]:open fail!!!!!\n");
        while (1);
    }
    struct vfs_attr attrs = {0};
    fget_attrs(file, &attrs);
    y_printf(">>>[test]:in part addr = %d, fsize = %d\n", attrs.sclust, attrs.fsize);
    char *part = zalloc(attrs.fsize);
    cfg_private_delete_file(file);
    file = cfg_private_open("mnt/sdfile/app/FATFSI", "w+");
    cfg_private_read(file, part, attrs.fsize);
    put_buf(part, attrs.fsize);
    free(part);
    while (1);
}


