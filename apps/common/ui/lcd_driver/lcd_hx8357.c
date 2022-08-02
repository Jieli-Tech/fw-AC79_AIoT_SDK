#include "system/includes.h"
#include "typedef.h"
#include "asm/pap.h"
#include "lcd_drive.h"
#include "lcd_te_driver.h"
#include "lcd_config.h"

/*  HX8357驱动说明 该驱动测试时使用的 wl80 79系列
 *  由于该IC推屏能力不够强 推屏的帧数较低 大概在25帧左右
 *  在推屏过程中需要使用TE屏幕帧中断 不然会有条纹
 *  由于HX8357横屏配置无法调出没有条纹的配置
 *  所有只能使用竖屏加RGB旋转来实现UI横屏显示
 */

/* //pap的这个三个配置如下 在板级文件中进行修改
    .timing_setup 	= 0,					//具体看pap.h
    .timing_hold  	= 0,					//具体看pap.h
    .timing_width 	= 1,					//具体看pap.h
*/

#if TCFG_LCD_HX8357_ENABLE

#define ROTATE_DEGREE_0  	0
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
#define BRRED 			 0XFC07 //棕红色
#define GRAY  			 0X8430 //灰色i

#define READ_ID 	0x04
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

void HX8357_SetRange(u16 xs, u16 xe, u16 ys, u16 ye)
{
    /******UI每次发送数据都会调用开窗告诉屏幕要刷新那个区域***********/
    get_lcd_ui_x_y(xs, xe, ys, ye);
}
void HX8357_SetRange_1(u16 xs, u16 xe, u16 ys, u16 ye)
{
    WriteCOM(0x2A);
    WriteDAT_8(ys >> 8);
    WriteDAT_8(ys);
    WriteDAT_8(ye >> 8);
    WriteDAT_8(ye);
    WriteCOM(0x2B);
    WriteDAT_8(xs >> 8);
    WriteDAT_8(xs);
    WriteDAT_8(xe >> 8);
    WriteDAT_8(xe);

}

void HX8357_clear_screen(u32 color)
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

void HX8357_Fill(u8 *img, u16 len)
{
    WriteCOM(0x2c);
    WriteDAT_DMA(img, len);
}

void HX8357_SleepInMode(void)
{
    WriteCOM(0x10); //Sleep in
    lcd_delay(120); //Delay 120ms
}

void HX8357_SleepOutMode(void)
{
    WriteCOM(0x11); //Sleep out
    lcd_delay(120);  //Delay 120ms
}

void st7789_shown_image(u8 *buff, u16 x_addr, u16 y_addr, u16 width, u16 height)
{
    HX8357_SetRange(x_addr, y_addr, width, height);
    WriteDAT_DMA(buff, width * height * 2);
}

static void HX8357_set_direction(u8 dir)
{
    WriteCOM(0x36);    //扫描方向控制

    if (dir == ROTATE_DEGREE_0) { //正向
#if HORIZONTAL_SCREEN
        WriteDAT_8(0x40);
#else
        WriteDAT_8(0x00);
#endif
        HX8357_SetRange(0, LCD_W - 1, 0, LCD_H - 1);
    } else if (dir == ROTATE_DEGREE_180) { //翻转180
#if HORIZONTAL_SCREEN
        WriteDAT_8(0xc0);
#else
        WriteDAT_8(0x80);
#endif
    }
}

static void HX8357_draw(u8 *map, u32 size)//获取Ui发送出来的数据
{
    ui_send_data_ready(map, size);
}

static void HX8357_draw_1(u8 *map, u32 size)//获取camera发送出来的数据 //数据帧数以camera为基准
{
    camera_send_data_ready(map, size);
}

static void HX8357_reset(void)
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
    {0xB4, 1, {0x00}},
    {0xC0, 5, {0x14, 0x3B, 0x00, 0x02, 0x11}},
    {0xC8, 12, {0x00, 0x15, 0x00, 0x22, 0x00, 0x08, 0x77, 0x26, 0x77, 0x22, 0x04, 0x00}},
    {0xD0, 3, {0x44, 0x41, 0x06}},
    {0xD1, 2, {0x39, 0x0B}},
    {0xD2, 2, {0x05, 0x12}},
    {0xE9, 1, {0x01}},
    {0xEA, 3, {0x03, 0x00, 0x00}},
    {0x20, 1, {0x00}},
    {0x36, 1, {0x10}},
    {0x3A, 1, {0x55}},
    {0x11, 0},
    {REGFLAG_DELAY, 240},
    {0x29, 0},
    {REGFLAG_DELAY, 10},
    {0x2A, 4, {0x00, 0x00, 0x01, 0x3F}},
    {0x2B, 4, {0x00, 0x00, 0x01, 0xDF}},
    {0x2C, 0},
};

static void HX8357_init_code(const InitCode *code, u8 cnt)
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

static void HX8357_led_ctrl(u8 status)
{
    //背光控制以及放在//lcd_te_driver.c 优化开机显示
    /*lcd_bl_pinstate(status);*/
}

void HX8357_test(void)
{
    lcd_bl_pinstate(BL_ON);
    while (1) {
        os_time_dly(100);
        HX8357_clear_screen(BLUE);
        printf("LCD_HX8357_TSET_BLUE\n");
        os_time_dly(100);
        HX8357_clear_screen(GRED);
        printf("LCD_HX8357_TSET_GRED\n");
        os_time_dly(100);
        HX8357_clear_screen(BRRED);
        printf("LCD_HX8357_TSET_BRRED\n");
        os_time_dly(100);
        HX8357_clear_screen(YELLOW);
        printf("LCD_HX8357_TSET_YELLOW\n");
    }
}

static int HX8357_init(void)
{
    printf("LCD_HX8357 init_start\n");
    HX8357_reset();
    HX8357_init_code(code1, sizeof(code1) / sizeof(code1[0]));
    HX8357_set_direction(ROTATE_DEGREE_0);
    init_TE(HX8357_Fill);
    /*HX8357_test();*/
    printf("LCD_HX8357 config succes\n");
    return 0;
}


REGISTER_LCD_DEV(LCD_HX8357) = {
    .name              = "HX8357",
    .lcd_width         = LCD_W,
    .lcd_height        = LCD_H,
    .color_format      = LCD_COLOR_RGB565,
    .column_addr_align = 1,
    .row_addr_align    = 1,
    .LCD_Init          = HX8357_init,
    .SetDrawArea       = HX8357_SetRange,
    .LCD_Draw          = HX8357_draw,
    .LCD_Draw_1        = HX8357_draw_1,
    .LCD_DrawToDev     = HX8357_Fill,//应用层直接到设备接口层，需要做好缓冲区共用互斥，慎用！
    .LCD_ClearScreen   = HX8357_clear_screen,
    .Reset             = HX8357_reset,
    .BackLightCtrl     = HX8357_led_ctrl,
};

#endif


