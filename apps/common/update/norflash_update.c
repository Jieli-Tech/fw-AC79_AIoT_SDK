#include "app_config.h"
#include "typedef.h"
#include "fs.h"
#include "norflash.h"
#include "spi/nor_fs.h"

typedef struct _nor_fs_part {
    u32 update_file_addr;
    u32 update_area_start_addr;
    u32 update_area_end_addr;
} nor_fs_parm;

nor_fs_parm update_norfs_parm;

#define NORFLASH_LOADER_PATH  "storage/nor_fs/C/loader.bin"
static void *fd = NULL;
void register_loader_write_handler(u32(*hdl)(void *, u32));
u16 CRC16_with_initval(const void *ptr, u32 len, u16 i_val);
u32 nor_get_absolute_addr(void);
u32 nor_get_start_addr(void);
u32 nor_get_capacity(void);

int get_nor_update_param(void *buf)
{
    if (buf) {
        printf("file_addr:0x%x start_addr:0x%x end_addr:0x%x\n", update_norfs_parm.update_file_addr, update_norfs_parm.update_area_start_addr, update_norfs_parm.update_area_end_addr);
        memcpy(buf, (u8 *)&update_norfs_parm, sizeof(update_norfs_parm));
    }
    return sizeof(update_norfs_parm);
}

int norflash_f_open(u32 loader_len)
{
    fd = fopen(NORFLASH_LOADER_PATH, "w+");
    if (!fd) {
        r_printf("update fopen err\n");
        return -1;
    } else {
        g_printf("update fopen succ\n");
        update_norfs_parm.update_file_addr = nor_get_absolute_addr();
        update_norfs_parm.update_area_start_addr = nor_get_start_addr();
        update_norfs_parm.update_area_end_addr = update_norfs_parm.update_area_start_addr + nor_get_capacity();
        if (loader_len > nor_get_capacity) {
            return -2;
        }
    }
    return 0;
}

u32 norflash_f_write(u8 *buff, u16 len)
{
    if (fd) {
        return fwrite(fd, buff, len);
    }
    return 0;
}

u32 norflash_update_verify(u32 loader_len, u32 loader_crc)
{
    u32 len = loader_len;
    u32 crc_temp = 0;
    if (fd) {
        fseek(fd, NOR_FS_SEEK_SET, 0);          //偏移到文件起始
        u16 r_len;

        u8 *temp_buf = malloc(512);
        if (NULL == temp_buf) {
            goto _ERR_RET;
        }
        u16 temp_buf_len = 512;

        while (len) {
            r_len = (len > temp_buf_len) ? temp_buf_len : len;

            fread(fd, temp_buf, r_len);
            crc_temp = CRC16_with_initval(temp_buf, r_len, crc_temp);

            len -= r_len;
        }

_ERR_RET:
        if (NULL != temp_buf) {
            free(temp_buf);
        }

        if (crc_temp == loader_crc) {
            return 0;
        }
    }
    return 1;
}

void norflash_update_close()
{
    if (fd) {
        fclose(fd);
    }
}

int  norflash_loader_start(u32 loader_len)
{
    register_loader_write_handler(norflash_f_write);
    return norflash_f_open(loader_len);
}
