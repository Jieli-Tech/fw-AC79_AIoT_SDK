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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "qcloud_iot_export.h"
#include "utils_timer.h"
#include "os/os_api.h"
#include "system/init.h"
#include "qiot_wifi_config.h"
#include "qiot_internal.h"
#include "tvs_api_config.h"

#if WIFI_PROV_BT_COMBO_CONFIG_ENABLE

extern bool sg_token_received;           //从ble接收到token标志位
__attribute__((weak)) u8 enter_ble_config_flag;        //是否可以按键退出蓝牙标志位

static bool             sg_bt_combo_config_start = false;
static eWiFiConfigState ap_connect_state         = WIFI_CONFIG_ING;


void set_bt_combo_config_state(eWiFiConfigState state)
{
    ap_connect_state = state;
}

static eWiFiConfigState get_bt_combo_config_state(void)
{
    return ap_connect_state;
}

static void set_bt_combo_apconn_state(u8 state)
{
    if (state) {
        ap_connect_state = WIFI_CONFIG_SUCCESS;
    }

}

extern void send_reply_token(char *para_format, int len);
static int bt_combo_send_message(char *buff, int buff_len)
{
    //yii:ble发送token返回状态值
    send_reply_token(buff, buff_len);
    return QCLOUD_RET_SUCCESS;
}

extern int tc_wait_wifi_conn(void);
static int bt_combo_report_wificonn()
{
    //yii:检测并发送配网状态给小程序
    return tc_wait_wifi_conn();
}

static int bt_combo_report_bind_result(int token_status)
{
    int        ret           = QCLOUD_ERR_FAILURE;
    char       json_str[256] = {0};
    DeviceInfo devinfo;

    ret = HAL_GetDevInfo(&devinfo);
    if (ret != QCLOUD_RET_SUCCESS) {
        Log_e("load dev info failed: %d", ret);
        return -1;
    }

    HAL_Snprintf(json_str, sizeof(json_str), "{\"status\":%d,\"productId\":\"%s\",\"deviceName\":\"%s\"}\r\n",
                 token_status, devinfo.product_id, devinfo.device_name);
    //yii:ble发送小程序toekn状态
    bt_combo_send_message(json_str, strlen(json_str));

    return ret;
}

//yii:蓝牙辅助配网主要函数
static void bt_combo_config_task(void *params)
{
    WifiConfigEventCallBack wifi_config_event_cb = params;
    int                     bind_reply_code      = -1;
    uint32_t                time_count           = WIFI_CONFIG_WAIT_TIME_MS / WIFI_CONFIG_BLINK_TIME_MS;
    uint32_t				received_time		 = 0;


    //yii:正在连接网络，连接成功与失败返回不同的cmd给小程序
    if (bt_combo_report_wificonn()) {
        set_bt_combo_config_state(WIFI_CONFIG_FAIL);
        printf("-------%s-----------%d", __func__, __LINE__);
        goto exit;
    }

    //yii:wifi连接成功，蓝牙还没断开

    set_bt_combo_config_state(WIFI_CONFIG_SUCCESS);

    while (1) {
        if (!sg_bt_combo_config_start || (time_count <= 0)) {
            printf("-------%s-----------%d", __func__, __LINE__);
            goto exit;
        }
        time_count--;
        //yii：等待获取到token
        if (sg_token_received) {
            /* time_count =  20 * 1000 / WIFI_CONFIG_BLINK_TIME_MS;	//小程序是30s超时，这里设置20s后超时 */
            received_time = os_wrapper_get_time_ms();	//获取到token的当前设备时间
            wifi_config_event_cb(EVENT_WIFI_CONFIG_SUCCESS, NULL);
            while (!qiot_device_bind_get_cloud_reply_code(&bind_reply_code)) {
                HAL_SleepMs(WIFI_CONFIG_BLINK_TIME_MS);
                if (os_wrapper_get_time_ms() - received_time > 30 * 1000) {
                    goto exit;
                }
            }
            bt_combo_report_bind_result(bind_reply_code);
            write_BTCombo(true);
            printf("-------------%s-----------------%d\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n", __func__, __LINE__);
            break;
        }

        HAL_SleepMs(WIFI_CONFIG_BLINK_TIME_MS);

        if (time_count % 10 == 0) {
            Log_d("bt combo task running!");
        }
    }

    Log_d("task end connect_success :%d, task_run :%d", get_bt_combo_config_state(), sg_bt_combo_config_start);

exit:
    if (0 >= time_count) {
        bt_combo_report_bind_result(bind_reply_code);	//超时发送 code -1
        wifi_config_event_cb(EVENT_WIFI_CONFIG_TIMEOUT, NULL);
    }
    HAL_BTComboConfig_Stop();
}

