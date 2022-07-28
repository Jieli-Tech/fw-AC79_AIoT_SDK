
#include "app_config.h"

#include "system/includes.h"
#include "yuv_soft_scalling.h"
#include "lcd_drive.h"
#include "lcd_te_driver.h"
#include "system/timer.h"
#include "sys_common.h"
#include "system/includes.h"
#include "ui/ui.h"
#include "lbuf.h"
#include "lcd_config.h"

#ifdef CONFIG_UI_ENABLE

/*#define turn_180   */

#define FILTER_COLOR 0x000000  //图像合成过滤纯黑色

#if 1
#define log_info(x, ...)    printf("\n[lcd_deta_driver]>>>>>>>>>>>>>>>>>>>###" x " \n", ## __VA_ARGS__)
#else
#define log_info(...)
#endif

OS_SEM compose_sem;
extern OS_SEM send_ok_sem;


static u8 data_mode = ui;
static u16 ui_xstart, ui_xend, ui_ystart, ui_yend;
static u8 aim_filter_color_l;//rgb565
static u8 aim_filter_color_h;

static u8 ui_save_buf[LCD_RGB565_DATA_SIZE]  ALIGNE(32);
static u8 camera_save_buf[LCD_RGB565_DATA_SIZE] ALIGNE(32);
static u8 show_buf[LCD_RGB565_DATA_SIZE]  ALIGNE(32);

struct lbuf_test_head {
    u32 len;
    u8 data[0];
};

static void *lbuf_ptr = NULL;
static struct lbuff_head *lbuf_handle_test = NULL;
static struct lbuff_head *lib_system_lbuf_test_init(u32 buf_size)
{
    struct lbuff_head *lbuf_handle = NULL;
    lbuf_ptr = malloc(buf_size);

    if (lbuf_ptr == NULL) {
        printf("lbuf malloc buf err");
        return NULL;
    }

    //lbuf初始化:
    lbuf_handle = lbuf_init(lbuf_ptr, buf_size, 4, sizeof(struct lbuf_test_head));

    return lbuf_handle;
}

/*===================================================================
 * 该文件主要处理图像和ui数据 以及选择数据输出源到LCD
 * 图像合成原理ui发送过来的数据保存在一帧数据缓存中并且对应好坐标位置
 * 摄像头数据数据一来就将数据和UI缓存BUF进行合成
 * 并以摄像头数据为基底将UI数据进行过滤合成
 *===================================================================*/

/*将两帧数据合成为一帧,并选择过滤颜色*/
static u8 compose_mode = 0;
static int user1_y_len = 0 ;
static int user2_y = 0 ;
static int user2_y_len = 0;
static int start_x = 0;//起始坐标
static int start_y = 0;
static int aim_compose_w = 0;//合成长宽
static int aim_compose_h = 0;

static void picture_compose(u8 *ui_in, u8 *camera_in, u16 in_LCD_W, u16 in_LCD_H)
{
    unsigned int n = 0;
    switch (compose_mode) {
    case 0://两张整图合成
        for (int j = 0; j < in_LCD_H; j++) {
            for (int i = 0; i < in_LCD_W; i++) {
                if (ui_in[n] != aim_filter_color_l && ui_in[n + 1] != aim_filter_color_h) {
                    camera_in[n] = ui_in[n];
                    camera_in[n + 1] = ui_in[n + 1];
                }
                n += 2;
            }
        }
        break;
    case 1://UI顶部条状合成
        for (int j = 0; j < user1_y_len; j++) {//Y
            for (int i = 0; i < LCD_W; i++) {//X
                if (ui_in[n] != aim_filter_color_l && ui_in[n + 1] != aim_filter_color_h) {
                    camera_in[n] = ui_in[n];
                    camera_in[n + 1] = ui_in[n + 1];
                }
                n += 2;
            }
        }
        break;
    case 2://ui顶部和底部条状合成
        for (int j = 0; j < user1_y_len; j++) {//Y
            for (int i = 0; i < LCD_W; i++) {//X
                if (ui_in[n] != aim_filter_color_l && ui_in[n + 1] != aim_filter_color_h) {
                    camera_in[n] = ui_in[n];
                    camera_in[n + 1] = ui_in[n + 1];
                }
                n += 2;
            }
        }

        n = user2_y * LCD_W * 2;
        for (int j = 0; j < user2_y_len; j++) {//Y
            for (int i = 0; i < LCD_W; i++) {//X
                if (ui_in[n] != aim_filter_color_l && ui_in[n + 1] != aim_filter_color_h) {
                    camera_in[n] = ui_in[n];
                    camera_in[n + 1] = ui_in[n + 1];
                }
                n += 2;
            }
        }
        break;

    case 3://指定坐标开始 告知 长宽 //任意位置覆盖处理
        for (int j = 0; j < aim_compose_h; j++) {//Y
            for (int i = 0; i < aim_compose_w * 2; i += 2) { //X
                n = start_x * 2 + start_y * LCD_W * 2 + i + j * LCD_W * 2;
                ui_in[n] = camera_in[i + j * aim_compose_w * 2 ];
                ui_in[n + 1] = camera_in[i + j * aim_compose_w * 2 + 1];
            }
        }
        break;
    }
}
void set_compose_mode(u8 mode, int mode1_y_len, int mode2_y, int mode2_y_len)
{
    compose_mode = mode;
    user1_y_len =  mode1_y_len;
    user2_y = mode2_y;
    user2_y_len =  mode2_y_len;
}

