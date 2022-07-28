/*
 * 航拍板
 */



#include "app_config.h"

#ifdef CONFIG_BOARD_7911B_DEVELOP_AERIAL

#include "system/includes.h"
#include "device/includes.h"
#include "device/iic.h"
#include "server/audio_dev.h"
#include "asm/includes.h"


#if (TCFG_IOKEY_ENABLE || TCFG_IRKEY_ENABLE || TCFG_RDEC_KEY_ENABLE || TCFG_ADKEY_ENABLE)
#include "key/iokey.h"
#include "key/adkey.h"
#include "key/irkey.h"
#include "key/rdec_key.h"
#include "key/key_driver.h"
#include "event/key_event.h"
#endif


#if TCFG_USB_SLAVE_ENABLE || TCFG_USB_HOST_ENABLE
#include "otg.h"
#include "usb_host.h"
#include "usb_common_def.h"
#include "usb_storage.h"
#include "asm/uvc_device.h"
#endif

#ifdef CONFIG_UI_ENABLE
#include "lcd_drive.h"
#include "ui_api.h"
#endif

#ifdef CONFIG_LTE_PHY_ENABLE
#include "lte_module.h"
#endif

// *INDENT-OFF*
#define DEBUG_IO	IO_PORTC_06
UART0_PLATFORM_DATA_BEGIN(uart0_data)
	.baudrate = 460800,
	.port = PORT_REMAP,
	.output_channel = OUTPUT_CHANNEL0,
	.input_channel  = INPUT_CHANNEL0,
	.tx_pin = DEBUG_IO,
	.rx_pin = -1,
	.max_continue_recv_cnt = 1024,
	.idle_sys_clk_cnt = 500000,
	.clk_src = PLL_48M,
	.flags = UART_DEBUG,
UART0_PLATFORM_DATA_END();


UART1_PLATFORM_DATA_BEGIN(uart1_data)
	.baudrate = 460800,
	.port = PORT_REMAP,
	/* .port = PORTH_6_7, */
	/* .port = PORTC_3_4, */
	/* .port = PORTUSB_B, */
	/* .port = PORTUSB_A, */
	/* .tx_pin = IO_PORTH_06, */
	/* .rx_pin = IO_PORTH_07, */
	/* .tx_pin = IO_PORTC_03, */
	/* .rx_pin = IO_PORTC_04, */
	/* .tx_pin = IO_PORT_USB_DPB, */
	/* .rx_pin = IO_PORT_USB_DMB, */
	/* .tx_pin = IO_PORT_USB_DPA, */
	/* .rx_pin = IO_PORT_USB_DMA, */
	.tx_pin = IO_PORTB_00,
	.rx_pin = IO_PORTB_01,
	.output_channel = OUTPUT_CHANNEL3,
	.input_channel  = INPUT_CHANNEL3,
	.max_continue_recv_cnt = 1024,
	.idle_sys_clk_cnt = 500000,
	.clk_src = PLL_48M,
	.flags = UART_DEBUG,
UART1_PLATFORM_DATA_END();


UART2_PLATFORM_DATA_BEGIN(uart2_data)
	.baudrate = 460800,
	/*.tx_pin = IO_PORTH_02,*/
	/*.rx_pin = IO_PORTH_03,*/
	.port = PORT_REMAP,
	.tx_pin = DEBUG_IO,
	.rx_pin = -1,
	/* .tx_pin = IO_PORTC_09, */
	/* .rx_pin = IO_PORTC_10, */
	/* .tx_pin = IO_PORTB_06, */
	/* .rx_pin = IO_PORTB_07, */
	/* .tx_pin = IO_PORTE_00,*/
	/* .rx_pin = IO_PORTE_01,*/
	.output_channel = OUTPUT_CHANNEL2,
	.input_channel  = INPUT_CHANNEL2,
	.max_continue_recv_cnt = 1024,
	.idle_sys_clk_cnt = 500000,
	.clk_src = PLL_48M,
	.flags = UART_DEBUG,
UART2_PLATFORM_DATA_END();



/****************SD模块**********************************************************
 * 暂只支持1个SD外设，如需要多个SD外设，自行去掉下面SD0/1_ENABLE控制，另外定义即可
********************************************************************************/
#if (TCFG_SD0_ENABLE || TCFG_SD1_ENABLE)
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
    .power                  = NULL,//sd_set_power,
