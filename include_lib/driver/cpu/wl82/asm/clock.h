#ifndef _CPU_CLOCK_
#define _CPU_CLOCK_

#include "typedef.h"


int clk_early_init();

enum sys_vdd_index {
    SYSVDD_VOL_SEL_INDEX = 0,
    VDC14_VOL_SEL_INDEX,
    VDDIOM_VOL_SEL_INDEX,
    SYS_VDD_NUM,
};
enum sys_clock_index {
    SYS_CLK_SEL_INDEX = 0,
    HSB_CLK_SEL_INDEX,
    SFC_CLK_SEL_INDEX,
    LSB_CLK_SEL_INDEX,
    P33_CLK_SEL_INDEX,
    SDRAM_CLK_SEL_INDEX,
    SYS_CLK_NUM,
};
enum pc0_clock_out_index {
    BT_OSC_CLK_OUT = 1,
    RTC_OSL_OUT,
    CTM_IN_OUT,
    LSB_CLK_OUT,
    HSB_CLK_OUT,
    SFC_CLK_OUT,
    HCO_CLK_OUT,
};
enum pa0_clock_out_index {
    RC_CLK_OUT = 1,//16M 250K
    LRC_CLK_OUT,//32K或200K
    WL_RCCL_CLK_OUT,
    DMC_CLK_OUT,//sdram clock
    RING_CLK_OUT,
    OSC_CLK_OUT,
    PLL_SYS_CLK_OUT,
};

#define 	MHZ_UNIT		1000000L
#define     SYS_CLK_320M	320000000L
#define     SYS_CLK_240M	240000000L
#define     SYS_CLK_192M	192000000L
#define     SYS_CLK_160M	160000000L
#define     SYS_CLK_120M	120000000L
#define     SYS_CLK_96M		96000000L
#define     SYS_CLK_48M		48000000L
#define     SYS_CLK_40M		40000000L
#define     SYS_CLK_24M		24000000L
#define     SYS_CLK_12M		12000000L
#define     SYS_CLK_8M		8000000L
#define     SYS_CLK_4M	    4800000L
#define     SDRAM_CLK_MIN	96000000L

/*
 * system enter critical and exit critical handle
 * */
struct clock_critical_handler {
    void (*enter)();
    void (*exit)();
};

#define CLOCK_CRITICAL_HANDLE_REG(name, enter, exit) \
	const struct clock_critical_handler clock_##name \
		 SEC_USED(.clock_critical_txt) = {enter, exit};

extern struct clock_critical_handler clock_critical_handler_begin[];
extern struct clock_critical_handler clock_critical_handler_end[];

#define list_for_each_loop_clock_critical(h) \
	for (h=clock_critical_handler_begin; h<clock_critical_handler_end; h++)


int clk_get(const char *name);

int clk_set(const char *name, int clk);

void clk_out_pc0(enum pc0_clock_out_index index);
void clk_out_pa0(enum pa0_clock_out_index index);
int sys_clk_source_set(char index);//0:BT_OSC,1:RTC_SOC,2:PLL_OSC
void sys_clk_source_resume(int value);
void sys_clk_source_rc_clk_on(int is_16M);//RC_16M / RC_250K
void sys_clk_source_rc_clk_off(void);
u32 clk_get_osc_cap();

//**********************应用层使用API****************************//
//系统空闲模式时钟切换函数：系统24M和Sdram时钟固定在120M
int system_clock_set_idle(void);

//系统时钟切换函数：时钟参数，clk范围：24000000 - 396000000 （24M-396M）
int system_clock_set(u32 clk);

//系统时钟+sdram时钟切换sys_clk时钟参数，范围：24000000 - 396000000 （24M-396M）；sdram_clk时钟参数，范围：96000000 - 244000000 （96M-244M）
int system_sdram_clock_set(u32 sys_clk, u32 sdram_clk);

//sdram时钟切换：clk时钟参数，范围：96000000 - 244000000 （96M-244M）
int sdram_clock_set(u32 clk);

//恢复SDK默认系统+sdram时钟：恢复到系统上电的初始值
int system_clock_set_default(void);

//自定义VDD和CLOCK等级
int system_vdd_clock_set(char index);
#endif

