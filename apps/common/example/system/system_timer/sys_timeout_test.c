#include "app_config.h"
#include "system/includes.h"
#include "system/timer.h"
#include "system/init.h"

#ifdef USE_SYS_TIMEOUT_TEST_DEMO

static int timeout_id;
static int j;   //j为回调次数,sys_timeout只回调一次
static const u8 test_array[10] =  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

static void callback_example(void)  //回调函数
{
    u8 i = 0;
    j++;
    printf("Callback_No%d_array_all:\r\n", j);
    for (i = 0; i < 10; i++) {
        printf("%d\r\n", test_array[i]);
//      printf("array_no.%d：%d\r\n", i, test_array[i]);
    }
}

static int sys_timeout_test(void)
{
    printf("\r\n\r\n-------%s-------%d\r\n\r\n", __func__, __LINE__);
    timeout_id = sys_timeout_add(NULL, callback_example, 5 * 1000);    //注册一个timeout，并返回id，定时时间到了自动删除timeout
#if 0   //条件0执行修改后的定时时间2秒;条件1删除定时时间为5秒的timeout
    printf("timeout_id = %d\r\n", timeout_id);
    sys_timeout_del(timeout_id);    //删除timeout，这里sys_timeout自动删除timeout，用不到该函数。
#else
    if (timeout_id) {
        printf("timeout_id = %d\r\n", timeout_id);
//      sys_timeout_modify(timeout_id, 2 * 1000);  //库里暂时没有这个函数,所以程序实际还是执行5秒
    }
#endif
    return 0;
}
late_initcall(sys_timeout_test);
#endif
