#include "app_config.h"
#include "system/includes.h"
#include "asm/wdt.h"


#ifdef USE_CPU_PERFORMANCE_TEST_DEMO

static void BenchMark_test_task(void *p)
{
    int argc;
    char *argv[4] = {"", 0};

    void dhry_main(unsigned int);
    dhry_main(10000000);

    void linpack_main(int arsize);
    linpack_main(100);

    argc = 0;
    argv[++argc] = "1000";
    int whetstone_main(int argc, char *argv[]);
    whetstone_main(++argc, argv);

    argc = 0;
    argv[++argc] = "10000"; //Number of digits of pi to calculate,注意精度越大消耗越大堆和任务的栈
    int ipi_fftcs_main(int argc, char *argv[]);
    ipi_fftcs_main(++argc, argv);

    puts("\r\n\r\n BenchMarkTest RUN END\r\n");

    while (1) {
        os_time_dly(1);
    }
}

static int c_main(void)
{
    printf("\r\n\r\n\r\n\r\n\r\n -----------BenchMark run %s -------------\r\n\r\n\r\n\r\n\r\n", __TIME__);

    wdt_close();    //关闭看门狗,防止占据CPU太猛导致复位

    os_task_create(BenchMark_test_task, NULL, 30, 3000, 0, "BenchMark_test_task");

    while (1) {
        os_time_dly(10000);
    }

    return 0;
}

late_initcall(c_main);

#endif //USE_CPU_PERFORMANCE_TEST_DEMO
