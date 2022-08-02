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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "qcloud_iot_export_log.h"
#include "qiot_internal.h"
#include "qiot_wifi_config.h"

static char sg_ssid[MAX_SSID_LEN + 1];
static char sg_psk[MAX_PSK_LEN + 1];

typedef struct {
    int (*config_start)(void *, WifiConfigEventCallBack);
    void (*config_stop)(void);
} WiFiConfigMethod;

static WiFiConfigMethod const sg_wifi_config_methods[] = {
    /* {soft_ap_provision_start, soft_ap_provision_stop},      // WIFI_CONFIG_TYPE_SOFT_AP */
    {NULL, NULL},                              				// WIFI_CONFIG_TYPE_SMART_CONFIG
    {NULL, NULL},                              				// WIFI_CONFIG_TYPE_SMART_CONFIG
    /* {airkiss_config_start, airkiss_config_stop},          	// WIFI_CONFIG_TYPE_AIRKISS */
    {NULL, NULL},  											// WIFI_CONFIG_TYPE_SIMPLE_CONFIG
    {NULL, NULL},  											// WIFI_CONFIG_TYPE_SIMPLE_CONFIG
    {HAL_BTComboConfig_Start, HAL_BTComboConfig_Stop}       // WIFI_CONFIG_TYPE_BTCombo_CONFIG
};

static const WiFiConfigMethod *sg_wifi_config_method_now = NULL;
static WifiConfigResultCallBack sg_wifi_config_result_cb = NULL;

static void _qiot_wifi_config_event_cb(eWiFiConfigEvent event, void *usr_data)
{
    switch (event) {
    case EVENT_WIFI_CONFIG_SUCCESS:
        if (qiot_device_bind()) {
            Log_e("Device bind failed!");
            sg_wifi_config_result_cb(RESULT_WIFI_CONFIG_FAILED, NULL);
        } else {
            Log_d("Device bind success!");
            sg_wifi_config_result_cb(RESULT_WIFI_CONFIG_SUCCESS, NULL);
        }
        break;

    case EVENT_WIFI_CONFIG_FAILED:
        Log_e("Wifi config failed!");
        sg_wifi_config_result_cb(RESULT_WIFI_CONFIG_FAILED, NULL);
        break;

    case EVENT_WIFI_CONFIG_TIMEOUT:
        Log_e("Wifi config timeout!");
        sg_wifi_config_result_cb(RESULT_WIFI_CONFIG_TIMEOUT, NULL);
        break;

    default:
        Log_e("Unknown wifi config error!");
        sg_wifi_config_result_cb(RESULT_WIFI_CONFIG_FAILED, NULL);
        break;
    }
}

/**
 * @brief Start WiFi config and device binding process
 *
 * @param type WiFi config type, only WIFI_CONFIG_TYPE_SIMPLE_CONFIG is supported now
 * @param params See rtk_simple_config.h
 * @param result_cb callback to get wifi config result
 *
 * @return 0 when success, or err code for failure
 */
//yii:配网入口
int qiot_wifi_config_start(eWiFiConfigType type, void *params, WifiConfigResultCallBack result_cb)
{
    if (type < WIFI_CONFIG_TYPE_SOFT_AP || type > WIFI_CONFIG_TYPE_BT_COMBO) {
        Log_e("Unknown wifi config type!");
        return ERR_UNKNOWN_WIFI_CONFIG_TYPE;
    }

    if (init_dev_log_queue()) {
        Log_e("Init dev log queue failed!");
        return ERR_WIFI_CONFIG_START_FAILED;
    }
    //yii:获取配网启动函数
    sg_wifi_config_method_now = &sg_wifi_config_methods[type];

    if (!sg_wifi_config_method_now->config_start) {
        sg_wifi_config_method_now = NULL;
        Log_e("Unsupported wifi config type!");
        return ERR_UNSUPPORTED_WIFI_CONFIG_TYPE;
    }

    sg_wifi_config_result_cb = result_cb;
    //yii:运行配网函数
    if (sg_wifi_config_method_now->config_start(params, _qiot_wifi_config_event_cb)) {      //yii：HAL_BTComboConfig_Start
        sg_wifi_config_method_now = NULL;
        Log_e("Wifi Config start failed!");
        return ERR_WIFI_CONFIG_START_FAILED;
    }
#if 0
    //yii:UDP的内容
    if (qiot_comm_service_start()) {
        sg_wifi_config_method_now->config_stop();
        sg_wifi_config_method_now = NULL;
        Log_e("Comm service start failed!");
        return ERR_COMM_SERVICE_START_FAILED;
    }
#endif
    return RET_WIFI_CONFIG_START_SUCCESS;
}

/**
 * @brief Stop WiFi config and device binding process
 */
void qiot_wifi_config_stop(void)
{
    qiot_comm_service_stop();

    //BUG TO BE FIXED
//    if (sg_wifi_config_method_now) {
//        if (sg_wifi_config_method_now->config_stop) {
//            sg_wifi_config_method_now->config_stop();
//        }
//    }

//    sg_wifi_config_method_now = NULL;
//    sg_wifi_config_result_cb  = NULL;
}

/**
 * @brief Send wifi config log to app
 */
int qiot_wifi_config_send_log(void)
{
#ifdef WIFI_LOG_UPLOAD
#define LOG_SOFTAP_CH	6
    int ret;

    Log_e("start softAP for log");
    ret =  start_softAP("ESP-LOG-QUERY", "86013388", LOG_SOFTAP_CH);
    if (ret) {
        Log_e("start_softAP failed: %d", ret);
        goto err_eixt;
    }

    ret =  qiot_log_service_start();
    if (ret) {
        Log_e("qiot_log_service_start failed: %d", ret);
    }

    return 0;

err_eixt:
    stop_softAP();
    delete_dev_log_queue();

    return ret;
#else
    return 0;
#endif
}

void qiot_wifi_ap_info_set(const uint8_t *ssid, const uint8_t *psw)
{
    memset(sg_ssid, '\0', MAX_SSID_LEN);
    memset(sg_psk,  '\0', MAX_PSK_LEN);

    memcpy(sg_ssid, ssid, MAX_SSID_LEN);
    memcpy(sg_psk,  psw,  MAX_PSK_LEN);
}

void qiot_wifi_ap_info_get(uint8_t *ssid, int ssid_buff_len, uint8_t *psk, int psk_buff_len)
{
    memset(ssid, '\0', ssid_buff_len);
    memset(psk,  '\0', psk_buff_len);

    memcpy(ssid, sg_ssid, MAX_SSID_LEN);
    memcpy(psk,  sg_psk,  MAX_PSK_LEN);
}

static void _wifi_config_result_cb(eWiFiConfigResult event, void *usr_data)
{
    Log_d("entry...");
    qiot_wifi_config_stop();

    switch (event) {
    case RESULT_WIFI_CONFIG_SUCCESS:
        Log_i("WiFi is ready, to do Qcloud IoT demo");
        break;

    case RESULT_WIFI_CONFIG_TIMEOUT:
        Log_e("wifi config timeout!");

    case RESULT_WIFI_CONFIG_FAILED:
        Log_e("wifi config failed!");
        qiot_wifi_config_send_log();
        break;

    default:
        break;
    }
}

void ble_wifi_config(void *para)
{
    qiot_wifi_config_start(WIFI_CONFIG_TYPE_BT_COMBO, NULL, _wifi_config_result_cb);
}
