#include "app_config.h"
#include "ui_api.h"
#include "ui/ui.h"
#include "res/mem_var.h"
#include "typedef.h"
#include "system/includes.h"
#include "btstack/avctp_user.h"
#include "event/touch_event.h"

#ifdef CONFIG_UI_ENABLE

#include "res_config.h"

#define CURR_WINDOW_MAIN (0)

#define UI_NO_ARG (-1)
#define UI_TASK_NAME "ui"
static DEFINE_SPINLOCK(lock);

enum {
    UI_MSG_OTHER,
    UI_MSG_KEY,
    UI_MSG_TOUCH,
    UI_MSG_SHOW,
    UI_MSG_HIDE,
    UI_TIME_TOUCH_RESUME,
    UI_TIME_TOUCH_SUPEND,
    UI_MSG_EXIT,
};


struct ui_server_env {
    u8 init: 1;
    u8 key_lock : 1;
    OS_SEM start_sem;
    int (*touch_event_call)(void);
    int touch_event_interval;
    u16 timer;
    u8 shut_down_time;
};

int lcd_backlight_status();
int lcd_backlight_ctrl(u8 on);
void ui_backlight_open(void);
void ui_backlight_close(void);
int lcd_sleep_ctrl(u8 enter);
extern u16 get_backlight_time_item(void);

static struct ui_server_env __ui_display = {0};
static u8 lcd_bl_idle_flag = 0;

int key_is_ui_takeover()
{
    return __ui_display.key_lock;
}

void key_ui_takeover(u8 on)
{
    __ui_display.key_lock = !!on;
}

static int post_ui_msg(int *msg, u8 len)
{
    int err = 0;
    int count = 0;
    if (!__ui_display.init) {
        return -1;
    }
__retry:
    err =  os_taskq_post_type(UI_TASK_NAME, msg[0], len - 1, &msg[1]);

    if (cpu_in_irq() || cpu_irq_disabled()) {
        return err;
    }

    if (err) {
        if (!strcmp(os_current_task(), UI_TASK_NAME)) {
            return err;
        }
        if (count > 20) {
            return -1;
        }
        count++;
        os_time_dly(1);
        goto __retry;
    }
    return err;
}


//=================================================================================//
// @brief: 显示主页 应用于非ui线程显示主页使用
//有针对ui线程进行处理，允许用于ui线程等同ui_show使用
//=================================================================================//
int ui_show_main(int id)
{
    u32 rets;//, reti;
    __asm__ volatile("%0 = rets":"=r"(rets));
    printf("__func__ %s %x\n", __func__, rets);

    static u32 mem_id[8] = {0};
    static OS_SEM sem;// = zalloc(sizeof(OS_SEM));
    int msg[8];
    int i;


    if (id <= 0) {
        if (mem_id[7 + id]) {
            id = mem_id[7 + id];
        } else {
            printf("[ui_show_main] id %d is invalid.\n", id);
            return 0;
        }
    }
    //else {
    if (mem_id[7] != id) {
        for (i = 1; i <= 7; i++) {
            mem_id[i - 1] = mem_id[i];
        }
        mem_id[7] = id;
    }
    //}

    printf("[ui_show_main] id = 0x%x\n", id);

    /* if (!strcmp(os_current_task(), UI_TASK_NAME)) { */
    /*     ui_show(id); */
    /*     return 0; */
    /* } */
    u8 need_pend = 0;
    if (!(cpu_in_irq() || cpu_irq_disabled())) {
        if (strcmp(os_current_task(), UI_TASK_NAME)) {
            need_pend = 1;
        }
    }
    msg[0] = UI_MSG_SHOW;
    msg[1] = id;
    msg[2] = 0;

    if (need_pend) {
        os_sem_create(&sem, 0);
        msg[2] = (int)&sem;
    }

    if (!post_ui_msg(msg, 3)) {
        if (need_pend) {
            os_sem_pend(&sem, 0);
        }
    }


    if (!lcd_backlight_status()) {
        printf("backlight is close ... ui_show_main_now\n");
        lcd_bl_idle_flag = 0;
        ui_auto_shut_down_enable();
        ui_touch_timer_start();
    }


    return 0;
}


