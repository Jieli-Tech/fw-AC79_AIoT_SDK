#include "app_config.h"
#include "system/includes.h"
#include "system/timer.h"
#include "system/init.h"

#ifdef USE_USR_TIMEOUT_TEST_DEMO

static int timeout_id;
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

static int  usr_timeout_test(void)
{
    printf("\r\n\r\n-------%s-------%d\r\n\r\n", __func__, __LINE__);
    timeout_id = usr_timeout_add((void *)1, callback_example, 2 * 1000, 1);  //注册一个高精度timeout，定时时间2秒，并返回id，只回调一次
#if 0   //条件0执行修改后的定时时间500毫秒;条件1删除定时时间为2秒的timeout
    printf("timeout_id = %d\r\n", timeout_id);
    usr_timeout_del(timeout_id);    //删除timeout
#else
    if (timeout_id) {
        printf("timeout_id = %d\r\n", timeout_id);
        usr_timeout_modify(timeout_id, 500);  //修改timeout的定时时间为500毫秒
    }
#endif
    return 0;
}

late_initcall(usr_timeout_test);
#endif
