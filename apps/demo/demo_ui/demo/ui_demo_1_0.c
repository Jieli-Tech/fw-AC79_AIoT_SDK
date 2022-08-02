/********************************测试例程说明************************************
 *  功能说明：
 *      演示一个UI控制界面 该UI界面包含了SD卡图标刷新显示 电量显示 ui高亮切换
 *      二级界面显示 开机图片显示 选择条高亮切换 以及基本控件的使用方法
 *  使用说明：
 *      显示屏使用LCD ST7789S,规格320*240  ui使用的横屏
 *      屏幕驱动接口使用 硬件PAP来进行并行发数据
 *      UI工程需要使用 ui_demo_1工程
 *      选择 <step2-打开UI资源生成工具> 点击资源生成文件
 *      系统会将字库文件以及资源文件自动导入
 *      本工程 79xx中的tool中的ui_res文件夹中
 * ******************************************************************************/
#include "ui/ui.h"
#include "ui_api.h"
#include "system/timer.h"
#include "server/server_core.h"
#include "os/os_api.h"
#include "asm/gpio.h"
#include "asm/port_waked_up.h"
#include "system/includes.h"
#include "server/audio_server.h"
#include "server/video_dec_server.h"
#include "app_config.h"
#include "font/font_textout.h"
#include "ui/includes.h"
#include "font/font_all.h"
#include "font/language_list.h"
#include "ename.h"
#include "asm/rtc.h"
#include "asm/p33.h"

#ifdef  UI_DEMO_1_0

#define     DELAY_TIME  50

#define     UI_INTERFACE_DEMO      //主界面演示
#define     UI_BATTERY_TEST        //电池电量控件显示
#define     UI_SD_TEST             //SD插入显示
#define     UI_PAGE_CHANGE_TEST    //切换页显示
#define     UI_TIMER_TEST          //时间控件  //使用RTC需要在app_config.h中 开启RTC有关的宏
#define     UI_TEXT_TEST           //文本控件测试
#define     UI_ANI_TEST            //动画效果
#define     UI_CHANGE_ATTRIBUTE    //切换控件属性测试
#define     UI_SLIDER_TEST         //进度条控件测试

#if 1
#define log_info(x, ...)    printf("\r\n>>>>>>[UI]" x " ", ## __VA_ARGS__)
#else
#define log_info(...)
#endif

#define __TIME1 BASEFORM_51 //年
#define __TIME2 BASEFORM_52 //月
#define __TIME3 BASEFORM_53 //日
#define __TIME4 BASEFORM_54 //时
#define __TIME5 BASEFORM_55 //分
#define __TIME6 BASEFORM_56 //秒

//初始化时间结构体
static struct sys_time test_rtc_time = {
    .sec = 44,
    .min = 23,
    .hour = 14,
    .day = 21,
    .month = 4,
    .year = 2021,
};

static u32 time_1 = 0;
static u32 time_2 = 0;
static u32 time_3 = 0;
static u32 time_4 = 0;

extern void post_msg_play_flash_mp3(char *file_name, u8 dec_volume);

static void SD_car_image_test();

#define    LCD_SIZE    (240*320)

static struct lcd_interface *lcd_hdl = NULL;
static struct lcd_info lcd_info = {0};


/* =====================================主界面=====================================  */
#ifdef UI_INTERFACE_DEMO //垂直列表测试界面0
static u8 page0_x;

static char page0_highlight_indax = 0;

static int v_grid_onkey(void *_grid, struct element_key_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)_grid;
    static u8 flag = 0;
    struct element *elm = NULL;

    switch (e->value) {
    case KEY_OK:
        page0_x = ui_grid_cur_item(grid);
        log_info(">>>KEY_OK page0_x = %d.\n", page0_x);
        break;

    case KEY_UP:
        page0_x = ui_grid_cur_item(grid);
        log_info(">>>KEY_UP page0_x = %d.\n", page0_x);
        break;

    case KEY_DOWN:
        page0_x = ui_grid_cur_item(grid);
        log_info(">>>KEY_DOWN page0_x = %d.\n", page0_x);
        break;

    default:
        return false;
        break;
    }
    return false;
}
static int v_grid_onchange(void *_grid, enum element_change_event e, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)_grid;
    int err;

    switch (e) {
    case ON_CHANGE_INIT:
        (grid)->hi_index = page0_highlight_indax;
        break;

    case ON_CHANGE_RELEASE:
        page0_highlight_indax = 0;
        break;

    default:
        return FALSE;
    }
    return FALSE;
}

