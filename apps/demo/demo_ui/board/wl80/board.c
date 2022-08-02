#include "app_config.h"
#include "system/includes.h"
#include "device/includes.h"
#include "device/iic.h"
#include "server/audio_dev.h"
#include "asm/includes.h"
#include "asm/sdmmc.h"

#ifdef CONFIG_UI_ENABLE
#include "lcd_drive.h"
#include "ui_api.h"
#endif

// *INDENT-OFF*


UART2_PLATFORM_DATA_BEGIN(uart2_data)
	.baudrate = 1000000,
	.port = PORT_REMAP,
	.output_channel = OUTPUT_CHANNEL0,
	.tx_pin = IO_PORTB_01,
	.rx_pin = -1,
	.max_continue_recv_cnt = 1024,
	.idle_sys_clk_cnt = 500000,
	.clk_src = PLL_48M,
	.flags = UART_DEBUG,
UART2_PLATFORM_DATA_END();

/**************************  POWER config ****************************/
static const struct low_power_param power_param = {
    .config         = TCFG_LOWPOWER_LOWPOWER_SEL,          //系统休眠
    .btosc_disable  = TCFG_LOWPOWER_BTOSC_DISABLE,         //进入低功耗时BTOSC是否保持
    .vddiom_lev     = TCFG_LOWPOWER_VDDIOM_LEVEL,          //强VDDIO等级,可选：2.2V  2.4V  2.6V  2.8V  3.0V  3.2V  3.4V  3.6V
    .vddiow_lev     = TCFG_LOWPOWER_VDDIOW_LEVEL,          //弱VDDIO等级,可选：2.1V  2.4V  2.8V  3.2V
	.vdc14_dcdc 	= TRUE,	   							   //打开内部1.4VDCDC，关闭则用外部
    .vdc14_lev		= VDC14_VOL_SEL_LEVEL, 				   //VDD1.4V配置
	.sysvdd_lev		= SYSVDD_VOL_SEL_LEVEL,				   //内核、sdram电压配置
	.vlvd_enable	= TRUE,                                //TRUE电压复位使能
	.vlvd_value		= VLVD_SEL_25V,                        //低电压复位电压值
};

#ifdef CONFIG_UI_ENABLE

SPI1_PLATFORM_DATA_BEGIN(spi1_data)   //pc8 pc9 pc10
    .clk    = 20000000,               //di  clk  do
    .mode   = SPI_STD_MODE,
    .port   = 'B',
    .attr	= SPI_SCLK_L_UPL_SMPH | SPI_UNIDIR_MODE,//主机，CLK低 更新数据低，单向模式
SPI1_PLATFORM_DATA_END()

static const struct ui_lcd_platform_data pdata = {
#if TCFG_LCD_ST7789S_ENABLE
    .rs_pin  = IO_PORTC_08,
    .rst_pin = IO_PORTC_06,
    .cs_pin  = IO_PORTC_07,
    .te_pin  = IO_PORTC_04,
    .bl_pin  = IO_PORTC_05,//背光IO
	//wr 固定 IO_PORTC_09
	//rd 固定 IO_PORTC_10  //通过配置re_en可以不使用rd 在不使用时候切记RD拉高(根据不同屏选择)
    .lcd_if  = LCD_PAP,
#endif
#if TCFG_LCD_ST7735S_ENABLE
    .spi_id  = "spi1",
    .rs_pin  = IO_PORTC_07,
    .rst_pin = IO_PORTC_06,
    .cs_pin  = IO_PORTC_05,
    .bl_pin  = IO_PORTC_04,//背光IO
    .lcd_if  = LCD_SPI,
#endif
};
static const struct pap_info pap_data = {
    .datawidth 		= PAP_PORT_8BITS,
    .endian    		= PAP_LE,				//数据输出大小端
    .cycle     		= PAP_CYCLE_ONE,		//1/2字节发送次数
    .pre			= PAP_READ_LOW,			//读取rd有效电平
    .pwe			= PAP_WRITE_LOW,		//写wr有效电平
    .use_sem		= TRUE,					//使用信号等待
	.rd_en			= TRUE,				//不使用rd读信号
    .port_sel		= PAP_PORT_A,			//PAP_PORT_A PAP_PORT_B
    .timing_setup 	= 0,					//具体看pap.h
    .timing_hold  	= 0,					//具体看pap.h
    .timing_width 	= 1,					//具体看pap.h
};
const struct ui_devices_cfg ui_cfg_data = {
    .type = TFT_LCD,
    .private_data = (void *)&pdata,
};
#endif




#ifdef CONFIG_VIDEO_ENABLE

