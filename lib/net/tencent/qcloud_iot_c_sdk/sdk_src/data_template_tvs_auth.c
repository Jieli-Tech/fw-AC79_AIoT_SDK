/*
 * Tencent is pleased to support the open source community by making IoT Hub
 available.
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

 * Licensed under the MIT License (the "License"); you may not use this file
 except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT

 * Unless required by applicable law or agreed to in writing, software
 distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 KIND,
 * either express or implied. See the License for the specific language
 governing permissions and
 * limitations under the License.
 *
 */
//===============tvs system property handle====================//

#include "lite-utils.h"
#include "qcloud_iot_export.h"
#include "qcloud_iot_import.h"
#include "utils_timer.h"
#include "tvs_authorize.h"
#include "tvs_api_config.h"
#include "tvs_auth_manager.h"
#include "data_template_tvs_auth.h"

#define KEY_CMD             		"cmd"
#define CMD_GET_INFO        		"get_reqinfo"
#define CMD_AUTH_RESP_INFO  		"authinfo_response"
#define CMD_AUTH_REQ_INFO   		"reqinfo_reply"
#define CMD_AUTH_RESULT     		"auth_result"
#define TVS_SYS_CMD_PROPERTY_KEY	"_sys_tvs_auth_cmd"
#define TVS_SYS_REPLY_PROPERTY_KEY	"_sys_tvs_auth_reply"
#define TVS_SYS_PROPERTY_BUFFF_LEN	1024
#define TOTAL_TVS_SYS_PROPERTY_COUNT 2
static  bool sg_boot_auth_fail  = false;
static  bool sg_property_init_flag  = false;

typedef struct _TVS_SYS_PROPERTY {
    TYPE_DEF_TEMPLATE_STRING m_tvs_auth_cmd[TVS_SYS_PROPERTY_BUFFF_LEN + 1];
    TYPE_DEF_TEMPLATE_STRING m_tvs_auth_reply[TVS_SYS_PROPERTY_BUFFF_LEN + 1];
} TVS_SYS_PROPERTY;

static   TVS_SYS_PROPERTY     sg_tvs_property;
static 	 sDataPoint    		  sg_tvs_data[TOTAL_TVS_SYS_PROPERTY_COUNT];

static void _init_tvs_property()
{
    sg_tvs_property.m_tvs_auth_cmd[0] = '\0';
    sg_tvs_data[0].data_property.data = sg_tvs_property.m_tvs_auth_cmd;
    sg_tvs_data[0].data_property.data_buff_len = TVS_SYS_PROPERTY_BUFFF_LEN;
    sg_tvs_data[0].data_property.key  = TVS_SYS_CMD_PROPERTY_KEY;
    sg_tvs_data[0].data_property.type = TYPE_TEMPLATE_STRING;
    sg_tvs_data[0].state = eNOCHANGE;

    /* sg_tvs_property.m_tvs_auth_reply[0] = '\0'; */
    sg_tvs_data[1].data_property.data = sg_tvs_property.m_tvs_auth_reply;
    sg_tvs_data[1].data_property.data_buff_len = TVS_SYS_PROPERTY_BUFFF_LEN;
    sg_tvs_data[1].data_property.key  = TVS_SYS_REPLY_PROPERTY_KEY;
    sg_tvs_data[1].data_property.type = TYPE_TEMPLATE_STRING;
    sg_tvs_data[1].state = eNOCHANGE;
};

static void _OnTvsReplyCallback(void *pClient, Method method, ReplyAck replyAck, const char *pJsonDocument,
                                void *pUserdata)
{
    Log_i("recv report_reply(ack=%d): %s", replyAck, pJsonDocument);
}

