#include "system/timer.h"
#include "asm/adc_api.h"
#include "asm/power_interface.h"
#include "app_power_manage.h"
#include "event/device_event.h"
#include "syscfg/syscfg_id.h"
#include "app_config.h"

#define LOG_TAG_CONST       APP_POWER
#define LOG_TAG             "[APP_POWER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

enum {
    VBAT_NORMAL = 0,
    VBAT_WARNING,
    VBAT_LOWPOWER,
} VBAT_STATUS;

#define VBAT_DETECT_CNT     6

static u16 vbat_slow_timer = 0;
static u16 vbat_fast_timer = 0;
static u16 lowpower_timer = 0;
static u8 old_battery_level = 9;
static u16 bat_val = 0;
static volatile u8 cur_battery_level = 0;
static u16 battery_full_value = 0;
static u8 cur_bat_st = VBAT_NORMAL;

static void vbat_check(void *priv);

static int get_charge_online_flag(void)
{
    return sys_power_is_charging();
}

void power_event_to_user(u8 event)
{
    struct device_event e;
    e.event = event;
    e.value = 0;
    device_event_notify(DEVICE_EVENT_FROM_POWER, &e);
}

u16 get_vbat_level(void)
{
    return (adc_get_voltage(AD_CH_VBAT) * 4 / 10);
}

__attribute__((weak)) u8 remap_calculate_vbat_percent(u16 bat_val)
{
    return 0;
}

u16 get_vbat_value(void)
{
    return bat_val;
}

u8 get_vbat_percent(void)
{
    u16 tmp_bat_val;
    u16 bat_val = get_vbat_level();

    if (battery_full_value == 0) {
        battery_full_value = 420;
    }

    if (bat_val <= LOW_POWER_OFF_VAL) {
        return 0;
    }

    tmp_bat_val = remap_calculate_vbat_percent(bat_val);
    if (!tmp_bat_val) {
        tmp_bat_val = ((u32)bat_val - LOW_POWER_OFF_VAL) * 100 / (battery_full_value - LOW_POWER_OFF_VAL);
        if (tmp_bat_val > 100) {
            tmp_bat_val = 100;
        }
    }
    return (u8)tmp_bat_val;
}

bool get_vbat_need_shutdown(void)
{
    if (bat_val <= LOW_POWER_SHUTDOWN) {
        return TRUE;
    }
    return FALSE;
}

//将当前电量转换为1~9级发送给手机同步电量
u8 battery_value_to_phone_level(u16 bat_val)
{
    u8 battery_level = 0;
    u8 vbat_percent = get_vbat_percent();

    if (vbat_percent < 5) { //小于5%电量等级为0，显示10%
        return 0;
    }

    battery_level = (vbat_percent - 5) / 10;

    return battery_level;
}

//获取自身的电量
u8 get_self_battery_level(void)
{
    return cur_battery_level;
}

u8 get_cur_battery_level(void)
{
    return cur_battery_level;
}

void vbat_check_slow(void *priv)
{
    if (vbat_fast_timer == 0) {
        vbat_fast_timer = usr_timer_add(NULL, vbat_check, 10, 1);
    }
    if (get_charge_online_flag()) {
        sys_timer_modify(vbat_slow_timer, 60 * 1000);
    } else {
        sys_timer_modify(vbat_slow_timer, 10 * 1000);
    }
}

void vbat_check_init(void)
{
    if (vbat_slow_timer == 0) {
        vbat_slow_timer = sys_timer_add_to_task("sys_timer", NULL, vbat_check_slow, 10 * 1000);
    } else {
        sys_timer_modify(vbat_slow_timer, 10 * 1000);
    }

    if (vbat_fast_timer == 0) {
        vbat_fast_timer = usr_timer_add(NULL, vbat_check, 10, 1);
    }
}

void vbat_timer_delete(void)
{
    if (vbat_slow_timer) {
        sys_timer_del(vbat_slow_timer);
        vbat_slow_timer = 0;
    }
    if (vbat_fast_timer) {
        usr_timer_del(vbat_fast_timer);
        vbat_fast_timer = 0;
    }
}

