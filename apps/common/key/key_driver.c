#include "event/key_event.h"
#include "system/init.h"
#include "system/timer.h"
#include "key/key_driver.h"
#include "key/iokey.h"
#include "key/adkey.h"
#include "key/slidekey.h"
#include "key/irkey.h"
#include "key/touch_key.h"
#include "key/rdec_key.h"
#include "key/ctmu_key.h"
#include "key/tent600_key.h"
#include "app_config.h"
#include "asm/power_interface.h"
#if TCFG_KEY_TONE_EN
#include "tone_player.h"
#endif

#if(TCFG_IRSENSOR_ENABLE == 1)
#include "irSensor/ir_manage.h"
#endif

#define LOG_TAG_CONST       KEY
#define LOG_TAG             "[KEY]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
//#define LOG_DUMP_ENABLE
//#define LOG_CLI_ENABLE
#include "debug.h"

#define KEY_EVENT_CLICK_ONLY_SUPPORT	1 	//是否支持某些按键只响应单击事件

#if TCFG_SPI_LCD_ENABLE
#ifndef ALL_KEY_EVENT_CLICK_ONLY
#define ALL_KEY_EVENT_CLICK_ONLY	1 	//是否全部按键只响应单击事件
#endif
#else
#ifndef ALL_KEY_EVENT_CLICK_ONLY
#define ALL_KEY_EVENT_CLICK_ONLY	0 	//是否全部按键只响应单击事件
#endif
#endif

static volatile u8 is_key_active = 0;

extern u32 timer_get_ms(void);

//=======================================================//
// 按键值重新映射函数:
// 用户可以实现该函数把一些按键值重新映射, 可用于组合键的键值重新映射
//=======================================================//
int __attribute__((weak)) key_event_remap(struct key_event *e)
{
    return true;
}

