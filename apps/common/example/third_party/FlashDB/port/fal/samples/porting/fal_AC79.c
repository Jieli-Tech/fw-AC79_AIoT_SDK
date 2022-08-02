/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fal.h>

extern int AC79_onchip_norflash_init(void);
extern int AC79_onchip_norflash_read(long offset, uint8_t *buf, size_t size);
extern int AC79_onchip_norflash_write(long offset, const uint8_t *buf, size_t size);
extern int AC79_onchip_norflash_erase(long offset, size_t size);

const struct fal_flash_dev AC79_onchip_norflash = {
    .name       = "AC79_onchip_norflash",
    .addr       = 0,
    .len        = 32 * 1024,    //FIX_ME:需要根据 isd_config.ini 的 EXIF_LEN 配置
    .blk_size   = 4 * 1024,
    .ops        = {AC79_onchip_norflash_init, AC79_onchip_norflash_read, AC79_onchip_norflash_write, AC79_onchip_norflash_erase},
    .write_gran = 1
};

