#ifndef _VIDEO_SYS_
#define _VIDEO_SYS_


#include "system/includes.h"
#include "server/video_server.h"

enum sys_language_style {
    SYS_CHINESE_SIMPLIFIED,
    SYS_CHINESE_TRADITIONAL,
    SYS_JAPANESE,
    SYS_ENGLISH,
    SYS_DEUTSCH,
};


struct sys_menu_sta {
    u8 lcd_protect;
    u8 auto_off;
    u8 led_fre_hz;
    u8 key_voice;
    u8 language;
    u8 tv_mod;
};

struct video_system_hdl {
    u16 lcd_pro_seconds;
    u16 auto_off_minute;
    u16 led_hz;
    u16 key_tone;
    u8 tv;
    struct server *ui;
    u8 rec_def_set;
    u8 tph_def_set;
    const char *version;
    enum sys_language_style lang;
    u8 default_set;
};

void sys_fun_restore();
void sys_exit_menu_post(void);
void sys_def_set_flag_start_all(void);
u8 sys_def_set_flag_get(const char *app_name);
void sys_def_set_flag_clear(const char *app_name);
u8 get_default_setting_st();
void clear_default_setting_st();
#endif

