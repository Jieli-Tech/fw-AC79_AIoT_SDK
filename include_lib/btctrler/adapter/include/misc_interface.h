#ifndef ADAPTER_MISC_INTERRUPT_H
#define ADAPTER_MISC_INTERRUPT_H

// #include "asm/hwi.h"
#include "asm/cpu.h"

u32 crc_get_32bit(const char *src);

u16 crc_get_16bit(const void *src, u32 len);

void pseudo_random_genrate(uint8_t *dest, unsigned size);

#endif
