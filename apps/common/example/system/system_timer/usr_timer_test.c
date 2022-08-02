#include "app_config.h"
#include "system/includes.h"
#include "system/timer.h"
#include "system/init.h"

#ifdef USE_USR_TIMER_TEST_DEMO

static int timer_id;
static int j;   //j为回调次数
static const u8 test_array[10] =  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

static void array_sequence(int sign)    //数组顺序函数
{
    u8 i = 0;
    j++;
    printf("Callback_No%d_array_all:\r\n", j);
    for (i = 0; i < 10; i++) {
        if (sign = 0) { //数组正序显示
            printf("%d\r\n", test_array[i]);
        } else {        //数组倒序显示
            printf("%d\r\n", test_array[9 - i]);
        }

    }
    if (j == 3) {
        usr_timer_del(timer_id);    //删除timer，回调3次即删除timer
        timer_id = 0;
    }
}

static void callback_example(int i)   //回调函数
{
    switch (i) {
    case 0:
        array_sequence(0);
        break;
    case 1:
        array_sequence(1);
        break;
    }
}

static int  usr_timer_test(void)
{
    printf("\r\n\r\n-------%s-------%d\r\n\r\n", __func__, __LINE__);
    timer_id = usr_timer_add((void *)1, callback_example, 2 * 1000, 1);  //注册一个高精度timer，定时时间2秒，并返回id
#if 0   //条件0执行修改后的定时时间500毫秒，条件1执行定时时间2秒
    printf("timer_id = %d\r\n", timer_id);
#else
    if (timer_id) {
        printf("timer_id = %d\r\n", timer_id);
        usr_timer_modify(timer_id, 500);  //修改timer的定时时间为500毫秒
    }
#endif
    return 0;
}

late_initcall(usr_timer_test);
#endif