void vbat_check(void *priv)
{
    static u8 unit_cnt = 0;
    static u8 low_warn_cnt = 0;
    static u8 low_off_cnt = 0;
    static u8 low_voice_cnt = 0;
    static u8 low_power_cnt = 0;
    static u8 power_normal_cnt = 0;
    static u8 charge_ccvol_v_cnt = 0;
    static u8 charge_online_flag = 0;
    static u8 low_voice_first_flag = 1;//进入低电后先提醒一次

    if (!bat_val) {
        bat_val = get_vbat_level();
    } else {
        bat_val = (get_vbat_level() + bat_val) / 2;
    }

    cur_battery_level = battery_value_to_phone_level(bat_val);

    //printf("bv:%d, bl:%d , check_vbat:%d\n", bat_val, cur_battery_level, adc_check_vbat_lowpower());

    unit_cnt++;

    /* if (bat_val < LOW_POWER_OFF_VAL) { */
    if (bat_val <= LOW_POWER_OFF_VAL) {
        low_off_cnt++;
    }
    /* if (bat_val < LOW_POWER_WARN_VAL) { */
    if (bat_val <= LOW_POWER_WARN_VAL) {
        low_warn_cnt++;
    }

    /* log_info("unit_cnt:%d\n", unit_cnt); */

    if (unit_cnt >= VBAT_DETECT_CNT) {
        if (get_charge_online_flag() == 0) {
            if (low_off_cnt > (VBAT_DETECT_CNT / 2)) { //低电关机
                low_power_cnt++;
                low_voice_cnt = 0;
                power_normal_cnt = 0;
                cur_bat_st = VBAT_LOWPOWER;
                if (low_power_cnt > 6) {
                    log_info("\n*******Low Power,enter softpoweroff********\n");
                    low_power_cnt = 0;
                    power_event_to_user(POWER_EVENT_POWER_LOW);
                    usr_timer_del(vbat_fast_timer);
                    vbat_fast_timer = 0;
                }
            } else if (low_warn_cnt > (VBAT_DETECT_CNT / 2)) { //低电提醒
                low_voice_cnt ++;
                low_power_cnt = 0;
                power_normal_cnt = 0;
                cur_bat_st = VBAT_WARNING;
                if ((low_voice_first_flag && low_voice_cnt > 1) || //第一次进低电10s后报一次
                    (!low_voice_first_flag && low_voice_cnt >= 5)) {
                    low_voice_first_flag = 0;
                    low_voice_cnt = 0;
                    if (!lowpower_timer) {
                        log_info("\n**Low Power,Please Charge Soon!!!**\n");
                        power_event_to_user(POWER_EVENT_POWER_WARNING);
                    }
                }
            } else {
                power_normal_cnt++;
                low_voice_cnt = 0;
                low_power_cnt = 0;
                if (power_normal_cnt > 2) {
                    if (cur_bat_st != VBAT_NORMAL) {
                        log_info("[Noraml power]\n");
                        cur_bat_st = VBAT_NORMAL;
                        power_event_to_user(POWER_EVENT_POWER_NORMAL);
                    }
                }
            }
        } else {
            power_event_to_user(POWER_EVENT_POWER_CHARGE);
        }

        unit_cnt = 0;
        low_off_cnt = 0;
        low_warn_cnt = 0;
        charge_ccvol_v_cnt = 0;

        if (cur_bat_st != VBAT_LOWPOWER) {
            usr_timer_del(vbat_fast_timer);
            vbat_fast_timer = 0;
            cur_battery_level = battery_value_to_phone_level(bat_val);
            if (cur_battery_level != old_battery_level) {
                power_event_to_user(POWER_EVENT_POWER_CHANGE);
            } else {
                if (charge_online_flag != get_charge_online_flag()) {
                    //充电变化也要交换，确定是否在充电仓
                    power_event_to_user(POWER_EVENT_POWER_CHANGE);
                }
            }
            charge_online_flag =  get_charge_online_flag();
            old_battery_level = cur_battery_level;
        }
    }
}

bool vbat_is_low_power(void)
{
    return (cur_bat_st != VBAT_NORMAL);
}

