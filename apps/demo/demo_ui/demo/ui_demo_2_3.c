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

#define ui_text_for_id(id) \
	({ \
		sjpeg_cbtruct element *elm = ui_core_get_element_by_id(id); \
	 	elm ? container_of(elm, struct ui_text, elm): NULL; \
	 })

#ifdef UI_DEMO_2_3

#define STYLE_NAME  JL
REGISTER_UI_STYLE(STYLE_NAME)


static int text_ontouch(void *_ctrl, struct element_touch_event *e)
{
    struct ui_text *text = (struct ui_text *)_ctrl;

    struct element *p;


    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        printf(">>>>>>>>>>>>>>ELM_EVENT_TOUCH_DOWN");

        break;

    case ELM_EVENT_TOUCH_MOVE:
        printf(">>>>>>>>>>>>>>ELM_EVENT_TOUCH_MOVE");

        break;

    case ELM_EVENT_TOUCH_HOLD:
        printf(">>>>>>>>>>>>>>ELM_EVENT_TOUCH_HOLD");

        break;

    case ELM_EVENT_TOUCH_UP:
        printf(">>>>>>>>>>>>>>ELM_EVENT_TOUCH_HOLD");

        break;
    }
    return 0;
}

static void form_highlight(u32 id, int status)
{
    struct element *elm = ui_core_get_element_by_id(id);
    int ret1 = ui_core_highlight_element(elm, status);
    int ret2 = ui_core_redraw(elm->parent);
}

int text_child_ontouch(void *_ctrl, struct element_touch_event *e)
{
    struct element *elm = (struct element *)_ctrl;

    printf("\n [ERROR] %s -yuyu %d\n", __FUNCTION__, __LINE__);
    int type = ui_id2type(elm->id);
    printf(">>>>>>>>>>>type=%d", type);
    if (type != 5) {
        printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@>>>>>>>>>>>type=%d", type);
    }

    switch (type) {
    case CTRL_TYPE_TEXT:
        ui_core_highlight_element(elm, 0);
        ui_core_redraw(elm->parent);
        printf("\n [ERROR] %s -yuyu %d\n", __FUNCTION__, __LINE__);
        break;

    }

    return 0;
}
static int list_wifi_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)_ctrl;
    int row, col;

    switch (event) {
    case ON_CHANGE_INIT:
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

    printf("\n [ERROR] %s -yuyu %d\n", __FUNCTION__, __LINE__);
    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        printf("\n [ERROR] %s -yuyu %d\n", __FUNCTION__, __LINE__);
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
REGISTER_UI_EVENT_HANDLER(PRJ2_BASEFORM_4)//??????????????????
.onchange = list_wifi_onchange,
 .ontouch = list_wifi_ontouch,
};
static char chinese[255] = {"??? ??? ??? ??? ??? ??? ??? ??? ??? ??? ?????? ??? ??? ??? ??? ??? ??? ??? ?????? delim ??? ??? ??? ??? ??? ??? ?????? ??? ??? ??? ??? ??? strtok ??? ??? ???, ??? ??? ??? ??? ??? ??? ??? ??? ??? ??? ??? ??? ??? ??? ??? ??? ??? ??? strToken ??? ??? ??? ??? ??? ??? ??? ??? ??? ???"};
static const char english[300] = "Decomposes a string into a set of strings. Is the string to be decomposed and delim is the delimiter string. When the strtok function is called for the first time";


