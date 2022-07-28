

#include "system/includes.h"
#include "typedef.h"
#include "asm/pap.h"
#include "app_config.h"
#include "lcd_drive.h"
#include "lcd_te_driver.h"

#if TCFG_LCD_ST7735S_ENABLE

#define st7735s_lcd_w     128
#define st7735s_lcd_h     128

#define ROTATE_DEGREE_0  	0
#define ROTATE_DEGREE_90  	1
#define ROTATE_DEGREE_180  	2
#define ROTATE_DEGREE_270 	3

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

static void ST7735S_SetRange(u16 xs, u16 xe, u16 ys, u16 ye)
{
    /******UI每次发送数据都会调用开窗告诉屏幕要刷新那个区域***********/
    get_lcd_ui_x_y(xs, xe, ys, ye);
}

static void ST7735S_SetRange_1(u16 xs, u16 xe, u16 ys, u16 ye)
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

static void ST7735S_io_init()
{
    lcd_rs_pinstate(0);
    lcd_cs_pinstate(1);
    lcd_rst_pinstate(1);
}


void ST7735S_set_direction(u8 dir)
{
    WriteCOM(0x36);    //扫描方向控制
    if (dir == ROTATE_DEGREE_0) { //正向
        WriteDAT_8(0x8 | BIT(4) | BIT(6) | BIT(7)); //BIT(4)垂直翻转
        ST7735S_SetRange_1(0, st7735s_lcd_w - 1, 0, st7735s_lcd_h - 1);
    } else if (dir == ROTATE_DEGREE_180) { //顺时针旋转180度
        WriteDAT_8(0x8);
        ST7735S_SetRange_1(0, st7735s_lcd_w - 1, 0, st7735s_lcd_h - 1);
    } else if (dir == ROTATE_DEGREE_90) { //顺时针旋转90度
        WriteDAT_8(0x8 | BIT(4) | BIT(5) | BIT(6));
        ST7735S_SetRange_1(0, st7735s_lcd_w - 1, 0, st7735s_lcd_h - 1);
    } else if (dir == ROTATE_DEGREE_270) { //顺时名旋转270度
        WriteDAT_8(0x8 | BIT(4) | BIT(5) | BIT(7));
        ST7735S_SetRange_1(0, st7735s_lcd_w - 1, 0, st7735s_lcd_h - 1);
    }
}

void ST7735S_clear_screen(u16 color)
{
    WriteCOM(0x2c);
    u8 *buf = malloc(st7735s_lcd_w * st7735s_lcd_h * 2);
    if (!buf) {
        printf("no men in %s \n", __func__);
        return;
    }
    for (u32 i = 0; i < st7735s_lcd_w * st7735s_lcd_h; i++) {
        buf[2 * i] = (color >> 8) & 0xff;
        buf[2 * i + 1] = color & 0xff;
    }
    WriteDAT_DMA(buf, st7735s_lcd_w * st7735s_lcd_h * 2);
    free(buf);
}

void ST7735S_Fill(u8 *img, u16 len)
{
    WriteCOM(0x2c);
    WriteDAT_DMA(img, len);
}

void ST7735SHS177PanelSleepInMode(void)
{
    WriteCOM(0x10); //Sleep in
    lcd_delay(120); //Delay 120ms
}

void ST7735SHS177PanelSleepOutMode(void)
{
    WriteCOM(0x11); //Sleep out
    lcd_delay(120);  //Delay 120ms
}

void st7735_shown_image(u8 *buff, u16 x_addr, u16 y_addr, u16 width, u16 height)
{
    u32 i = 0;
    st7735s_set_xy_addr(x_addr, y_addr, width, height);
    WriteDAT_DMA(buff, width * height * 2);
}

static void ST7735s_draw(u8 *map, u32 size)//获取Ui发送出来的数据
{
    ui_send_data_ready(map, size);
}

static void ST7735s_draw_1(u8 *map, u32 size)//获取camera发送出来的数据 //数据帧数以camera为基准
{
    camera_send_data_ready(map, size);
}

static void ST7735s_reset(void)
{

}

static void ST7735s_led_ctrl(u8 status)
{

}

void st7735s_clr(void)
{
    ST7735S_clear_screen(0x00);
}

