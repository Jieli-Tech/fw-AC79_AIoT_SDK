#ifndef __CPU_CRC16_H__
#define __CPU_CRC16_H__

#include "typedef.h"

int __crc16_mutex_init();

u32 CRC32(const u8 *frame, u32 frame_len);

u16 CRC16(const void *ptr, u32 len);

u16 CRC16_with_code(const void *ptr, u32 len, u16 code);

u16 crc_get_16bit(const void *src, u32 len);

u32 crc_get_32bit(const char *src);

void spi_crc16_set(u16 crc);

u16 spi_crc16_get(void);

void CrcDecode(void *buf, u16 len);

u32 get_system_enc_key(void);

u8 get_vbg_trim(void);

u16 get_chip_id(void);

#endif


