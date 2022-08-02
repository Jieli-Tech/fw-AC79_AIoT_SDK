


#include "system/includes.h"
#include "typedef.h"
#include "asm/pap.h"
#include "lcd_drive.h"
#include "lcd_te_driver.h"
#include "lcd_config.h"
#include "uart.h"
#include "system/includes.h"
#include "gpio.h"
#include "app_config.h"

#if TCFG_LCD_ILI9481_ENABLE

#define ROTATE_DEGREE_0      	0
#define ROTATE_DEGREE_180       1

#define WHITE         	 0xFFFF
#define BLACK         	 0x0000
#define BLUE         	 0x001F
#define BRED             0XF81F
#define GRED 			 0XFFE0
#define GBLUE			 0X07FF
#define RED           	 0xF800
#define MAGENTA       	 0xF81F
#define GREEN         	 0x07E0
#define CYAN          	 0x7FFF
#define YELLOW        	 0xFFE0
#define BROWN 			 0XBC40 //棕色
#define BRRED 			 0XF803 //棕红色
#define BRRED 			 0XFC07 //棕红色
#define GRAY  			 0X8430 //灰色

#define REGFLAG_DELAY 0x45

#define DMA_DSPLY	1 //DMA operation

static u8 ui_data_ready = 0;

/*调用lcd_TE_driver完成数据发送*/
extern void send_date_ready();
void msleep(unsigned int ms);

#define lcd_delay(x) msleep(x)

static void WriteCOM(u8 cmd)
{
    lcd_cs_pinstate(0);
    lcd_rs_pinstate(0);//cmd
    lcd_send_byte(cmd);
    lcd_cs_pinstate(1);
}

static void WriteDAT_8(u8 dat)
{
    lcd_cs_pinstate(0);
    lcd_rs_pinstate(1);//dat
    lcd_send_byte(dat);
    lcd_cs_pinstate(1);
}

static void WriteDAT_DMA(u8 *dat, u32 len)
{
    lcd_cs_pinstate(0);
    lcd_rs_pinstate(1);//dat
    lcd_send_map(dat, len);
    lcd_cs_pinstate(1);
}

static void ReadDAT_DMA(u8 *dat, u16 len)
{
    lcd_cs_pinstate(0);
    lcd_rs_pinstate(1);//dat
    lcd_read_data(dat, len);
    lcd_cs_pinstate(1);
}

static u8 ReadDAT_8(void)
{
    u8 dat = 0;
    lcd_cs_pinstate(0);
    lcd_rs_pinstate(1);//dat
    lcd_read_data(&dat, 1);
    lcd_cs_pinstate(1);
    return dat;
}

static u8 ReadDAT_16(void)
{
    u16 dat = 0;
    lcd_cs_pinstate(0);
    lcd_rs_pinstate(1);//dat
    lcd_read_data((u8 *)&dat, 2);
    lcd_cs_pinstate(1);
    return dat;
}

static u32 Read_ID(u8 index)
{
    u32 id = 0;
    WriteCOM(index);
    /* id = ReadDAT_16() & 0xff00; */
    id = ReadDAT_16();
    id |= ReadDAT_16() << 16;
    return id;
}

void ili9481_SetRange(u16 xs, u16 xe, u16 ys, u16 ye)
{
    /******UI每次发送数据都会调用开窗告诉屏幕要刷新那个区域***********/
    get_lcd_ui_x_y(xs, xe, ys, ye);
}
void ili9481_SetRange_1(u16 xs, u16 xe, u16 ys, u16 ye)
{
    WriteCOM(0x2A);
    WriteDAT_8(xs >> 8);
    WriteDAT_8(xs);
    WriteDAT_8(xe >> 8);
    WriteDAT_8(xe);
    WriteCOM(0x2B);
    WriteDAT_8(ys >> 8);
    WriteDAT_8(ys);
    WriteDAT_8(ye >> 8);
    WriteDAT_8(ye);
}