static void ST7735S_reg_cfg(void)
{
    lcd_rst_pinstate(1);
    lcd_delay(50);
    lcd_rst_pinstate(0);
    lcd_delay(20);
    lcd_rst_pinstate(1);
    lcd_delay(120);

    WriteCOM(0x11); //sleep out and booter on
    lcd_delay(120);

    WriteCOM(0xB1); //
    WriteDAT_8(0x01);
    WriteDAT_8(0x2C);
    WriteDAT_8(0x2D);

    WriteCOM(0xB2); //
    WriteDAT_8(0x01);
    WriteDAT_8(0x2C);
    WriteDAT_8(0x2D);

    WriteCOM(0xB3); //
    WriteDAT_8(0x01);
    WriteDAT_8(0x2C);
    WriteDAT_8(0x2D);
    WriteDAT_8(0x01);
    WriteDAT_8(0x2C);
    WriteDAT_8(0x2D);

    WriteCOM(0xB4); //
    WriteDAT_8(0x07);

    //============ power control setting ==========================
    WriteCOM(0xC0); //
    WriteDAT_8(0xA2); //
    WriteDAT_8(0x02); //

    WriteDAT_8(0x84); //

    WriteCOM(0xC1); // set VCL,VGH,VGL,AVDD
    WriteDAT_8(0xC5);

    WriteCOM(0xC2); //
    WriteDAT_8(0x0A); //
    WriteDAT_8(0x00);

    WriteCOM(0xC3); //
    WriteDAT_8(0x8A); //
    WriteDAT_8(0x2A);

    WriteCOM(0xC4); //
    WriteDAT_8(0x8A); //
    WriteDAT_8(0xEE); //

    WriteCOM(0xC5); //
    WriteDAT_8(0x0E); //

    WriteCOM(0x36); //set VCOMH,VCOML voltage
    WriteDAT_8(0xC8); //VCOMH=3.275V
    WriteCOM(0x20);
    WriteCOM(0x21);

    //===== gamma"+"polarity correction characteristic setting ===================
    WriteCOM(0xE0);
    WriteDAT_8(0x02);
    WriteDAT_8(0x1c);
    WriteDAT_8(0x07);
    WriteDAT_8(0x12);
    WriteDAT_8(0x37);
    WriteDAT_8(0x32);
    WriteDAT_8(0x29);
    WriteDAT_8(0x2d);
    WriteDAT_8(0x29);
    WriteDAT_8(0x25);
    WriteDAT_8(0x2b);
    WriteDAT_8(0x39);
    WriteDAT_8(0x00);
    WriteDAT_8(0x01);
    WriteDAT_8(0x03);
    WriteDAT_8(0x10);

    //===== gamma"-"polarity correction characteristic setting ===================
    WriteCOM(0xE1);
    WriteDAT_8(0x03);
    WriteDAT_8(0x1d);
    WriteDAT_8(0x07);
    WriteDAT_8(0x06);
    WriteDAT_8(0x2e);
    WriteDAT_8(0x2c);
    WriteDAT_8(0x29);
    WriteDAT_8(0x2d);
    WriteDAT_8(0x2e);
    WriteDAT_8(0x2e);
    WriteDAT_8(0x37);
    WriteDAT_8(0x3f);
    WriteDAT_8(0x00);
    WriteDAT_8(0x00);
    WriteDAT_8(0x02);
    WriteDAT_8(0x10);

    WriteCOM(0x20);     //
    WriteCOM(0x36);     // memory access control
    WriteDAT_8(0xcc);
    WriteCOM(0x3A);     //E0H or E1 Register enable or disabl
    WriteDAT_8(0x05);   //E0H or E1 Register enable
    WriteCOM(0x29);     //display on
}
static void st7735s_test(void)
{
    while (1) {
        os_time_dly(100);
        ST7735S_clear_screen(BLUE);
        printf("LCD_ST7735S_TSET_BLUE\n");
        os_time_dly(100);
        ST7735S_clear_screen(GRED);
        printf("LCD_ST7735S_TSET_GRED\n");
        os_time_dly(100);
        ST7735S_clear_screen(BRRED);
        printf("LCD_ST7735S_TSET_BRRED\n");
        os_time_dly(100);
        ST7735S_clear_screen(YELLOW);
        printf("LCD_ST7735S_TSET_YELLOW\n");
    }
}

static int ST7735S_init(void)
{
    ST7735S_io_init();
    lcd_d("LCD_ST7735S config...\n");
    ST7735S_reg_cfg();
    ST7735S_set_direction(0);
    init_TE(ST7735S_Fill);
    st7735s_test();
    lcd_d("LCD_ST7735S config succes\n");
    return 0;
}

// *INDENT-OFF*
REGISTER_LCD_DEV(LCD_ST7735S) = {
    .name              = "st7735s",
    .lcd_width         = st7735s_lcd_h,
    .lcd_height        = st7735s_lcd_w,
    .color_format      = LCD_COLOR_RGB565,
    .column_addr_align = 1,
    .row_addr_align    = 1,
    .LCD_Init          = ST7735S_init,
    .SetDrawArea       = ST7735S_SetRange,
    .LCD_Draw          = ST7735s_draw,
    .LCD_Draw_1        = ST7735s_draw_1,
    .LCD_ClearScreen   = ST7735S_clear_screen,
    .Reset             = ST7735s_reset,
    .BackLightCtrl     = ST7735s_led_ctrl,
};
// *INDENT-ON*

#endif