REGISTER_UI_EVENT_HANDLER(BASEFORM_32)
.onkey = v_grid_onkey,
 .onchange = v_grid_onchange,
};
static void v_grid_test(void)
{
    ui_show_main(PAGE_0);
}

/* =====================================电量电量显示测试=====================================  */
#ifdef UI_BATTERY_TEST

static void battery_change_handler(void *priv)
{
    static u8 percent = 0;
    putchar('B');
    ui_battery_level_change(percent, 0);
    percent += 20;

    if (percent >= 120) {
        percent = 0;
    }
}

static int battery_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_battery *battery = (struct ui_battery *)ctr;
    static u32 timer = 0;

    switch (e) {
    case ON_CHANGE_INIT:
        if (!time_1) {
            time_1 = sys_timer_add(NULL, battery_change_handler, 1000); //开启进入动态电池电量显示测试
        }

        break;

    case ON_CHANGE_RELEASE:
        if (time_1) {
            sys_timer_del(time_1);
            time_1 = 0;
        }

        break;

    default:
        return false;
    }

    return false;
}

REGISTER_UI_EVENT_HANDLER(BASEFORM_33)
.onchange = battery_onchange,
};
#endif


/* =====================================时间控件显示测试=====================================  */
#ifdef UI_TIMER_TEST

static void timer_change_handler(void *priv)
{
    void *dev = NULL;
    static u32 sec = 0;
    struct utime time_r;


    dev = dev_open("rtc", NULL);
    /* 获取时间信息 */
    dev_ioctl(dev, IOCTL_GET_SYS_TIME, (u32)&test_rtc_time);


    time_r.year = test_rtc_time.year;
    time_r.month = test_rtc_time.month;
    time_r.day = test_rtc_time.day;
    time_r.hour = test_rtc_time.hour;
    time_r.min = test_rtc_time.min;
    time_r.sec = test_rtc_time.sec;

    ui_time_update_by_id(BASEFORM_57, &time_r);
    putchar('C');
    /* 关闭RTC设备 */
    dev_close(dev);
}


static int timer_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_time *time = (struct ui_time *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:

        time_2 = sys_timer_add(NULL, timer_change_handler, 1000);
        log_info(">>>>>>>>>>time_2 = %d", time_2);
        break;

    case ON_CHANGE_HIDE:
        time->year = 2021;
        time->month = 4;
        time->day = 1;
        time->hour = 14;
        time->min = 50;
        time->sec = 10;
        break;

    case ON_CHANGE_SHOW_PROBE:
        break;

    case ON_CHANGE_SHOW_POST:
        break;

    default:
        return false;
    }

    return false;
}


REGISTER_UI_EVENT_HANDLER(BASEFORM_57)
.onchange = timer_onchange,
};

#endif

/* ================================SD卡插入图片切换=====================================  */
#ifdef UI_SD_TEST

static int image_mub = 0;
static void SD_car_image_change(void *priv)
{
    image_mub = !image_mub;
    ui_pic_show_image_by_id(BASEFORM_13, image_mub);
    image_mub = image_mub % 2; //开启进入动态SD卡图标切换测试
}

static void SD_car_image_test()
{
    time_3 = sys_timer_add(NULL, SD_car_image_change, 1000); //开启进入动态SD卡图标切换测试
}
#endif

/* =====================================二级菜单测试===================================  */
#ifdef UI_PAGE_CHANGE_TEST

struct ui_grid *gridd;
static u8 page1_x;//保存二级界面选中的位置行号
static int doub_grid_onkey(void *_grid, struct element_key_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)_grid;
    static u8 flag = 0;
    struct element *elm = NULL;

    log_info(">>>>>>>>>>>>>doub_display_run ");
    switch (e->value) {
    case KEY_OK:
        page1_x = ui_grid_cur_item(grid);
        log_info(">>>KEY_OK page1_x = %d.\n", page1_x);
        break;

    case KEY_UP:
        page1_x = ui_grid_cur_item(grid);
        log_info(">>>KEY_UP page1_x = %d.\n", page1_x);
        break;

    case KEY_DOWN:
        page1_x = ui_grid_cur_item(grid);
        log_info(">>>KEY_DOWN page1_x = %d.\n", page1_x);
        break;

    default:
        return false;
        break;
    }

    return false;
}

