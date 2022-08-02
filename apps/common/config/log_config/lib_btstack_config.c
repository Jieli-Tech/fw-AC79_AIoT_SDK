/*********************************************************************************************
    *   Filename        : btstack_config.c

    *   Description     : Optimized Code & RAM (编译优化配置)

    *   Author          : Bingquan

    *   Email           : caibingquan@zh-jieli.com

    *   Last modifiled  : 2019-03-16 11:49

    *   Copyright:(c)JIELI  2011-2019  @ , All Rights Reserved.
*********************************************************************************************/
#include "app_config.h"

/**
 * @brief Bluetooth Stack Module
 */

const int CONFIG_BTSTACK_BIG_FLASH_ENABLE = 1;

#if TCFG_BT_SUPPORT_AAC
const int CONFIG_BTSTACK_SUPPORT_AAC = 1;
#else
const int CONFIG_BTSTACK_SUPPORT_AAC = 0;
#endif

