#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "generic/typedef.h"

//LSB
#define DBG_BT 	                0x40

#define DBG_FUSB                0x41
#define DBG_FUSB1               0x42
//
#define DBG_WFSDD               0x43
#define DBG_WFSDC               0x44
//
#define DBG_SPI0                0x45
#define DBG_SPI1                0x46
#define DBG_SPI2                0x47
#define DBG_SPI3                0x48
//
#define DBG_UART0_WR            0x49
#define DBG_UART1_WR            0x4a
#define DBG_UART0_RD            0x4b
#define DBG_UART1_RD            0x4c
#define DBG_UART2_WR            0x4d
#define DBG_UART2_RD            0x4e
//
#define DBG_SS_D	            0x4f
#define DBG_SS_I	            0x50

#define DBG_PLNK0               0x51
#define DBG_PLNK1               0x52
#define DBG_ALNK0               0x53
#define DBG_ALNK1               0x54
#define DBG_AUDIO               0x55

#define DBG_PAP                 0x56
#define DBG_EMI                 0x57

#define DBG_SD0D                0x58
#define DBG_SD1D                0x59
#define DBG_SD0C                0x5a
#define DBG_SD1C                0x5b

#define DBG_ISP	                0x5c
#define DBG_ANC	                0x5d
#define DBG_SHA	                0x5e
#define DBG_SBC	                0x5f
#define DBG_SRC	                0x60
#define DBG_EQ	                0x61


#define DBG_ISC	                0x00
#define DBG_JPG                 0x80
#define DBG_CPU0                0xC0
#define DBG_CPU1                0x100
#define DBG_SDR                 0x140
#define DBG_DCP                 0x180




int axi_ram_write_range_limit(u32 low_addr, u32 high_addr, u32 is_allow_write_out, int id0, int id1);
int sdr_read_write_range_limit(u32 low_addr, u32 high_addr, int id0, int id1, u8 rd0_permit, u8 wr0_permit, u8 rd1_permit, u8 wr1_permit);
int dev_write_range_limit(int limit_index, void *low_addr, void *high_addr, u32 is_allow_write_out, int dev_id);
int cpu_write_range_limit(void *low_addr, u32 win_size);
void cpu_write_range_unlimit(void *low_addr);
void pc_rang_limit(void *low_addr0, void *high_addr0, void *low_addr1, void *high_addr1);
void debug_init(void);
void debug_clear(void);
void debug_msg_clear(void);
void cpu_effic_init(void);
void cpu_effic_calc(void);



extern u32 text_rodata_begin;
extern u32 text_rodata_end;
extern u32 ram_text_rodata_begin;
extern u32 ram_text_rodata_end;



#endif