//=================================================================================//
// @brief: 关闭主页 应用于非ui线程关闭使用
//有针对ui线程进行处理，允许用于ui线程等同ui_hide使用
//=================================================================================//
int ui_hide_main(int id)
{
    u32 rets;//, reti;
    __asm__ volatile("%0 = rets":"=r"(rets));
    printf("__func__ %s %x\n", __func__, rets);


    static OS_SEM sem;// = zalloc(sizeof(OS_SEM));
    int msg[8];

    u8 need_pend = 0;;
    if (!(cpu_in_irq() || cpu_irq_disabled())) {
        if (strcmp(os_current_task(), UI_TASK_NAME)) {
            need_pend = 1;
        }
    }
    /* if (!strcmp(os_current_task(), UI_TASK_NAME)) { */
    /*     if (CURR_WINDOW_MAIN == id) { */
    /*         id = ui_get_current_window_id(); */
    /*     } */
    /*     ui_hide(id); */
    /*     return 0; */
    /* } */
    msg[0] = UI_MSG_HIDE;
    msg[1] = id;
    msg[2] = 0;

    if (need_pend) {
        os_sem_create(&sem, 0);
        msg[2] = (int)&sem;
    }

    if (!post_ui_msg(msg, 3)) {
        if (need_pend) {
            os_sem_pend(&sem, 0);
        }
    }
    return 0;
}

//=================================================================================//
// @brief: 关闭当前主页 灵活使用自动判断当前主页进行关闭
//用户可以不用关心当前打开的主页,特别适用于一个任务使用了多个主页的场景
//=================================================================================//
int ui_hide_curr_main()
{
    return ui_hide_main(CURR_WINDOW_MAIN);
}

static int page_return_tab[3];
static u8 return_index;
u8 get_return_index()
{
    return return_index;
}
//=================================================================================//
// @brief:用于处理当前页面左右滑动时是返回上一级页面还是直接滑屏切换页面
//=================================================================================//
u8 ui_return_page_pop(u8 dir)
{
    if (return_index) {
        if (dir == 1) {
            ui_hide_curr_main();
            return_index--;
            ui_show_main(page_return_tab[return_index]);
        }

        return 1; //返回
    } else {
        if (dir == 0) {
            /*app_task_put_key_msg(KEY_CHANGE_PAGE, 0);*/
        } else {
            /*app_task_put_key_msg(KEY_CHANGE_PAGE, 1);*/
        }

        return 2; //滑屏
    }
}

//=================================================================================//
// @brief:用于处理页面返回时，返回的页面存储在page_return_tab
//=================================================================================//
void ui_return_page_push(int page_id)
{
    u32 rets;//, reti;
    __asm__ volatile("%0 = rets":"=r"(rets));

    if (return_index == sizeof(page_return_tab) / 4) {
        printf("ui_return_page_push over rets =%x,id =%x\n", rets, page_id);

        for (int i = 0; i < return_index; i++) {
            printf("[%d],id =%x\n", i, page_return_tab[i]);
        }
    }

    page_return_tab[return_index] = page_id;
    return_index++;

    ASSERT(return_index <= sizeof(page_return_tab) / 4);
}
//=================================================================================//
// @brief:用于清除页面返回记录
//=================================================================================//
void ui_return_page_clear()
{
    return_index = 0;
}


//=================================================================================//
// @brief: 应用往ui发送消息，ui主页需要注册回调函数关闭当前主页
// //消息发送方法demo： UI_MSG_POST("test1:a=%4,test2:bcd=%4,test3:efgh=%4,test4:hijkl=%4", 1,2,3,4);
// 往test1、test2、test3、test4发送了字符为a、bcd、efgh、hijkl，附带变量为1、2、3、4
// 每次可以只发一个消息，也可以不带数据例如:UI_MSG_POST("test1")
//=================================================================================//
int ui_server_msg_post(const char *msg, ...)
{
    int ret = 0;
    int argv[9];
    argv[0] = UI_MSG_OTHER;
    argv[1] = (int)msg;
    va_list argptr;
    va_start(argptr, msg);
    for (int i = 2; i < 7; i++) {
        argv[i] = va_arg(argptr, int);
    }
    va_end(argptr);
    return post_ui_msg(argv, 9);
}

//=================================================================================//
// @brief: 应用往ui发送key消息，由ui控件分配
//=================================================================================//
volatile u16 touch_msg_counter = 0;
int ui_key_msg_post(int key)
{
    u8 count = 0;
    int msg[8];

    if (key >= 0x80) {
        return -1;
    }

    msg[0] = UI_MSG_KEY;
    msg[1] = key;
    /* touch_msg_counter++; */
    return post_ui_msg(msg, 2);
}


