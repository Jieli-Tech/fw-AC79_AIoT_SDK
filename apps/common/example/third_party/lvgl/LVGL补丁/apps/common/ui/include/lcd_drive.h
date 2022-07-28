#ifndef __ALL_LCD_DRIVER_H__
#define __ALL_LCD_DRIVER_H__


#include "asm/spi.h"
#include "typedef.h"
#include "asm/port_waked_up.h"
#include "app_config.h"
/* LCD 调试等级，
 * 0只打印错误，
 * 1打印错误和警告，
 * 2全部内容都调试内容打印 */
#define LCD_DEBUG_ENABLE    0

#if (LCD_DEBUG_ENABLE == 0)
#define lcd_d(...)
#define lcd_w(...)
#define lcd_e(fmt, ...)	printf("[LCD ERROR]: "fmt, ##__VA_ARGS__)
#elif (LCD_DEBUG_ENABLE == 1)
#define lcd_d(...)
#define lcd_w(fmt, ...)	printf("[LCD WARNING]: "fmt, ##__VA_ARGS__)
#define lcd_e(fmt, ...)	printf("[LCD ERROR]: "fmt, ##__VA_ARGS__)
#else
#define lcd_d(fmt, ...)	printf("[LCD DEBUG]: "fmt, ##__VA_ARGS__)
#define lcd_w(fmt, ...)	printf("[LCD WARNING]: "fmt, ##__VA_ARGS__)
#define lcd_e(fmt, ...)	printf("[LCD ERROR]: "fmt, ##__VA_ARGS__)
#endif

// 两毫秒延时
// extern void delay_2ms(int cnt);
// #define delay2ms(t) delay_2ms(t)


enum LCD_COLOR {
    LCD_COLOR_RGB565,
    LCD_COLOR_RGB666,
    LCD_COLOR_RGB888,
    LCD_COLOR_MONO,
};

enum LCD_BL {
#if TCFG_LCD_ILI9481_ENABLE
    BL_ON,
    BL_OFF,
#else
    BL_OFF,
    BL_ON,
#endif
};

//LCD接口类型
// enum LCD_IF {
enum LCD_INTERFACE {
    LCD_SPI,
    LCD_EMI,
    LCD_PAP,
    LCD_IMD,
};

struct lcd_info {
    u16 width;
    u16 height;
    u8 color_format;
    u8 col_align;
    u8 row_align;
};

//LCD板级数据
struct ui_lcd_platform_data {
    u32 rst_pin;
    u32 cs_pin;
    u32 rs_pin;
    u32 bl_pin;
    u32 te_pin;
    u32 touch_int_pin;
    u32 touch_reset_pin;
    char *spi_id;
    enum LCD_INTERFACE lcd_if;
};

struct lcd_device {
    char *name;			// 名称
    u16 lcd_width;
    u16 lcd_height;
    u8 color_format;
    u8 column_addr_align;
    u8 row_addr_align;
    void *lcd_priv;//LCD私有协议，比如RGB屏相关配置
    int (*LCD_Init)(void);
    void (*SetDrawArea)(u16, u16, u16, u16);
    void (*LCD_Draw)(u8 *map, u32 size);
    void (*LCD_Draw_1)(u8 *map, u32 size);
    void (*LCD_DrawToDev)(u8 *map, u32 size);
    void (*LCD_Lvgl_Full)(u16 xs, u16 xe, u16 ys, u16 ye, char *img);
    void (*LCD_ClearScreen)(u32 color);
    void (*Reset)(void);
    void (*BackLightCtrl)(u8);
};

struct lcd_interface {
    int (*init)(void *);
    void (*get_screen_info)(struct lcd_info *info);
    void (*buffer_malloc)(u8 **buf, u32 *size);
    void (*buffer_free)(u8 *buf);
    void (*draw)(u8 *buf, u32 len, u8 wait);
    void (*draw_1)(u8 *buf, u32 len, u8 wait);
    void (*set_draw_area)(u16 xs, u16 xe, u16 ys, u16 ye);
    void (*draw_lvgl)(u16 xs, u16 xe, u16 ys, u16 ye, char *img);
    void (*clear_screen)(u32 color);
    int (*backlight_ctrl)(u8 on);
};

#define REGISTER_LCD_DEV(lcd_dev) \
static const struct lcd_device lcd_dev SEC_USED(.lcd)

extern struct lcd_interface lcd_interface_begin[];
extern struct lcd_interface lcd_interface_end[];
extern int lcd_device_begin[];
extern int lcd_device_end[];

#define list_for_each_lcd_driver(lcd_dev) \
	for (lcd_dev = (struct lcd_device*)lcd_device_begin; lcd_dev < (struct lcd_device*)lcd_device_end; lcd_dev++)

#define REGISTER_LCD_INTERFACE(lcd) \
    static const struct lcd_interface lcd sec(.lcd_if_info) __attribute__((used))

struct lcd_interface *lcd_get_hdl();


void lcd_rst_pinstate(u8 state);
void lcd_cs_pinstate(u8 state);
void lcd_rs_pinstate(u8 state);
void lcd_bl_pinstate(u8 state);
void touch_int_pinstate(u8 state);
void touch_reset_pinstate(u8 state);
int lcd_send_byte(u8 data);
int lcd_send_map(u8 *map, u32 len);
u8 *lcd_req_buf(u32 *size);
void lcd_req_buf_update(u8 *buf, u32 *size);
int lcd_read_data(u8 *buf, u32 len);
void init_TE(void (*cb)(u8 *buf, u32 buf_size));
void user_lcd_init(void); //只初始化屏
void user_ui_lcd_init(void); //调用该函数完成ui和LCD初始化
void ui_show_frame(u8 *buf, u32 len);//该接口主要用于显示UI数据  RGB数据接口
void lcd_show_frame(u8 *buf, u32 len);//数据为YUV数据
void play_gif_to_lcd(char *gif_path, char play_speed);//播放GIF动图 seepd = 1 = 10ms
void get_te_fps(char *fps); //获取底层发送显示帧数
void lcd_set_draw_area(u16 xs, u16 xe, u16 ys, u16 ye);//



#endif
