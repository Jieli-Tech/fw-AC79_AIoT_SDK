#include "app_config.h"
#include "typedef.h"
#include "fs.h"
#include "norflash.h"
#include "spi/nor_fs.h"
#include "rec_nor/nor_interface.h"
#include "update_loader_download.h"

#define LOG_TAG "[UP_FILE_DNLD]"
#define LOG_INFO_ENABLE
#define LOG_ERROR_ENABLE
#include "system/debug.h"

#define EX_FLASH_FILE_NAME	"res.bin"

#define ONCE_REQ_SIZE		(0x200)
typedef struct _resource_update_info_t {
    user_chip_update_info_t info;
    void *dev_hdl;
    update_op_api_t *file_ops;
    u32 buf[ONCE_REQ_SIZE / 4];
} res_update_info_t;

static res_update_info_t *res_info = NULL;
#define __this (res_info)
static int norflash_f_open(u32 file_len)
{
    int ret = 0;
    if (__this) {
        __this->dev_hdl = dev_open("res_nor", NULL);
        if (__this->dev_hdl) {
            log_info("open dev succ 0x%x", __this->dev_hdl);
        } else {
            log_error("open dev fail");
            ret = -1;
        }
    } else {
        ret = -2;
    }

    return ret;
}

static u16 norflash_f_read(void *buf, u32 addr, u32 len)
{
    int rlen = 0;

    if (__this && __this->dev_hdl) {
        wdt_clear();
        //printf("%s: addr = 0x%x, rlen = 0x%x", __func__, addr, len);
        rlen = dev_bulk_read(__this->dev_hdl, buf, addr, len);
        //put_buf(buff, 128);
    }

    return rlen;
}

typedef enum _FLASH_ERASER {
    CHIP_ERASER,
    BLOCK_ERASER,
    SECTOR_ERASER,
    PAGE_ERASER,
} FLASH_ERASER;

static u16 norflash_f_erase(u32 cmd, u32 addr)
{
    u32 type = 0;

    switch (cmd) {
    case BLOCK_ERASER:
        type = IOCTL_ERASE_BLOCK;
        break;
    case SECTOR_ERASER:
        type = IOCTL_ERASE_SECTOR;
        break;
    }

    if (__this && __this->dev_hdl && type) {
        wdt_clear();
        dev_ioctl(__this->dev_hdl, type, addr);
    } else {
        log_error("f_erase parm err\n");
    }

    return 0;
}

static u16 norflash_f_write(void *buf, u32 addr, u32 len)
{
    int rlen = 0;

    if (__this && __this->dev_hdl) {
        wdt_clear();
        //printf("%s: addr = 0x%x, rlen = 0x%x", __func__, addr, len);
        rlen = dev_bulk_write(__this->dev_hdl, buf, addr, len);
        //put_buf(buff, 128);
    }

    return rlen;
}

static int ex_flash_update_file_init(void *priv, update_op_api_t *file_ops)
{
    int ret = 0;

    user_chip_update_info_t *info = (user_chip_update_info_t *)priv;

    if (priv && file_ops) {
        __this = malloc(sizeof(res_update_info_t));
        if (!__this) {
            ret = -1;
            goto _ERR_RET;
        }
        memset((u8 *)__this, 0x00, sizeof(res_update_info_t));
        memcpy((u8 *)&__this->info, (u8 *)info, sizeof(user_chip_update_info_t));
        __this->file_ops = file_ops;

        log_info("ADDR:%x LEN:%x CRC:%x dev_addr:%x\n", __this->info.addr, __this->info.len, __this->info.crc, __this->info.dev_addr);

        if (norflash_f_open(__this->info.len)) {
            ret = -3;
        }
    } else {
        log_error("not find target file :%s\n", EX_FLASH_FILE_NAME);
        ret = -2;
        goto _ERR_RET;
    }

_ERR_RET:
    return ret;
}

static int ex_flash_update_file_get_len(void)
{
    if (__this) {
        return __this->info.len;
    } else {
        return 0;
    }
}
typedef struct _program_info_t {
    u32 remote_file_begin;
    u32 remote_file_length;
    u32 local_program_addr;
    u32(*remote_file_read)(void *buf, u32 addr, u32 len);
    u32(*local_write_hdl)(void *buf, u32 addr, u32 len);
    u32(*local_erase_hdl)(u32 cmd, u32 addr);
    u8 *buf;
} program_info_t;

