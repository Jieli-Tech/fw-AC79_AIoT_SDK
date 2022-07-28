#include "ui/ui.h"
#include "app_config.h"
/*#include "ui_style.h"*/
/*#include "app_action.h"*/
#include "system/timer.h"
#include "font/language_list.h"
#include "res/resfile.h"
#include "syscfg_id.h"

#ifdef CONFIG_UI_ENABLE
/*#include "ui_sys_param.h"*/

struct SYS_INFO {
    u32 version_log;                 //版本日期
    u32 space_log;                   //内存大小
};

struct UI_DISPLAY {
    u16 backlight_time;             //背光时间
    u16 backlight_brightness;       //背光亮度
};

typedef struct _UI_SYS_PARAM {
    u16 cur_auto_close_time;        //当前设置的关机时间
    struct SYS_INFO sys_infomation; //系统信息
    struct UI_DISPLAY display;      //显示信息
} UI_SYS_PARAM;

//背光亮度
static const char table_system_lcd_value[] = {
    100,
    80,
    60,
    30,
};

//背光时间
static const u16 table_system_lcd_protect[] = {
    0,
    30,
    60,
    120,
};

//自动关机时间
static const u16 table_system_auto_close[] = {
    0,
    15 * 60,
    30 * 60,
    60 * 60,
    120 * 60,
    180 * 60,
};

static UI_SYS_PARAM sys_param;
static int system_auto_close_timer;                 //自动关机定时器
#define __this 	(&sys_param)

extern void sys_enter_soft_poweroff(void *priv);
void auto_power_off_timer_close(void);
//*********************************************************************************//
//                                 配置开始                                        //
//*********************************************************************************//
void set_sys_param_default(void)
{
    __this->display.backlight_time = table_system_lcd_protect[0];
    __this->display.backlight_brightness = table_system_lcd_value[0];
    __this->cur_auto_close_time = table_system_auto_close[0];
    auto_power_off_timer_close();
}

void sys_param_write2vm(void)
{
    /*syscfg_write(CFG_UI_SYS_INFO, __this, sizeof(UI_SYS_PARAM));*/
}

void sys_param_init(void)
{
    /* int ret = 0; */
    /* r_printf("sys_param_init\n"); */
    /* ret = syscfg_read(CFG_UI_SYS_INFO, __this, sizeof(UI_SYS_PARAM)); */
    /* if (ret != sizeof(UI_SYS_PARAM)) { */
    /* r_printf("sys_param read err\n"); */
    /* memset(__this, 0, sizeof(UI_SYS_PARAM)); //暂时默认都为0； */
    /* set_sys_param_default(); */
    /* sys_param_write2vm(); */
    /* } */
    /* if (__this->cur_auto_close_time && (!system_auto_close_timer)) { */
    /*每次重新开机把上次设置的自动关机时间设置进去*/
    /*system_auto_close_timer = sys_timeout_add(NULL, sys_enter_soft_poweroff, (__this->cur_auto_close_time * 1000));*/
    /* } */
}

//*********************************************************************************//
//                                 系统信息配置                                    //
//*********************************************************************************//
void set_version_log(u32 log)
{
    __this->sys_infomation.version_log = log;
}

u32 get_version_log(void)
{
    return __this->sys_infomation.version_log;
}

void set_space_log(u32 log)
{
    __this->sys_infomation.space_log = log;
}

u32 get_space_log(void)
{
    return __this->sys_infomation.space_log;
}

//*********************************************************************************//
//                                 自动关机配置                                    //
//*********************************************************************************//
void set_auto_poweroff_timer(int sel_item)
{
    __this->cur_auto_close_time = table_system_auto_close[sel_item];
    if (system_auto_close_timer) {
        sys_timer_modify(system_auto_close_timer, __this->cur_auto_close_time * 1000);
    } else {
        system_auto_close_timer = sys_timeout_add(NULL, sys_enter_soft_poweroff, (__this->cur_auto_close_time * 1000));
    }
    sys_param_write2vm();
}

void auto_power_off_timer_close(void)
{
    if (system_auto_close_timer) {
        sys_timeout_del(system_auto_close_timer);
        system_auto_close_timer = 0;
    }
}

u16 get_cur_auto_power_time(void)
{
    return __this->cur_auto_close_time;
}

//*********************************************************************************//
//                                 背光配置                                        //
//*********************************************************************************//
void set_backlight_time(u16 time)
{
    __this->display.backlight_time = table_system_lcd_protect[time];
}

u16 get_backlight_time(void)
{
    return __this->display.backlight_time;
}

void set_backlight_brightness(u16 brightness)
{
    __this->display.backlight_brightness = table_system_lcd_value[brightness];
}

u16 get_backlight_brightness(void)
{
    return __this->display.backlight_brightness;
}

//*********************************************************************************//
//                                 其他配置                                        //
//*********************************************************************************//
void set_sys_info_reset(void)
{
    set_sys_param_default();
    sys_param_write2vm();
}

#endif //TCFG_SPI_LCD_ENABLE
