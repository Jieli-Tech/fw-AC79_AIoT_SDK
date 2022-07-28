#include "app_config.h"

#ifdef USE_UI_TOUCH_DEMO

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




/*page0 拨号键盘输入测试*/
struct dial_info {
    char number[256];
    int index;
};
struct dial_info dial_info_t = {0};

static void dial_add_number(char ch)
{
    static struct unumber n;

    if (dial_info_t.index >= 29) {
        return;
    }
    dial_info_t.number[dial_info_t.index++] = ch;
    dial_info_t.number[dial_info_t.index] = '\0';
    n.type = TYPE_STRING;
    n.num_str = (u8 *)dial_info_t.number;
    ui_number_update_by_id(DIAL_NUM, &n);
}

static void dial_remove_number()
{
    static struct unumber n;

    if (dial_info_t.index <= 0) {
        return;
    }
    dial_info_t.number[--dial_info_t.index] = '\0';
    n.type = TYPE_STRING;
    n.num_str = (u8 *)dial_info_t.number;
    ui_number_update_by_id(DIAL_NUM, &n);
}

static int DIAL_Common_ontouch(void *ctrl, struct element_touch_event *event)
{
    struct element *elm = (struct element *)ctrl;

    printf("DIAL_Common_ontouch");
    switch (event->event) {
    case ELM_EVENT_TOUCH_DOWN:
        switch (elm->id) {
        case DIAL_1:
            printf("DIAL_1 DOWN\n");
            dial_add_number('1');
            break;
        case DIAL_2:
            printf("DIAL_2 DOWN\n");
            dial_add_number('2');
            break;
        case DIAL_3:
            printf("DIAL_3 DOWN\n");
            dial_add_number('3');
            break;
        case DIAL_4:
            printf("DIAL_4 DOWN\n");
            dial_add_number('4');
            break;
        case DIAL_5:
            printf("DIAL_5 DOWN\n");
            dial_add_number('5');
            break;
        case DIAL_6:
            printf("DIAL_6 DOWN\n");
            dial_add_number('6');
            break;
        case DIAL_7:
            printf("DIAL_7 DOWN\n");
            dial_add_number('7');
            break;
        case DIAL_8:
            printf("DIAL_8 DOWN\n");
            dial_add_number('8');
            break;
        case DIAL_9:
            printf("DIAL_9 DOWN\n");
            dial_add_number('9');
            break;
        case DIAL_0:
            printf("DIAL_0 DOWN\n");
            dial_add_number('0');
            break;
        case DIAL_CTL:
            printf("DIAL_CTL DOWN\n");
            dial_add_number('*');
            break;
        case DIAL_DEL:
            printf("DIAL_DEL DOWN\n");
            dial_remove_number();
            break;
        default:
            break;
        }
        ui_core_redraw(elm);
        break;
    case ELM_EVENT_TOUCH_MOVE:
        ui_hide_main(PAGE_0);
        ui_show_main(PAGE_1);
        break;
    }
    return true;
}

REGISTER_UI_EVENT_HANDLER(DIAL_1)
.ontouch = DIAL_Common_ontouch,
};
REGISTER_UI_EVENT_HANDLER(DIAL_2)
.ontouch = DIAL_Common_ontouch,
};
REGISTER_UI_EVENT_HANDLER(DIAL_3)
.ontouch = DIAL_Common_ontouch,
};
REGISTER_UI_EVENT_HANDLER(DIAL_4)
.ontouch = DIAL_Common_ontouch,
};
REGISTER_UI_EVENT_HANDLER(DIAL_5)
.ontouch = DIAL_Common_ontouch,
};
REGISTER_UI_EVENT_HANDLER(DIAL_6)
.ontouch = DIAL_Common_ontouch,
};
REGISTER_UI_EVENT_HANDLER(DIAL_7)
.ontouch = DIAL_Common_ontouch,
};
REGISTER_UI_EVENT_HANDLER(DIAL_8)
.ontouch = DIAL_Common_ontouch,
};
REGISTER_UI_EVENT_HANDLER(DIAL_9)
.ontouch = DIAL_Common_ontouch,
};
REGISTER_UI_EVENT_HANDLER(DIAL_0)
.ontouch = DIAL_Common_ontouch,
};
REGISTER_UI_EVENT_HANDLER(DIAL_CTL)//*号键
.ontouch = DIAL_Common_ontouch,
};
REGISTER_UI_EVENT_HANDLER(DIAL_DEL)//删除键
.ontouch = DIAL_Common_ontouch,
};