SW_IIC_PLATFORM_DATA_BEGIN(sw_iic0_data)
	.clk_pin = IO_PORTC_01,
	.dat_pin = IO_PORTC_02,
	.sw_iic_delay = 50,
SW_IIC_PLATFORM_DATA_END()

static const struct camera_platform_data camera0_data = {//默认开发板接口 DVP摄像头
    .xclk_gpio      = IO_PORTC_00,//注意： 如果硬件xclk接到芯片IO，则会占用OUTPUT_CHANNEL1
    /*.reset_gpio     = IO_PORTC_03,*/
    .reset_gpio     = -1,
    .online_detect  = NULL,
    .pwdn_gpio      = -1,
    .power_value    = 0,
    .interface      = SEN_INTERFACE0,//SEN_INTERFACE_CSI2,
    .dvp={
        .pclk_gpio   = IO_PORTA_08,
        .hsync_gpio  = IO_PORTA_09,
        .vsync_gpio  = IO_PORTA_10,
		.group_port  = ISC_GROUPA,
        .data_gpio={
                IO_PORTA_07,
                IO_PORTA_06,
                IO_PORTA_05,
                IO_PORTA_04,
                IO_PORTA_03,
                IO_PORTA_02,
                IO_PORTA_01,
                IO_PORTA_00,
				-1,
				-1,
        },
    }
};
static const struct video_subdevice_data video0_subdev_data[] = {
    { VIDEO_TAG_CAMERA, (void *)&camera0_data },
};
static const struct video_platform_data video0_data = {
    .data = video0_subdev_data,
    .num = ARRAY_SIZE(video0_subdev_data),
};
#endif//CONFIG_VIDEO_ENABLE

/****************SD模块**********************************************************
 * 暂只支持1个SD外设，如需要多个SD外设，自行去掉下面SD0/1_ENABLE控制，另外定义即可
********************************************************************************/
#if TCFG_SD0_ENABLE
//sd0
#define SD_PLATFORM_DATA_BEGIN() SD0_PLATFORM_DATA_BEGIN(sd0_data)
#define SD_PLATFORM_DATA_END() SD0_PLATFORM_DATA_END()
#define SD_CLK_DETECT_FUNC		sdmmc_0_clk_detect
#elif TCFG_SD1_ENABLE
//sd1
#define SD_PLATFORM_DATA_BEGIN() SD1_PLATFORM_DATA_BEGIN(sd1_data)
#define SD_PLATFORM_DATA_END() SD1_PLATFORM_DATA_END()
#define SD_CLK_DETECT_FUNC		sdmmc_1_clk_detect
#endif

#if TCFG_SD0_ENABLE || TCFG_SD1_ENABLE

SD_PLATFORM_DATA_BEGIN()
	.port 					= TCFG_SD_PORTS,
	.priority 				= 3,
	.data_width 			= TCFG_SD_DAT_WIDTH,
	.speed 					= TCFG_SD_CLK,
	.detect_mode 			= TCFG_SD_DET_MODE,
#if (TCFG_SD_DET_MODE == SD_CLK_DECT)
	.detect_func 			= SD_CLK_DETECT_FUNC,
#elif (TCFG_SD_DET_MODE == SD_IO_DECT)
	.detect_func 			= sdmmc_0_io_detect,
#else
	.detect_func 			= NULL,
#endif
SD_PLATFORM_DATA_END()

#endif
/****************SD模块*********************/

REGISTER_DEVICES(device_table) = {
#if TCFG_LCD_ST7789S_ENABLE
#if !defined CONFIG_NO_SDRAM_ENABLE
    { "pap",   &pap_dev_ops, (void *)&pap_data },
#endif
#endif
#if TCFG_SD0_ENABLE
	{ "sd0",  &sd_dev_ops, (void *)&sd0_data },
#endif
	{"uart2", &uart_dev_ops, (void *)&uart2_data },
	{"rtc"  , &rtc_dev_ops, NULL},
#if TCFG_LCD_ST7735S_ENABLE
    { "spi1", &spi_dev_ops, (void *)&spi1_data },
#endif
#ifdef CONFIG_VIDEO_ENABLE
    { "video0.*",  &video_dev_ops, (void *)&video0_data },
	{ "iic0",  &iic_dev_ops, (void *)&sw_iic0_data },
#endif
};

#ifdef CONFIG_DEBUG_ENABLE
void debug_uart_init()
{
    uart_init(&uart2_data);
}
#endif

void board_early_init()
{
    devices_init();
}

static void board_power_init(void)
{

    power_init(&power_param);
}

void board_init()
{
	board_power_init();
}

