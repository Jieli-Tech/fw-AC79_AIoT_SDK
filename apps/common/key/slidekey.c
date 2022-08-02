#include "key/key_driver.h"
#include "key/slidekey.h"
#include "device/gpio.h"
#include "app_config.h"

#if (TCFG_SLIDE_KEY_ENABLE)

#include "asm/adc_api.h"

#define SLIDEKEY_FLIT_CNT_MAX	3
#define SLIDEKEY_MAX_CH   		10
#define SLIDEKEY_SCAN_TIME		50

struct slide_key {
    u16 old_ad[SLIDEKEY_MAX_CH];
    u16 flit_cnt[SLIDEKEY_MAX_CH];
    const struct slidekey_platform_data *parm;
};

static struct slide_key key;
#define __this	(&key)

static void slide_key_scan(void *p)
{
    u8 i = 0;
    u16 ad_value_cur = 0;
    u16 offset;
    u16 distance = 0;

    if (!__this->parm->enable) {
        return;
    }

    for (i = 0; i < __this->parm->num; i++) {
        ad_value_cur = (u16)adc_get_value(__this->parm->port[i].ad_channel);

        //printf("nl = %d,nd = %d\n",slide_now_level[i],adc_get_value(__this->parm->port[i].ad_channel));
        if (ad_value_cur > __this->old_ad[i]) {
            offset = ad_value_cur - __this->old_ad[i];
        } else {
            offset = __this->old_ad[i] - ad_value_cur;
        }
        u16 level = (u16)(__this->parm->port[i].level);
        distance = 1023 / (level + 1);

        if (offset > distance / 2) {
            __this->flit_cnt[i]++;
            if (__this->flit_cnt[i] >= SLIDEKEY_FLIT_CNT_MAX) {
                __this->flit_cnt[i] = 0;
                __this->old_ad[i] = ad_value_cur;
                u16 cur_level =  ad_value_cur / distance;
                if (cur_level > level) {
                    cur_level = level;
                }
                //y_printf("key\n");
                app_task_put_key_msg(__this->parm->port[i].msg, cur_level);
            }
        } else {
            __this->flit_cnt[i] = 0;
        }
    }
}

void slidekey_update(void)
{
    local_irq_disable();
    for (int i = 0; i < SLIDEKEY_MAX_CH; i++) {
        __this->old_ad[i] = (u16) - 1; //设置重置,可实现手动更新一次， 更新通过消息通知
        __this->flit_cnt[i] = 0;
    }
    local_irq_enable();
}

int slidekey_init(const struct slidekey_platform_data *slidekey_data)
{
    u8 i = 0;
    //memset(__this, 0x0, sizeof(struct slide_key));
    for (i = 0; i < SLIDEKEY_MAX_CH; i++) {
        __this->old_ad[i] = (u16) - 1; //设置初值， 防止第一次没有变化， 没有发出单位变化
        __this->flit_cnt[i] = 0;
    }

    __this->parm = slidekey_data;
    if (__this->parm == NULL) {
        return -EINVAL;
    }

    for (i = 0; i < __this->parm->num; i++) {
        adc_add_sample_ch(__this->parm->port[i].ad_channel);

        gpio_set_die(__this->parm->port[i].io, 0);
        gpio_set_direction(__this->parm->port[i].io, 1);
        gpio_set_pull_down(__this->parm->port[i].io, 0);
        if (__this->parm->port[i].io_up_en) {
            gpio_set_pull_up(__this->parm->port[i].io, 1);
        } else {
            gpio_set_pull_up(__this->parm->port[i].io, 0);
        }
    }

    sys_s_hi_timer_add(NULL, slide_key_scan, SLIDEKEY_SCAN_TIME); //注册按键扫描定时器

    return 0;
}

#endif  /* #if TCFG_SLIDE_KEY_ENABLE */