SD_PLATFORM_DATA_END()
#endif
/****************SD模块*********************/


HW_IIC0_PLATFORM_DATA_BEGIN(hw_iic1_data)
	/* .clk_pin = IO_PORTC_01, */
	/* .dat_pin = IO_PORTC_02, */
	.clk_pin = IO_PORTH_00,
	.dat_pin = IO_PORTH_01,
	.baudrate = 0x3f,//3f:385k
HW_IIC0_PLATFORM_DATA_END()


SW_IIC_PLATFORM_DATA_BEGIN(sw_iic0_data)
	/*.clk_pin = IO_PORTA_03,*/
	/*.dat_pin = IO_PORTA_04,*/
	.clk_pin = IO_PORTC_01,
	.dat_pin = IO_PORTC_02,
	.sw_iic_delay = 50,
SW_IIC_PLATFORM_DATA_END()

#if TCFG_ADKEY_ENABLE
/*-------------ADKEY GROUP 1----------------*/
#define ADKEY_UPLOAD_R  22
#define ADC_VDDIO (0x3FF)
#define ADC_09   (0x3FF * 220 / (220 + ADKEY_UPLOAD_R))
#define ADC_08   (0x3FF * 120 / (120 + ADKEY_UPLOAD_R))
#define ADC_07   (0x3FF * 51  / (51  + ADKEY_UPLOAD_R))
#define ADC_06   (0x3FF * 33  / (33  + ADKEY_UPLOAD_R))
#define ADC_05   (0x3FF * 22  / (22  + ADKEY_UPLOAD_R))
#define ADC_04   (0x3FF * 15  / (15  + ADKEY_UPLOAD_R))
#define ADC_03   (0x3FF * 10  / (10  + ADKEY_UPLOAD_R))
#define ADC_02   (0x3FF * 62  / (62  + ADKEY_UPLOAD_R * 10))
#define ADC_01   (0x3FF *  3  / ( 3  + ADKEY_UPLOAD_R))
#define ADC_00   (0)

#define ADKEY_V_9      	((ADC_09 + ADC_VDDIO)/2)
#define ADKEY_V_8 		((ADC_08 + ADC_09)/2)
#define ADKEY_V_7 		((ADC_07 + ADC_08)/2)
#define ADKEY_V_6 		((ADC_06 + ADC_07)/2)
#define ADKEY_V_5 		((ADC_05 + ADC_06)/2)
#define ADKEY_V_4 		((ADC_04 + ADC_05)/2)
#define ADKEY_V_3 		((ADC_03 + ADC_04)/2)
#define ADKEY_V_2 		((ADC_02 + ADC_03)/2)
#define ADKEY_V_1 		((ADC_01 + ADC_02)/2)
#define ADKEY_V_0 		((ADC_00 + ADC_01)/2)

const struct adkey_platform_data adkey_data = {//按键不需要添加到设备列表
    .enable     = 1,
	.adkey_pin  = IO_PORTB_01,
    .extern_up_en = 1,
	.ad_channel = 3,
    .ad_value = {
        ADKEY_V_0,
        ADKEY_V_1,
        ADKEY_V_2,
        ADKEY_V_3,
        ADKEY_V_4,
        ADKEY_V_5,
        ADKEY_V_6,
        ADKEY_V_7,
        ADKEY_V_8,
        ADKEY_V_9,
    },
    .key_value = {
        KEY_MENU,  /*0*/
        KEY_PHOTO, /*1*/
        KEY_ENC,   /*2*/
        KEY_F1,    /*3*/
        KEY_MODE,  /*4*/
        KEY_CANCLE,/*5*/
        KEY_OK,    /*6*/
        KEY_DOWN,  /*7*/
        KEY_UP,    /*8*/
		NO_KEY,
    },
};
#endif

#if TCFG_IOKEY_ENABLE
static const struct iokey_port iokey_list[] = {
    {
        .connect_way = ONE_PORT_TO_LOW,         //IO按键的连接方式
        .key_type.one_io.port = IO_PORTB_01,    //IO按键对应的引脚
        .key_value = KEY_MENU,            		//按键值
    }
};
const struct iokey_platform_data iokey_data = {//按键不需要添加到设备列表
    .enable = 1,                              //是否使能IO按键
    .num = ARRAY_SIZE(iokey_list),            //IO按键的个数
    .port = iokey_list,                       //IO按键参数表
};
#endif


