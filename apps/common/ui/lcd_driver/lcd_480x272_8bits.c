#include "app_config.h"

#ifdef CONFIG_CPU_WL82
#include "generic/typedef.h"
#include "asm/WL82_imd.h"
#include "lcd_drive.h"
#include "lcd_config.h"
#include "gpio.h"
#include "yuv_to_rgb.h"

#if TCFG_LCD_480x272_8BITS

void lcd_480x272_8bits_text(void);

static void ui_data_handle(u8 *data, u32 size)
{
    ui_send_data_ready(data, size);
}
static void camera_data_handle(u8 *data, u32 size)
{
    camera_send_data_ready(data, size);
}

static void send_data_to_lcd(u8 *data, u32 size)//最终通过NO_te线程发送数据
{
    u8 *req_buf = NULL;
    u32 req_len = 0;

    req_buf = lcd_req_buf(&req_len);//RGB-DVP屏幕使用IMD接口，先获取内部可用缓冲区，再在缓冲区进行数据转换
    if (req_buf && req_len) {
        RGB565_to_RGB888(data, req_buf, LCD_W, LCD_H);
        lcd_req_buf_update(req_buf, req_len);
    } else {
        req_buf = malloc(LCD_RGB888_DATA_SIZE);
        if (req_buf) {
            RGB565_to_RGB888(data, req_buf, LCD_W, LCD_H);
            camera_data_handle(req_buf, LCD_RGB888_DATA_SIZE);
            free(req_buf);
        }
    }
}

static void lcd_480x272_clearscreen(u32 color)
{
    lcd_send_map(NULL, color);
}
static int lcd_480x272_8bits_init(void)
{
    printf("lcd 480x272 8bits init ...\n");
    /*lcd_480x272_8bits_text();*/
    init_TE(send_data_to_lcd);

    return 0;
}
static void lcd_480x272_8bit_SetRange(u16 xs, u16 xe, u16 ys, u16 ye)
{
    /******UI每次发送数据都会调用开窗告诉屏幕要刷新那个区域***********/
    get_lcd_ui_x_y(xs, xe, ys, ye);

}
static void lcd_480x272_8bit_reset(void)
{

}
static void lcd_480x272_8bit_led_ctrl(u8 status)
{
    //背光控制以及放在//lcd_te_driver.c 优化开机显示
    /*lcd_bl_pinstate(status);*/
}

void lcd_480x272_8bits_text(void)
{
    u8 *buf = malloc(LCD_RGB888_DATA_SIZE);
    while (1) {
        printf("[test]>>>>>>>>lcd_480x272_8bits_text");
        memset(buf, 0x00, LCD_RGB888_DATA_SIZE);
        for (u32 i = 0; i < LCD_RGB888_DATA_SIZE; i += 3) {
            buf[i] = 0xff;
        }
        lcd_send_map(buf, LCD_RGB888_DATA_SIZE);
        os_time_dly(100);

        printf("[test]>>>>>>>>lcd_480x272_8bits_text");
        memset(buf, 0x00, LCD_RGB888_DATA_SIZE);
        memset(buf, 0x00, LCD_RGB888_DATA_SIZE);
        for (u32 i = 1; i < LCD_RGB888_DATA_SIZE; i += 3) {
            buf[i] = 0xff;
        }
        lcd_send_map(buf, LCD_RGB888_DATA_SIZE);
        os_time_dly(100);

        printf("[test]>>>>>>>>lcd_480x272_8bits_text");
        memset(buf, 0x00, LCD_RGB888_DATA_SIZE);
        memset(buf, 0x00, LCD_RGB888_DATA_SIZE);
        for (u32 i = 2; i < LCD_RGB888_DATA_SIZE; i += 3) {
            buf[i] = 0xff;
        }
        lcd_send_map(buf, LCD_RGB888_DATA_SIZE);
        os_time_dly(100);
    }
}

