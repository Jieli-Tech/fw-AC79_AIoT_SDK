/*
 * Copyright (c) 2020 Tencent Cloud. All rights reserved.

 * Licensed under the MIT License (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT

 * Unless required by applicable law or agreed to in writing, software distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __QCLOUD_IOT_WIFI_CONFIG_H__
#define __QCLOUD_IOT_WIFI_CONFIG_H__

#include "stdint.h"

#define WIFI_PROV_SOFT_AP_ENABLE         0  ///< wifi provisioning method: device AP, need Wechat Applets
#define WIFI_PROV_SMART_CONFIG_ENABLE    0  ///< wifi provisioning method: smart config, need Wechat Applets
#define WIFI_PROV_AIRKISS_CONFIG_ENABLE  0  ///< wifi provisioning method: airkiss, need Wechat Applets
#define WIFI_PROV_BT_COMBO_CONFIG_ENABLE 1  ///< wifi provisioning method: bt combo config, need Wechat Applets

#define MAX_SSID_LEN 					32  //max ssid len
#define MAX_PSK_LEN  					65  //max psk len

typedef enum {
    WIFI_CONFIG_TYPE_SOFT_AP       = 0, /* Soft ap */
    WIFI_CONFIG_TYPE_SMART_CONFIG  = 1, /* Smart config*/
    WIFI_CONFIG_TYPE_AIRKISS       = 2, /* Airkiss */
    WIFI_CONFIG_TYPE_SIMPLE_CONFIG = 3, /* Simple config  */
    WIFI_CONFIG_TYPE_BT_COMBO      = 4, /* BT Combo config */
} eWiFiConfigType;

typedef enum {
    RET_WIFI_CONFIG_START_SUCCESS    = 0,
    ERR_UNKNOWN_WIFI_CONFIG_TYPE     = -1,
    ERR_UNSUPPORTED_WIFI_CONFIG_TYPE = -2,
    ERR_WIFI_CONFIG_START_FAILED     = -3,
    ERR_COMM_SERVICE_START_FAILED    = -4
} eWiFiConfigErrCode;

typedef enum {
    RESULT_WIFI_CONFIG_SUCCESS, /* WiFi config success */
    RESULT_WIFI_CONFIG_FAILED,  /* WiFi config failed */
    RESULT_WIFI_CONFIG_TIMEOUT  /* WiFi config timeout */
} eWiFiConfigResult;

typedef void (*WifiConfigResultCallBack)(eWiFiConfigResult result, void *usr_data);

/**
 * @brief Start WiFi config and device binding process
 *
 * @param type @see eWiFiConfigType, only WIFI_CONFIG_TYPE_SIMPLE_CONFIG is supported now
 * @param params @see rtk_simple_config.h
 * @param result_cb callback to get wifi config result
 *
 * @return @see eWiFiConfigErrCode
 */
int qiot_wifi_config_start(eWiFiConfigType type, void *params, WifiConfigResultCallBack result_cb);

/**
 * @brief Stop WiFi config and device binding process immediately
 */
void qiot_wifi_config_stop(void);

/**
 * @brief Send wifi config log to mini program
 */
int qiot_wifi_config_send_log(void);

/**
 * @brief set the ap ssid and psk fetched by wifi config
 */
void qiot_wifi_ap_info_set(const uint8_t *ssid, const uint8_t *psw);

/**
 * @brief get the ap ssid and psk fetched by wifi config
 */
void qiot_wifi_ap_info_get(uint8_t *ssid, int ssid_buff_len, uint8_t *psk, int psk_buff_len);

#endif  //__QCLOUD_WIFI_CONFIG_H__