//=================================================================================//
// @brief: 应用往ui发送触摸消息，由ui控件分配
//=================================================================================//
int ui_touch_msg_post(struct touch_event *event)
{
    int msg[8];
    int i = 0;
    int err;

    ui_auto_shut_down_modify();
    if (!lcd_backlight_status()) {
        ui_backlight_open();
        return 0;
    }
    msg[0] = UI_MSG_TOUCH;
    msg[1] = (int)NULL;
    memcpy(&msg[2], event, sizeof(struct touch_event));
    if (touch_msg_counter < 16) {
        spin_lock(&lock);
        touch_msg_counter++;
        spin_unlock(&lock);
        err = post_ui_msg(msg, sizeof(struct touch_event) / 4 + 2);
        if (err) {
            printf("post_ui_msg err! %d, %d, %d\n", event->action, event->x, event->y);
            spin_lock(&lock);
            touch_msg_counter--;
            spin_unlock(&lock);
            return -1;
        }
    } else {
        printf("touch msg drop! %d, %d, %d\n", event->action, event->x, event->y);
        return -1;
    }
    return 0;
}

//=================================================================================//
// @brief: 应用往ui发送触摸消息，由ui控件分配
//=================================================================================//
int ui_touch_msg_post_withcallback(struct touch_event *event, void (*cb)(u8 finish))
{
    int msg[8];
    int i = 0;
    int err;

    msg[0] = UI_MSG_TOUCH;
    msg[1] = (int)cb;
    memcpy(&msg[2], event, sizeof(struct touch_event));
    if (touch_msg_counter < 16) {
        spin_lock(&lock);
        touch_msg_counter++;
        spin_unlock(&lock);
        err = post_ui_msg(msg, sizeof(struct touch_event) / 4 + 2);
        if (err) {
            printf("post_ui_msg err! %d, %d, %d\n", event->action, event->x, event->y);
            spin_lock(&lock);
            touch_msg_counter--;
            spin_unlock(&lock);
            return -1;
        }
    } else {
        printf("touch msg drop! %d, %d, %d\n", event->action, event->x, event->y);
        return -1;
    }
    return 0;
}



const char *str_substr_iter(const char *str, char delim, int *iter)
{
    const char *substr;
    ASSERT(str != NULL);
    substr = str + *iter;
    if (*substr == '\0') {
        return NULL;
    }
    for (str = substr; *str != '\0'; str++) {
        (*iter)++;
        if (*str == delim) {
            break;
        }
    }
    return substr;
}


static int do_msg_handler(const char *msg, va_list *pargptr, int (*handler)(const char *, u32))
{
    int ret = 0;
    int width;
    int step = 0;
    u32 arg = 0x5678;
    int m[16];
    char *t = (char *)&m[3];
    va_list argptr = *pargptr;

    if (*msg == '\0') {
        handler((const char *)' ', 0);
        return 0;
    }

    while (*msg && *msg != ',') {
        switch (step) {
        case 0:
            if (*msg == ':') {
                step = 1;
            }
            break;
        case 1:
            switch (*msg) {
            case '%':
                msg++;
                if (*msg >= '0' && *msg <= '9') {
                    if (*msg == '1') {
                        arg = va_arg(argptr, int) & 0xff;
                    } else if (*msg == '2') {
                        arg = va_arg(argptr, int) & 0xffff;
                    } else if (*msg == '4') {
                        arg = va_arg(argptr, int);
                    }
                } else if (*msg == 'p') {
                    arg = va_arg(argptr, int);
                }
                m[2] = arg;
                handler((char *)&m[3], m[2]);
                t = (char *)&m[3];
                step = 0;
                break;
            case '=':
                *t = '\0';
                break;
            case ' ':
                break;
            default:
                *t++ = *msg;
                break;
            }
            break;
        }
        msg++;
    }
    *pargptr = argptr;
    return ret;
}


int ui_message_handler(int id, const char *msg, va_list argptr)
{
    int iter = 0;
    const char *str;
    const struct uimsg_handl *handler;
    struct window *window = (struct window *)ui_core_get_element_by_id(id);
    if (!window || !window->private_data) {
        return -EINVAL;
    }
    handler = (const struct uimsg_handl *)window->private_data;
    while ((str = str_substr_iter(msg, ',', &iter)) != NULL) {
        for (; handler->msg != NULL; handler++) {
            if (!memcmp(str, handler->msg, strlen(handler->msg))) {
                do_msg_handler(str + strlen(handler->msg), &argptr, handler->handler);
                break;
            }
        }
    }

    return 0;
}