// *INDENT-OFF*
IMD_RGB_PLATFORM_DATA_BEGIN(lcd_rgb_data)
	.test_mode		= TRUE,					//初始化颜色显示
	.double_buffer  = TRUE,					//底层使用双buffer切换
	.req_buf_waite  = FALSE,				//底层buff更新是否需要等待在中断更新：小分辨率等待，大分辨率不等带
	.test_color		= 0xFFFF00,				//初始化颜色(r<<16|g<<8|b)
	.group_port 	= IMD_IO_GROUPC,		//IO口选择:IMD_IO_GROUPA/IMD_IO_GROUPC
	.irq_en			= TRUE,//FALSE,				//使能GB传输的帧中断,频率较高建议关闭:TRUE/FALSE
	.clk_edge		= IMD_CLK_UPDATE_H,		//更新数据边缘选择:IMD_CLK_UPDATE_H/IMD_CLK_UPDATE_L
	.sync0			= IMD_VSYN_EN | IMD_SYN_EDGE_H,		//行帧同步信号选择:
	.sync1			= IMD_HSYN_EN | IMD_SYN_EDGE_H,		//行帧同步信号选择:
	.sync2			= IMD_DE_EN | IMD_SYN_EDGE_H,		//行帧步信号选择:
	//.sync2			= IMD_SYNC_DIS,		//行帧同步信号选择:
	/*.data_out_mode 	= IMD_DATA_LSB,			//输出输出大小端:IMD_DATA_MSB/IMD_DATA_LSB*/
	.data_out_mode 	= IMD_DATA_MSB,			//输出输出大小端:IMD_DATA_MSB/IMD_DATA_LSB
	.data_format	= IMD_RGB_888,			//RGB输出格式:IMD_RGB_888/IMD_RGB_565
	.data_shift_en	= FALSE,					//RGB666数据右移位2位(先右移在按照MSB/LSB输出)
	.pll_clk_div	= IMD_PLL_DIV24,//WL82_imd.h
	//IMD_PLL_DIV28 一帧数据28ms
	//IMD_PLL_DIV24 一帧数据24ms
	//IMD_PLL_DIV20 一帧数据20ms //依次类推 DIV后面数值就是一帧数据的时间 帧数为 1000ms/28ms = 35帧
	.hsync_forward	= 10,					//行前沿宽度周期个数
	.hsync_behind	= 10,					//行后沿宽度周期个数
	.hsync_time		= 268,	//行同步脉冲宽度周期个数
	.vsync_forward 	= 8,					//帧前沿宽度的行时间个数
	.vsync_behind	= 3,					//帧后沿宽度的行时间个数
	.vsync_time		= 5,					//帧同步脉冲宽的行时间个数
	.width			= LCD_W,					//RGB屏宽
	.height			= LCD_H,					//RGB屏高
IMD_RGB_PLATFORM_DATA_END()

REGISTER_LCD_DEV(lcd_480x271_bits_dev) = {
	.name              = "LCD_480x272_8BITS",
	.lcd_priv		   = (void*)&lcd_rgb_data,
    .lcd_width         = LCD_W,
    .lcd_height        = LCD_H,
    .color_format      = LCD_COLOR_RGB888,
    .column_addr_align = 1,
    .row_addr_align    = 1,
	.LCD_Init          = lcd_480x272_8bits_init,
    .SetDrawArea       = lcd_480x272_8bit_SetRange,
    .LCD_Draw          = ui_data_handle,
    .LCD_Draw_1        = camera_data_handle,
    .LCD_DrawToDev     = lcd_send_map,//应用层直接到设备接口层，需要做好缓冲区共用互斥，慎用！
    .LCD_ClearScreen   = lcd_480x272_clearscreen,
    .Reset             = lcd_480x272_8bit_reset,
    .BackLightCtrl     = lcd_480x272_8bit_led_ctrl,
};
// *INDENT-ON*

#endif
#endif
