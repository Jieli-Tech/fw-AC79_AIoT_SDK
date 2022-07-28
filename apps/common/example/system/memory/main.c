#include "app_config.h"
#include "system/includes.h"
extern u32 HEAP_BEGIN;


#ifdef USE_MEMORY_TEST_DEMO

//强制把变量对齐到4字节
ALIGNED(4) static char addr_align_test;

//变量定义到flash
static const char const_data[4] = {1, 2, 3, 4};

//变量定义到sdram(如果有) 的data段
static char sdram_data[4] = {1, 2, 3, 4};

//变量定义到sdram(如果有) 的bss段
static char sdram_buf[4];

//变量定义到内部ram的data段
SEC_USED(.volatile_ram) static char ram_data[4] = {1, 2, 3, 4};

//变量定义到内部ram的bss段
SEC_USED(.sram) static char ram_buf[4];

//函数定义到内部ram的data段
SEC_USED(.volatile_ram_code)
static void test_func_in_ram(void)
{
    printf("\r\n test_func_in_ram run addr = 0x%x \r\n", test_func_in_ram);
}

#ifndef CONFIG_NO_SDRAM_ENABLE
//函数定义到sdram的data段,如果有sdram的话
SEC_USED(.data)
static void test_func_in_sdram(void)
{
    printf("\r\n test_func_in_sdram run addr = 0x%x \r\n", test_func_in_sdram);
}
#endif

//函数定义到flash
static void test_func_in_flash(void)
{
    printf("\r\n test_func_in_flash run addr = 0x%x \r\n", test_func_in_flash);
}

static int c_main(void)
{
#ifdef CONFIG_CPU_WL80
    printf("\r\n   ram0 addr begin at  0x1c00000 \r\n\
                 sdram addr begin at 0x4000000 \r\n\
                 flash addr begin at 0x2000000 \r\n\
                 heap addr  begin at 0x%x \r\n\
           ", (u32)&HEAP_BEGIN);
#endif // CONFIG_CPU_WL80

    test_func_in_ram();
#ifndef CONFIG_NO_SDRAM_ENABLE
    test_func_in_sdram();
#endif
    test_func_in_flash();

    int *malloc_addr = malloc(32);
    free(malloc_addr);

    printf("\r\n const_data in addr = 0x%x \r\n \
              sdram_data in addr = 0x%x \r\n \
              sdram_buf in addr = 0x%x \r\n \
              ram_data in addr = 0x%x \r\n \
              ram_buf in addr = 0x%x \r\n \
              malloc_addr = 0x%x \r\n \
              addr_align_test = 0x%x \r\n \
            ", const_data, sdram_data, sdram_buf, ram_data, ram_buf, malloc_addr, &addr_align_test);

    malloc_stats();

    return 0;
}
late_initcall(c_main);
#endif
