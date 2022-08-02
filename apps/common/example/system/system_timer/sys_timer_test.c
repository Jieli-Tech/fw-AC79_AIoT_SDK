#include "app_config.h"
#include "system/includes.h"
#include "system/timer.h"
#include "system/init.h"


#ifdef USE_SYS_TIMER_TEST_DEMO

static int timer_id;
static int j;   //j为回调次数
static const u8 test_array[10] =  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

static void callback_example(void)  //回调函数
{
    u8 i = 0;
    j++;
    printf("Callback_No%d_array_all:\r\n", j);
    for (i = 0; i < 10; i++) {
        printf("%d\r\n", test_array[i]);
//        printf("array_no.%d：%d\r\n", i, test_array[i]);
    }
    if (j == 3) {
        sys_timer_del(timer_id);    //删除timer，回调3次即删除timer
        timer_id = 0;
    }
}

static int sys_timer_test(void)
{
    printf("\r\n\r\n-------%s-------%d\r\n\r\n", __func__, __LINE__);
    timer_id = sys_timer_add(NULL, callback_example, 5 * 1000);    //注册一个定时器timer，定时时间5秒，并返回id
#if 0   //条件0执行修改后的定时时间2秒，条件1执行定时时间5秒
    printf("timer_id = %d\r\n", timer_id);
#else
    if (timer_id) {
        printf("timer_id = %d\r\n", timer_id);
        sys_timer_modify(timer_id, 2 * 1000);  //修改timer的定时时间为2秒
    }
#endif
    return 0;
}
late_initcall(sys_timer_test);
#endif
