#include "key/key_driver.h"
#include "key/rdec_key.h"
#include "device/gpio.h"
#include "app_config.h"

#if TCFG_RDEC_KEY_ENABLE

static const struct rdec_platform_data *__this = NULL;

struct key_driver_para rdec_key_scan_para = {
    .scan_time 	  	  = 10,				//按键扫描频率, 单位: ms
    .last_key 		  = NO_KEY,  		//上一次get_value按键值, 初始化为NO_KEY;
    .filter_time  	  = 0,				//按键消抖延时;
    .long_time 		  = 75,  			//按键判定长按数量
    .hold_time 		  = (75 + 15),  	//按键判定HOLD数量
    .click_delay_time = 0,				//按键被抬起后等待连击延时数量
    .key_type		  = KEY_DRIVER_TYPE_RDEC,
    .get_value 		  = rdec_get_key_value,
};

u8 rdec_get_key_value(void)
{
    int i;
    s8 rdec_data;
    u8 key_value = 0;

    if (!__this->enable) {
        return NO_KEY;
    }

    for (i = 0; i < 3; i++) {
        rdec_data = get_rdec_rdat(i);
        if (rdec_data < 0) {
            key_value = __this->rdec[i].key_value0;
            return key_value;
        } else if (rdec_data > 0) {
            key_value = __this->rdec[i].key_value1;
            return key_value;
        }
    }

    return NO_KEY;
}

int rdec_key_init(const struct rdec_platform_data *rdec_key_data)
{
    __this = rdec_key_data;
    if (!__this) {
        return -EINVAL;
    }
    if (!__this->enable) {
        return KEY_NOT_SUPPORT;
    }

    return rdec_init(rdec_key_data);
}

#endif  /* #if TCFG_RDEC_KEY_ENABLE */