static u32 remote_file_read(void *buf, u32 addr, u32 len)
{
    u32 ret = 0;

    if (__this && __this->file_ops) {
        putchar('%');
        __this->file_ops->f_seek(NULL, SEEK_SET, addr);
        if ((u16) - 1 == __this->file_ops->f_read(NULL, buf, len)) {
            ret = (u32) - 1;
        }
    }

    return ret;
}

#define FLASH_SECTOR_SIZE	(4096L)
#define __SECTOR_4K_ALIGN(len) (((len) + FLASH_SECTOR_SIZE -1 )/FLASH_SECTOR_SIZE * FLASH_SECTOR_SIZE)

extern void flash_erase_by_blcok_n_sector(u32 start_addr, u32 len, int (*erase_hdl)(int cmd, u32 addr));
static int ex_flash_program_loop(program_info_t *info)
{
    int ret = 0;
    u32 remain_len;
    u32 remote_addr;
    u32 local_addr;

    if (info->local_erase_hdl) {
        remain_len = __SECTOR_4K_ALIGN(info->remote_file_length);
        local_addr = __SECTOR_4K_ALIGN(info->local_program_addr);
        flash_erase_by_blcok_n_sector(local_addr, remain_len, info->local_erase_hdl);
    }

    remain_len = info->remote_file_length;
    remote_addr = info->remote_file_begin;
    local_addr = info->local_program_addr;

    u16 r_len = 0;

    do {
        r_len = (remain_len % ONCE_REQ_SIZE) ? (remain_len % ONCE_REQ_SIZE) : ONCE_REQ_SIZE;
        remain_len -= r_len;

        if (info->remote_file_read && info->buf) {
            ret = info->remote_file_read(info->buf, remote_addr + remain_len, r_len);
            if (ret != 0) {
                log_error("read addr:%x len:%x err\n", remote_addr + remain_len, r_len);
                ret = -1;
                break;
            }
        }

        if (info->local_write_hdl && info->buf) {
            if (r_len  != info->local_write_hdl(info->buf, local_addr + remain_len, r_len)) {
                log_error("write addr:%x len:%x err\n", local_addr + remain_len, r_len);
                ret = -2;
                break;
            }
        }
    } while (remain_len);

    return ret;
}

extern u16 calc_crc16_with_init_val(u16 init_crc, u8 *ptr, u16 len);
static u16 ex_flash_local_file_verify(u8 *buf, u32 addr, u32 len, u32(*read_func)(u8 *buf, u32 addr, u32 len))
{
    u16 crc_temp = 0;
    u16 r_len;

    log_info("verify-addr:%x len:%x\n", addr, len);
    while (len) {
        r_len = (len > ONCE_REQ_SIZE) ? ONCE_REQ_SIZE : len;

        if (read_func) {
            read_func(buf, addr, r_len);
        }
        crc_temp = calc_crc16_with_init_val(crc_temp, buf, r_len);

        addr += r_len;
        len -= r_len;
    }

    return crc_temp;
}

static int ex_flash_file_download_loop(void)
{
    int ret = 0;
    program_info_t info = {
        .remote_file_begin = __this->info.addr,
        .remote_file_length = __this->info.len,
        .local_program_addr = __this->info.dev_addr,
        .remote_file_read = remote_file_read,
        .local_write_hdl = norflash_f_write,
        .local_erase_hdl = norflash_f_erase,
        .buf = __this->buf,
    };

    ret = ex_flash_program_loop(&info);
    if (ret) {
        ret = -1;
        goto _ERR_RET;
    }

    if (__this->info.crc != \
        ex_flash_local_file_verify(__this->buf, __this->info.dev_addr, __this->info.len, norflash_f_read)) {
        log_error("update crc verify err\n");
        ret = -2;
    } else {
        log_info("update crc verify succ\n");
    }

_ERR_RET:
    return ret;
}

const static user_chip_update_t ex_flash_update_ins = {
    .file_name = EX_FLASH_FILE_NAME,
    .update_init = ex_flash_update_file_init,
    .update_get_len = ex_flash_update_file_get_len,
    .update_loop = ex_flash_file_download_loop,
};

void ex_flash_file_download_init(void)
{
    register_user_chip_update_handle(&ex_flash_update_ins);
}