extern void sys_param_init(void);

void ui_touch_timer_delete()
{
    if (!__ui_display.init) {
        return;
    }

    int msg[8];
    msg[0] = UI_TIME_TOUCH_SUPEND;
    post_ui_msg(msg, 1);

#if UI_WATCH_RES_ENABLE
    local_irq_disable();
    if (!__ui_display.timer) {
        local_irq_enable();
        return;
    }
    local_irq_enable();
#endif

}


void ui_touch_timer_start()
{
    if (!__ui_display.init) {
        return;
    }

    int msg[8];
    msg[0] = UI_TIME_TOUCH_RESUME;
    post_ui_msg(msg, 1);

    local_irq_disable();
    if (__ui_display.timer) {
        local_irq_enable();
        return;
    }
    local_irq_enable();
}

void ui_set_touch_event(int (*touch_event)(void), int interval)
{
    __ui_display.touch_event_call = touch_event;
    __ui_display.touch_event_interval = interval;
}


void ui_auto_shut_down_enable(void);
void ui_auto_shut_down_disable(void);
static int ui_auto_shut_down_timer = 0;
static u8 open_flag = 0;
void ui_backlight_open(void)
{

    u32 rets;//, reti;
    __asm__ volatile("%0 = rets":"=r"(rets));

    lcd_bl_idle_flag = 0;
    if (!__ui_display.init) {
        return;
    }

    if (open_flag) {
        return;
    }

    printf("__func__ %s %x\n", __func__, rets);
    if (!lcd_backlight_status()) {

        open_flag = 1;


        /* UI_MSG_POST("ui_lp_cb:exit=%4", 1); */
        /* sys_key_event_enable();//关闭按键 */
        ui_auto_shut_down_enable();
        //lcd_sleep_ctrl(0);//屏幕退出低功耗
#if UI_WATCH_RES_ENABLE
        ui_show_main(0);//恢复当前ui
#endif
        ui_touch_timer_start();
    }
}

void ui_backlight_close(void)
{

    if (!__ui_display.init) {
        return;
    }


    open_flag = 0;

    if (lcd_backlight_status()) {
        /* UI_MSG_POST("ui_lp_cb:enter=%4", 0); */
        /* sys_key_event_disable();//关闭按键 */
#if UI_WATCH_RES_ENABLE
        ui_hide_curr_main();//关闭当前页面
#endif
        ui_auto_shut_down_disable();
        ui_touch_timer_delete();
    }
    lcd_bl_idle_flag = 1;
}


#if TCFG_UI_SHUT_DOWN_TIME
void ui_set_shut_down_time(u8 time)
{
    __ui_display.shut_down_time = time;
}
int ui_get_shut_down_time()
{
    return  __ui_display.shut_down_time;
}
#endif

void ui_auto_shut_down_enable(void)
{
#if TCFG_UI_SHUT_DOWN_TIME
    if (!__ui_display.init) {
        return;
    }
    if (get_call_status() != BT_CALL_HANGUP) {
        return;
    }
    if (ui_auto_shut_down_timer == 0) {
        if (__ui_display.shut_down_time == 0) {
            __ui_display.shut_down_time = 10;
        }
        if (__ui_display.shut_down_time > 20) {
            __ui_display.shut_down_time = 20;
        }
        g_printf("ui shut down time:%d", __ui_display.shut_down_time);
        ui_auto_shut_down_timer = sys_timeout_add(NULL, ui_backlight_close, __ui_display.shut_down_time * 1000);
    }
#endif
}

void ui_auto_shut_down_disable(void)
{
#if TCFG_UI_SHUT_DOWN_TIME
    if (ui_auto_shut_down_timer) {
        sys_timeout_del(ui_auto_shut_down_timer);
        ui_auto_shut_down_timer = 0;
    }
#endif
}

void ui_auto_shut_down_modify(void)
{
#if TCFG_UI_SHUT_DOWN_TIME
    if (ui_auto_shut_down_timer) {
        sys_timer_modify(ui_auto_shut_down_timer, __ui_display.shut_down_time * 1000);
    }
#endif
}

static u8 lcd_bl_idle_query(void)
{
    return lcd_bl_idle_flag;
}
/*REGISTER_LP_TARGET(lcd_backlight_lp_target) = {*/
/*.name = "lcd_backlight",*/
/*.is_idle = lcd_bl_idle_query,*/
/*};*/

