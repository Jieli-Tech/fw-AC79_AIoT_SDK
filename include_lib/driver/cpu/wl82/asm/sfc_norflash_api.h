#ifndef __SFC_NORFLASH_API_H__
#define __SFC_NORFLASH_API_H__

#include "typedef.h"
#include "asm/cpu.h"
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

extern const char norflash_write_cpu_unhold;

__attribute__((always_inline))
static void MEMCPY_FLASH(void *buf, void *read_addr, u32 size)
{
    if (norflash_write_cpu_unhold == 0) { //跑SFC模式下,直接使用cpu读
        memcpy(buf, read_addr, size);
    } else if ((u32)read_addr >= __SFC_ADDR_BEGIN__ && (u32)read_addr < __SFC_ADDR_END__) {
        /*printf("--->read_flash_addr = 0x%x \n", read_addr);*/
        norflash_read(NULL, buf, size, cpu_addr2flash_addr((u32)(read_addr)));
    } else {
        memcpy(buf, read_addr, size);
    }
}

#endif