#if TCFG_RDEC_KEY_ENABLE
const struct rdec_device rdeckey_list[] = {
    {
        .index = RDEC0 ,
        .sin_port0 = IO_PORTC_09,
        .sin_port1 = IO_PORTC_10,
        .key_value0 = KEY_VOLUME_DEC | BIT(7),
        .key_value1 = KEY_VOLUME_INC | BIT(7),
    },
};
const struct rdec_platform_data rdec_key_data = {//按键不需要添加到设备列表
	.enable = 1,
	.num = ARRAY_SIZE(rdeckey_list),            //RDEC按键的个数
	.rdec = rdeckey_list,                       //RDEC按键参数表
};
#endif


/*
 * spi0接falsh
 */
SPI0_PLATFORM_DATA_BEGIN(spi0_data)
	.clk    = 20000000,
	.mode   = SPI_STD_MODE,
	.port   = 'A',
SPI0_PLATFORM_DATA_END()

SPI1_PLATFORM_DATA_BEGIN(spi1_data)
	.clk    = 80000000,
	.mode   = SPI_STD_MODE,
    .attr	= SPI_SCLK_L_UPH_SMPH | SPI_BIDIR_MODE,
	.port   = 'B',
SPI1_PLATFORM_DATA_END()

SPI2_PLATFORM_DATA_BEGIN(spi2_data)
	.clk    = 80000000,
	.mode   = SPI_STD_MODE,
    .attr	= SPI_SCLK_L_UPH_SMPH | SPI_BIDIR_MODE,
	.port   = 'A',
SPI2_PLATFORM_DATA_END()

static const struct spiflash_platform_data spiflash_data = {
    .name           = "spi0",
    .mode           = FAST_READ_OUTPUT_MODE,//FAST_READ_IO_MODE,
    .sfc_run_mode   = SFC_FAST_READ_DUAL_OUTPUT_MODE,
};

#ifdef CONFIG_UI_ENABLE
static const struct ui_lcd_platform_data pdata = {
#if 0 /*EMI*/
    .cs_pin  = IO_PORTH_00,
    .rs_pin  = IO_PORTH_01,
    .rd_pin  = IO_PORTH_02,
    .rst_pin = IO_PORTH_03,
    .lcd_if  = LCD_EMI,
#else /*PAP*/
    /*.cs_pin  = IO_PORTH_00,*/
    /*.rs_pin  = IO_PORTH_01,*/
    /*.rd_pin  = IO_PORTH_02,*/
    /*.rst_pin = IO_PORTH_03,*/
    /*.lcd_if  = LCD_PAP,*/

    /*IMD RGB*/
    /*.bl_pin = IO_PORTC_04,//RGB的电源背光控制接口*/
    .lcd_if  = LCD_IMD,
#endif
};
const struct ui_devices_cfg ui_cfg_data = {
    .type = TFT_LCD,
    .private_data = (void *)&pdata,
};
#endif

#define EMI_BAUD_DIV	EMI_BAUD_DIV8
static const struct emi_platform_data emi_data = {
	.bits_mode		= EMI_8BITS_MODE,
	.baudrate		= EMI_BAUD_DIV,			//clock = HSB_CLK / (baudrate + 1) , HSB分频
	.colection		= EMI_FALLING_COLT,		//EMI_FALLING_COLT / EMI_RISING_COLT : 下降沿 上升沿 采集数据
	.time_out		= 1*1000,				//最大写超时时间ms
	.data_bit_en 	= 0,					//0默认根据bits_mode数据位来配置
	.th				= EMI_TWIDTH_NO_HALF,	//具体看emi.h
	.ts				= 0,					//具体看emi.h
	.tw				= (EMI_BAUD_DIV > 1) ? EMI_BAUD_DIV / 2 : 1,//具体看emi.h
};
static const struct pap_info pap_data = {
    .datawidth 		= PAP_PORT_8BITS,
    .endian    		= PAP_LE,				//数据输出大小端
    .cycle     		= PAP_CYCLE_ONE,		//1/2字节发送次数
    .pre			= PAP_READ_LOW,			//读取rd有效电平
    .pwe			= PAP_WRITE_LOW,		//写wr有效电平
    .use_sem		= TRUE,					//使用信号等待
	.rd_en			= FALSE,				//不使用rd读信号
    .port_sel		= PAP_PORT_A,			//PAP_PORT_A PAP_PORT_B
    .timing_setup 	= 1,					//具体看pap.h
    .timing_hold  	= 1,					//具体看pap.h
    .timing_width 	= 2,					//具体看pap.h
};
static const struct dac_platform_data dac_data = {
    .pa_mute_port = 0xff,
    .pa_mute_value = 1,
    .differ_output = 0,
    .hw_channel = 0x01,
    .ch_num = 1,	//差分只需开一个通道
    .vcm_init_delay_ms = 1000,
};
static const struct adc_platform_data adc_data = {
    .mic_channel = LADC_CH_MIC1_P_N,
    .mic_ch_num = 1,
    .isel = 2,
};
static const struct audio_pf_data audio_pf_d = {
    .adc_pf_data = &adc_data,
    .dac_pf_data = &dac_data,
};
static const struct audio_platform_data audio_data = {
    .private_data = (void *) &audio_pf_d,
};


