/*********************************************************************************************
    *   Filename        : log_config.c

    *   Description     : Optimized Code & RAM (编译优化Log配置)

    *   Author          : Bingquan

    *   Email           : caibingquan@zh-jieli.com

    *   Last modifiled  : 2019-03-18 14:45

    *   Copyright:(c)JIELI  2011-2019  @ , All Rights Reserved.
*********************************************************************************************/
#include "system/includes.h"
#include "app_config.h"

/**
 * @brief Bluetooth Controller Log
 */
/*-----------------------------------------------------------*/
const char log_tag_const_v_SETUP AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_i_SETUP AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_w_SETUP AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_d_SETUP AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_e_SETUP AT(.LOG_TAG_CONST) = TRUE;

const char log_tag_const_v_BOARD AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_i_BOARD AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_d_BOARD AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_w_BOARD AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_e_BOARD AT(.LOG_TAG_CONST) = TRUE;

const char log_tag_const_v_BT AT(.LOG_TAG_CONST)  = FALSE;
const char log_tag_const_i_BT AT(.LOG_TAG_CONST)  = TRUE;
const char log_tag_const_d_BT AT(.LOG_TAG_CONST)  = TRUE;
const char log_tag_const_w_BT AT(.LOG_TAG_CONST)  = TRUE;
const char log_tag_const_e_BT AT(.LOG_TAG_CONST)  = TRUE;

const char log_tag_const_v_UI AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_i_UI AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_d_UI AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_w_UI AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_e_UI AT(.LOG_TAG_CONST) = TRUE;

const char log_tag_const_v_COLOR_LED AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_i_COLOR_LED AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_d_COLOR_LED AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_w_COLOR_LED AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_e_COLOR_LED AT(.LOG_TAG_CONST) = TRUE;

const char log_tag_const_v_APP_CHARGE AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_i_APP_CHARGE AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_d_APP_CHARGE AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_w_APP_CHARGE AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_e_APP_CHARGE AT(.LOG_TAG_CONST) = TRUE;

const char log_tag_const_v_KEY_EVENT_DEAL AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_i_KEY_EVENT_DEAL AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_d_KEY_EVENT_DEAL AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_w_KEY_EVENT_DEAL AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_e_KEY_EVENT_DEAL AT(.LOG_TAG_CONST) = TRUE;

const char log_tag_const_v_APP_CHARGESTORE AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_i_APP_CHARGESTORE AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_d_APP_CHARGESTORE AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_w_APP_CHARGESTORE AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_e_APP_CHARGESTORE AT(.LOG_TAG_CONST) = TRUE;

const char log_tag_const_v_APP_IDLE AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_i_APP_IDLE AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_d_APP_IDLE AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_w_APP_IDLE AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_e_APP_IDLE AT(.LOG_TAG_CONST) = TRUE;

const char log_tag_const_v_APP_POWER AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_i_APP_POWER AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_d_APP_POWER AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_w_APP_POWER AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_e_APP_POWER AT(.LOG_TAG_CONST) = TRUE;

const char log_tag_const_v_APP AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_i_APP AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_d_APP AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_w_APP AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_e_APP AT(.LOG_TAG_CONST) = TRUE;

const char log_tag_const_v_USER_CFG AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_i_USER_CFG AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_d_USER_CFG AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_w_USER_CFG AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_e_USER_CFG AT(.LOG_TAG_CONST) = TRUE;

const char log_tag_const_v_APP_TONE AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_i_APP_TONE AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_d_APP_TONE AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_w_APP_TONE AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_e_APP_TONE AT(.LOG_TAG_CONST) = TRUE;

const char log_tag_const_v_BT_BLE AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_i_BT_BLE AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_d_BT_BLE AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_w_BT_BLE AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_e_BT_BLE AT(.LOG_TAG_CONST) = TRUE;

const char log_tag_const_v_APP_ACTION AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_i_APP_ACTION AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_d_APP_ACTION AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_w_APP_ACTION AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_e_APP_ACTION AT(.LOG_TAG_CONST) = TRUE;

const char log_tag_const_v_APP_FILE_OPERATE AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_i_APP_FILE_OPERATE AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_d_APP_FILE_OPERATE AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_w_APP_FILE_OPERATE AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_e_APP_FILE_OPERATE AT(.LOG_TAG_CONST) = TRUE;

const char log_tag_const_v_APP_MUSIC AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_i_APP_MUSIC AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_d_APP_MUSIC AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_w_APP_MUSIC AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_e_APP_MUSIC AT(.LOG_TAG_CONST) = TRUE;

const char log_tag_const_v_APP_LINEIN AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_i_APP_LINEIN AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_d_APP_LINEIN AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_w_APP_LINEIN AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_e_APP_LINEIN AT(.LOG_TAG_CONST) = TRUE;

const char log_tag_const_v_APP_FM AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_i_APP_FM AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_d_APP_FM AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_w_APP_FM AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_e_APP_FM AT(.LOG_TAG_CONST) = TRUE;

const char log_tag_const_v_APP_PC AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_i_APP_PC AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_d_APP_PC AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_w_APP_PC AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_e_APP_PC AT(.LOG_TAG_CONST) = TRUE;

const char log_tag_const_v_APP_RTC AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_i_APP_RTC AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_d_APP_RTC AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_w_APP_RTC AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_e_APP_RTC AT(.LOG_TAG_CONST) = TRUE;

const char log_tag_const_v_APP_RECORD AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_i_APP_RECORD AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_d_APP_RECORD AT(.LOG_TAG_CONST) = FALSE;
const char log_tag_const_w_APP_RECORD AT(.LOG_TAG_CONST) = TRUE;
const char log_tag_const_e_APP_RECORD AT(.LOG_TAG_CONST) = TRUE;

