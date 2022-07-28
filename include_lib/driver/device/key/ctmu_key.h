#ifndef DEVICE_CTMU_KEY_H
#define DEVICE_CTMU_KEY_H

#include "generic/typedef.h"

struct ctmu_key_port {
    u8 port; 			//触摸按键IO
    u8 key_value; 		//按键返回值
};

struct ctmu_touch_key_platform_data {
    u8 num; 	//触摸按键个数
    s16 press_cfg;  	//按下判决门限
    s16 release_cfg0; 	//释放判决门限0
    s16 release_cfg1; 	//释放判决门限1
    const struct ctmu_key_port *port_list;
};

/* =========== ctmu API ============= */
//ctmu 初始化
int ctmu_touch_key_init(const struct ctmu_touch_key_platform_data *ctmu_touch_key_data);

//获取plcnt按键状态
u8 ctmu_touch_key_get_value(void);

#endif

