/*
 * File      : fal_cfg.h
 * This file is part of FAL (Flash Abstraction Layer) package
 * COPYRIGHT (C) 2006 - 2018, RT-Thread Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-05-17     armink       the first version
 */

#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

#define FAL_DEBUG 1
#define FAL_PART_HAS_TABLE_CFG

#define  FAL_PART_TABLE_FLASH_DEV_NAME      "AC79_onchip_norflash"


/* ===================== Flash device Configuration ========================= */
extern const struct fal_flash_dev AC79_onchip_norflash;

/* flash device table */
#define FAL_FLASH_DEV_TABLE                                          \
{                                                                    \
    &AC79_onchip_norflash,                                           \
}
/* ====================== Partition Configuration ========================== */
#ifdef FAL_PART_HAS_TABLE_CFG
/* partition table */
//FIX_ME:需要根据 isd_config.ini 的 EXIF_LEN 配置
#define FAL_PART_TABLE                                                               \
{                                                                                     \
    {FAL_PART_MAGIC_WORD, "fdb_tsdb1", FAL_PART_TABLE_FLASH_DEV_NAME,         0, 8*1024, 0}, \
    {FAL_PART_MAGIC_WORD,  "fdb_kvdb1", FAL_PART_TABLE_FLASH_DEV_NAME, 8*1024, 8*1024, 0}, \
}
#endif /* FAL_PART_HAS_TABLE_CFG */

#endif /* _FAL_CFG_H_ */