void ili9481_clear_screen(u32 color)
{
    WriteCOM(0x2c);

    u8 *buf = malloc(LCD_W * LCD_H * 2);

    if (!buf) {
        printf("no men in %s \n", __func__);
        return;
    }

    for (u32 i = 0; i < LCD_W * LCD_H; i++) {
        buf[2 * i] = (color >> 8) & 0xff;
        buf[2 * i + 1] = color & 0xff;
    }

    WriteDAT_DMA(buf, LCD_W * LCD_H * 2);
    free(buf);
}

void ili9481_Fill(u8 *img, u16 len)
{
    WriteCOM(0x2c);
    WriteDAT_DMA(img, len);
}

void ili9481_SleepInMode(void)
{
    WriteCOM(0x10); //Sleep in
    lcd_delay(120); //Delay 120ms
}

void ili9481_SleepOutMode(void)
{
    WriteCOM(0x11); //Sleep out
    lcd_delay(120);  //Delay 120ms
}

void st7789_shown_image(u8 *buff, u16 x_addr, u16 y_addr, u16 width, u16 height)
{
    ili9481_SetRange(x_addr, y_addr, width, height);
    WriteDAT_DMA(buff, width * height * 2);
}

static void ili9481_set_direction(u8 dir)
{
    WriteCOM(0x36);    //扫描方向控制

    if (dir == ROTATE_DEGREE_0) { //正向
#if HORIZONTAL_SCREEN
        WriteDAT_8(0x48);
#else
        WriteDAT_8(0x48);
#endif
    } else if (dir == ROTATE_DEGREE_180) { //翻转180
#if HORIZONTAL_SCREEN
        WriteDAT_8(0x48);
#else
        WriteDAT_8(0x48);
#endif
    }

    ili9481_SetRange(0, LCD_W - 1, 0, LCD_H - 1);
}

static void ili9481_draw(u8 *map, u32 size)//获取Ui发送出来的数据
{
    ui_send_data_ready(map, size);
}

static void ili9481_draw_1(u8 *map, u32 size)//获取camera发送出来的数据 //数据帧数以camera为基准
{
    camera_send_data_ready(map, size);
}

static void ili9481_reset(void)
{
    printf("reset \n");
    lcd_rst_pinstate(1);
    lcd_rs_pinstate(1);
    lcd_cs_pinstate(1);

    lcd_rst_pinstate(1);
    lcd_delay(60);
    lcd_rst_pinstate(0);
    lcd_delay(10);
    lcd_rst_pinstate(1);
    lcd_delay(100);
}

typedef struct {
    u8 cmd;
    u8 cnt;
    u8 dat[128];
} InitCode;

static const InitCode code1[] = {

    {0x01, 0},
    {REGFLAG_DELAY, 200},
    {0x11, 0},
    {REGFLAG_DELAY, 280},

    {0x35, 1, {0x00}},//开TE 关TE 0x34
    {0x44, 2, {0x01, 0X50}}, //有关TE时间控制
#ifdef USE_DevKitBoard_TEST_DEMO
    {0xc5, 1, {0x04}},
#else
    {0xc5, 1, {0x07}},
#endif
    {0xE4, 1, {0xA0}},
    {0xf3, 2, {0x02, 0x1a}},
    {0xd0, 3, {0x07, 0x41, 0x16}},
    {0xd1, 3, {0x00, 0x04, 0x1f}},
    {0xd2, 2, {0x01, 0x00}},
    {0xc0, 5, {0x00, 0x3b, 0x00, 0x02, 0x11}},
    /*{0xC8, 12, {0X00, 0x01, 0x47, 0x60, 0x04, 0x16, 0x03, 0x67, 0x67, 0x06, 0x0f, 0x00}},*/
    /*{0xC8,12, {0x00, 0x37, 0x25, 0x06, 0x04, 0x1e, 0x26, 0x42, 0x77, 0x44, 0x0f, 0x12}},*/
    {0xC8, 12, {0X00, 0x26, 0x21, 0x00, 0x00, 0x1f, 0x65, 0x23, 0x77, 0x00, 0x0f, 0x00}},
    /*{0xc8, 12, {0x00, 0x30, 0x36, 0x45, 0x04, 0x16, 0x37, 0x75, 0x77, 0x54, 0x0f, 0x00}},*/
    {0x3A, 1, {0x55}},
    {0x2A, 4, {0X00, 0x00, 0x01, 0x3F}},
    {0x2B, 4, {0X00, 0x00, 0x01, 0xDF}},

    {REGFLAG_DELAY, 200},

    {0x29, 0},
    {0x2C, 0},
};

