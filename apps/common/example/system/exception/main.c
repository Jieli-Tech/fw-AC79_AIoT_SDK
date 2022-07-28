#include "app_config.h"
#include "system/includes.h"
#include "asm/debug.h"


/*
[00:00:00.114]---------------------exception error ------------------------

[00:00:00.114]CPU0 run addr = 0x2000af6         进入异常中断前的另外一个CPU运行的地址
[00:00:00.114]cpu 1 exception_analyze DEBUG_MSG = 0x10, EMU_MSG = 0x0 C1_CON=1000000
[00:00:00.114]CPU0 1c015e4 --> 1c02c3c --> 1c02ccc --> 2000fa4
[00:00:00.114]CPU1 2002f5a --> 2002f70 --> 2002f6c --> 2001e1e
[00:00:00.114]R0: abcdef
[00:00:00.114]R1: 0
[00:00:00.114]R2: 0
[00:00:00.114]R3: 1
[00:00:00.114]R4: 20096ac
[00:00:00.114]R5: 1c04230
[00:00:00.114]R6: 20096b0
[00:00:00.114]R7: a5a5a5a5
[00:00:00.114]R8: a5a5a5a5
[00:00:00.114]R9: a5a5a5a5
[00:00:00.114]R10: a5a5a5a5
[00:00:00.114]R11: a5a5a5a5
[00:00:00.114]R12: a5a5a5a5
[00:00:00.114]R13: a5a5a5a5
[00:00:00.114]R14: a5a5a5a5
[00:00:00.114]R15: a5a5a5a5
[00:00:00.114]icfg: 7010280
[00:00:00.114]psr:  6
[00:00:00.114]rets: 0x2002f70       代表死机前被哪个函数调用,    可以通过定位异常地址.bat定位死机的位置
[00:00:00.114]reti: 0x2001e28       代表进入异常中断前的死机地址,可以通过定位异常地址.bat定位死机的位置
[00:00:00.114]usp : 1c0bd74, ssp : 1c06cb8 sp: 1c06cb8

[00:00:00.114]AXI_WR_INV_ID : 0x1ff, AXI_RD_INV_ID : 0x100, PRP_WR_LIMIT_ID : 0x81, PRP_MMU_ERR_RID : 0x1ff, PRP_MMU_ERR_WID : 0x1ff
[00:00:00.114]exception reason : axi_rd_inv         死机原因
[00:00:00.114]system_reset...
*/

#ifdef USE_EXCEPTION_TEST_DEMO

SEC_USED(.volatile_ram_code)
static int test_func(void)
{
    puts("test_func run \r\n");
    return 0;
}

static int c_main(void)
{
#if 0   //非对齐访问导致死机
    static u32 buf[4];
    u16 *prt1 = (u16 *)((u32)buf + 1);
    u32 *prt2 = (u32 *)((u32)buf + 1);
    printf("misalign_err = 0x%x,0x%x \r\n", *prt1, *prt2);
#endif

#if 0   //读访问超出芯片限制的非法地址导致死机
    u32 *prt = (u32 *)0xabcdef;
    printf("axi_rd_inv = 0x%x\r\n", *prt);
#endif

#if 0   //写访问超出芯片限制的非法地址导致死机
    u32 *prt = (u32 *)0xabcdef;
    *prt = 0x12345678;
    printf("axi_wr_inv = 0x%x\r\n", *prt);
#endif

#if 0   //执行非法指令导致死机
    int (*p)(void) = (int (*)(void))0xabcdef;
    printf("c0_if_bus_inv or c1_if_bus_inv = 0x%x\r\n", p());
#endif

#if 0   //定义临时变量过大导致栈溢出死机
    u8 buf[500 * 1024];
    put_buf(buf, sizeof(buf));
#endif

#if 0   //递归层数太深导致栈溢出死机
    int (*p)(void) = (int (*)(void))c_main;
    printf("stackoverflow current_task : app_core = 0x%x\r\n", p());
#endif

#if 0   //代码被改执行非法指令导致死机
    memset(test_func, 0x13, 8);
    printf("c1_rd_bus_inv = 0x%x\r\n", test_func());
#endif

#if 0   //代码跑飞超出硬件限制范围导致死机
    int (*p)(void) = (int (*)(void))((u32)&ram_text_rodata_end + 32);
    printf("c0_pc_limit_err_r or c1_pc_limit_err_r = 0x%x\r\n", p());
#endif

#if 0   //程序卡死导致看门狗死机
    CPU_CRITICAL_ENTER();
    while (1);
    printf(" wdt_timout_err \r\n");
#endif

#if 0   //除0导致死机
    volatile int tmp;
    while (1) {
        tmp = rand32() / (u8)rand32();
    }
    printf("div0_err = 0x%x\r\n", tmp);
#endif


#if 0   //使用硬件保护软件不能够改写的地址范围, 如果软件改写就触发异常
    while (1) {
        char *ptr = malloc(1025);
        memset(ptr, 0xab, 1024);
        cpu_write_range_limit(ptr, 1024);    //加上保护范围

        ptr[1024] = 1; //正常范围改写

        putchar('-');

        if ((rand32() % 66) == 0) { //模拟不知道什么时候超出范围改写就触发异常了
            printf(" 不知道什么时候超出范围改写就触发异常了\r\n");
            ptr[1023] = -1;
        }


        putchar('_');
        cpu_write_range_unlimit(ptr);   //有时候需要合理改动内存就临时关闭保护
        ptr[0] = 0x88;
        cpu_write_range_limit(ptr, 1024 - 1);   //加上保护范围

        os_time_dly(10);

        cpu_write_range_unlimit(ptr);   //free函数会改写内存,因此需要先停止监测
        free(ptr);
    }
#endif

    return 0;
}
late_initcall(c_main);

#endif //USE_EXCEPTION_TEST_DEMO
