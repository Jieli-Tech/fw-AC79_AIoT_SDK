#ifndef __SFC_NORFLASH_API_H__
#define __SFC_NORFLASH_API_H__

#include "typedef.h"
#include "device/device.h"

int norflash_init(const struct dev_node *node, void *arg);
int norflash_open(const char *name, struct device **device, void *arg);
int norflash_read(struct device *device, void *buf, u32 len, u32 offset);
int norflash_origin_read(u8 *buf, u32 offset, u32 len);
int norflash_write(struct device *device, void *buf, u32 len, u32 offset);
int norflash_erase_zone(struct device *device, u32 addr, u32 len);
int norflash_ioctl(struct device *device, u32 cmd, u32 arg);
int norflash_write_protect(u32 cmd);
u32 flash_addr2cpu_addr(u32 offset);
u32 cpu_addr2flash_addr(u32 offset);
u8 *get_norflash_uuid(void);
u32 get_norflash_vm_addr(void);
void norflash_set_spi_con(u32 arg);
int norflash_protect_resume(void);
int norflash_protect_suspend(void);

void norflash_enter_spi_code(void);
void norflash_exit_spi_code(void);
void norflash_spi_cs(char cs);
void norflash_spi_write_byte(unsigned char data);
u8 norflash_spi_read_byte(void);
int norflash_wait_busy(void);
int norflash_eraser_otp(void);
int norflash_write_otp(u8 *buf, int len);
int norflash_read_otp(u8 *buf, int len);



#endif
