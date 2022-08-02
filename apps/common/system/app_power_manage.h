#ifndef _APP_POWER_MANAGE_H_
#define _APP_POWER_MANAGE_H_

#include "generic/typedef.h"
#include "asm/system_reset_reason.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LOW_POWER_SHUTDOWN      320  //低电直接关机电压, 如果VDDIO使用内部LDO供电, 建议低于3.6V关机
#define LOW_POWER_OFF_VAL   	330  //低电关机电压
#define LOW_POWER_WARN_VAL   	340  //低电提醒电压
#define LOW_POWER_WARN_TIME   	(60 * 1000)  //低电提醒时间

enum {
    POWER_EVENT_POWER_NORMAL,
    POWER_EVENT_POWER_WARNING,
    POWER_EVENT_POWER_LOW,
    POWER_EVENT_POWER_CHANGE,
    POWER_EVENT_POWER_AUTOOFF,
    POWER_EVENT_POWER_CHARGE,
};

#define PWR_DELAY_INFINITE      0xffffffff
#define PWR_WKUP_PORT           "wkup_port"
#define PWR_WKUP_ALARM          "wkup_alarm"
#define PWR_WKUP_PWR_ON         "wkup_pwr_on"
#define PWR_WKUP_ABNORMAL       "wkup_abnormal"
#define PWR_WKUP_SHORT_KEY      "wkup_short_key"

void sys_power_init(void);
int sys_power_get_battery_voltage(void);
int sys_power_get_battery_persent(void);
int sys_power_is_charging(void);
void sys_power_poweroff(void);
/*
 * @brief 软关机，触发DEVICE_EVENT_POWER_SHUTDOWN事件，app捕获事件释放资源再调用sys_power_poweroff()
 */
void sys_power_shutdown(void);
/*
 * @brief 倒计时自动关机
 * @parm dly_secs 延时关机时间，赋值0为永不关机
 * @return none
 */
void sys_power_auto_shutdown_start(u32 dly_secs);
void sys_power_auto_shutdown_pause(void);
void sys_power_auto_shutdown_resume(void);
void sys_power_auto_shutdown_clear(void);
void sys_power_auto_shutdown_stop(void);

#ifdef __cplusplus
}
#endif
#endif
