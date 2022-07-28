#include "system/includes.h"
#include "app_config.h"
#include "gpcnt.h"
#include "device/device.h"

#ifdef USE_GPCNT_TEST_DEMO
static void gpcnt_demo_test(void *priv)
{
    void *gpcnt_hd = 0;
    int M, P ;
    u32 gpcnt = 1;
    gpcnt_hd = dev_open("gpcnt", NULL);
    while (1) {
        dev_ioctl(gpcnt_hd, IOCTL_GET_GPCNT, (u32)&gpcnt);
        printf(">>>>>>>>>>>gpcnt = %d", gpcnt);
        os_time_dly(100);
    }
}

static int gocnt_test_task_init(void)
{
    puts("gocnt_test_task_init \n\n");
    return thread_fork("gpcnt_demo_test", 10, 1024, 0, 0, gpcnt_demo_test, NULL);
}

late_initcall(gocnt_test_task_init);
#endif