#ifdef CONFIG_VIDEO_ENABLE
#define CAMERA_GROUP_PORT	ISC_GROUPA
/* #define CAMERA_GROUP_PORT	ISC_GROUPC */
static const struct camera_platform_data camera0_data = {
    .xclk_gpio      = IO_PORTC_00,//注意： 如果硬件xclk接到芯片IO，则会占用OUTPUT_CHANNEL1
    .reset_gpio     = IO_PORTC_03,
    .online_detect  = NULL,
    .pwdn_gpio      = -1,
    .power_value    = 0,
    .interface      = SEN_INTERFACE0,//SEN_INTERFACE_CSI2,
    .dvp={
#if (CAMERA_GROUP_PORT == ISC_GROUPA)
        .pclk_gpio   = IO_PORTA_08,
        .hsync_gpio  = IO_PORTA_09,
        .vsync_gpio  = IO_PORTA_10,
#else
        .pclk_gpio   = IO_PORTC_08,
        .hsync_gpio  = IO_PORTC_09,
        .vsync_gpio  = IO_PORTC_10,
#endif
		.group_port  = CAMERA_GROUP_PORT,
        .data_gpio={
#if (CAMERA_GROUP_PORT == ISC_GROUPA)
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
#else
                IO_PORTC_07,
                IO_PORTC_06,
                IO_PORTC_05,
                IO_PORTC_04,
                IO_PORTC_03,
                IO_PORTC_02,
                IO_PORTC_01,
                IO_PORTC_00,
				-1,
				-1,
#endif
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
#endif
#ifdef CONFIG_VIDEO1_ENABLE
static const struct camera_platform_data camera1_data = {
    .xclk_gpio      = -1,//IO_PORTB_02,//IO_PORTC_08,//IO_PORTC_06,//注意： 如果硬件xclk接到芯片IO，则会占用OUTPUT_CHANNEL1
    .reset_gpio     = -1,
    .online_detect  = NULL,
    .pwdn_gpio      = -1,
    .power_value    = 0,
    .interface      = -1,
    .dvp={
		.group_port  = -1,
		.pclk_gpio   = -1,
		.hsync_gpio  = -1,
		.vsync_gpio  = -1,
		.data_gpio={-1},
    }
};
static const struct video_subdevice_data video1_subdev_data[] = {
    { VIDEO_TAG_CAMERA, (void *)&camera1_data },
};
static const struct video_platform_data video1_data = {
    .data = video1_subdev_data,
    .num = ARRAY_SIZE(video1_subdev_data),
};
#endif

LED_PLATFORM_DATA_BEGIN(pwm_led_data)
    .led_pin                = IO_PORTB_00,
    .led_cfg.led0_bright    = 2,   //1 ~ 4, value越大, (红灯)亮度越高
    .led_cfg.led1_bright    = 2,   //1 ~ 4, value越大, (蓝灯)亮度越高
    .led_cfg.single_led_slow_freq   = 3,   //1 ~ 8, value越大, LED单独慢闪速度越慢, value * 0.5s闪烁一次
    .led_cfg.single_led_fast_freq   = 3,   //1 ~ 4, value越大, LED单独快闪速度越慢, value * 100ms闪烁一次
    .led_cfg.double_led_slow_freq   = 3,   //1 ~ 8, value越大, LED交替慢闪速度越慢, value * 0.5s闪烁一次
    .led_cfg.double_led_fast_freq   = 3,   //1 ~ 4, value越大, LED交替快闪速度越慢, value * 100ms闪烁一次
LED_PLATFORM_DATA_END()

PWM_PLATFORM_DATA_BEGIN(pwm_data0)
	.port	= 0,
    .pwm_ch = PWMCH0_H | PWMCH0_L,//初始化可选多通道,如:PWMCH0_H | PWMCH0_L | PWMCH1_H ... | PWMCH7_H | PWMCH7_L | PWM_TIMER2_OPCH2 | PWM_TIMER3_OPCH3 ,
    .freq   = 1000,//频率
    .duty   = 50,//占空比
    .point_bit = 0,//根据point_bit值调节占空比小数点精度位: 0<freq<=4K,point_bit=2;4K<freq<=40K,point_bit=1; freq>40K,point_bit=0;
PWM_PLATFORM_DATA_END()

PWM_PLATFORM_DATA_BEGIN(pwm_data1)
	.port	= IO_PORTA_03,//选择定时器的TIMER PWM任意IO，pwm_ch加上PWM_TIMER3_OPCH3或PWM_TIMER2_OPCH2有效,只支持2个PWM,占用output_channel2/3，其他外设使用output_channel需留意
    .pwm_ch = PWM_TIMER2_OPCH2,//初始化可选多通道,如:PWMCH0_H | PWMCH0_L | PWMCH1_H ... | PWMCH7_H | PWMCH7_L | PWM_TIMER2_OPCH2 | PWM_TIMER3_OPCH3 ,
    .freq   = 1000,//频率
    .duty   = 50,//占空比
    .point_bit = 0,//根据point_bit值调节占空比小数点精度位: 0<freq<=4K,point_bit=2;4K<freq<=40K,point_bit=1; freq>40K,point_bit=0;
PWM_PLATFORM_DATA_END()

#if TCFG_USB_SLAVE_ENABLE || TCFG_USB_HOST_ENABLE
static const struct otg_dev_data otg_data = {
    .usb_dev_en = 0x01,
#if TCFG_USB_SLAVE_ENABLE
    .slave_online_cnt = 10,
    .slave_offline_cnt = 10,
#endif
#if TCFG_USB_HOST_ENABLE
    .host_online_cnt = 10,
	.host_offline_cnt = 10,
#endif
	.detect_mode = OTG_HOST_MODE | OTG_SLAVE_MODE | OTG_CHARGE_MODE,
    .detect_time_interval = 50,
};
#endif


#ifdef CONFIG_LTE_PHY_ENABLE
extern const struct device_operations lte_module_dev_ops;
LTE_MODULE_DATA_BEGIN(lte_module_data)
	.name = (u8 *)"usb_rndis",
LTE_MODULE_DATA_END()
#endif


#if defined CONFIG_BT_ENABLE || defined CONFIG_WIFI_ENABLE
#include "wifi/wifi_connect.h"
const struct wifi_calibration_param wifi_calibration_param = {
    .xosc_l     = 0xb,// 调节左晶振电容
    .xosc_r     = 0xa,// 调节右晶振电容
    .pa_trim_data  ={1, 5, 6, 6, 9, 0, 7},// 根据MP测试生成PA TRIM值
	.mcs_dgain     ={
		55,//11B_1M
	  	55,//11B_2.2M
	  	55,//11B_5.5M
		55,//11B_11M

		64,//11G_6M
		64,//11G_9M
		64,//11G_12M
		64,//11G_18M
		64,//11G_24M
		58,//11G_36M
		50,//11G_48M
		36,//11G_54M

		64,//11N_MCS0
		64,//11N_MCS1
		64,//11N_MCS2
		64,//11N_MCS3
		60,//11N_MCS4
		48,//11N_MCS5
		36,//11N_MCS6
		32,//11N_MCS7
	}
};
#endif

REGISTER_DEVICES(device_table) = {
    { "pap",   &pap_dev_ops, (void *)&pap_data},
    { "emi",   &emi_dev_ops, (void *)&emi_data},
    { "imd",   &imd_dev_ops, NULL},
	{ "pwm0",   &pwm_dev_ops,  (void *)&pwm_data0},
	{ "pwm1",   &pwm_dev_ops,  (void *)&pwm_data1},

	{ "iic0",  &iic_dev_ops, (void *)&sw_iic0_data },
	{ "iic1",  &iic_dev_ops, (void *)&hw_iic1_data },

	{ "audio", &audio_dev_ops, (void *)&audio_data },

#if TCFG_SD0_ENABLE
	{ "sd0",  &sd_dev_ops, (void *)&sd0_data },
#endif

#if TCFG_SD1_ENABLE
	{ "sd1",  &sd_dev_ops, (void *)&sd1_data },
#endif

#ifdef CONFIG_VIDEO_ENABLE
    { "video0.*",  &video_dev_ops, (void *)&video0_data },
#endif

#ifdef CONFIG_VIDEO1_ENABLE
    { "video1.*",  &video_dev_ops, (void *)&video1_data },//vdieo1为spi摄像头
    { "video3.*",  &video_dev_ops, (void *)&video1_data },//video3为双路(jpeg+yuv)dvp摄像头
#endif

#ifdef CONFIG_VIDEO_DEC_ENABLE
    { "video_dec",  &video_dev_ops, NULL },
#endif

#ifdef CONFIG_UVC_VIDEO2_ENABLE
    {"uvc", &uvc_dev_ops, NULL},
    { "video2.*",  &video_dev_ops, NULL },
#endif

#ifndef CONFIG_SFC_ENABLE
    { "spi0", &spi_dev_ops, (void *)&spi0_data },
    { "spiflash", &spiflash_dev_ops, (void *)&spiflash_data },
#endif
    { "spi1", &spi_dev_ops, (void *)&spi1_data },
    { "spi2", &spi_dev_ops, (void *)&spi2_data },
	{"rtc", &rtc_dev_ops, NULL},
	{"uart0", &uart_dev_ops, (void *)&uart0_data },
	{"uart1", &uart_dev_ops, (void *)&uart1_data },
	{"uart2", &uart_dev_ops, (void *)&uart2_data },

#if TCFG_UDISK_ENABLE
    { "udisk0", &mass_storage_ops, NULL },
#endif

#if TCFG_USB_SLAVE_ENABLE || TCFG_USB_HOST_ENABLE
	{ "otg", &usb_dev_ops, (void *)&otg_data},
#endif

#ifdef CONFIG_LTE_PHY_ENABLE
    { "lte",  &lte_module_dev_ops, (void *) &lte_module_data},
#endif
};


/************************** LOW POWER config ****************************/
static const struct low_power_param power_param = {
    .config         = TCFG_LOWPOWER_LOWPOWER_SEL,          //系统休眠
    .btosc_disable  = TCFG_LOWPOWER_BTOSC_DISABLE,         //进入低功耗时BTOSC是否保持
    .vddiom_lev     = TCFG_LOWPOWER_VDDIOM_LEVEL,          //强VDDIO等级,可选：2.8V  2.9V  3.0V  3.1V  3.2V  3.3V  3.4V  3.5V
    .vddiow_lev     = TCFG_LOWPOWER_VDDIOW_LEVEL,          //弱VDDIO等级,可选：2.1V  2.4V  2.8V  3.2V
	.vdc14_dcdc 	= TRUE,	   							   //打开内部1.4VDCDC，关闭则用外部
    .vdc14_lev		= VDC14_VOL_SEL_LEVEL, 				   //VDD1.4V配置
	.sysvdd_lev		= SYSVDD_VOL_SEL_LEVEL,				   //内核、sdram电压配置
	.vlvd_enable	= TRUE,                                //TRUE电压复位使能
	.vlvd_value		= VLVD_SEL_25V,                        //低电压复位电压值
};



/************************** PWR config ****************************/
//#define PORT_VCC33_CTRL_IO		IO_PORTA_03					//VCC33 DCDC控制引脚,该引脚控制DCDC器件输出的3.3V连接芯片HPVDD、VDDIO、VDD33
#define PORT_WAKEUP_IO			IO_PORTB_01					//软关机和休眠唤醒引脚
#define PORT_WAKEUP_NUM			(PORT_WAKEUP_IO/IO_GROUP_NUM)//默认:0-7:GPIOA-GPIOH, 可以指定0-7组

static const struct port_wakeup port0 = {
    .edge       = FALLING_EDGE,                            //唤醒方式选择,可选：上升沿\下降沿
    .attribute  = BLUETOOTH_RESUME,                        //保留参数
    .iomap      = PORT_WAKEUP_IO,                          //唤醒口选择
    .low_power	= POWER_SLEEP_WAKEUP|POWER_OFF_WAKEUP,    //低功耗IO唤醒,不需要写0
};

static const struct long_press lpres_port = {
	.enable 	= FALSE,
	.use_sec4 	= TRUE,										//enable = TRUE , use_sec4: TRUE --> 4 sec , FALSE --> 8 sec
	.edge		= FALLING_EDGE,								//长按方式,可选：FALLING_EDGE /  RISING_EDGE --> 低电平/高电平
	.iomap 		= PORT_WAKEUP_IO,							//长按复位IO和IO唤醒共用一个IO
};

static const struct sub_wakeup sub_wkup = {
    .attribute  = BLUETOOTH_RESUME,
};

static const struct charge_wakeup charge_wkup = {
    .attribute  = BLUETOOTH_RESUME,
};

static const struct wakeup_param wk_param = {
    .port[PORT_WAKEUP_NUM] = &port0,
    .sub = &sub_wkup,
    .charge = &charge_wkup,
	.lpres = &lpres_port,
};

unsigned char PWR_CTL(unsigned char on_off)
{
    return 0;
}

#define USB_WKUP_IO 	0//IO_PORT_PR_01
unsigned char usb_is_charging()
{
#if 0
    static unsigned char init = 0;
    if (!init) {
        init = 1;
        gpio_latch_en(USB_WKUP_IO, 0);
        gpio_direction_input(USB_WKUP_IO);
        gpio_set_pull_up(USB_WKUP_IO, 0);
        gpio_set_pull_down(USB_WKUP_IO, 0);
        gpio_set_die(USB_WKUP_IO, 1);
        delay(10);
    }

    return (gpio_read(USB_WKUP_IO));//no usb charing == false
#else
    return 0;
#endif
}

unsigned char read_power_key()
{
	unsigned char usb_wk;
	static unsigned char pkey_init = 0;
	if (!pkey_init) {
	    pkey_init = 1;
	    gpio_latch_en(PORT_WAKEUP_IO, 0);
	    gpio_direction_input(PORT_WAKEUP_IO);
	    gpio_set_pull_up(PORT_WAKEUP_IO, 0);
	    gpio_set_pull_down(PORT_WAKEUP_IO, 0);
	    gpio_set_die(PORT_WAKEUP_IO, 1);
	    delay(200);
	}
	delay_us(3000);
	return !gpio_read(PORT_WAKEUP_IO);
    return 0;
}

unsigned int get_usb_wkup_gpio()
{
    return (USB_WKUP_IO);
}

/*进软关机之前默认将IO口都设置成高阻状态，需要保留原来状态的请修改该函数*/
static void board_set_soft_poweroff(void)
{
    u32 IO_PORT;
    JL_PORT_FLASH_TypeDef *gpio[] = {JL_PORTA, JL_PORTB, JL_PORTC, JL_PORTD, JL_PORTE, JL_PORTF, JL_PORTG, JL_PORTH};

    for (u8 p = 0; p < 8; ++p) {
        //flash sdram PD PE PF PG口不能进行配置,由内部完成控制
        if (gpio[p] == JL_PORTD || gpio[p] == JL_PORTE || gpio[p] == JL_PORTF || gpio[p] == JL_PORTG) {
            continue;
        }
        for (u8 i = 0; i < 16; ++i) {
            IO_PORT = IO_PORTA_00 + p * 16 + i;
            gpio_set_pull_up(IO_PORT, 0);
            gpio_set_pull_down(IO_PORT, 0);
            gpio_set_direction(IO_PORT, 1);
            gpio_set_die(IO_PORT, 0);
            gpio_set_dieh(IO_PORT, 0);
			gpio_set_hd(IO_PORT, 0);
            gpio_set_hd1(IO_PORT, 0);
            gpio_latch_en(IO_PORT, 1);
        }
    }
#ifdef PORT_VCC33_CTRL_IO
    gpio_latch_en(PORT_VCC33_CTRL_IO, 0);
    gpio_direction_output(PORT_VCC33_CTRL_IO, 0);
    gpio_set_pull_up(PORT_VCC33_CTRL_IO, 0);
    gpio_set_pull_down(PORT_VCC33_CTRL_IO, 1);
    gpio_set_direction(PORT_VCC33_CTRL_IO, 1);
    gpio_set_die(PORT_VCC33_CTRL_IO, 0);
    gpio_set_dieh(PORT_VCC33_CTRL_IO, 0);
    gpio_latch_en(PORT_VCC33_CTRL_IO, 1);
#endif
}

static void sleep_exit_callback(u32 usec)
{
#ifdef PORT_VCC33_CTRL_IO
	gpio_direction_output(PORT_VCC33_CTRL_IO, 1);
	gpio_set_pull_up(PORT_VCC33_CTRL_IO, 1);
	gpio_set_pull_down(PORT_VCC33_CTRL_IO,0);
#endif
    /* putchar('-'); */
}

static void sleep_enter_callback(u8  step)
{

    /* 此函数禁止添加打印 */

    if (step == 1) {
        /*dac_power_off();*/
    } else {
#ifdef PORT_VCC33_CTRL_IO
		gpio_direction_output(PORT_VCC33_CTRL_IO, 0);
		gpio_set_pull_up(PORT_VCC33_CTRL_IO, 0);
		gpio_set_pull_down(PORT_VCC33_CTRL_IO, 1);
		gpio_set_direction(PORT_VCC33_CTRL_IO, 1);
		gpio_set_die(PORT_VCC33_CTRL_IO, 0);
#endif
	}
}

static void board_power_init(void)
{
	gpio_direction_input(IO_PORTB_08);
	gpio_set_pull_up(IO_PORTB_08,0);
	gpio_set_pull_down(IO_PORTB_08,0);

	gpio_direction_input(IO_PORTC_05);
	gpio_set_pull_up(IO_PORTC_05,0);
	gpio_set_pull_down(IO_PORTC_05,0);

	gpio_direction_input(IO_PORTH_07);
	gpio_set_pull_up(IO_PORTH_07,0);
	gpio_set_pull_down(IO_PORTH_07,0);

	gpio_direction_input(IO_PORTH_06);
	gpio_set_pull_up(IO_PORTH_06,0);
	gpio_set_pull_down(IO_PORTH_06,0);

#ifdef PORT_VCC33_CTRL_IO
	gpio_direction_output(PORT_VCC33_CTRL_IO, 1);
    gpio_set_pull_up(PORT_VCC33_CTRL_IO, 1);
	gpio_set_pull_down(PORT_VCC33_CTRL_IO,0);
#endif

    power_init(&power_param);

    power_set_callback(TCFG_LOWPOWER_LOWPOWER_SEL, sleep_enter_callback, sleep_exit_callback, board_set_soft_poweroff);

    power_keep_state(POWER_KEEP_RESET);//0, POWER_KEEP_DACVDD | POWER_KEEP_RTC | POWER_KEEP_RESET

#ifdef CONFIG_OSC_RTC_ENABLE
    power_keep_state(POWER_KEEP_RTC);//0, POWER_KEEP_DACVDD | POWER_KEEP_RTC | POWER_KEEP_RESET
#endif

    power_wakeup_init(&wk_param);
}

#ifdef CONFIG_DEBUG_ENABLE
void debug_uart_init()
{
    uart_init(&uart2_data);
}
#endif

void board_early_init()
{
    dac_early_init(0, 0, 1000);
    devices_init();
}

void board_init()
{
#ifdef CONFIG_OSC_RTC_ENABLE
    rtc_early_init(1);
#else
    rtc_early_init(0);
#endif

	board_power_init();

    adc_init();

#if (TCFG_IOKEY_ENABLE || TCFG_IRKEY_ENABLE || TCFG_RDEC_KEY_ENABLE || TCFG_ADKEY_ENABLE)
	key_driver_init();
#endif

    //摄像头的复位脚引用了DACVDD，需要先初始化DAC后才能初始化摄像头
    extern void vcm_early_init(u32 delay_ms);
    vcm_early_init(1000);
}


#endif

