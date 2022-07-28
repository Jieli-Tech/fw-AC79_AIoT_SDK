#include "app_config.h"
#include "device/device.h"//u8
#include "server/video_dec_server.h"//fopen
#include "system/includes.h"//GPIO
#include "lcd_drive.h"
#include "sys_common.h"
#include "yuv_soft_scalling.h"
#include "asm/jpeg_codec.h"
#include "lcd_te_driver.h"
#include "system/timer.h"
#include "get_yuv_data.h"

#include "ename.h"
#include "res_config.h"
#include "ui/ui.h"
#include "ui_api.h"
#include "ui/includes.h"
#include "font/font_all.h"
#include "font/font_textout.h"
#include "font/language_list.h"


#ifdef UI_DEMO_2_4

#define STYLE_NAME  JL
REGISTER_UI_STYLE(STYLE_NAME)


extern void set_chinese_simplified(u8 mub);

static void ui_text_test_init(void *priv) //测试显示不同大小文字
{
    static char str1[] = "起来走走";
    user_ui_lcd_init();
    ui_show_main(PAGE_1);

    set_chinese_simplified(0);//选择字体0 font.c中配置文件路径
    ui_text_set_textu_by_id(BASEFORM_7, str1, strlen(str1), FONT_DEFAULT | FONT_SHOW_MULTI_LINE);

    set_chinese_simplified(1);//选择字体1 font.c中配置文件路径
    ui_text_set_textu_by_id(BASEFORM_9, str1, strlen(str1), FONT_DEFAULT | FONT_SHOW_MULTI_LINE);
    while (1) {

        os_time_dly(300);
    }
}

static int ui_text_task_init(void)
{
    puts("ui_text_task_init \n\n");
    return thread_fork("ui_text_test_init", 11, 1024, 0, 0, ui_text_test_init, NULL);
}
late_initcall(ui_text_task_init);

#endif //UI_TEXT_TEST



