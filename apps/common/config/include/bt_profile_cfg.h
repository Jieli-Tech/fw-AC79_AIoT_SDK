#ifndef _BT_PROFILE_CFG_H_
#define _BT_PROFILE_CFG_H_

#include "app_config.h"
#include "btcontroller_modules.h"

///---sdp service record profile- 用户选择支持协议--///
#ifdef CONFIG_RF_TEST_ENABLE
#define USER_SUPPORT_PROFILE_SPP    0
#define USER_SUPPORT_PROFILE_HFP    1
#else
#if __FLASH_SIZE__ > (1 * 1024 * 1024)
#define USER_SUPPORT_PROFILE_SPP    1
#define USER_SUPPORT_PROFILE_HFP    1
#else
#define USER_SUPPORT_PROFILE_SPP    0
#define USER_SUPPORT_PROFILE_HFP    0
#endif
#endif
#define USER_SUPPORT_PROFILE_A2DP   1
#define USER_SUPPORT_PROFILE_AVCTP  1
#define USER_SUPPORT_PROFILE_HID    0
#define USER_SUPPORT_PROFILE_PNP    0
#define USER_SUPPORT_PROFILE_PBAP   0
#define USER_SUPPORT_PROFILE_HFP_AG 0


//ble demo的例子
#define DEF_BLE_DEMO_NULL                 0 //ble 没有使能
#define DEF_BLE_DEMO_MULTI                1
#define DEF_BLE_DEMO_TRANS_DATA           6
#define DEF_BLE_DEMO_NET_CFG              7
#define DEF_BLE_DEMO_NET_CENTRAL          8
#define DEF_BLE_DEMO_HOGP                 9
#define DEF_BLE_DEMO_MI                   10
#define DEF_BLE_DEMO_NET_CFG_DUI          11
#define DEF_BLE_DEMO_NET_CFG_TURING       12
#define DEF_BLE_DEMO_NET_MAMBO            13
#define DEF_BLE_DEMO_NET_CFG_TENCENT      14


//配置选择的demo
#if TCFG_USER_BLE_ENABLE

#if TRANS_MULTI_BLE_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_MULTI

#elif TRANS_DATA_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_TRANS_DATA

#elif BT_NET_CFG_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_NET_CFG

#elif BT_NET_MAMBO_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_NET_MAMBO

#elif BT_NET_CFG_DUI_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_NET_CFG_DUI

#elif BT_NET_CFG_TURING_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_NET_CFG_TURING

#elif BT_NET_CFG_TENCENT_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_NET_CFG_TENCENT

#elif BT_NET_CENTRAL_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_NET_CENTRAL

#elif BT_NET_HID_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_HOGP

#elif XIAOMI_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_MI

#endif

#else
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_NULL//ble is closed
#endif

#endif
