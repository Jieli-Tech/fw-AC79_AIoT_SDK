#include "asm/cpu.h"
#include "asm/clock.h"
#include "generic/typedef.h"
#include "asm/p33.h"
#include "asm/power_interface.h"
#include "init.h"
#include "app_config.h"

#ifdef CONFIG_SYS_VDD_CLOCK_ENABLE
enum SYSTEM_CLOCK {
    SYSTEM_CLK_3M,
    SYSTEM_CLK_4M,
    SYSTEM_CLK_6M,
    SYSTEM_CLK_8M,
    SYSTEM_CLK_9M,
    SYSTEM_CLK_12M,
    SYSTEM_CLK_13M,
    SYSTEM_CLK_16M,
    SYSTEM_CLK_19M,
    SYSTEM_CLK_20M,
    SYSTEM_CLK_24M,
    SYSTEM_CLK_26M,
    SYSTEM_CLK_27M,
    SYSTEM_CLK_32M,
    SYSTEM_CLK_38M,
    SYSTEM_CLK_40M,
    SYSTEM_CLK_48M,
    SYSTEM_CLK_53M,
    SYSTEM_CLK_60M,
    SYSTEM_CLK_64M,
    SYSTEM_CLK_80M,
    SYSTEM_CLK_96M,
    SYSTEM_CLK_106M,
    SYSTEM_CLK_120M,
    SYSTEM_CLK_160M,
    SYSTEM_CLK_192M,
    SYSTEM_CLK_240M,
    SYSTEM_CLK_320M,
#ifdef CONFIG_OVERCLOCKING_ENABLE //超频
    SYSTEM_CLK_396M,
#endif
};

static const u8 sys_vdd_table[][SYS_VDD_NUM] ALIGNE(4) = {//务必4对齐
    //1.2V内核电压        VDC1.4V电压         IO电压
    {SYSVDD_VOL_SEL_120V, VDC14_VOL_SEL_140V, VDDIOM_VOL_30V},//3M
    {SYSVDD_VOL_SEL_120V, VDC14_VOL_SEL_140V, VDDIOM_VOL_30V},//4M
    {SYSVDD_VOL_SEL_120V, VDC14_VOL_SEL_140V, VDDIOM_VOL_30V},//6M
    {SYSVDD_VOL_SEL_120V, VDC14_VOL_SEL_140V, VDDIOM_VOL_30V},//8M
    {SYSVDD_VOL_SEL_120V, VDC14_VOL_SEL_140V, VDDIOM_VOL_30V},//9M
    {SYSVDD_VOL_SEL_120V, VDC14_VOL_SEL_140V, VDDIOM_VOL_30V},//12M
    {SYSVDD_VOL_SEL_120V, VDC14_VOL_SEL_140V, VDDIOM_VOL_30V},//13M
    {SYSVDD_VOL_SEL_120V, VDC14_VOL_SEL_140V, VDDIOM_VOL_30V},//16M
    {SYSVDD_VOL_SEL_120V, VDC14_VOL_SEL_140V, VDDIOM_VOL_30V},//19M
    {SYSVDD_VOL_SEL_120V, VDC14_VOL_SEL_140V, VDDIOM_VOL_30V},//20M
    {SYSVDD_VOL_SEL_120V, VDC14_VOL_SEL_140V, VDDIOM_VOL_30V},//24M
    {SYSVDD_VOL_SEL_120V, VDC14_VOL_SEL_140V, VDDIOM_VOL_30V},//26M
    {SYSVDD_VOL_SEL_120V, VDC14_VOL_SEL_140V, VDDIOM_VOL_30V},//27M
    {SYSVDD_VOL_SEL_120V, VDC14_VOL_SEL_140V, VDDIOM_VOL_30V},//32M
    {SYSVDD_VOL_SEL_120V, VDC14_VOL_SEL_140V, VDDIOM_VOL_30V},//38M
    {SYSVDD_VOL_SEL_120V, VDC14_VOL_SEL_140V, VDDIOM_VOL_30V},//40M
    {SYSVDD_VOL_SEL_120V, VDC14_VOL_SEL_140V, VDDIOM_VOL_30V},//48M
    {SYSVDD_VOL_SEL_120V, VDC14_VOL_SEL_140V, VDDIOM_VOL_30V},//53M
    {SYSVDD_VOL_SEL_120V, VDC14_VOL_SEL_140V, VDDIOM_VOL_30V},//60M
    {SYSVDD_VOL_SEL_120V, VDC14_VOL_SEL_140V, VDDIOM_VOL_30V},//64M
    {SYSVDD_VOL_SEL_120V, VDC14_VOL_SEL_140V, VDDIOM_VOL_30V},//80M
    {SYSVDD_VOL_SEL_120V, VDC14_VOL_SEL_140V, VDDIOM_VOL_30V},//96M
    {SYSVDD_VOL_SEL_120V, VDC14_VOL_SEL_140V, VDDIOM_VOL_30V},//106M
    {SYSVDD_VOL_SEL_120V, VDC14_VOL_SEL_140V, VDDIOM_VOL_30V},//120M
    {SYSVDD_VOL_SEL_126V, VDC14_VOL_SEL_145V, VDDIOM_VOL_32V},//160M
    {SYSVDD_VOL_SEL_126V, VDC14_VOL_SEL_145V, VDDIOM_VOL_32V},//192M
    {SYSVDD_VOL_SEL_126V, VDC14_VOL_SEL_145V, VDDIOM_VOL_32V},//240M
    {SYSVDD_VOL_SEL_126V, VDC14_VOL_SEL_145V, VDDIOM_VOL_32V},//320M
#ifdef CONFIG_OVERCLOCKING_ENABLE //超频
    {SYSVDD_VOL_SEL_129V, VDC14_VOL_SEL_145V, VDDIOM_VOL_32V},//396M
#endif
};

