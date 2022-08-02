#include "app_config.h"
#include "system/includes.h"
#include "syscfg/syscfg_id.h"

struct syscfg_test_info {
    u8 buf[512];
    u32 cnt;
};
static struct syscfg_test_info syscfg_test_info;

static int c_main(void)
{
    printf("\r\n\r\n\r\n\r\n\r\n -----------syscfg_read example run %s-------------\r\n\r\n\r\n\r\n\r\n", __TIME__);

    int ret;

    while (1) {
        ret = syscfg_read(CFG_USER_DEFINE_BEGIN, &syscfg_test_info, sizeof(syscfg_test_info));
        if (ret < 0) {
            printf("syscfg_read fail = %d\r\n", ret);    //所读配置项不存在,还没写入过配置项
        } else {
            printf("syscfg_read succ, syscfg_test_info.cnt = 0x%x\r\n", syscfg_test_info.cnt);
            put_buf(syscfg_test_info.buf, sizeof(syscfg_test_info.buf));
        }

        ++syscfg_test_info.cnt;
        syscfg_test_info.buf[16] = 0X12 + syscfg_test_info.cnt;
        syscfg_test_info.buf[17] = 0X34 + syscfg_test_info.cnt;
        syscfg_test_info.buf[510] = 0X56 + syscfg_test_info.cnt;
        syscfg_test_info.buf[511] = 0X78 + syscfg_test_info.cnt;

        syscfg_write(CFG_USER_DEFINE_BEGIN, &syscfg_test_info, sizeof(syscfg_test_info));

//        system_reset();     //保存完配置项后复位,查看复位后读取的配置项是否已经记忆生效

        os_time_dly(300);
    }
}
late_initcall(c_main);
