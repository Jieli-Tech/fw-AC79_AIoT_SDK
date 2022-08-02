#include "key/key_driver.h"
#include "key/tent600_key.h"
#include "key/adkey.h"
#include "device/gpio.h"
#include "app_config.h"

#if TCFG_TENT600_KEY_ENABLE

#include "asm/adc_api.h"

//使用demo时，将下列宏剪切到板级的头文件

//*******************************************************//
//                tent600_key 配置
//*******************************************************//
#define TCFG_TENT600_KEY_PORT                   IO_PORTA_03
#define TCFG_TENT600_KEY_AD_CHANNEL             AD_CH_PA3


//使用demo时，将下列参数配置剪切到板级的C文件

/**********************TENT600_KEY**********************/
static const struct tent600_key_platform_data key_data = {
    .enalbe = TCFG_TENT600_KEY_ENABLE,
    .extern_up_en = 0,
    .tent600_key_pin = TCFG_TENT600_KEY_PORT,
    .ad_channel = TCFG_TENT600_KEY_AD_CHANNEL,
};

static const struct tent600_key_platform_data *__this = NULL;

struct key_driver_para tent600_key_scan_para = {
    .scan_time        = 10,             //按键扫描频率, 单位: ms
    .last_key         = NO_KEY,         //上一次get_value按键值, 初始化为NO_KEY;
    .filter_time      = 2,              //按键消抖延时;
    .long_time        = 75,             //按键判定长按数量
    .hold_time        = (75 + 15),      //按键判定HOLD数量
    .click_delay_time = 20,             //按键被抬起后等待连击延时数量
    .key_type         = KEY_DRIVER_TYPE_BRIGHTNESS,
    .get_value        = tent600_get_key_value,
};

u8 tent600_get_key_value(void)
{
    u16 ad_data;
    ad_data = adc_get_value(__this->ad_channel);
    /* printf("ad_value = %d \n", ad_data); */
    u8 ad_level = ad_data / 32;
    static u8 count = 0;
    static u8 old_ad_level = NO_KEY;
    if (ad_level != old_ad_level) {
        count++;
        if (count > 50) {
            old_ad_level = ad_level;
            count = 0;
        }
        return ad_level;
    }

    return NO_KEY;
}

int tent600_key_init(const struct tent600_key_platform_data *tent600_key_data)
{
    if (!tent600_key_data) {
        __this = &key_data;
    } else {
        __this = tent600_key_data;
    }

    adc_add_sample_ch(__this->ad_channel);

    gpio_set_die(__this->tent600_key_pin, 0);
    gpio_set_direction(__this->tent600_key_pin, 1);
    gpio_set_pull_down(__this->tent600_key_pin, 0);
    gpio_set_pull_up(__this->tent600_key_pin, 0);

    return 0;
}

#endif