//=======================================================//
// 按键扫描函数: 扫描所有注册的按键驱动
//=======================================================//
static void key_driver_scan(void *_scan_para)
{
    struct key_driver_para *scan_para = (struct key_driver_para *)_scan_para;

    u8 key_event = 0;
    u8 cur_key_value = NO_KEY;
    u8 key_value = 0;
    struct key_event e = {0};

    cur_key_value = scan_para->get_value();

    if (cur_key_value != NO_KEY) {
        is_key_active = 35;      //35*10Ms
    } else if (is_key_active) {
        is_key_active --;
    }
//===== 按键消抖处理
    if (cur_key_value != scan_para->filter_value && scan_para->filter_time) {	//当前按键值与上一次按键值如果不相等, 重新消抖处理, 注意filter_time != 0;
        scan_para->filter_cnt = 0; 		//消抖次数清0, 重新开始消抖
        scan_para->filter_value = cur_key_value;	//记录上一次的按键值
        return; 		//第一次检测, 返回不做处理
    } 		//当前按键值与上一次按键值相等, filter_cnt开始累加;
    if (scan_para->filter_cnt < scan_para->filter_time) {
        scan_para->filter_cnt++;
        return;
    }
    //为了滤掉adkey与mic连在一起时电容充放电导致的按键误判,一般用于type-c耳机
    /* if ((cur_key_value != scan_para->last_key) && (scan_para->last_key != NO_KEY) && (cur_key_value != NO_KEY)) { */
    /*     return; */
    /* } */
//===== 按键消抖结束, 开始判断按键类型(单击, 双击, 长按, 多击, HOLD, (长按/HOLD)抬起)
    if (cur_key_value != scan_para->last_key) {
        if (cur_key_value == NO_KEY) {  //cur_key = NO_KEY; last_key = valid_key -> 按键被抬起
#if MOUSE_KEY_SCAN_MODE
            /* if (scan_para->press_cnt >= scan_para->long_time) {  //长按/HOLD状态之后被按键抬起; */
            key_event = KEY_EVENT_UP;
            key_value = scan_para->last_key;
            goto _notify;  	//发送抬起消息
            /* } */
#else
            if (scan_para->press_cnt >= scan_para->long_time) {  //长按/HOLD状态之后被按键抬起;
                key_event = KEY_EVENT_UP;
                key_value = scan_para->last_key;
                goto _notify;  	//发送抬起消息
            }
#endif

            scan_para->click_delay_cnt = 1;  //按键等待下次连击延时开始
        } else {  //cur_key = valid_key, last_key = NO_KEY -> 按键被按下
            scan_para->press_cnt = 1;  //用于判断long和hold事件的计数器重新开始计时;
            if (cur_key_value != scan_para->notify_value) {  //第一次单击/连击时按下的是不同按键, 单击次数重新开始计数
                scan_para->click_cnt = 1;
                scan_para->notify_value = cur_key_value;
            } else {
                scan_para->click_cnt++;  //单击次数累加
            }
        }
        goto _scan_end;  //返回, 等待延时时间到
    } else { 	//cur_key = last_key -> 没有按键按下/按键长按(HOLD)
        if (cur_key_value == NO_KEY) {  //last_key = NO_KEY; cur_key = NO_KEY -> 没有按键按下
            if (scan_para->click_cnt > 0) {  //有按键需要消息需要处理
#if ALL_KEY_EVENT_CLICK_ONLY//彩屏方案支持单击
                key_event = KEY_EVENT_CLICK;  //单击
                key_value = scan_para->notify_value;
                goto _notify;

#endif
#if KEY_EVENT_CLICK_ONLY_SUPPORT 	//是否支持某些按键只响应单击事件
                if (scan_para->notify_value & BIT(7)) {  //BIT(7)按键特殊处理标志, 只发送单击事件, 也可以用于其它扩展
                    key_event = KEY_EVENT_CLICK;  //单击
                    key_value = scan_para->notify_value;
                    goto _notify;
                }
#endif
                if (scan_para->click_delay_cnt > scan_para->click_delay_time) { //按键被抬起后延时到
                    //TODO: 在此可以添加任意多击事件
                    if (scan_para->click_cnt >= 5) {
                        key_event = KEY_EVENT_FIRTH_CLICK;  //五击
                    } else if (scan_para->click_cnt >= 4) {
                        key_event = KEY_EVENT_FOURTH_CLICK;  //4击
                    } else if (scan_para->click_cnt >= 3) {
                        key_event = KEY_EVENT_TRIPLE_CLICK;  //三击
                    } else if (scan_para->click_cnt >= 2) {
                        key_event = KEY_EVENT_DOUBLE_CLICK;  //双击
                    } else {
                        key_event = KEY_EVENT_CLICK;  //单击
                    }
                    key_value = scan_para->notify_value;
                    goto _notify;
                } else {	//按键抬起后等待下次延时时间未到
                    scan_para->click_delay_cnt++;
                    goto _scan_end; //按键抬起后延时时间未到, 返回
                }
            } else {
                goto _scan_end;  //没有按键需要处理
            }
        } else {  //last_key = valid_key; cur_key = valid_key, press_cnt累加用于判断long和hold
            scan_para->press_cnt++;
            if (scan_para->press_cnt == scan_para->long_time) {
                key_event = KEY_EVENT_LONG;
            } else if (scan_para->press_cnt == scan_para->hold_time) {
                key_event = KEY_EVENT_HOLD;
                scan_para->press_cnt = scan_para->long_time;
            } else {
                goto _scan_end;  //press_cnt没到长按和HOLD次数, 返回
            }
            //press_cnt没到长按和HOLD次数, 发消息
            key_value = cur_key_value;
            goto _notify;
        }
    }

_notify:
#if TCFG_IRSENSOR_ENABLE
    if (get_irSensor_event() == IR_SENSOR_EVENT_FAR) {  //未佩戴的耳机不响应按键
        return;
    }
#endif

    key_value &= ~BIT(7);  //BIT(7) 用作按键特殊处理的标志

    e.type = scan_para->key_type;//区分按键类型
    e.action = key_event;
    e.value = key_value;

    scan_para->click_cnt = 0;  //单击次数清0
    scan_para->notify_value = NO_KEY;

    /* printf("key_value: 0x%x, event: %d\n", key_value, key_event); */
    if (key_event_remap(&e)) {
        key_event_notify(KEY_EVENT_FROM_KEY, &e);
#if TCFG_KEY_TONE_EN
        audio_key_tone_play();
#endif
    }

_scan_end:
    scan_para->last_key = cur_key_value;
}

//wakeup callback
void key_active_set(u8 port)
{
    is_key_active = 35;      //35*10Ms
}

