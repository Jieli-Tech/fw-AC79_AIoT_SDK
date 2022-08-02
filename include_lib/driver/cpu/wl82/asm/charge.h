#ifndef _CHARGE_H_
#define _CHARGE_H_

#include "typedef.h"

/*------充满电电压选择 3.869V-4.567V-------*/
#define CHARGE_FULL_V_3869		0
#define CHARGE_FULL_V_3907		1
#define CHARGE_FULL_V_3946		2
#define CHARGE_FULL_V_3985		3
#define CHARGE_FULL_V_4026		4
#define CHARGE_FULL_V_4068		5
#define CHARGE_FULL_V_4122		6
#define CHARGE_FULL_V_4157		7
#define CHARGE_FULL_V_4202		8
#define CHARGE_FULL_V_4249		9
#define CHARGE_FULL_V_4295		10
#define CHARGE_FULL_V_4350		11
#define CHARGE_FULL_V_4398		12
#define CHARGE_FULL_V_4452		13
#define CHARGE_FULL_V_4509		14
#define CHARGE_FULL_V_4567		15
/*****************************************/

/*充满电电流选择 2mA-30mA*/
#define CHARGE_FULL_mA_2		0
#define CHARGE_FULL_mA_5		1
#define CHARGE_FULL_mA_7	 	2
#define CHARGE_FULL_mA_10		3
#define CHARGE_FULL_mA_15		4
#define CHARGE_FULL_mA_20		5
#define CHARGE_FULL_mA_25		6
#define CHARGE_FULL_mA_30		7

/*
 	充电电流选择
	恒流：20-220mA
*/
#define CHARGE_mA_20			0
#define CHARGE_mA_30			1
#define CHARGE_mA_40			2
#define CHARGE_mA_50			3
#define CHARGE_mA_60			4
#define CHARGE_mA_70			5
#define CHARGE_mA_80			6
#define CHARGE_mA_90			7
#define CHARGE_mA_100			8
#define CHARGE_mA_110			9
#define CHARGE_mA_120			10
#define CHARGE_mA_140			11
#define CHARGE_mA_160			12
#define CHARGE_mA_180			13
#define CHARGE_mA_200			14
#define CHARGE_mA_220			15

#define DEVICE_EVENT_FROM_CHARGE	(('C' << 24) | ('H' << 16) | ('G' << 8) | '\0')
#define CHARGE_CCVOL_V		300		//最低充电电流档转向用户设置充电电流档的电压转换点(AC693X无涓流充电，电池电压低时采用最低电流档，电池电压大于设置的值时采用用户设置的充电电流档)


struct charge_platform_data {
    u8 charge_en;	//内置充电使能
    u8 charge_poweron_en;	//开机充电使能
    u8 charge_full_V;	//充满电电压大小
    u8 charge_full_mA;	//充满电电流大小
    u8 charge_mA;	//充电电流大小
};

#define CHARGE_PLATFORM_DATA_BEGIN(data) \
		struct charge_platform_data data  = {

#define CHARGE_PLATFORM_DATA_END()  \
};


enum {
    CHARGE_FULL_33V = 0,	//充满标记位
    TERMA_33V,				//模拟测试信号
    VBGOK_33V,				//模拟测试信号
    CICHARGE_33V,			//涓流转恒流信号
};

enum {
    CHARGE_EVENT_CHARGE_START,
    CHARGE_EVENT_CHARGE_CLOSE,
    CHARGE_EVENT_CHARGE_FULL,
    CHARGE_EVENT_CHARGE_ERR,
    CHARGE_EVENT_LDO5V_IN,
    CHARGE_EVENT_LDO5V_OFF,
};


extern void set_charge_online_flag(u8 flag);
extern u8 get_charge_online_flag(void);
extern u8 get_charge_poweron_en(void);
extern void charge_start(void);
extern void charge_close(void);
extern const struct device_operations charge_dev_ops;

#endif    //_CHARGE_H_