static void double_ui_text_project_task(void *priv)
{
    static char str1[6][5] = {"ui", "???", "???", "???", "???", "!"};
    struct element *elm;

    char *str_p;
    struct ui_touch_event e = {0};
    s32 y = 0;
    user_ui_lcd_init();
    ui_show_main(PAGE_3);
    ui_set_default_handler(text_child_ontouch, NULL, NULL);

    create_control_by_id(RES_PATH"prj2.tab", PRJ2_PAGE_2, PRJ2_BASEFORM_4, BASEFORM_45);//????????????UI???????????? ??????ID
    elm = ui_core_get_element_by_id(PRJ2_BASEFORM_4);
    /*elm->css.left = 800 * i * 2;*/  //???????????????0???0*/
    /*elm->css.top/* :16 */
    elm->id = PRJ2_BASEFORM_4;
    elm->css.width = 10000;  // 10000/10000 * lcd_w 128  = 128
    elm->css.height = 10000 * 3; // 10000/10000 * lcd_h 128  = 128
    ui_core_redraw(elm);//??????????????????

    str_p = strtok(chinese, " ");

    for (u8 j = 0; j < 5; j++) { //?????????????????????????????????
        for (u8 i = 0; i < 5; i++) { //????????????????????????
            create_control_by_id(RES_PATH"prj2.tab", PRJ2_PAGE_1, PRJ2_BASEFORM_2, PRJ2_BASEFORM_4);//????????????UI????????????
            elm = ui_core_get_element_by_id(PRJ2_BASEFORM_2);
            if (elm == NULL) {
                printf(">>>>>>>>>>>>elm = NULL");
            }
            elm->id = PRJ2_BASEFORM_2 + 5 + i + j * 6;
            elm->css.left = 670 * i * 3 ;
            elm->css.top = 670 * j ;
            elm->css.height = 650; // 10000/10000 * lcd_h 128  = 128
            elm = ui_core_get_element_by_id(PRJ2_BASEFORM_2 + 5 + i + j * 6);
            ui_core_redraw(elm->parent);//??????????????????

            /*ui_set_default_handler(text_child_ontouch, NULL, NULL);*/
            str_p =	strtok(NULL, " ");
            ui_text_set_textu_by_id(PRJ2_BASEFORM_2 + 5 + i + j * 6, str_p, strlen(str_p), FONT_DEFAULT | FONT_SHOW_MULTI_LINE);

        }
    }

    create_control_by_id(RES_PATH"prj2.tab", PRJ2_PAGE_1, PRJ2_BASEFORM_2, PRJ2_BASEFORM_4);//????????????UI????????????
    elm = ui_core_get_element_by_id(PRJ2_BASEFORM_2);
    if (elm == NULL) {
        printf(">>>>>>>>>>>>elm = NULL");
    }
    elm->id = PRJ2_BASEFORM_2 + 39;
    /*elm->css.left = 9000 ;*/
    elm->css.top = 2000 * 3 ;
    elm->css.width = 10000;  // 10000/10000 * lcd_w 128  = 128
    elm->css.height = 10000 / 3; // 10000/10000 * lcd_h 128  = 128
    elm = ui_core_get_element_by_id(PRJ2_BASEFORM_2 + 39);
    ui_core_redraw(elm->parent);//??????????????????

    ui_text_set_textu_by_id(PRJ2_BASEFORM_2 + 39, english, strlen(english), FONT_DEFAULT | FONT_SHOW_MULTI_LINE);
    struct ui_text *text = ui_text_for_id(PRJ2_BASEFORM_2 + 39);
    text->attrs.y_offset -= 45;
    ui_core_redraw(text->elm.parent);

    /*y -= 1;*/
    /*e.event = ELM_EVENT_TOUCH_DOWN;*/ //?????????????????? ELM_EVENT_TOUCH_DOWN
    /*e.x = 50;*/
    /*e.y = -10 + y;*/
    /*int ret = ui_touch_msg_post(&e);*/
    u8 i = 0;
    while (1) {
        i++ ;
        elm = ui_core_get_element_by_id(PRJ2_BASEFORM_4);
        elm->css.top = -100 * i;
        ui_core_redraw(elm->parent);//???????????????????????????
        os_time_dly(2);
    }
}


static int double_ui_text_project_task_init(void)
{
    puts("double_ui_text_project_task_init \n\n");
    return thread_fork("double_ui_text_project_task", 11, 1024, 0, 0, double_ui_text_project_task, NULL);
}
late_initcall(double_ui_text_project_task_init);

#endif //DOUBLE_PROJECT_TEST


