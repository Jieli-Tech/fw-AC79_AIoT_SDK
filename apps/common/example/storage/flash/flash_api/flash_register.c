#include "app_config.h"
#include "system/includes.h"
#include "asm/sfc_norflash_api.h"


//所有的函数都需要定义在spi_code段
#define AT_SPI_CODE SEC_USED(.spi_code)

AT_SPI_CODE
static u32 user_flash_spi_test(void)
{
    u8 reg1 = 0, reg2 = 0;
    int ret;

    printf("---> user_flash_spi_test \n");
    /******************************************************
      在norflash_enter_spi_code和norflash_exit_spi_code框内：
      所有代码和数据和const数组和变量只能在sdram或者内部sram，
      数据和const数组和变量可以指定内部sram的SEC_USED(.sram)或
      者sdram的SEC_USED(.data)，如果系统的代码是跑flash（即打开了CONFIG_SFC_ENABLE宏），
      则不能加打印信息
    *******************************************************/
    //1 用户独自操作flash先norflash_enter_spi_code
    norflash_enter_spi_code();

    //2 等待flash忙结束
    ret = norflash_wait_busy();
    if (!ret) { //不忙读取寄存器测试
        //3 读取寄存器1
        norflash_spi_cs(0);
        norflash_spi_write_byte(0x05);
        reg1 = norflash_spi_read_byte();
        norflash_spi_cs(1);

        //4 读取寄存器2
        norflash_spi_cs(0);
        norflash_spi_write_byte(0x35);
        reg2 = norflash_spi_read_byte();
        norflash_spi_cs(1);
    }

    //5 用户独自操作flash完成则需要norflash_exit_spi_code
    norflash_exit_spi_code();

    /******************************************************
       在norflash_enter_spi_code和norflash_exit_spi_code框内：
      所有代码和数据和const数组和变量只能在sdram或者内部sram，
      数据和const数组和变量可以指定内部sram的SEC_USED(.sram)或
      者sdram的SEC_USED(.data)，如果系统的代码是跑flash（即打开了CONFIG_SFC_ENABLE宏），
      则不能加打印信息
    *******************************************************/
    //6 因此下列打印需要放在退出之后才能使用
    printf("---> reg1 = 0x%x , reg2 = 0x%x \n", reg1, reg2);
}

static int c_main(void)
{
    user_flash_spi_test();
}
late_initcall(c_main);