void set_compose_mode3(int x, int y, int w, int h)
{
    compose_mode = 3;
    start_x = x;
    start_y = y;
    aim_compose_w = w;
    aim_compose_h = h;
}

void set_lcd_show_data_mode(u8 choice_data_mode)
{
    data_mode = choice_data_mode;

    switch (choice_data_mode) {
    case camera:
        printf("[LCD_MODE == camera]");
        break;
    case ui:
        printf("[LCD_MODE == ui]");
        break;
    case ui_camera:
        printf("[LCD_MODE == ui_camera]");
        break;
    case ui_user:
        printf("[LCD_MODE == ui_user]");
        break;
    case ui_qr:
        printf("[LCD_MODE == ui_qr]");
        break;
    }
}

u8 get_lcd_show_deta_mode(void)
{
    return data_mode;
}
void get_lcd_ui_x_y(u16 xstart, u16 xend, u16 ystart, u16 yend)
{
    ui_xstart = xstart;
    ui_xend = xend ;
    ui_ystart = ystart ;
    ui_yend = yend;
}

/*将获取到的UI数据保存在一帧数据中并且是对应位置关系*/
/*ui那边发送的数据会告知开窗坐标数据buf*/
void ui_send_data_ready(u8 *data_buf, u32 data_size)
{
    unsigned int n = 0;
    for (int j = 0; j < (ui_yend - ui_ystart + 1);  j++) {
        for (int i = 0; i < (ui_xend - ui_xstart + 1); i++) {
            ui_save_buf[n + (LCD_W - ui_xend - 1) * 2 * j + ui_ystart * LCD_W * 2 + ui_xstart * 2 * (j + 1)] = data_buf[n];
            ui_save_buf[n + 1 + (LCD_W - ui_xend - 1) * 2 * j + ui_ystart * LCD_W * 2 + ui_xstart * 2 * (j + 1)] = data_buf[n + 1];
            n += 2;
        }
    }
    if (data_mode == ui) {
        os_sem_post(&compose_sem);
    }
    os_sem_pend(&send_ok_sem, 0);
}

void camera_send_data_ready(u8 *data_buf, u32 data_size)
{
    struct lbuf_test_head *wbuf = NULL;
    if (data_mode == camera || data_mode == ui_camera) {
        if (lbuf_free_space(lbuf_handle_test) < (LCD_W * LCD_H * 3 / 2)) { //查询LBUF空闲数据块是否有足够长度
            log_info("%s >>>note lbuf lbuf_free_space fail\r\n", __func__);
            return ;
        }
        wbuf = (struct lbuf_test_head *)lbuf_alloc(lbuf_handle_test, LCD_W * LCD_H * 3 / 2); //lbuf内申请一块空间
        if (wbuf != NULL) {
            memcpy(wbuf->data, data_buf, LCD_W * LCD_H * 3 / 2);
            wbuf->len = LCD_W * LCD_H * 3 / 2;
            lbuf_push(wbuf, BIT(0));//把数据块推送更新到lbuf的通道0
            os_sem_post(&compose_sem);
        } else {
            log_info("%s >>>lbuf no buf\r\n", __func__);
            return ;
        }
    }
    if (data_mode == ui_user) {
        memcpy(show_buf, data_buf, data_size);
        os_sem_post(&compose_sem);
    }
}

void qr_data_updata(u8 *data_buf, u32 data_size)
{
    memcpy(show_buf, data_buf, data_size);
    os_sem_post(&compose_sem);
}

void send_data_to_lcd(u8 *buf, u32 size)
{
    te_send_data(buf, size);
}

static void get_filter_color(void)
{
    u32 rgb888_color;
    u8 r, g, b;
    u16 rgb565_color;

    rgb888_color = FILTER_COLOR; //将过滤的颜色 RGB888  拆解为RGB565
    r = rgb888_color >> 19;
    g = rgb888_color >> 8;
    g = g >> 2;
    b = rgb888_color;
    b = b >> 3;

    rgb565_color = r;
    rgb565_color <<= 6;
    rgb565_color |= g;
    rgb565_color <<= 5;
    rgb565_color |= b;

    aim_filter_color_l = rgb565_color >> 8;
    aim_filter_color_h = rgb565_color;
}

