#ifndef DEVICE_TOUCH_KEY_H
#define DEVICE_TOUCH_KEY_H

#include "generic/typedef.h"

//计数参考时钟选择, 频率越高, 精度越高
enum touch_key_clk {
    TOUCH_KEY_OSC_CLK = 0,
    TOUCH_KEY_MUX_IN_CLK,  //外部输入, ,一般不用, 保留
    TOUCH_KEY_PLL_192M_CLK,
    TOUCH_KEY_PLL_240M_CLK,
};

struct touch_key_port {
    u8 port; 			//触摸按键IO
    u8 key_value; 		//按键返回值
};

struct touch_key_platform_data {
    u8 num; 	//触摸按键个数;
    u8 clock; 	//触摸按键选择时钟;
    u8 change_gain; 	//变化放大倍数
    s16 press_cfg;  	//按下判决门限
    s16 release_cfg0; 	//释放判决门限0
    s16 release_cfg1; 	//释放判决门限1
    const struct touch_key_port *port_list; //触摸按键列表
};

/* =========== touch key API ============= */
//触摸按键初始化
int touch_key_init(const struct touch_key_platform_data *touch_key_data);

//获取触摸按键键值
u8 touch_key_get_value(void);

#endif

