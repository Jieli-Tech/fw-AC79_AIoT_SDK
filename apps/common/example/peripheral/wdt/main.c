#include "app_config.h"
#include "system/includes.h"
#include "asm/wdt.h"

//重写覆盖库内弱函数,可以选择不再使用库内清看门狗
void wdt_clear(void)
{
//    wdt_or_con(BIT(WDTCLR));
}


static void wdt_test_task(void *p)
{
    printf("\r\n\r\n\r\n\r\n\r\n -----------wdt_test_task run begin %s -------------\r\n\r\n\r\n\r\n\r\n", __TIME__);
    while (1); //迫使看门狗溢出复位
}

static void wdt_clear_user(void *p)
{
    while (1) {
        wdt_or_con(BIT(6));
        os_time_dly(1);
    }
}

static int c_main(void)
{
//    wdt_close();    //关闭看门狗,程序跑飞不会复位

    thread_fork("wdt_clear_user", 11, 2048, 0, 0, wdt_clear_user, NULL);       //用户自己外部清狗线程，防止死机复位，注释会死机
//    os_task_create(wdt_test_task, NULL, 30, 1000, 0, "wdt_test_task");

    while (1) {
        os_time_dly(10000);
    }

    return 0;
}

late_initcall(c_main);
