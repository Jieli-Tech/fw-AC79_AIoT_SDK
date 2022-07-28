#ifndef _PWM_LED_H_
#define _PWM_LED_H_

/*******************************************************************
*   本文件为LED灯配置的接口头文件
*
*	约定:
*		1)两盏灯为单IO双LED接法;
*		2)LED0: RLED, 蓝色, 低电平亮灯;
*		3)LED1: BLED, 红色, 高电平亮灯;
*********************************************************************/

// LED实现的效果有:
// 1.两盏LED全亮;
// 2.LED单独亮灭;
// 3.LED单独慢闪和快闪;
// 4.LED 5s内单独闪一次和两次;
// 5.LED交替快闪和慢闪;
// 6.LED单独呼吸;
// 7.LED交替呼吸

/*
 * 	LED各个效果可供配置以下参数, 请按照参数后面的注释说明的范围进行配置
 */

/***************** LED0/LED1单独每隔5S双闪时, 可供调节参数 ********************/
#define LED0_DOUBLE_FLASH_5S_1ST_TIME_CONFIG		10  //第一次亮时间时长, 设置范围: 1~20, 值越大, 时间越长, 步调:20ms, 调节范围:20ms ~ 400ms
#define LED0_DOUBLE_FLASH_5S_TIME_GAP_CONFIG		10   //两个亮灯时间间隔, 设置范围: 1~20, 值越大, 熄灭越长, 步调:20ms, 调节范围:20ms ~ 400ms
#define LED0_DOUBLE_FLASH_5S_2ND_TIME_CONFIG		10   //第二次亮灯时长, 设置范围: 1~20, 值越大, 时间越长, 步调:20ms, 调节范围:20ms ~ 400ms

#define LED1_DOUBLE_FLASH_5S_1ST_TIME_CONFIG		10  //第一次亮时间时长, 设置范围: 1~20, 值越大, 时间越长, 步调:20ms, 调节范围:20ms ~ 400ms
#define LED1_DOUBLE_FLASH_5S_TIME_GAP_CONFIG		10   //两个亮灯时间间隔, 设置范围: 1~20, 值越大, 熄灭越长, 步调:20ms, 调节范围:20ms ~ 400ms
#define LED1_DOUBLE_FLASH_5S_2ND_TIME_CONFIG		10   //第二次亮灯时长, 设置范围: 1~20, 值越大, 时间越长, 步调:20ms, 调节范围:20ms ~ 400ms


/***************** LED0/LED1单独每隔5S单闪时, 可供调节参数 ********************/
#define LED0_ONE_FLASH_5S_LIGHT_TIME_CONFIG			10		//LED0亮时间时长, 设置范围: 1~20, 值越大, 亮灯时间越长, 步调:20ms, 调节范围:20ms ~ 400ms
#define LED1_ONE_FLASH_5S_LIGHT_TIME_CONFIG			10		//LED1亮时间时长, 设置范围: 1~20, 值越大, 亮灯时间越长, 步调:20ms, 调节范围:20ms ~ 400ms


/***************** LED0/LED1呼吸时, 可供调节参数 ********************/
#define LED_BREATHE_MAX_CLASS_CONFIG 				250 //LED呼吸到极限亮度级数, 设置范围:1~255, 呼吸到这个极限亮度需要的时间 = MAX_CLASS * 12.5ms
#define LED0_BREATHE_HIGHTEST_BRIGHT_CONFIG			100	//LED0呼吸最高亮度(呼吸时间), 期望亮度 = (HIGHTEST_BRIGHT_CONFIG)/ MAX_CLASS), 设置范围:1~MAX_CLASS, 呼吸时间调节步调:12.5ms, 呼吸时间 = 12.5ms * HIGHTEST_BRIGHT_CONFIG
#define LED1_BREATHE_HIGHTEST_BRIGHT_CONFIG			100	//LED1呼吸最高亮度(呼吸时间), 期望亮度 = (HIGHTEST_BRIGHT_CONFIG)/ MAX_CLASS), 设置范围:1~MAX_CLASS, 呼吸时间调节步调:12.5ms, 呼吸时间 = 12.5ms * HIGHTEST_BRIGHT_CONFIG
#define LED_BREATHE_BLANK_DELAY_TIME_CONFIG			100  //呼吸灭灯时间延长, 设置范围:1~255, 值越大, 灭灯时间越长, 调节步调: 12.5ms, 调节范围: 12.5ms ~ 3.2s
//单独控制LED的最高亮度
#define LED0_BREATHE_LIGHT_DELAY_TIME_CONFIG		10  //LED0呼吸到最亮时间延长, 1~255, 值越大, 亮灯灯时间越长, 调节步调: 12.5ms, 调节范围: 12.5ms ~ 3.2s
#define LED1_BREATHE_LIGHT_DELAY_TIME_CONFIG		10  //LED1呼吸到最亮时间延长, 1~255, 值越大, 亮灯灯时间越长, 调节步调: 17ms, 调节范围: 12.5ms ~ 3.2s

