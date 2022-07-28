#include "app_config.h"
#include "system/includes.h"
#include "printf.h"

#ifdef USE_PRINTF_TEST_DEMO
static int c_main(void)
{
    /*
    如果要测试用户打印,需要在app_config.h 增加两个宏定义
    #define CONFIG_USER_DEBUG_ENABLE
    #define CONFIG_SYS_DEBUG_DISABLE
    */
    user_putchar('U');
    user_puts("\r\n\r\n\r\n user_puts  \r\n\r\n\r\n");
    user_printf("\r\n\r\n\r\n ----------- user_printf %s-------------\r\n\r\n\r\n", __TIME__);
    user_put_buf(c_main, 32);


    return 0;
}
late_initcall(c_main);
#endif