int _bt_combo_config_task_start(void *params)
{
    static ThreadParams thread_params;
    thread_params.thread_func = bt_combo_config_task;
    thread_params.thread_name = "bt_combo_config_task";
    thread_params.stack_size  = 10240;
    thread_params.priority    = 3;
    thread_params.user_arg    = params;
    int ret                   = HAL_ThreadCreate(&thread_params);
    if (QCLOUD_RET_SUCCESS != ret) {
        Log_e("HAL_ThreadCreate(_bt_combo_config_task_start) failed!");
        return -1;
    }

    return 0;
}

int start_device_btcomboconfig(void)
{
    //yii:在外面用按键打开了，这里不做处理

    return QCLOUD_RET_SUCCESS;
}

int stop_device_btcomboconfig(void)
{
    //yii:等待2s后，断开蓝牙的连接
    extern void bt_ble_exit(void);
    HAL_SleepMs(WIFI_CONFIG_BLINK_TIME_MS * 10);
    bt_ble_exit();
    return QCLOUD_RET_SUCCESS;
}

#endif

/**
 * @brief Start bt combo config
 *
 * @param params is null
 *
 * @return 0 when success, or err code for failure
 */
int HAL_BTComboConfig_Start(void *params, WifiConfigEventCallBack event_cb)
{
    int ret = QCLOUD_RET_SUCCESS;
#if WIFI_PROV_BT_COMBO_CONFIG_ENABLE
    set_bt_combo_config_state(WIFI_CONFIG_ING);

    ret = start_device_btcomboconfig();
    if (QCLOUD_RET_SUCCESS != ret) {
        Log_e("start bt combo config failed! ret:%d", ret);
        return ret;
    }

    sg_bt_combo_config_start = true;
    ret = _bt_combo_config_task_start(event_cb);
    if (QCLOUD_RET_SUCCESS != ret) {
        HAL_BTComboConfig_Stop();
    }
#endif
    return ret;
}

/**
 * @brief Stop WiFi config and device binding process
 */
int HAL_BTComboConfig_Stop(void)
{
    int ret = QCLOUD_RET_SUCCESS;
#if WIFI_PROV_BT_COMBO_CONFIG_ENABLE
    set_bt_combo_config_state(WIFI_CONFIG_ING);

    if (sg_bt_combo_config_start) {
        Log_i("HAL_BTComboConfig_Stop");

        sg_bt_combo_config_start = false;



        ret = stop_device_btcomboconfig();
        enter_ble_config_flag = 0; //标志位清零
    }
#endif
    return ret;
}

/*********配网标志位函数实现*************/
bool read_BTCombo(void)
{
    bool flag;
    BTCOMBO_LOCK
    flag = BTCombo_complete;
    BTCOMBO_UNLOCK
    return flag;
}

void write_BTCombo(bool flag)
{
    BTCOMBO_LOCK
    BTCombo_complete = flag;
    BTCOMBO_UNLOCK
}

static int create_BTCombo_signal(void)
{
    BTCombo_mutex = os_wrapper_create_signal_mutex(1);
    BTCOMBO_LOCK
    BTCombo_complete = true;
    BTCOMBO_UNLOCK
    return 0;
}
late_initcall(create_BTCombo_signal);