/***************** LED0和LED1交替呼吸时, 可供调节参数 ********************/
#define	LED0_LED1_BREATHE_TRIG_PEROID_CONFIG		0  //0: 全周期变色, 1:半周期变色, 注: 半周期为从灭到最亮; 而全周期是指灭->最亮->灭(一呼一吸)


enum pwm_led_clk_source {
    PWM_LED_CLK_OSC32K,  	//no use
    PWM_LED_CLK_RC32K, 		//use
    PWM_LED_CLK_BTOSC_12M, 	//no use
    PWM_LED_CLK_RCLK_250K,  //no use
    PWM_LED_CLK_BTOSC_24M,  //use
};

enum pwm_led_mode {
    PWM_LED_MODE_START,

    PWM_LED_ALL_OFF,         		//mode1: 全灭
    PWM_LED_ALL_ON,              	//mode2: 全亮

    PWM_LED0_ON,             		//mode3: 蓝亮
    PWM_LED0_OFF,            		//mode4: 蓝灭
    PWM_LED0_SLOW_FLASH,           	//mode5: 蓝慢闪
    PWM_LED0_FAST_FLASH,           	//mode6: 蓝快闪
    PWM_LED0_DOUBLE_FLASH_5S,  		//mode7: 蓝灯5秒连闪两下
    PWM_LED0_ONE_FLASH_5S,    		//mode8: 蓝灯5秒连闪1下

    PWM_LED1_ON,             		//mode9:  红亮
    PWM_LED1_OFF,            		//mode10: 红灭
    PWM_LED1_SLOW_FLASH,           	//mode11: 红慢闪
    PWM_LED1_FAST_FLASH,            //mode12: 红快闪
    PWM_LED1_DOUBLE_FLASH_5S,  		//mode13: 红灯5秒连闪两下
    PWM_LED1_ONE_FLASH_5S,    		//mode14: 红灯5秒闪1下

    PWM_LED0_LED1_FAST_FLASH,   	//mode15: 红蓝交替闪（快闪）
    PWM_LED0_LED1_SLOW_FLASH, 		//mode16: 红蓝交替闪（慢闪）

    PWM_LED0_BREATHE,				//mode17: 蓝灯呼吸灯模式
    PWM_LED1_BREATHE,				//mode18: 红灯呼吸灯模式
    PWM_LED0_LED1_BREATHE,			//mode19: 红蓝交替呼吸灯模式

    PWM_LED_MODE_END,

    PWM_LED1_FLASH_THREE,           //自定义状态，不能通过pmd_led_mode去设置

    PWM_LED_NULL = 0xFF,
};

struct pwm_led_cfg {
    u8 led0_bright;   //1 ~ 4, 值越大, (蓝灯)亮度越高
    u8 led1_bright;   //1 ~ 4, 值越大,(红灯) 亮度越高
    u8 single_led_slow_freq;  //1 ~ 8, 值越大, LED单独慢闪速度越慢, value * 0.5s闪烁一次
    u8 single_led_fast_freq;  //1 ~ 4, 值越大, LED单独快速度越慢, value * 100ms闪烁一次
    u8 double_led_slow_freq;  //1 ~ 8, 值越大, LED交替慢闪速度越慢, value * 0.5s闪烁一次
    u8 double_led_fast_freq;  //1 ~ 4, 值越大, LED交替快速度越慢, value * 100ms闪烁一次
};

struct pwm_led_two_io_mode {
    u8 two_io_mode_enable;
    u8 led0_pin;
    u8 led1_pin;
};

struct led_platform_data {
    u8 led_pin;
    struct pwm_led_cfg led_cfg;
    struct pwm_led_two_io_mode two_io;
};

#define LED_PLATFORM_DATA_BEGIN(data) \
		const struct led_platform_data data = {

#define LED_PLATFORM_DATA_END() \
};

extern const struct device_operations pwm_led_ops;


void pwm_led_mode_set(u8 fre_mode);

//LED时钟源切换, input: PWM_LED_CLK_RC32K or PWM_LED_CLK_BTOSC_24M
void pwm_led_clk_set(enum pwm_led_clk_source src);

//闪烁状态复位, 重新开始一个周期
void pwm_led_display_mode_reset(void);

//获取led当前的闪烁模式
enum pwm_led_mode pwm_led_display_mode_get(void);

#endif //_PWM_LED_H_
