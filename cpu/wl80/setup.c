#include "app_config.h"
#include "asm/includes.h"
#include "system/includes.h"
#include "asm/system_reset_reason.h"

void setup_arch()
{
    /*wdt_close();*/
    wdt_init(0xc);

    clk_early_init();

    interrupt_init();

#ifdef CONFIG_DEBUG_ENABLE
    extern void debug_uart_init();
    debug_uart_init();
#ifdef __LOG_ENABLE
    log_early_init(10 * 1024);
#endif
#endif

    system_reset_reason_get();

    puts("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    printf("\n   WL80(AC790N) CHIP_ID: 0x%x  setup_arch %s %s", JL_INTEST->CHIP_ID, __DATE__, __TIME__);
    extern u32 SDRAM_SIZE, RAM0_SIZE, CACHE_RAM_SIZE, text_size, data_size, bss_size, ram0_bss_size, ram0_data_size, cache_ram_bss_size, cache_ram_data_size, HEAP_END, HEAP_BEGIN;

    printf("\nsys_clk = %d,sdram_clk = %d,hsb_clk = %d,lsb_clk = %d, sfc_clk = %d", clk_get("sys"), clk_get("sdram"), clk_get("hsb"), clk_get("timer"), clk_get("hsb") / (JL_SFC->BAUD + 1));

    printf("\nCODE+CONST SIZE = %d", (u32)&text_size);
    printf("\nSDRAM_SIZE = %d, DATA_SIZE = %d,BSS_SIZE = %d,REMAIN_SIZE = %d", (u32)&SDRAM_SIZE, (u32)&data_size, (u32)&bss_size, (u32)&SDRAM_SIZE - (u32)&data_size - (u32)&bss_size);
    printf("\nRAM_SIZE = %d,DATA_SIZE = %d,BSS_SIZE = %d,REMAIN_SIZE = %d", (u32)&RAM0_SIZE, (u32)&ram0_data_size, (u32)&ram0_bss_size, (u32)&RAM0_SIZE - (u32)&ram0_data_size - (u32)&ram0_bss_size);
    printf("\nCACHE_RAM_SIZE = %d,DATA_SIZE = %d, BSS_SIZE = %d,REMAIN_SIZE = %d", (u32)&CACHE_RAM_SIZE, (u32)&cache_ram_data_size, (u32)&cache_ram_bss_size, (u32)&CACHE_RAM_SIZE - (u32)&cache_ram_data_size - (u32)&cache_ram_bss_size);
    printf("\nHEAP_SIZE = %d", (u32)&HEAP_END - (u32)&HEAP_BEGIN);
    puts("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

    debug_init();

    __crc16_mutex_init();

#ifdef CONFIG_DISABLE_P3_FSPG_CON
    p33_tx_1byte(P3_FSPG_CON, 0);
#endif

    p33_io_latch_init();
}