REGISTER_UI_EVENT_HANDLER(BASEFORM_15)
.onkey = doub_grid_onkey,
};

#endif

/**********************页面控制操作************************************************************/
static void picture_quality(char page)  //选中图片质量 弹出二级界面
{
    ui_hide_curr_main(); //关闭上一个画面
    if (page == 0) {
        ui_show_main(PAGE_0);
    }
    if (page == 1) {
        ui_show_main(PAGE_1);
    }
}
/**********************日期时间************************************************************/
void time_updata(struct utime *time)
{
    struct unumber timer;

    timer.type = TYPE_NUM;
    timer.numbs = 2;
    timer.number[0] = time->year;
    ui_number_update_by_id(__TIME1, &timer);

    timer.number[0] = time->month;
    ui_number_update_by_id(__TIME2, &timer);

    timer.number[0] = time->day;
    ui_number_update_by_id(__TIME3, &timer);

    timer.number[0] = time->hour;
    ui_number_update_by_id(__TIME4, &timer);

    timer.number[0] = time->min;
    ui_number_update_by_id(__TIME5, &timer);

    timer.number[0] = time->sec;
    ui_number_update_by_id(__TIME6, &timer);
}

static void Date_time_show(char page)
{

    struct utime time;

    time.year = 2021;
    time.month = 11;
    time.day = 14;
    time.hour = 21;
    time.min = 12;
    time.sec = 01;
    ui_hide_curr_main(); //关闭上一个画面
    if (page == 0) {
        ui_show_main(PAGE_0);
    }
    if (page == 1) {
        ui_show_main(PAGE_3);
        time_updata(&time);
    }
}

/* =====================================文本控件显示测试=====================================  */
#ifdef UI_TEXT_TEST
/*************中文测试***************/
static void utf8_text_display_handler(void *priv)
{
    static int num = 0;
    static char str1[] = "起来走走";
    static char str2[] = "多多饮水";
    int ret = 0;

    if (num++ % 2) {
        ret = ui_text_set_textu_by_id(BASEFORM_66, str1, strlen(str1), FONT_DEFAULT | FONT_SHOW_MULTI_LINE);
    } else {
        ret = ui_text_set_textu_by_id(BASEFORM_66, str2, strlen(str2), FONT_DEFAULT);
    }
}

static int utf8_text_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_text *text = (struct ui_text *)ctr;
    static u32 time_id = 0;
    int err;

    switch (e) {
    case ON_CHANGE_INIT:
        time_id = sys_timer_add(NULL, utf8_text_display_handler, 300);
        break;

    case ON_CHANGE_RELEASE:
        sys_timer_del(time_id);
        break;


    default:
        return FALSE;
    }
    return FALSE;
}

REGISTER_UI_EVENT_HANDLER(BASEFORM_66)
.onchange = utf8_text_onchange,
};

/*************数字测试***************/
static void number_display_handler(void *priv)
{
    struct unumber timer;
    static u16 show_nub = 0;

    show_nub++;

    timer.numbs = 2;
    timer.type = TYPE_NUM;
    timer.number[0] = show_nub;
    ui_number_update_by_id(BASEFORM_67, &timer);
}

static int number_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_text *text = (struct ui_text *)ctr;
    static u32 time_id = 0;
    int err;

    switch (e) {
    case ON_CHANGE_INIT:
        time_id = sys_timer_add(NULL, number_display_handler, 300);
        break;

    case ON_CHANGE_RELEASE:
        sys_timer_del(time_id);
        break;


    default:
        return FALSE;
    }
    return FALSE;
}

REGISTER_UI_EVENT_HANDLER(BASEFORM_67)
.onchange = number_onchange,
};

/***********多行测试***************/
static const char multiline_str[] = "/多行文本显示/多行文本显示/多行文本显示/多行文本显示/多行文本显示/多行文本显示/多行文本显示/多行文本显示/多行文本显示/";