static void ili9481_init_code(const InitCode *code, u8 cnt)
{
    for (u8 i = 0; i < cnt; i++) {
        if (code[i].cmd == REGFLAG_DELAY) {
            lcd_delay(code[i].cnt);
        } else {
            WriteCOM(code[i].cmd);

            for (u8 j = 0; j < code[i].cnt; j++) {
                WriteDAT_8(code[i].dat[j]);
            }
        }
    }
}

static void ili9481_led_ctrl(u8 status)
{
    //背光控制以及放在//lcd_te_driver.c 优化开机显示
    lcd_bl_pinstate(status);
}

void ili9481_test(void)
{
    while(1){
        lcd_bl_pinstate(BL_ON);
        os_time_dly(100);
        ili9481_clear_screen(BLUE);
        printf("LCD_ILI9481_TSET_BLUE\n");
        os_time_dly(100);
        ili9481_clear_screen(GRED);
        printf("LCD_ILI9481_TSET_GRED\n");
        os_time_dly(100);
        ili9481_clear_screen(BRRED);
        printf("LCD_ILI9481_TSET_BRRED\n");
        os_time_dly(100);
        ili9481_clear_screen(YELLOW);
        printf("LCD_ILI9481_TSET_YELLOW\n");
    }
}

static void ILI9481_lvgl_Fill(u16 xs, u16 xe, u16 ys, u16 ye, char *img)
{
    u32 len = 0;
    len = (xe + 1 - xs) * (ye + 1 - ys) * 2;
    WriteCOM(0x2A);
    WriteDAT_8(xs >> 8);
    WriteDAT_8(xs);
    WriteDAT_8(xe >> 8);
    WriteDAT_8(xe);
    WriteCOM(0x2B);
    WriteDAT_8(ys >> 8);
    WriteDAT_8(ys);
    WriteDAT_8(ye >> 8);
    WriteDAT_8(ye);
    WriteCOM(0x2c);
    WriteDAT_DMA(img, len);
}

static int ili9481_init(void)
{
    printf("LCD_ili9481 init_start\n");
    lcd_bl_pinstate(BL_ON);
    ili9481_init_code(code1, sizeof(code1) / sizeof(code1[0]));
    ili9481_set_direction(ROTATE_DEGREE_0);
    init_TE(ili9481_Fill);
//    ili9481_test();
    printf("LCD_ili9481 config succes\n");

    return 0;
}


REGISTER_LCD_DEV(LCD_ili9481) = {
    .name              = "ili9481",
    .lcd_width         = LCD_W,
    .lcd_height        = LCD_H,
    .color_format      = LCD_COLOR_RGB565,
    .column_addr_align = 1,
    .row_addr_align    = 1,
    .LCD_Init          = ili9481_init,
    .SetDrawArea       = ili9481_SetRange,
    .LCD_Draw          = ili9481_draw,
    .LCD_Draw_1        = ili9481_draw_1,
    .LCD_DrawToDev     = ili9481_Fill,//应用层直接到设备接口层，需要做好缓冲区共用互斥，慎用！
    .LCD_Lvgl_Full     = ILI9481_lvgl_Fill,//LVGL发送数据接口
    .LCD_ClearScreen   = ili9481_clear_screen,
    .Reset             = ili9481_reset,
    .BackLightCtrl     = ili9481_led_ctrl,
};

#endif


