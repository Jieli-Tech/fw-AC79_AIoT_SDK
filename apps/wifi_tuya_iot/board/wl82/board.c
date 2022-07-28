#include "app_config.h"

#include "system/includes.h"
#include "device/includes.h"
#include "asm/includes.h"

// *INDENT-OFF*
/** 业务串口*/
UART1_PLATFORM_DATA_BEGIN(uart1_data)
     .baudrate = 9600,
     .port = PORT_REMAP,
     .tx_pin = IO_PORTC_09,
     .rx_pin = IO_PORTC_10,
     .output_channel = OUTPUT_CHANNEL3,
     .input_channel = INPUT_CHANNEL3,
     .max_continue_recv_cnt = 1024,
     .idle_sys_clk_cnt = 500000,
     .clk_src = PLL_48M,
     .disable_tx_irq=1,
UART1_PLATFORM_DATA_END();

/** 调试串口*/
UART2_PLATFORM_DATA_BEGIN(uart2_data)
	.baudrate = 1000000,
	.port = PORT_REMAP,
	.output_channel = OUTPUT_CHANNEL0,
	.tx_pin = IO_PORTC_00,
	.rx_pin = -1,
	.max_continue_recv_cnt = 1024,
	.idle_sys_clk_cnt = 500000,
	.clk_src = PLL_48M,
	.flags = UART_DEBUG,
UART2_PLATFORM_DATA_END();

/** ad key*/
#if TCFG_ADKEY_ENABLE
/*-------------ADKEY GROUP 1----------------*/
#define ADKEY_UPLOAD_R  22

#define ADC_VDDIO (0x3FF)
#define ADC_09   (0x3FF * 220 / (220 + ADKEY_UPLOAD_R))
#define ADC_08   (0x3FF * 100 / (100 + ADKEY_UPLOAD_R))
#define ADC_07   (0x3FF * 51  / (51  + ADKEY_UPLOAD_R))
#define ADC_06   (0x3FF * 33  / (33  + ADKEY_UPLOAD_R))
#define ADC_05   (0x3FF * 22  / (22  + ADKEY_UPLOAD_R))
#define ADC_04   (0x3FF * 15  / (15  + ADKEY_UPLOAD_R))
#define ADC_03   (0x3FF * 10  / (10  + ADKEY_UPLOAD_R))
#define ADC_02   (0x3FF * 51  / (51  + ADKEY_UPLOAD_R * 10))
#define ADC_01   (0x3FF * 22  / (22  + ADKEY_UPLOAD_R * 10))
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

const struct adkey_platform_data adkey_data = {
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

/************************** LOW POWER config ****************************/
static const struct low_power_param power_param = {
    .config         = TCFG_LOWPOWER_LOWPOWER_SEL,
    .btosc_disable  = TCFG_LOWPOWER_BTOSC_DISABLE,         //进入低功耗时BTOSC是否保持
    .vddiom_lev     = TCFG_LOWPOWER_VDDIOM_LEVEL,          //强VDDIO等级,可选：2.2V  2.4V  2.6V  2.8V  3.0V  3.2V  3.4V  3.6V
    .vddiow_lev     = TCFG_LOWPOWER_VDDIOW_LEVEL,          //弱VDDIO等级,可选：2.1V  2.4V  2.8V  3.2V
	.vdc14_dcdc 	= TRUE,	   							   //打开内部1.4VDCDC，关闭则用外部
    .vdc14_lev		= VDC14_VOL_SEL_LEVEL, 				   //VDD1.4V配置
	.sysvdd_lev		= SYSVDD_VOL_SEL_LEVEL,				   //内核、sdram电压配置
	.vlvd_enable	= TRUE,                                //TRUE电压复位使能
	.vlvd_value		= VLVD_SEL_25V,                        //低电压复位电压值
};

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
	{"uart2", &uart_dev_ops, (void *)&uart2_data },
	{"uart1", &uart_dev_ops, (void *)&uart1_data },
	{"rtc", &rtc_dev_ops, NULL},
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
	adc_init();

#if TCFG_ADKEY_ENABLE
    key_driver_init();
#endif
}