static int multiline_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_text *text = (struct ui_text *)ctr;
    int err;

    switch (e) {
    case ON_CHANGE_INIT:
        ui_text_set_text_attrs(text, multiline_str, strlen(multiline_str), FONT_ENCODE_UTF8, 0, FONT_DEFAULT | FONT_SHOW_MULTI_LINE);
        break;


    default:
        return FALSE;
    }
    return FALSE;
}

REGISTER_UI_EVENT_HANDLER(BASEFORM_68)
.onchange = multiline_onchange,
};

/***********多行文本滚动显示效果***************/

static const char multi_str[] = "/多行文本滚动显示/多行文本滚动显示/多行文本滚动显示/多行文本滚动显示/多行文本滚动显示/多行文本滚动显示/多行文本滚动显示/多行文本滚动显示/多行文本滚动显示/多行文本滚动显示/多行文本滚动显示/多行文本滚动显示/多行文本滚动显示/多行文本滚动显示/多行文本滚动显示/多行文本滚动显示/多行文本滚动显示/多行文本滚动显示/多行文本滚动显示/多行文本滚动显示/多行文本滚动显示/多行文本滚动显示/多行文本滚动显示/多行文本滚动显示/多行文本滚动显示/多行文本滚动显示/";

static void multi_text_display_handler(void *priv)
{
    struct ui_text *text = (struct ui_text *)priv;
    /*static u16 x=1;*/

    text->attrs.y_offset += 1;
    ui_core_redraw(text->elm.parent);
}

static int multi_text_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_text *text = (struct ui_text *)ctr;
    static u32 time_id = 0;
    int err;

    switch (e) {
    case ON_CHANGE_INIT:
        ui_text_set_text_attrs(text, multi_str, strlen(multi_str), FONT_ENCODE_UTF8, 0, FONT_DEFAULT | FONT_SHOW_MULTI_LINE);
        time_id = sys_timer_add(text, multi_text_display_handler, 150);
        break;

    case ON_CHANGE_RELEASE:
        sys_timer_del(time_id);
        break;

    default:
        return FALSE;
    }
    return FALSE;
}

/****触摸使用****/
/* static int multi_text_ontouch(void *ctr, struct element_touch_event *e) */
/* { */
/* struct ui_text *text = (struct ui_text *)ctr; */
/* static struct position mem_pos = {0}; */
/* int x_offset = 0, y_offset = 0; */

/* switch (e->event) { */
/* case ELM_EVENT_TOUCH_DOWN: */
/* log_info("ELM_EVENT_TOUCH_DOWN.\n"); */
/* memcpy(&mem_pos, &e->pos, sizeof(struct position)); */
/* return true; */

/* case ELM_EVENT_TOUCH_MOVE: */
/* log_info("ELM_EVENT_TOUCH_MOVE.\n"); */
/* x_offset += e->pos.x - mem_pos.x; */
/* y_offset += e->pos.y - mem_pos.y; */
/* printf("x_offset = %d, y_offset = %d", x_offset, y_offset); */
/* memcpy(&mem_pos, &e->pos, sizeof(struct position)); */
/* text->attrs.y_offset += y_offset; */
/* ui_core_redraw(text->elm.parent); */
/* break; */

/* default : */
/* break; */
/* } */

/* return false; */
/* } */

REGISTER_UI_EVENT_HANDLER(BASEFORM_69)
.onchange = multi_text_onchange,
 /*.ontouch  = multi_text_ontouch,*/
};
/***********ASCII测试***************/
static const char ascii_str[] = "ABCDEFGHIJK";

static int ascii_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_text *text = (struct ui_text *)ctr;
    int err;

    switch (e) {
    case ON_CHANGE_INIT:
        /*ui_text_set_text_attrs(text,ascii_str,strlen(ascii_str),FONT_ENCODE_UTF8,0,FONT_DEFAULT);*/
        ui_text_set_text_attrs(text, ascii_str, strlen(ascii_str), FONT_ENCODE_ANSI, 0, FONT_DEFAULT);
        break;

    default:
        return FALSE;
    }
    return FALSE;
}

REGISTER_UI_EVENT_HANDLER(BASEFORM_70)
.onchange = ascii_onchange,
};


#endif