int _tvs_auth_reply(void *pTemplate_client)
{
    int rc = QCLOUD_RET_SUCCESS;
    if ((sg_tvs_data[1].state == eCHANGED) || (sg_boot_auth_fail == true)) {
        char data_buff[TVS_SYS_PROPERTY_BUFFF_LEN + 1];
        //clear control otherwise get_status will return the control msg
        if (false == sg_boot_auth_fail) {
            sReplyPara      replyPara;
            memset((char *)&replyPara, 0, sizeof(sReplyPara));
            replyPara.code          = eDEAL_SUCCESS;
            replyPara.timeout_ms    = QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;
            replyPara.status_msg[0] = '\0';  // add extra info to replyPara.status_msg when error occured
            rc = IOT_Template_ControlReply(pTemplate_client, data_buff, TVS_SYS_PROPERTY_BUFFF_LEN, &replyPara);
            if (rc != QCLOUD_RET_SUCCESS) {
                Log_e("tvs system property msg reply failed, err: %d", rc);
            }
        } else {
            sg_boot_auth_fail = false;
        }

        //auth reply
        DeviceProperty *pReportDataList[TOTAL_TVS_SYS_PROPERTY_COUNT];
        pReportDataList[0] = &sg_tvs_data[1].data_property;
        rc = IOT_Template_JSON_ConstructReportArray(pTemplate_client, data_buff, TVS_SYS_PROPERTY_BUFFF_LEN, \
                1, pReportDataList);
        Log_d("auth reply:%s", data_buff);
        if (rc == QCLOUD_RET_SUCCESS) {
            rc = IOT_Template_Report(pTemplate_client, data_buff, TVS_SYS_PROPERTY_BUFFF_LEN,
                                     _OnTvsReplyCallback, NULL, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
            if (rc == QCLOUD_RET_SUCCESS) {
                Log_i("data template reporte success");
            } else {
                Log_e("data template reporte failed, err: %d", rc);
            }
        } else {
            Log_e("construct reporte data failed, err: %d", rc);
        }
        sg_tvs_data[1].state = eNOCHANGE;
    }

    return rc;
}

static void  _tvs_authorize_cb(bool result, char *auth_info, int auth_info_len, const char *client_id, int error)
{
    Log_d("auth result %s", (result == true) ? "success" : "fail");
    memset(sg_tvs_property.m_tvs_auth_reply, '\0', TVS_SYS_PROPERTY_BUFFF_LEN);
    if (true == result) {
        if (0 != strcmp(auth_info, "boot_auth_success")) {
            save_auth_info(auth_info, auth_info_len);
        }
        HAL_Snprintf(sg_tvs_property.m_tvs_auth_reply, TVS_SYS_PROPERTY_BUFFF_LEN, \
                     "{\\\"cmd\\\":\\\"%s\\\", \\\"result\\\":\\\"success\\\", \\\"code\\\":\\\"0\\\"}", CMD_AUTH_RESULT);
    } else {
        HAL_Snprintf(sg_tvs_property.m_tvs_auth_reply, TVS_SYS_PROPERTY_BUFFF_LEN, \
                     "{\\\"cmd\\\":\\\"%s\\\", \\\"result\\\":\\\"fail\\\", \\\"code\\\":\\\"%d\\\"}",  CMD_AUTH_RESULT, error);
    }

    Log_d("auth_reply:%s", sg_tvs_property.m_tvs_auth_reply);
    if (!auth_info || (auth_info && !strcmp(auth_info, "boot_auth_fail"))) {
        sg_boot_auth_fail = true;
    }
    sg_tvs_data[1].state = eCHANGED;

}

static int _handle_tvs_auth(void *pTemplate_client, TVS_SYS_PROPERTY *pSysProperty)
{
    int ret = QCLOUD_RET_SUCCESS;
    char *cmd = NULL;

    LITE_string_strip_char(pSysProperty->m_tvs_auth_cmd, '\\');
    Log_d("auth message: %s", pSysProperty->m_tvs_auth_cmd);
    cmd = LITE_json_value_of(KEY_CMD, pSysProperty->m_tvs_auth_cmd);
    if (!cmd) {
        Log_e("message illegal");
        ret = QCLOUD_ERR_INVAL;
        goto exit;
    }
    if (!strcmp(cmd, CMD_GET_INFO)) {
        char tvs_productId[MAX_SIZE_OF_TVS_PRODUCT_ID + 1] = {0};
        char tvs_dsn[MAX_SIZE_OF_TVS_DEVICE_NAME + 1] = {0};
        HAL_GetTvsInfo(tvs_productId, tvs_dsn);
        char authReqInfo[150] = {0};
        tvs_authorize_manager_build_auth_req(authReqInfo, sizeof(authReqInfo));
        char *codeChallenge = LITE_json_value_of("codeChallenge", authReqInfo);
        char *sessionId = LITE_json_value_of("sessionId", authReqInfo);
        if (!codeChallenge || !sessionId) {
            Log_e("build auth req fail");
            ret = QCLOUD_ERR_INVAL;
            goto exit;
        }
        memset(pSysProperty->m_tvs_auth_reply, '\0', TVS_SYS_PROPERTY_BUFFF_LEN);
        HAL_Snprintf(pSysProperty->m_tvs_auth_reply, TVS_SYS_PROPERTY_BUFFF_LEN, "{\\\"cmd\\\":\\\"%s\\\", \\\"pid\\\":\\\"%s\\\","
                     "\\\"dsn\\\":\\\"%s\\\", \\\"authReqInfo\\\":{\\\"codeChallenge\\\":\\\"%s\\\","
                     "\\\"sessionId\\\":\\\"%s\\\"}}",
                     CMD_AUTH_REQ_INFO, tvs_productId, tvs_dsn, codeChallenge, sessionId);

        IOT_Tvs_Auth_Handle(pTemplate_client);
        sg_tvs_data[1].state = eCHANGED;
        HAL_Free(codeChallenge);
        HAL_Free(sessionId);
        Log_d("auth_reqInfo:%s", sg_tvs_property.m_tvs_auth_reply);
    } else if (!strcmp(cmd, CMD_AUTH_RESP_INFO)) {
        char *clientId = LITE_json_value_of("clientId", pSysProperty->m_tvs_auth_cmd);
        if (!clientId) {
            Log_e("no clientId found");
            ret = QCLOUD_ERR_INVAL;
            goto exit;
        }
        char *authResp = LITE_json_value_of("authRespInfo", pSysProperty->m_tvs_auth_cmd);
        if (!authResp) {
            Log_e("no authRespInfo found");
            HAL_Free(clientId);
            ret = QCLOUD_ERR_INVAL;
            goto exit;
        }
        LITE_string_strip_char(authResp, '\\');
        tvs_authorize_init(_tvs_authorize_cb);
        start_authorize_with_manuf(clientId, authResp);
        HAL_SleepMs(2000);
        IOT_Tvs_Auth_Handle(pTemplate_client);
        HAL_Free(clientId);
        HAL_Free(authResp);
    }

exit:

    if (cmd) {
        HAL_Free(cmd);
    }

    return ret;
}

static void _OnTvsControlMsgCallback(void *pTemplate_client, const char *pJsonValueBuffer, uint32_t valueLength, DeviceProperty *pProperty)
{
    if (strcmp(TVS_SYS_CMD_PROPERTY_KEY, pProperty->key) == 0) {
        sg_tvs_data[1].state = eCHANGED;
        _handle_tvs_auth(pTemplate_client, &sg_tvs_property);
    }
}

// register data template properties
static int _register_tvs_property(void *pTemplate_client)
{
    int i, rc;

    for (i = 0; i < TOTAL_TVS_SYS_PROPERTY_COUNT; i++) {
        rc = IOT_Template_Register_Property(pTemplate_client, &sg_tvs_data[i].data_property, _OnTvsControlMsgCallback);
        if (rc != QCLOUD_RET_SUCCESS) {
            return rc;
        }
    }

    return QCLOUD_RET_SUCCESS;
}

int IOT_Tvs_Auth_Init(void *pTemplate_client)
{
    _init_tvs_property();
    sg_property_init_flag  = true;

    return _register_tvs_property(pTemplate_client);
}

int IOT_Tvs_Auth_Handle(void *pTemplate_client)
{
    int rc = QCLOUD_RET_SUCCESS;

    if (sg_property_init_flag == true) {
        rc = _tvs_auth_reply(pTemplate_client);
    }

    return rc;
}

void IOT_Tvs_Auth_Error_Cb(bool result, int error)
{
    char *auth_info = (result == true) ? "boot_auth_success" : "boot_auth_fail";

    _tvs_authorize_cb(result, auth_info, 0, NULL, error);
}