static void picture_compose_task(void *priv)
{
    struct lbuf_test_head *rbuf = NULL;

    get_filter_color(); //开始时候计算一遍过滤颜色值

    while (1) {
        os_sem_pend(&compose_sem, 0);
        switch (data_mode) {
        case camera: //屏数据只有摄像头数据
            if (lbuf_empty(lbuf_handle_test)) {//查询LBUF内是否有数据帧
                log_info("%s >>>note lbuf lbuf_empty fail\r\n", __func__);
                break;
            }
            rbuf = (struct lbuf_test_head *)lbuf_pop(lbuf_handle_test, BIT(0));//从lbuf的通道0读取数据块
            if (rbuf == NULL) {
                log_info("%s >>>note lbuf rbuf == NULL\r\n", __func__);
                break;
            } else {
                yuv420p_quto_rgb565(rbuf->data, camera_save_buf, LCD_W, LCD_H, 1);
                lbuf_free(rbuf);
            }

#if HORIZONTAL_SCREEN
#ifdef turn_180
            RGB_Soft_90(0, show_buf, camera_save_buf, LCD_W,  LCD_H);
#else
            RGB_Soft_90(1, show_buf, camera_save_buf, LCD_W,  LCD_H);
#endif
#else
            memcpy(show_buf, camera_save_buf, LCD_RGB565_DATA_SIZE);
#endif
            send_data_to_lcd(show_buf, LCD_RGB565_DATA_SIZE);
            break;
        case ui: //屏数据只有UI数据
#if HORIZONTAL_SCREEN
#ifdef turn_180
            RGB_Soft_90(0, show_buf, ui_save_buf, LCD_W,  LCD_H);
#else
            RGB_Soft_90(1, show_buf, ui_save_buf, LCD_W,  LCD_H);
#endif
            send_data_to_lcd(show_buf, LCD_RGB565_DATA_SIZE);
#else
            send_data_to_lcd(ui_save_buf, LCD_RGB565_DATA_SIZE);
#endif
            break;
        case ui_camera://屏数据有实时更新的摄像头数据已经ui数据 二者做图像合成
            if (lbuf_empty(lbuf_handle_test)) { //查询LBUF内是否有数据帧
                goto updata;//在没有帧数据也进行刷新防止没有camera数据时ui数据不更新
            }
            rbuf = (struct lbuf_test_head *)lbuf_pop(lbuf_handle_test, BIT(0));//从lbuf的通道0读取数据块
            if (rbuf == NULL) {
                goto updata;//如果读取失败也进行刷新数据
            } else {
                yuv420p_quto_rgb565(rbuf->data, camera_save_buf, LCD_W, LCD_H, 1);
                lbuf_free(rbuf);
            }

updata:
            picture_compose(ui_save_buf, camera_save_buf, LCD_W, LCD_H);
#if HORIZONTAL_SCREEN
#ifdef turn_180
            RGB_Soft_90(0, show_buf, camera_save_buf, LCD_W,  LCD_H);
#else
            RGB_Soft_90(1, show_buf, camera_save_buf, LCD_W,  LCD_H);
#endif
#else
            memcpy(show_buf, camera_save_buf, LCD_RGB565_DATA_SIZE);
#endif
            send_data_to_lcd(show_buf, LCD_RGB565_DATA_SIZE);
            break;

        case ui_user: //屏数据为使用者进行查看照片更新UI时需要切换的模式
            yuv420p_quto_rgb565(show_buf, camera_save_buf, LCD_W, LCD_H, 1);

            picture_compose(ui_save_buf, camera_save_buf, LCD_W, LCD_H);
#if HORIZONTAL_SCREEN
#ifdef turn_180
            RGB_Soft_90(0, show_buf, camera_save_buf, LCD_W,  LCD_H);
#else
            RGB_Soft_90(1, show_buf, camera_save_buf, LCD_W,  LCD_H);
#endif
#else
#endif
            send_data_to_lcd(show_buf, LCD_RGB565_DATA_SIZE);
            break;

        case ui_qr://屏数据有实时更新的摄像头数据已经ui数据 二者做图像合成
            yuv420p_quto_rgb565(show_buf, camera_save_buf, aim_compose_w, aim_compose_h, 1);

            picture_compose(ui_save_buf, camera_save_buf, LCD_W, LCD_H);
#if HORIZONTAL_SCREEN
#ifdef turn_180
            RGB_Soft_90(0, show_buf, ui_save_buf, LCD_W,  LCD_H);
#else
            RGB_Soft_90(1, show_buf, ui_save_buf, LCD_W,  LCD_H);
#endif
#else
            memcpy(show_buf, ui_save_buf, LCD_RGB565_DATA_SIZE);
#endif
            send_data_to_lcd(show_buf, LCD_RGB565_DATA_SIZE);
            break;
        }
    }
}

static int picture_compose_task_init(void)
{
    static struct lcd_device *lcd_dev;
    os_sem_create(&compose_sem, 0);

    lbuf_handle_test = lib_system_lbuf_test_init(LCD_YUV420_DATA_SIZE * 2 + 128);

    return thread_fork("picture_compose_task", 27, 512, 32, NULL, picture_compose_task, NULL);
}
late_initcall(picture_compose_task_init);
#endif