__attribute__((weak)) void ui_sysinfo_init()
{

}

struct ui_msg {
    struct list_head entry;
    struct touch_event touch;
    void(*msg_hook)(u8);
};

struct ui_msg msg_handle;

static void ui_task(void *p)
{
    int msg[32];
    int ret;
    struct element_key_event e = {0};

    /*sys_param_init();*/
    /* struct ui_style style; */
    /* style.file = NULL; */
    extern void ui_sysinfo_init();
    ui_sysinfo_init();

    ui_framework_init(p);
    /* ret =  ui_set_style_file(&style); */
    /* if (ret) { */
    /* printf("ui task fail!\n"); */
    /* return; */
    /* } */
    __ui_display.init = 1;
    os_sem_post(&(__ui_display.start_sem));
    struct touch_event *touch;
    struct element_touch_event t;

    if (__ui_display.touch_event_call && __ui_display.touch_event_interval) {
        __ui_display.timer = sys_timer_add((void *)NULL, __ui_display.touch_event_call, __ui_display.touch_event_interval); //注册按键扫描定时器
    }

    INIT_LIST_HEAD(&msg_handle.entry);

    u8 touch_move_msg = 0;
    struct ui_msg ui_msg_move;
    while (1) {
        ret = os_taskq_pend(NULL, msg, ARRAY_SIZE(msg)); //500ms_reflash
        if (ret != OS_TASKQ) {
            continue;
        }

        if (msg[0] == UI_MSG_TOUCH) {
            touch = (struct touch_event *)&msg[2];
            if (touch->action == ELM_EVENT_TOUCH_MOVE) {
                touch_move_msg++;
                ui_msg_move.msg_hook = (void (*)(u8))msg[1];
                memcpy((u8 *)&ui_msg_move.touch, (u8 *)&msg[2], sizeof(struct touch_event));
            } else {
                if (touch_move_msg) {
                    struct ui_msg *ui_msg = (struct ui_msg *)malloc(sizeof(struct ui_msg));
                    ui_msg->msg_hook = ui_msg_move.msg_hook;
                    memcpy((u8 *)&ui_msg->touch, (u8 *)&ui_msg_move.touch, sizeof(struct touch_event));
                    list_add_tail(&ui_msg->entry, &msg_handle.entry);
                    touch_move_msg = 0;
                }
                struct ui_msg *ui_msg = (struct ui_msg *)malloc(sizeof(struct ui_msg));
                ui_msg->msg_hook = (void (*)(u8))msg[1];
                memcpy((u8 *)&ui_msg->touch, (u8 *)&msg[2], sizeof(struct touch_event));
                list_add_tail(&ui_msg->entry, &msg_handle.entry);
            }
            spin_lock(&lock);
            touch_msg_counter--;
            spin_unlock(&lock);
            if (touch_msg_counter) {
                continue;
            } else {
                if (touch_move_msg) {
                    struct ui_msg *ui_msg = (struct ui_msg *)malloc(sizeof(struct ui_msg));
                    ui_msg->msg_hook = ui_msg_move.msg_hook;
                    memcpy((u8 *)&ui_msg->touch, (u8 *)&ui_msg_move.touch, sizeof(struct touch_event));
                    list_add_tail(&ui_msg->entry, &msg_handle.entry);
                    touch_move_msg = 0;
                }

                while (!list_empty(&msg_handle.entry)) {
                    struct ui_msg *ui_msg = list_first_entry(&msg_handle.entry, struct ui_msg, entry);
                    touch = (struct touch_event *)&ui_msg->touch;
                    t.event = touch->action;
                    t.pos.x = touch->x;
                    t.pos.y = touch->y;

                    if (ui_msg->msg_hook) {
                        ((void(*)(u8))ui_msg->msg_hook)(0);
                    }
                    ui_event_ontouch(&t);
                    if (ui_msg->msg_hook) {
                        ((void(*)(u8))ui_msg->msg_hook)(1);
                    }
                    list_del(&ui_msg->entry);
                    free(ui_msg);
                }
                continue;
            }
        } else {//其他消息直接处理
            if (touch_move_msg) {
                struct ui_msg *ui_msg = (struct ui_msg *)malloc(sizeof(struct ui_msg));
                ui_msg->msg_hook = ui_msg_move.msg_hook;
                memcpy((u8 *)&ui_msg->touch, (u8 *)&ui_msg_move.touch, sizeof(struct touch_event));
                list_add_tail(&ui_msg->entry, &msg_handle.entry);
                touch_move_msg = 0;
            }
            while (!list_empty(&msg_handle.entry)) {
                struct ui_msg *ui_msg = list_first_entry(&msg_handle.entry, struct ui_msg, entry);
                touch = (struct touch_event *)&ui_msg->touch;
                t.event = touch->action;
                t.pos.x = touch->x;
                t.pos.y = touch->y;

                if (ui_msg->msg_hook) {
                    ((void(*)(u8))ui_msg->msg_hook)(0);
                }
                ui_event_ontouch(&t);
                if (ui_msg->msg_hook) {
                    ((void(*)(u8))ui_msg->msg_hook)(1);
                }
                list_del(&ui_msg->entry);
                free(ui_msg);
            }
        }

        switch (msg[0]) { //action
        case UI_MSG_EXIT:
            os_sem_post((OS_SEM *)msg[1]);
            os_time_dly(10000);
            break;
        case UI_MSG_OTHER:
            ui_message_handler(ui_get_current_window_id(), (const char *)msg[1], (void *)&msg[2]);
            break;
        case UI_MSG_KEY:
            e.value = msg[1];
            ui_event_onkey(&e);
            break;
        case UI_MSG_TOUCH:
            touch = (struct touch_event *)&msg[2];
            t.event = touch->action;
            t.pos.x = touch->x;
            t.pos.y = touch->y;
            /* printf("event = %d %d %d \n", t.event, t.pos.x, t.pos.y); */
            if (msg[1]) {
                ((void(*)(u8))msg[1])(0);
            }
            ui_event_ontouch(&t);
            if (msg[1]) {
                ((void(*)(u8))msg[1])(1);
            }

            break;
        case UI_MSG_SHOW:
            if (!lcd_backlight_status()) {
                lcd_sleep_ctrl(0);//退出低功耗
            }

            if ((ui_id2type(msg[1]) == CTRL_TYPE_WINDOW) && (ui_get_current_window_id() > 0)) {
                printf("ui = %x show repeat.....\n", ui_get_current_window_id());
                ui_hide(ui_get_current_window_id());
            }

            ui_show(msg[1]);
            if (msg[2]) {
                os_sem_post((OS_SEM *)msg[2]);
            }
            break;
        case UI_MSG_HIDE:
            if (CURR_WINDOW_MAIN == msg[1]) {
                ui_hide(ui_get_current_window_id());
            } else {
                ui_hide(msg[1]);
            }

            if (msg[2]) {
                os_sem_post((OS_SEM *)msg[2]);
            }
            break;
        case UI_TIME_TOUCH_RESUME:
#if (TCFG_UI_ENABLE&&(CONFIG_UI_STYLE == STYLE_JL_SOUNDBOX))
            if (!lcd_backlight_status()) {
                set_backlight_time(get_backlight_time_item());//防止在息屏状态下跳转到PC模式后不亮屏
            }
#endif
            lcd_backlight_ctrl(true);
            if (__ui_display.timer) {
                break;
            }
            if (__ui_display.touch_event_call && __ui_display.touch_event_interval) {
                __ui_display.timer = sys_timer_add((void *)NULL, __ui_display.touch_event_call, __ui_display.touch_event_interval); //注册按键扫描定时器
            }
            break;
        case UI_TIME_TOUCH_SUPEND:
            if (__ui_display.timer) {
                sys_timer_del(__ui_display.timer);
                __ui_display.timer = 0;
            }
            lcd_backlight_ctrl(false);
            break;
        default:
            break;
        }
    }
}


extern int ui_file_check_valid();
extern int ui_upgrade_file_check_valid();

int lcd_ui_init(void *arg)
{
    int err = 0;
    /*clock_add_set(DEC_UI_CLK);*/

#if  CONFIG_WATCH_CASE_ENABLE
    mem_var_init(3 * 1024, false);
#else
    mem_var_init(0, false);
#endif

    printf("open_resouece_start...\n");
    if (ui_file_check_valid()) {
        printf("ui_file_check_valid fail!!!\n");
        if (ui_upgrade_file_check_valid()) {
            printf("ui_upgrade_file_check_valid fail!!!\n");
            return -1;
        }
    }

    os_sem_create(&(__ui_display.start_sem), 0);
    err = task_create(ui_task, arg, UI_TASK_NAME);
    os_sem_pend(&(__ui_display.start_sem), 0);
    return err;
}

#endif