//有选择触摸异常序号
/*page1 垂直列表触摸测试*/
static u8 list_touch_index;
static u8 touch_flog = 0;
static int shortcut_key_onchange(void *ctr, enum element_change_event event, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    int row, col;
    switch (event) {
    case ON_CHANGE_INIT:
        row = 8;
        col = 1;
        ui_grid_init_dynamic(grid, &row, &col);//初始化垂直列表的个数用于判断触摸行号使用
        ui_grid_set_slide_direction(grid, SCROLL_DIRECTION_UD);//限制控件滑动方向
        break;
    }
    return false;
}
static int shortcut_key_ontouch(void *ctr, struct element_touch_event *e)
{
    int indx;
    struct ui_grid *grid = (struct ui_grid *)ctr;
    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        indx = ui_grid_cur_item_dynamic(grid);
        if (indx) {
            list_touch_index = indx;
        }

        printf(">>>>>>>>>touch_index = %d", list_touch_index);
        break;
    case ELM_EVENT_TOUCH_MOVE:
        touch_flog++;
        if (touch_flog > 250) {
            touch_flog = 200;
        }
        /*printf("<<<<<<<<<<<<<<<<<ELM_EVENT_TOUCH_MOVE");*/
        break;
    case ELM_EVENT_TOUCH_R_MOVE:
        ui_hide_main(PAGE_1);
        ui_show_main(PAGE_2);
        break;
    }
    return false;//不接管消息
}
REGISTER_UI_EVENT_HANDLER(BASEFORM_2)
.onchange = shortcut_key_onchange,
 .ontouch = shortcut_key_ontouch,
};

static int tips_common_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    static u8 move_flag = 0;
    int sel_item;
    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        touch_flog = 1;
        /*printf(">>>>>>>>>>>>>>ELM_EVENT_TOUCH_DOWN");*/
        /*return true;//接管消息*/
        break;
    case ELM_EVENT_TOUCH_UP:
        touch_flog++;
        if (touch_flog == 2) {
            /*printf(">>>>>>>>>>>>>>ELM_EVENT_TOUCH_UP");*/
            ui_hide_main(BASEFORM_1);//隐藏垂直列表
            ui_show_main(BASEFORM_40);
        }
        break;
    }
    return false;
}

REGISTER_UI_EVENT_HANDLER(LIST_1)
.ontouch = tips_common_ontouch,
};
REGISTER_UI_EVENT_HANDLER(LIST_2)
.ontouch = tips_common_ontouch,
};
REGISTER_UI_EVENT_HANDLER(LIST_3)
.ontouch = tips_common_ontouch,
};
REGISTER_UI_EVENT_HANDLER(LIST_4)
.ontouch = tips_common_ontouch,
};
REGISTER_UI_EVENT_HANDLER(LIST_5)
.ontouch = tips_common_ontouch,
};
REGISTER_UI_EVENT_HANDLER(LIST_6)
.ontouch = tips_common_ontouch,
};
REGISTER_UI_EVENT_HANDLER(LIST_7)
.ontouch = tips_common_ontouch,
};
REGISTER_UI_EVENT_HANDLER(LIST_8)
.ontouch = tips_common_ontouch,
};

static int phone_off_ontouch(void *ctr, struct element_touch_event *e)
{
    static char str1[] = "你按下了挂断图标";
    struct ui_grid *grid = (struct ui_grid *)ctr;
    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        printf("phone_off_ontouch_down");
        ui_text_set_textu_by_id(BASEFORM_41, str1, strlen(str1), FONT_DEFAULT | FONT_SHOW_MULTI_LINE);
        break;
    }
    return true;//接管消息
}

static int phone_on_ontouch(void *ctr, struct element_touch_event *e)
{
    static char str2[] = "你按下了接听图标";
    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        printf("phone_on_ontouch_down");
        ui_text_set_textu_by_id(BASEFORM_41, str2, strlen(str2), FONT_DEFAULT);
        break;
    }
    return true;//接管消息
}

static int phone_main_onchange(void *ctr, enum element_change_event event, void *arg)
{
    static char str1[40];
    struct ui_grid *grid = (struct ui_grid *)ctr;
    switch (event) {
    case ON_CHANGE_INIT:
        sprintf(str1, "你选择的是列表%d", list_touch_index);
        ui_text_set_textu_by_id(BASEFORM_41, str1, strlen(str1), FONT_DEFAULT | FONT_SHOW_MULTI_LINE);
        break;
    }
    return false;

}


static int phone_main_ontouch(void *ctr, struct element_touch_event *e)
{
    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        ui_hide_main(BASEFORM_40);
        ui_show_main(BASEFORM_1);
        break;
    }
    return false;
}

REGISTER_UI_EVENT_HANDLER(BASEFORM_42)//挂断电话图片
.ontouch = phone_off_ontouch,
};
REGISTER_UI_EVENT_HANDLER(BASEFORM_43)//接听电话图片
.ontouch = phone_on_ontouch,
};
REGISTER_UI_EVENT_HANDLER(BASEFORM_40)//布局界面
.onchange = phone_main_onchange,
 .ontouch = phone_main_ontouch,
};

/*page2 滑块控件触摸测试*/
static u8 slider_percentx, vslider_percenty;

