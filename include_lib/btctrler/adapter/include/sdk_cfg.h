/*********************************************************************************************
    *   Filename        : sdk_cfg.h

    *   Description     :

    *   Author          : Bingquan

    *   Email           : bingquan_cai@zh-jieli.com

    *   Last modifiled  : 2017-02-03 15:28

    *   Copyright:(c)JIELI  2011-2016  @ , All Rights Reserved.
*********************************************************************************************/
#ifndef ADAPTER_SDK_CONFIG_
#define ADAPTER_SDK_CONFIG_



/********************************************************************************/
/*
 *           --------蓝牙射频类配置
 */

///<配置蓝牙射频发射功率,可选配置：0~15
#ifndef APP_BT_RF_TXPOWER
#define APP_BT_RF_TXPOWER               7
#endif

///<配置蓝牙频偏校准电容(L/R),推荐配置：0x10/0x11/0x12
#ifndef APP_BT_INTERNAL_CAP
#define APP_BT_INTERNAL_CAP             0x11, 0x11
#endif

///<配置蓝牙MAC地址,6 bytes
#ifndef APP_BT_MAC_ADDR
#define APP_BT_MAC_ADDR                 0x20, 0x22, 0x33, 0x44, 0x55, 0x66
#warning "APP_BT_MAC_ADDR default setting (0x20, 0x22, 0x33, 0x44, 0x55, 0x66)"
#endif

/********************************************************************************/
/*
 *           --------系统低功耗设置
 */
///<   ______                  ______
///<___|    |__________________|    |___________
//      9mA        300uA
//      3ms         17ms
///<BLE 功耗估算公式：(3/20*9mA + 17/20*0.3mA) ~= 1.6mA/h
//
#ifndef APP_LOWPOWER_BTOSC_DISABLE
#define APP_LOWPOWER_BTOSC_DISABLE      0
#endif

///<  0:  不使用低功耗模式
///<  BT_SLEEP_EN:  使用睡眠模式(低功耗)
//
///配置低功耗模式,可选配置：0/SLEEP_EN
#ifndef APP_LOWPOWER_SELECT
#define APP_LOWPOWER_SELECT             SLEEP_EN
#endif

///<  BT_OSC:   使用蓝牙晶振作为低功耗运行时钟
///<  RTC_OSCH: 使用低速RTC晶振作为低功耗运行时钟
///<  RTC_OSCL: 使用高速蓝牙晶振作为低功耗运行时钟
///<  LRC_32K:  使用RC晶振作为低功耗运行时钟

///<配置低功耗模式使用的晶振,可选配置：BT_OSC/RTC_OSCH/RTC_OSCL
#ifndef APP_LOWPOWER_SELECT
#define APP_LOWPOWER_SELECT             BT_OSC
#endif

///<配置晶振频率,单位赫兹,可选配置：24000000L
#ifndef APP_LOWPOWER_OSC_HZ
#define APP_LOWPOWER_OSC_HZ             24000000L
#endif

///<配置系统定时器运行节拍,单位毫秒,可选配置：10~1000
#ifndef APP_TICKS_UNIT
#define APP_TICKS_UNIT                  10
#endif

#endif