/* =====================================动画效果测试=====================================  */
#ifdef UI_ANI_TEST
static int idx = 0;
static void animation_timer_handler(void *priv)
{
    /* static int idx = 0; */
    int id = 0, num = 0;

    id = *((int *)priv);
    num = *(((int *)priv) + 1);

    log_info("idx =%d.\n", idx);
    ui_pic_show_image_by_id(id, idx);

    idx++;
    if (idx >= num) {
        idx = 0;
    }
}


//功能：开始播放动画
//说明：使用UI工具在图片列表中按顺序插入动画帧
//参数：
//      int page_id   : 页面ID
//      int ani_id    : 图片控件ID
//      int frame_num : 帧数目
//      int fram_rate : 帧率（帧/秒）
static int start_animation_show(int page_id, int ani_id, int frame_num, int fram_rate)
{
    static int params[2] = {0};
    int ret = -1;

    params[0] = ani_id;
    params[1] = frame_num;

    ret = ui_show_main(page_id);
    if (ret == 0) {
        sys_timer_add(params, animation_timer_handler, 1000 / fram_rate);
    }

    return ret;
}


static int stop_animation_show(int page_id, int timer_id)
{
    if (page_id && timer_id) {
        sys_timer_del(timer_id);
        ui_hide_main(page_id);
        idx = 0; //序号清0
        return 0;
    }

    return -1;
}


static void animation_test(void)
{
    static int timer_id = -1;
    timer_id = start_animation_show(PAGE_5, BASEFORM_75, 11, 14); //320*240 最快14fps
}

/* =====================================滑块控件测试=====================================  */
#ifdef UI_SLIDER_TEST
static void slider_move_timer_handler(void *priv)
{
    static int per = 0;

    ui_slider_set_persent_by_id(NEWLAYOUT_21, per);

    per += 10;
    if (per >= 120) {
        per = 0;
    }
}

static void slider_test(void)
{
    ui_show_main(PAGE_6);
    sys_timer_add(NULL, slider_move_timer_handler, 300);
}
#endif
/* =====================================切换控件属性测试=====================================  */
#ifdef UI_CHANGE_ATTRIBUTE

//功能：高亮控件
//说明：需在UI工具中复制添加新的控件属性
//参数：
//      u32 id     : 控件ID
//      int status : 高亮状态
static void form_highlight(u32 id, int status)
{
    struct element *elm = ui_core_get_element_by_id(id);
    /* log_info("elm = %x.\n", elm); */
    int ret1 = ui_core_highlight_element(elm, status);
    /* log_info("ret1 = %d.\n", ret1); */
    int ret2 = ui_core_redraw(elm->parent);
    /* log_info("ret2 = %d.\n", ret2); */
}


static void form_highlight_timer_handler(void *priv)
{
    static int st = 1;
    /*form_highlight(BASEFORM_66, st);*/ //该列子没有使用到
    st ^= 1;
}


static void form_highlight_test(void)
{
    /*ui_show_main(PAGE_11);*/
    sys_timer_add(NULL, form_highlight_timer_handler, 500);
}
#endif

#endif


static void ui_server_init(void)
{
    extern const struct ui_devices_cfg ui_cfg_data;
    int lcd_ui_init(void *arg);
    lcd_ui_init(&ui_cfg_data);
}
#endif