static int silider_x_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_slider *slider = (struct ui_slider *)ctr;
    int slider_percent;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        slider_percentx = slider_get_percent(slider);
        break;
    case ELM_EVENT_TOUCH_MOVE:
        /*printf(">>>>>>>vsliderMOVE");*/
        slider_touch_slider_move(slider, e);
        break;
    }
    return true;//接管消息
}

static int vilider_y_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_vslider *vslider = (struct ui_vslider *)ctr;
    int vslider_percent;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        vslider_percenty = vslider_get_percent(vslider);
        break;
    case ELM_EVENT_TOUCH_MOVE:
        /*printf(">>>>>>>vsliderMOVE");*/
        vslider_touch_slider_move(vslider, e);
        break;
    }
    return true;//接管消息
}

static int minus_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_slider *slider = (struct ui_slider *)ctr;
    struct ui_vslider *vslider = (struct ui_vslider *)ctr;
    switch (e->event) {


    case ELM_EVENT_TOUCH_DOWN:
        if (slider_percentx < 10) {
            slider_percentx = 0;
        } else {
            slider_percentx -= 10;
        }
        if (vslider_percenty < 10) {
            vslider_percenty = 0;
        } else {
            vslider_percenty -= 10;
        }
        ui_slider_set_persent_by_id(NEWLAYOUT_21, slider_percentx);
        ui_slider_set_persent_by_id(NEWLAYOUT_22, vslider_percenty);
        break;
    }
    return false;
}

static int add_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_slider *slider = (struct ui_slider *)ctr;
    struct ui_vslider *vslider = (struct ui_vslider *)ctr;
    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        if (slider_percentx > 90) {
            slider_percentx = 100;
        } else {
            slider_percentx += 10;
        }
        if (vslider_percenty > 90) {
            vslider_percenty = 100;
        } else {
            vslider_percenty += 10;
        }
        ui_slider_set_persent_by_id(NEWLAYOUT_21, slider_percentx);
        ui_slider_set_persent_by_id(NEWLAYOUT_22, vslider_percenty);
        break;
    case ELM_EVENT_TOUCH_R_MOVE:
        ui_hide_main(PAGE_2);
        ui_show_main(PAGE_0);
        break;
    }
    return false;
}

REGISTER_UI_EVENT_HANDLER(NEWLAYOUT_21)//水平滑块控件
.ontouch = silider_x_ontouch,
};
REGISTER_UI_EVENT_HANDLER(NEWLAYOUT_22)//垂直滑块控件
.ontouch = vilider_y_ontouch,
};
REGISTER_UI_EVENT_HANDLER(BASEFORM_47)//减号图片
.ontouch = minus_ontouch,
};
REGISTER_UI_EVENT_HANDLER(BASEFORM_48)//加号图片
.ontouch = add_ontouch,
};

/*page3 动态列表触摸测试*/
/* REGISTER_UI_EVENT_HANDLER(BASEFORM_63)//设置小时垂直列表 */
/* .ontouch = set_hour_ontouch, */
/* }; */
/* REGISTER_UI_EVENT_HANDLER(BASEFORM_78)//设置分钟垂直列表 */
/* .ontouch = set_min_ontouch, */
/* }; */
/* REGISTER_UI_EVENT_HANDLER(BASEFORM_91)//确定按钮键 */
/* .ontouch = add_ontouch, */
/* }; */
/* REGISTER_UI_EVENT_HANDLER(BASEFORM_4)//时间控件 */
/* .ontouch = add_ontouch, */
/* }; */

/*page4 时钟表盘触摸测试*/

void ui_page_list_init()
{
    ui_page_init();
    ui_page_add(PAGE_0);
    ui_page_add(PAGE_1);
    ui_page_add(PAGE_2);
    ui_page_add(PAGE_3);
    ui_page_add(PAGE_4);

    ui_page_list_all();
}

#if 0
void ui_send_event(u16 event, u32 val)
{
    struct sys_event e;
    e.type = SYS_KEY_EVENT;
    e.u.key.event = event;
    e.u.key.value = val;
    sys_event_notify(&e);
}
#endif
/*ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_16);*/


static void touch_test_init(void *priv)
{
    user_ui_lcd_init();
    ui_show_main(PAGE_0);
    /*os_time_dly(300);*/
    /*ui_hide_main(PAGE_0);*/
    /*ui_show_main(PAGE_1);*/
    /*os_time_dly(300);*/
    /*ui_hide_main(PAGE_1);*/
    /*ui_show_main(PAGE_2);*/
    /*os_time_dly(300);*/
    /*ui_hide_main(PAGE_2);*/
    /*ui_show_main(PAGE_3);*/
    while (1) {
        os_time_dly(300);
    }
}

static int touch_test_task_init(void)
{
    puts("touch_test_task_init \n\n");
    return thread_fork("touch_test_init", 11, 1024, 0, 0, touch_test_init, NULL);
}
late_initcall(touch_test_task_init);

#endif //UI_TEXT_TEST