int app_power_event_handler(struct device_event *dev)
{
    int ret = false;

    switch (dev->event) {
    case POWER_EVENT_POWER_NORMAL:
        if (lowpower_timer) {
            sys_timer_del(lowpower_timer);
            lowpower_timer = 0;
        }
        break;
    case POWER_EVENT_POWER_WARNING:
        if (lowpower_timer == 0) {
            lowpower_timer = sys_timer_add_to_task("sys_timer", (void *)POWER_EVENT_POWER_WARNING, (void (*)(void *))power_event_to_user, LOW_POWER_WARN_TIME);
        }
        break;
    case POWER_EVENT_POWER_LOW:
        r_printf(" POWER_EVENT_POWER_LOW");
        vbat_timer_delete();
        if (lowpower_timer) {
            sys_timer_del(lowpower_timer);
            lowpower_timer = 0 ;
        }
        break;
    case POWER_EVENT_POWER_CHANGE:
        /* user_send_cmd_prepare(USER_CTRL_HFP_CMD_UPDATE_BATTARY, 0, NULL); */
        break;
    case POWER_EVENT_POWER_CHARGE:
        if (lowpower_timer) {
            sys_timer_del(lowpower_timer);
            lowpower_timer = 0 ;
        }
        break;
    default:
        break;
    }

    return ret;
}

/*---------------------------------- shutdown for the countdown ---------------------------------------------*/
static u32 auto_shdn_cnt = 0;
static u8 auto_shdn_lock = 0;
static u16 auto_shdn_timer = 0;

static void sys_power_auto_shutdown_schedule(void *priv)
{
    u32 time = (u32)priv;

    if (auto_shdn_lock) {
        auto_shdn_cnt = 0;
        return;
    }
    /* log_d("auto poweroff cnt = %d to %d", auto_shdn_cnt, time); */
    if (++auto_shdn_cnt >= time) {
        auto_shdn_cnt = 0;
        log_i("count down to 0 to shutdown\n");
        power_event_to_user(POWER_EVENT_POWER_AUTOOFF);
        auto_shdn_lock = 1;
    }
}

//倒计时关机启动
void sys_power_auto_shutdown_start(u32 dly_secs)
{
    auto_shdn_cnt = 0;
    auto_shdn_lock = 0;

    if (dly_secs) {
        if (!auto_shdn_timer) {
            auto_shdn_timer = sys_timer_add_to_task("sys_timer", (void *)dly_secs, sys_power_auto_shutdown_schedule, 1000);
        }
    } else {
        if (auto_shdn_timer) {
            sys_timer_del(auto_shdn_timer);
            auto_shdn_timer = 0;
        }
    }
}

static int sys_power_event_handler(struct sys_event *event)
{
    //倒计时关机变量清空
    auto_shdn_cnt = 0;

    return 0;
}

SYS_EVENT_STATIC_HANDLER_REGISTER(sys_power_enent) = {
    .event_type     = SYS_KEY_EVENT,
    .prob_handler   = sys_power_event_handler,
    .post_handler   = NULL,
};

void sys_power_auto_shutdown_stop(void)
{
    if (auto_shdn_timer) {
        sys_timer_del(auto_shdn_timer);
        auto_shdn_timer = 0;
    }
}

void sys_power_auto_shutdown_pause(void)
{
    auto_shdn_lock = 1;
}

void sys_power_auto_shutdown_resume(void)
{
    auto_shdn_lock = 0;
}

void sys_power_auto_shutdown_clear(void)
{
    auto_shdn_cnt = 0;
}

void sys_power_shutdown(void)
{
    power_event_to_user(POWER_EVENT_POWER_AUTOOFF);
}

__attribute__((weak)) void sys_power_poweroff_wait_powerkey_up(void)
{

}

void sys_power_poweroff(void)
{
    sys_power_poweroff_wait_powerkey_up();
    power_set_soft_poweroff();
}

int sys_power_get_battery_voltage(void)
{
    return get_vbat_value();
}

int sys_power_get_battery_persent(void)
{
    return get_vbat_percent();
}

void sys_power_init(void)
{
    vbat_check_init();

    if (get_vbat_level() <= LOW_POWER_SHUTDOWN) {
        power_set_soft_poweroff();
    }
#ifdef CONFIG_AUTO_SHUTDOWN_ENABLE
    u8 auto_off_time = 0;
    syscfg_read(CFG_AUTO_OFF_TIME_ID, &auto_off_time, sizeof(auto_off_time));
    sys_power_auto_shutdown_start(auto_off_time * 60);
#endif
}