#define SDRAM_FREQ_CAN_NOT_CHANGE	0 //SDRAM时钟频率默认不能动态改变（经测试，改变可能有风险，SYS:320MHZ+SDRAM:192MHZ与SYS:320MHZ+SDRAM:120MHZ的功耗相差2mA）
static const u32 sys_clock_table[][SYS_CLK_NUM] ALIGNE(4) = {//务必4对齐
    //时钟倍数关系: hsb_clk = sys_clk/n, sfc_clk = hsb_clk/n, lsb_clk = hsb_clk/n, p33_clk = lsb_clk/n
    //  sys_clk      hsb_clk:<=160M  sfc_clk:<=80M  lsb_clk:<=80M  p33_clk:<=20M  sdram_clk:默认不能动态改变
    {  3 * MHZ_UNIT,   3 * MHZ_UNIT,  3 * MHZ_UNIT,  2 * MHZ_UNIT,  2 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//3M
    {  4 * MHZ_UNIT,   4 * MHZ_UNIT,  4 * MHZ_UNIT,  2 * MHZ_UNIT,  2 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//4M
    {  6 * MHZ_UNIT,   6 * MHZ_UNIT,  6 * MHZ_UNIT,  3 * MHZ_UNIT,  3 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//6M
    {  8 * MHZ_UNIT,   8 * MHZ_UNIT,  8 * MHZ_UNIT,  4 * MHZ_UNIT,  4 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//8M
    {  9 * MHZ_UNIT,   9 * MHZ_UNIT,  9 * MHZ_UNIT,  4 * MHZ_UNIT,  4 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//9M
    { 12 * MHZ_UNIT,  12 * MHZ_UNIT, 12 * MHZ_UNIT,  6 * MHZ_UNIT,  6 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//12M
    { 13 * MHZ_UNIT,  13 * MHZ_UNIT, 13 * MHZ_UNIT,  6 * MHZ_UNIT,  6 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//13M
    { 16 * MHZ_UNIT,  16 * MHZ_UNIT, 16 * MHZ_UNIT,  8 * MHZ_UNIT,  8 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//16M
    { 19 * MHZ_UNIT,  19 * MHZ_UNIT, 19 * MHZ_UNIT,  9 * MHZ_UNIT,  9 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//19M
    { 20 * MHZ_UNIT,  20 * MHZ_UNIT, 20 * MHZ_UNIT, 10 * MHZ_UNIT, 10 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//20M
    { 24 * MHZ_UNIT,  24 * MHZ_UNIT, 24 * MHZ_UNIT, 12 * MHZ_UNIT, 12 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//24M
    { 26 * MHZ_UNIT,  26 * MHZ_UNIT, 26 * MHZ_UNIT, 13 * MHZ_UNIT, 13 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//26M
    { 27 * MHZ_UNIT,  27 * MHZ_UNIT, 27 * MHZ_UNIT, 13 * MHZ_UNIT, 13 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//27M
    { 32 * MHZ_UNIT,  32 * MHZ_UNIT, 32 * MHZ_UNIT, 16 * MHZ_UNIT,  8 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//32M
    { 38 * MHZ_UNIT,  38 * MHZ_UNIT, 38 * MHZ_UNIT, 19 * MHZ_UNIT,  9 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//38M
    { 40 * MHZ_UNIT,  40 * MHZ_UNIT, 40 * MHZ_UNIT, 20 * MHZ_UNIT, 10 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//40M
    { 48 * MHZ_UNIT,  48 * MHZ_UNIT, 48 * MHZ_UNIT, 24 * MHZ_UNIT, 12 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//48M
    { 53 * MHZ_UNIT,  53 * MHZ_UNIT, 53 * MHZ_UNIT, 27 * MHZ_UNIT, 13 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//53M
    { 60 * MHZ_UNIT,  60 * MHZ_UNIT, 60 * MHZ_UNIT, 30 * MHZ_UNIT, 10 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//60M
    { 64 * MHZ_UNIT,  64 * MHZ_UNIT, 64 * MHZ_UNIT, 32 * MHZ_UNIT, 11 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//64M
    { 80 * MHZ_UNIT,  80 * MHZ_UNIT, 40 * MHZ_UNIT, 40 * MHZ_UNIT, 13 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//80M
    { 96 * MHZ_UNIT,  96 * MHZ_UNIT, 48 * MHZ_UNIT, 48 * MHZ_UNIT, 16 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//96M
    {106 * MHZ_UNIT, 106 * MHZ_UNIT, 53 * MHZ_UNIT, 53 * MHZ_UNIT, 17 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//106M
    {120 * MHZ_UNIT, 120 * MHZ_UNIT, 60 * MHZ_UNIT, 60 * MHZ_UNIT, 20 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//120M
    {160 * MHZ_UNIT, 160 * MHZ_UNIT, 80 * MHZ_UNIT, 53 * MHZ_UNIT, 17 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//160M
    {192 * MHZ_UNIT,  96 * MHZ_UNIT, 48 * MHZ_UNIT, 48 * MHZ_UNIT, 16 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//192M
    {240 * MHZ_UNIT, 120 * MHZ_UNIT, 60 * MHZ_UNIT, 60 * MHZ_UNIT, 20 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//240M
    {320 * MHZ_UNIT, 160 * MHZ_UNIT, 80 * MHZ_UNIT, 53 * MHZ_UNIT, 17 * MHZ_UNIT, SDRAM_FREQ_CAN_NOT_CHANGE},//320M
#ifdef CONFIG_OVERCLOCKING_ENABLE //超频
    {396 * MHZ_UNIT, 198 * MHZ_UNIT, 98 * MHZ_UNIT, 98 * MHZ_UNIT, 19 * MHZ_UNIT, 240 * MHZ_UNIT},//396M
#endif
};
#define SYSVDDCLK_MAX_NUM		SYSTEM_CLK_320M

/*
 *系统使用函数
 */
void *sys_vdd_table_get(void)
{
    return (void *)sys_vdd_table;
}
/*
 *系统使用函数
 */
void *sys_clock_table_get(void)
{
    return (void *)sys_clock_table;
}

//=========应用层用户测试函数,用户需根据实际场景进行电源和时钟,自行进行调节===========//
//系统时钟切换时间参考如下:
//24M->320M:   6.20ms
//320M->24M:   4.05ms
//24M->32M:    7.16ms
//240M->320M:  3.21ms
//原始时钟较慢，则切换时间就较长
//====================================================================================//
#if 0
#define SYSVDDCLK_SDRAM_TEST	0 //时钟修改+sdram测试
static void sysvddclock_test()
{
    char index = 0;
    u32 cnt = 0;
    u8 step = 0;
    os_time_dly(500);
    while (1) {
#if !SYSVDDCLK_SDRAM_TEST
        os_time_dly(300);
        system_vdd_clock_set(index);
        index = JL_RAND->R64L % SYSVDDCLK_MAX_NUM;
#else
        //时钟修改+sdram测试
        if (cnt % 10 == 0) {
            system_vdd_clock_set(index);
            index = JL_RAND->R64L % SYSVDDCLK_MAX_NUM;
        }
        volatile u32 len = (JL_RAND->R64L % 2048) * 1024;
        if (len < 10) {
            continue;
        }
        volatile u8 *paddr = malloc(len);
        volatile u8 *tmp = paddr;
        volatile u32 crcw, crcr;
        if (paddr) {
            while ((u32)paddr < ((u32)tmp  + len - 4)) {
                *paddr++ = JL_RAND->R64L;
            }
            __asm_csync();
            crcw = CRC32(tmp, len - 4);
            __asm_csync();
            flushinv_dcache(tmp, len);
            __asm_csync();
            crcr = CRC32(tmp, len - 4);
            __asm_csync();
            if (crcw != crcr) {
                printf("---> err in crcw = 0x%x , crcr = 0x%x \n", crcw, crcr);
            }
            free(tmp);
            putchar('#');
        }
        cnt++;
#endif
    }
}

static void sys_vdd_clock_init(void)
{
    os_task_create(sysvddclock_test, NULL, 22, 1000, 0, "sysvddclock_test");
}
late_initcall(sys_vdd_clock_init);
#endif
#endif

