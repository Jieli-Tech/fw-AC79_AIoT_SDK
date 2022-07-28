#ifndef ADAPTER_UART_INTERFACE_H
#define ADAPTER_UART_INTERFACE_H

// #include "asm/hwi.h"
#include "typedef.h"
#include "gpio.h"

void put_u16hex(u16 dat);
void put_u32hex(u32 dat);
void put_u8hex(u8 dat);
int puts(const char *out);
void printf_buf(u8 *buf, u32 len);

#endif
