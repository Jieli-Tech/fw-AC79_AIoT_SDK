/*
 * @Author: wls
 * @email: wuls@tuya.com
 * @LastEditors: wls
 * @file name: light_control.h
 * @Description: light control include file
 * @Copyright: HANGZHOU TUYA INFORMATION TECHNOLOGY CO.,LTD
 * @Company: http://www.tuya.com
 * @Date: 2019-04-26 13:55:40
 * @LastEditTime: 2019-05-21 10:13:39
 */

#ifndef __LIHGT_SYSTEM_H__
#define __LIHGT_SYSTEM_H__


#include "light_types.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief wifi(realtek 8710bn) system reboot reason
 */
typedef enum {
    REASON_DEFAULT_RST = 0,         /* normal startup by power on */
    REASON_WDT_RST,                 /* hardware watch dog reset */
    REASON_EXCEPTION_RST,           /* exception reset, GPIO status won't change */
    REASON_SOFT_WDT_RST,            /* software watch dog reset, GPIO status won't change */
    REASON_SOFT_RESTART = 9,        /* software restart ,system_restart , GPIO status won't change */
    REASON_DEEP_SLEEP_AWAKE,        /* wake up from deep-sleep */
    REASON_EXT_SYS_RST              /* external system reset */
} RST_REASON_E;

///OEM
//#define _IS_OEM
#ifdef _IS_OEM
/// oem firmware
#define FIRMWARE_KEY    "keyarerryyqmqqng"
#define PRODUCT_KEY     "keyarerryyqmqqng"
#else
/// customize firmware
//#define FIRMWARE_KEY    "keypkcmvhp7kexqj"//"key8kjsr7wdqdrs3"
//#define PRODUCT_KEY     "75a0e4zk3lh94zch"//"vixfmwd07jzqrnoj"//"sybpq0nolwqldzy7"

#define FIRMWARE_KEY    "keyxhfuy7red8xf5"//"key8kjsr7wdqdrs3"
#define PRODUCT_KEY     "joqyez0pq15hooaf"//"vixfmwd07jzqrnoj"//"sybpq0nolwqldzy7"
#endif


#define DPID_SWITCH          20     /// switch_led
#define DPID_MODE            21     /// work_mode
#define DPID_BRIGHT          22     /// bright_value
#define DPID_TEMPR           23     /// temp_value
#define DPID_COLOR           24     /// colour_data
#define DPID_SCENE           25     /// scene_data
#define DPID_COUNTDOWN       26     /// countdown
#define DPID_MUSIC           27     /// music_data
#define DPID_CONTROL         28
#define DPID_STA_KEEP


#define DEFAULT_CONFIG "{Jsonver:1.1.0,module:WB3S,cmod:rgbcw,dmod:0,cwtype:0,onoffmode:0,pmemory:1,title20:0,defcolor:c,defbright:100,deftemp:100,cwmaxp:100,brightmin:10,brightmax:100,colormin:10,colormax:100,wfcfg:spcl,rstmode:0,rstnum:3,rstcor:c,rstbr:50,rsttemp:100,pwmhz:1000,r_pin:19,r_lv:1,g_pin:18,g_lv:1,b_pin:8,b_lv:1,c_pin:6,c_lv:1,w_pin:7,w_lv:1,}"


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif  /* __LIHGT_SYSTEM_H__ */

