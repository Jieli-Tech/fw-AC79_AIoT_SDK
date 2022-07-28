#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "generic/typedef.h"

//LSB
#define DBG_BT 	                0x80

#define DBG_FUSB                0x81
#define DBG_FUSB1               0x82
//
#define DBG_WFSDD               0x83
#define DBG_WFSDC               0x84
//
#define DBG_SPI0                0x85
#define DBG_SPI1                0x86
#define DBG_SPI2                0x87
#define DBG_SPI3                0x88
//
#define DBG_UART0_WR            0x89
#define DBG_UART1_WR            0x8a
#define DBG_UART0_RD            0x8b
#define DBG_UART1_RD            0x8c
#define DBG_UART2_WR            0x8d
#define DBG_UART2_RD            0x8e
//
#define DBG_SS_D	            0x8f
#define DBG_SS_I	            0x90

#define DBG_PLNK0               0x91
#define DBG_PLNK1               0x92
#define DBG_ALNK0               0x93
#define DBG_ALNK1               0x94
#define DBG_AUDIO               0x95

#define DBG_PAP                 0x96
// #define DBG_EMI                 0x97

#define DBG_SD0D                0x98
#define DBG_SD1D                0x99
#define DBG_SD0C                0x9a
#define DBG_SD1C                0x9b

#define DBG_ISP	                0x9c
#define DBG_ANC	                0x9d
#define DBG_SHA	                0x9e
#define DBG_SBC	                0x9f
#define DBG_SRC	                0xa0
#define DBG_EQ	                0xa1


#define DBG_ISC0	            0x00
#define DBG_ISC1	            0x40
#define DBG_LSB                 0x80
#define DBG_JPG                 0xc0
#define DBG_CPU0                0x100
#define DBG_CPU1                0x140
#define DBG_SDR                 0x180
#define DBG_DCP                 0x1c0
#define DBG_EMI                 0x200
#define DBG_IMD                 0x240




int axi_ram_write_range_limit(u32 low_addr, u32 high_addr, u32 is_allow_write_out, int id0, int id1);
int dev_write_range_limit(int limit_index, void *low_addr, void *high_addr, u32 is_allow_write_out, int dev_id);
int cpu_write_range_limit(void *low_addr, u32 win_size);
void cpu_write_range_unlimit(void *low_addr);
void pc_rang_limit(void *low_addr0, void *high_addr0, void *low_addr1, void *high_addr1);
void debug_init(void);
void debug_clear(void);
void debug_msg_clear(void);
void cpu_effic_init(void);
void cpu_effic_calc(void);

int sdr_read_write_range_limit(u32 low_addr, u32 high_addr,
                               int id0, int id1,
                               u8 rd0_permit, u8 wr0_permit,
                               u8 rd1_permit, u8 wr1_permit,
                               u8 wr0_over_area_permit, u8 wr1_over_area_permit);
int sdr_read_write_range_unlimit(void);


extern u32 text_rodata_begin;
extern u32 text_rodata_end;
extern u32 ram_text_rodata_begin;
extern u32 ram_text_rodata_end;



#endif