//=======================================================//
// 按键初始化函数: 初始化所有注册的按键驱动
//=======================================================//
int key_driver_init(void)
{
    int err;

#if TCFG_IOKEY_ENABLE
    extern const struct iokey_platform_data iokey_data;
    extern struct key_driver_para iokey_scan_para;
    err = iokey_init(&iokey_data);
#ifdef TCFG_IOKEY_TIME_REDEFINE
    extern struct key_driver_para iokey_scan_user_para;
    if (err == 0) {
        sys_s_hi_timer_add((void *)&iokey_scan_user_para, key_driver_scan, iokey_scan_user_para.scan_time); //注册按键扫描定时器
    }
#else
    if (err == 0) {
        sys_s_hi_timer_add((void *)&iokey_scan_para, key_driver_scan, iokey_scan_para.scan_time); //注册按键扫描定时器
    }
#endif
#endif

#if TCFG_ADKEY_ENABLE
    extern const struct adkey_platform_data adkey_data;
    extern struct key_driver_para adkey_scan_para;
    err = adkey_init(&adkey_data);
    if (err == 0) {
        sys_s_hi_timer_add((void *)&adkey_scan_para, key_driver_scan, adkey_scan_para.scan_time); //注册按键扫描定时器
    }
#endif

#if TCFG_IRKEY_ENABLE
    extern const struct irkey_platform_data irkey_data;
    extern struct key_driver_para irkey_scan_para;
    err = irkey_init(&irkey_data);
    if (err == 0) {
        sys_s_hi_timer_add((void *)&irkey_scan_para, key_driver_scan, irkey_scan_para.scan_time); //注册按键扫描定时器
    }
#endif

#if TCFG_TOUCH_KEY_ENABLE
    extern const struct touch_key_platform_data touch_key_data;
    extern struct key_driver_para touch_key_scan_para;
    err = touch_key_init(&touch_key_data);
    if (err == 0) {
        sys_s_hi_timer_add((void *)&touch_key_scan_para, key_driver_scan, touch_key_scan_para.scan_time); //注册按键扫描定时器
    }
#endif

#if TCFG_ADKEY_RTCVDD_ENABLE
    extern const struct adkey_rtcvdd_platform_data adkey_rtcvdd_data;
    extern struct key_driver_para adkey_rtcvdd_scan_para;
    err = adkey_rtcvdd_init(&adkey_rtcvdd_data);
    if (err == 0) {
        sys_s_hi_timer_add((void *)&adkey_rtcvdd_scan_para, key_driver_scan, adkey_rtcvdd_scan_para.scan_time); //注册按键扫描定时器
    }
#endif

#if TCFG_RDEC_KEY_ENABLE
    extern const struct rdec_platform_data rdec_key_data;
    extern struct key_driver_para rdec_key_scan_para;
    err = rdec_key_init(&rdec_key_data);
    if (err == 0) {
        sys_s_hi_timer_add((void *)&rdec_key_scan_para, key_driver_scan, rdec_key_scan_para.scan_time); //注册按键扫描定时器
    }
#endif

#if TCFG_CTMU_TOUCH_KEY_ENABLE
    extern const struct ctmu_touch_key_platform_data ctmu_touch_key_data;
    extern struct key_driver_para ctmu_touch_key_scan_para;
    err = ctmu_touch_key_init(&ctmu_touch_key_data);
    if (err == 0) {
        sys_s_hi_timer_add((void *)&ctmu_touch_key_scan_para, key_driver_scan, ctmu_touch_key_scan_para.scan_time); //注册按键扫描定时器
    }
#endif

#if TCFG_SLIDE_KEY_ENABLE
    extern const struct slidekey_platform_data slidekey_data;
    err = slidekey_init(&slidekey_data);
    if (err == 0) {
    }
#endif

#if TCFG_6083_ADKEY_ENABLE
    extern int adkey_init_6083();
    err = adkey_init_6083();
    if (err == 0) {
    }
#endif

#if TCFG_TENT600_KEY_ENABLE
    extern struct key_driver_para tent600_key_scan_para;
    err = tent600_key_init(NULL);
    if (err == 0) {
        sys_s_hi_timer_add((void *)&tent600_key_scan_para, key_driver_scan, tent600_key_scan_para.scan_time); //注册按键扫描定时器
    }
#endif

    return 0;
}

static u8 key_idle_query(void)
{
    return !is_key_active;
}

#if !TCFG_LP_TOUCH_KEY_ENABLE
REGISTER_LP_TARGET(key_lp_target) = {
    .name = "key",
    .is_idle = key_idle_query,
};
#endif /* #if !TCFG_LP_TOUCH_KEY_ENABLE */

