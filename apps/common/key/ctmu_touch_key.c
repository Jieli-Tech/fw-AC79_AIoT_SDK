#include "key/key_driver.h"
#include "key/ctmu_key.h"
#include "app_config.h"

#if TCFG_CTMU_TOUCH_KEY_ENABLE
#include "asm/ctmu.h"

static const struct ctmu_touch_key_platform_data *__this = NULL;

//按键驱动扫描参数列表
struct key_driver_para ctmu_touch_key_scan_para = {
    .scan_time 	  	  = 10,				//按键扫描频率, 单位: ms
    .last_key 		  = NO_KEY,  		//上一次get_value按键值, 初始化为NO_KEY;
    .filter_time  	  = 2,				//按键消抖延时;
    .long_time 		  = 55,  			//按键判定长按数量
    .hold_time 		  = (55 + 15),  	//按键判定HOLD数量
    .click_delay_time = 40,				//按键被抬起后等待连击延时数量
    .key_type		  = KEY_DRIVER_TYPE_CTMU_TOUCH,
    .get_value 		  = ctmu_touch_key_get_value,
};

u8 ctmu_touch_key_get_value(void)
{
    u8 key = get_ctmu_value();

    if (key != NO_KEY) {
        return __this->port_list[key].key_value;
    }

    return NO_KEY;
}

int ctmu_touch_key_init(const struct ctmu_touch_key_platform_data *ctmu_touch_key_data)
{
    __this = ctmu_touch_key_data;
    if (!__this) {
        return -EINVAL;
    }

    return ctmu_init((void *)ctmu_touch_key_data);
}

#endif  /* #if TCFG_CTMU_TOUCH_KEY_ENABLE */

