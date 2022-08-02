#include "app_config.h"
#include "system/includes.h"
#include "init.h"


#ifdef USE_INIT_CALL_TEST_DEMO
static int early_initcall_test(void)
{
    printf("\r\n ----------- early_initcall_test run in %s-------------\r\n\r\n", os_current_task());
    return 0;
}
early_initcall(early_initcall_test);

static int platform_initcall_test(void)
{
    printf("\r\n ----------- platform_initcall_test run in %s-------------\r\n", os_current_task());
    return 0;
}
platform_initcall(platform_initcall_test);

static int __initcall_test(void)
{
    printf("\r\n ----------- __initcall_test run in %s-------------\r\n", os_current_task());
    return 0;
}
__initcall(__initcall_test);


static int module_initcall_test(void)
{
    printf("\r\n ----------- module_initcall_test run in %s-------------\r\n", os_current_task());
    return 0;
}
module_initcall(module_initcall_test);

static int late_initcall_test(void)
{
    printf("\r\n ----------- late_initcall_test run in %s-------------\r\n", os_current_task());
    return 0;
}
late_initcall(late_initcall_test);
#endif