static void ui_demo_test(void *priv)
{
    static u8 language_indx = 0;

    log_info(">>>>>>>>>>>>>>>>>>ui_demo_test start");

    ui_server_init();//初始化ui服务和lcd

    /******开机图片************/
    ui_show_main(PAGE_2); //开机图片
    os_time_dly(100);
    ui_hide_main(PAGE_2);

    /******文本测试************/
    ui_show_main(PAGE_4); //开机图片
    os_time_dly(1000);
    ui_hide_main(PAGE_4);

    /******滑块控件可以用来做进度条***********/
    slider_test();
    os_time_dly(1000);
    ui_hide_main(PAGE_6);

    /******主界面开始***********/
#ifdef UI_INTERFACE_DEMO
    v_grid_test();//主界面开始
#endif
#ifdef UI_SD_TEST
    SD_car_image_test();//sd卡图标变换
#endif

    /******演示模拟按键按下*****ui_key_msg_post*该函数是控制UI选择框的位置的************/
    /***************KEY_DOWN**KEY_UP***这里选择KEY_DOWN演示*****************************/
    os_time_dly(DELAY_TIME);
    ui_key_msg_post(KEY_DOWN); //这里的按键下是发命令给底层UI控制控件向下 不是按键的方向
    os_time_dly(DELAY_TIME);
    ui_key_msg_post(KEY_DOWN); //这里的按键下是发命令给底层UI控制控件向下 不是按键的方向
    os_time_dly(DELAY_TIME);
    ui_key_msg_post(KEY_DOWN); //这里的按键下是发命令给底层UI控制控件向下 不是按键的方向
    os_time_dly(DELAY_TIME);
    ui_key_msg_post(KEY_DOWN); //这里的按键下是发命令给底层UI控制控件向下 不是按键的方向
    os_time_dly(DELAY_TIME);
    ui_key_msg_post(KEY_DOWN); //这里的按键下是发命令给底层UI控制控件向下 不是按键的方向
    os_time_dly(DELAY_TIME);
    ui_key_msg_post(KEY_DOWN); //这里的按键下是发命令给底层UI控制控件向下 不是按键的方向
    os_time_dly(DELAY_TIME);
    /******当选中图片质量选择框***进入图片质量二级界面***********************************/
    picture_quality(1);//进入二级界面
    os_time_dly(DELAY_TIME);
    ui_key_msg_post(KEY_UP);
    os_time_dly(DELAY_TIME);
    ui_key_msg_post(KEY_UP);
    os_time_dly(DELAY_TIME);
    picture_quality(0);//退出二级界面 返回主界面
    /******接着选择语言设置****进行语言的切换***********************************/
    os_time_dly(DELAY_TIME);
    ui_key_msg_post(KEY_DOWN);
    os_time_dly(DELAY_TIME);
    language_indx = 1;//选择繁体
    ui_text_show_index_by_id(BASEFORM_1, language_indx);//切换语言文字
    os_time_dly(DELAY_TIME);
    language_indx = 0;//选择中文
    ui_text_show_index_by_id(BASEFORM_1, language_indx);//切换语言文字
    os_time_dly(DELAY_TIME);
    language_indx = 1;//选择繁体
    ui_text_show_index_by_id(BASEFORM_1, language_indx);//切换语言文字
    ui_language_set(Chinese_Traditional);//将语言设置为繁体
    os_time_dly(DELAY_TIME);
    /******接着选择时间设置***************************************/
    ui_key_msg_post(KEY_DOWN);
    os_time_dly(DELAY_TIME);
    Date_time_show(1);//切换到时间设置二级界面
    os_time_dly(DELAY_TIME);
    ui_key_msg_post(KEY_DOWN); //注意这里的按键消息是控制时间设置界面控制框的位置的
    os_time_dly(DELAY_TIME);
    ui_key_msg_post(KEY_DOWN);
    os_time_dly(DELAY_TIME);
    ui_key_msg_post(KEY_DOWN);
    os_time_dly(DELAY_TIME);
    ui_key_msg_post(KEY_UP);
    os_time_dly(DELAY_TIME);
    page0_highlight_indax = 2;//注意一下这个参数 直接影响到切页后 高亮行的位置
    Date_time_show(0);//切换主界面
    /*******继续*在主页面向下*************************************/
    os_time_dly(DELAY_TIME);
    ui_key_msg_post(KEY_DOWN);
    os_time_dly(DELAY_TIME);
    ui_key_msg_post(KEY_DOWN);
    os_time_dly(DELAY_TIME);
    ui_key_msg_post(KEY_DOWN);
    sys_timer_del(time_1);
    sys_timer_del(time_2);
    sys_timer_del(time_3);
    os_time_dly(DELAY_TIME);
    /*******动画测试**********/
    ui_hide_curr_main(); //关闭上一个画面
    animation_test();
    while (1) {
        os_time_dly(3000);
    }
}

void ui_test_uninit(void)//退出ui需要关掉于ui有关的 //例如启用了定时器需要先把有关定时器先关掉
{
    int lcd_ui_uninit(void);
    lcd_ui_uninit();
}

static int ui_complete_demo_task_init(void)   //主要是create wifi 线程的
{
    puts("ui_complete_demo_task_init \n\n");
    return thread_fork("ui_demo_test", 10, 1024, 0, 0, ui_demo_test, NULL);
}

late_initcall(ui_complete_demo_task_init);

#endif
