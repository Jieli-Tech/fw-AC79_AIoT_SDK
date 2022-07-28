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
#include "ui/ui.h"
#include "ui_api.h"
#include "ui/includes.h"
#include "font/font_all.h"
#include "font/font_textout.h"

#ifdef UI_DEMO_2_1

/******文件说明*******/
/* 使用屏128*128测试 ui工程名 ui_touch_page_128*128
 * 使用前将app_config中MIAN_PAGE2_TEST开启 其余全关闭
 * 该文件展示如果动态创建列表并向列表中填入文字
 */

#define STYLE_NAME  JL
REGISTER_UI_STYLE(STYLE_NAME)//注册杰理UI

#define WIFI_PAGE     PAGE_1      //垂直列表所在page
#define VLIST_DY_WIFI BASEFORM_27 //垂直列表控件ID
#define TEXT_WIFI0    BASEFORM_33 //垂直列表中文本ID
#define TEXT_WIFI1    BASEFORM_34
#define TEXT_WIFI2    BASEFORM_35
#define TEXT_WIFI3    BASEFORM_38
#define TEXT_WIFI4    BASEFORM_39
#define JPG_ID        BASEFORM_40
#define TITLE_TEXT    BASEFORM_41

static u8 wifi_NUM = 0;

/***********wifi列表**************/
/****说明该wifi列表只演示文本的填充********/
//假设获取到了wifi名如下
char wifi_name[17][30] = {"wifi_列表",
                          "wifi_name1",
                          "wifi_name2",
                          "wifi_name3",
                          "wifi_name4",
                          "wifi_name5",
                          "wifi_name6",
                          "wifi_name7",
                          "wifi_name8",
                          "wifi_name9",
                          "wifi_name10",
                          "wifi_name11",
                          "wifi_name12",
                          "wifi_name13",
                          "wifi_name14",
                          "wifi_name15",
                          "wifi_name16"
                         };


static int list_wifi_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)_ctrl;
    int row, col;

    switch (event) {
    case ON_CHANGE_INIT:
        row = wifi_NUM;
        col = 1;
        ui_grid_init_dynamic(grid, &row, &col);
        /*printf("dynamic_grid %d X %d\n", row, col);*/
        ui_grid_set_slide_direction(grid, SCROLL_DIRECTION_UD);
        break;
    case ON_CHANGE_RELEASE:
        break;
    }
    return 0;
}
static int list_wifi_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    int index;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        index = ui_grid_cur_item_dynamic(grid); //这个为高亮的控件行数 printf("\n\n\nwifi sel %d\n\n\n", index);
        printf("hilight_index %d\n", index);    //这里的index为每次进入后每行的对应标号即刷新到第几行了
        if (index >= wifi_NUM) {                //说明一下这个高亮位置会折中高亮 所以一开始会可能就直接是2 不是0,1
            break;
        }
        break;
    case ELM_EVENT_TOUCH_DOWN:
        break;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(VLIST_DY_WIFI)//主垂直列表项
.onchange = list_wifi_onchange,
 .onkey = NULL,
  .ontouch = list_wifi_ontouch,
};


static int list_text_wifi_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_text *text = (struct ui_text *)_ctrl;
    static u16  store_buf[5][2];
    u8 *store_buf_text;
    u8 index_buf[1];
    char *show_text_buf;

    int index;

    static u8 time = 0;
    switch (event) {
    case ON_CHANGE_INIT:
        break;
    case ON_CHANGE_HIGHLIGHT:
        break;
    case ON_CHANGE_UPDATE_ITEM:

        index = (u32)arg;
        printf("wifi_index %d\n", index);  //这里的index为每次进入后每行的对应标号即刷新到第几行了
        if (index >= wifi_NUM) {
            break;
        }
        switch (text->elm.id) {
        case TITLE_TEXT: //第一行的标题
            if (index == 0) {
                store_buf_text = store_buf[0];
            } else {
                /*ui_hide_main(TITLE_TEXT);*/
                return FALSE;
            }
            break;
        case JPG_ID: //第一行的图片
            if (index != 0) {
                /*ui_hide_main(JPG_ID);*/
            }
            return FALSE;
            break;
        case TEXT_WIFI0:
            if (index != 0) {
                store_buf_text = store_buf[0];
                if (time == 0) {
                    time = 1;
                }
            } else {
                return FALSE;
            }
            break;
        case TEXT_WIFI1:
            store_buf_text = store_buf[1];
            break;
        case TEXT_WIFI2:
            store_buf_text = store_buf[2];
            break;
        case TEXT_WIFI3:
            store_buf_text = store_buf[3];
            break;
        case TEXT_WIFI4:
            store_buf_text = store_buf[4];
            break;
        default:
            return FALSE;
        }

        //输入文本
        show_text_buf = wifi_name[index]; //没消失一行会进这里更新5行的数据

        text->attrs.offset = 0;
        text->attrs.format = "text";
        text->attrs.str    = show_text_buf;
        text->attrs.strlen = strlen(show_text_buf);;
        text->attrs.encode = FONT_ENCODE_UTF8;
        text->attrs.flags  = FONT_DEFAULT;
        text->elm.css.invisible = 0;

        index_buf[0] = index;
        ui_text_set_combine_index(text, store_buf_text, index_buf, 1);
        break;
    }
    return FALSE;
}
REGISTER_UI_EVENT_HANDLER(TEXT_WIFI0)//垂直列表中的文字控件 //触摸事件回下发选择中的控件中 在选中的控件可以注册触摸事件来接收触摸消息
.onchange = list_text_wifi_onchange,
 .onkey = NULL,    //按键事件回调
  .ontouch = NULL, //选中控制触摸消息回调
};
REGISTER_UI_EVENT_HANDLER(TEXT_WIFI1)//垂直列表中的文字控件
.onchange = list_text_wifi_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_WIFI2)//垂直列表中的文字控件
.onchange = list_text_wifi_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_WIFI3)//垂直列表中的文字控件
.onchange = list_text_wifi_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_WIFI4)//垂直列表中的文字控件
.onchange = list_text_wifi_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(JPG_ID)//标题图片
.onchange = list_text_wifi_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TITLE_TEXT)//标题文本
.onchange = list_text_wifi_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};


static void main_page_test_task(void *priv)
{
    user_ui_lcd_init();

    s32 y = 0;

    wifi_NUM =  17; //初始化假设扫描到8个wifi

    ui_show_main(PAGE_1);

    os_time_dly(200);

    struct ui_touch_event e = {0};
    e.event = ELM_EVENT_TOUCH_DOWN;
    e.x = 50;
    e.y = 0;
    ui_touch_msg_post(&e);
    printf(">>>>>>>>>>>>>>>touch_msg_post");

    static u8 time = 0;
    while (1) {
        /*os_time_dly(100);*/
        //模拟触摸动作
        y -= 1;
        e.event = ELM_EVENT_TOUCH_MOVE;
        e.x = 50;
        e.y = -10 + y;

        if (time == 0 && y < (-10)) {
            time = 1;
            ui_hide_main(TITLE_TEXT);
            ui_hide_main(JPG_ID);
        }
        int ret = ui_touch_msg_post(&e);
        /*printf("touch msg text, ret = %d", ret);*/
        os_time_dly(5);
    }

    ui_show_main(PAGE_2);
}

static int main_page_test_task_init(void)
{
    puts("main_page_test_task_init \n\n");
    return thread_fork("main_page_test_task", 11, 1024, 32, 0, main_page_test_task, NULL);
}
late_initcall(main_page_test_task_init);

#endif
