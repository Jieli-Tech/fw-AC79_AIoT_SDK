#include "app_config.h"
#include "system/includes.h"
#include "system/sys_common.h"
#include "wait.h"

#ifdef USE_WAIT_COMPLETION_TEST_DEMO

static u8 test_wait_time_sec;
static int wait_condition(void)
{
    if (timer_get_sec() > test_wait_time_sec) {
        puts("wait_condition return OK!\r\n");
        return 1;
    }

    return 0;
}

static int wait_completion_callback(void *priv)
{
    printf("wait_completion_callback run 0x%x!\r\n", (u32)priv);
    return 0;
}

static void wait_completion_test_task(void *p)
{
    os_time_dly(100); //wait_completion初始化完成后才可以测试


    test_wait_time_sec = 1;
    wait_completion_add_to_task("app_core", wait_condition, wait_completion_callback, (void *)0x12345678);
    os_time_dly(test_wait_time_sec * 100 + 100);


    test_wait_time_sec = 30;
    u16 wait_id = wait_completion_add_to_task("app_core", wait_condition, wait_completion_callback, NULL);
    os_time_dly(100);
    wait_completion_del(wait_id);//中途取消,删除wait_completion
    printf("wait_completion_del id[0x%x] OK!\r\n", wait_id);

    while (1) {
        os_time_dly(300);
    }
}
static int c_main(void)
{
    os_task_create(wait_completion_test_task, NULL, 10, 1000, 128, "wait_completion_test_task");
    return 0;
}
late_initcall(c_main);
#endif
