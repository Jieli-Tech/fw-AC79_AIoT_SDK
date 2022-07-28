
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

// #include <cJSON.h>
#include "cJSON_common/cJSON.h"

#include "os/os_api.h"
#include "lwip/sockets.h"

#include "qcloud_iot_export.h"
#include "qiot_internal.h"

#include "tvs_authorize.h"
#include "tvs_api_config.h"
#include "qcloud_iot_import.h"

static bool sg_comm_task_run = false;

static int _app_reply_dev_info(comm_peer_t *peer)
{
    int ret;
    DeviceInfo devinfo;
    cJSON_Hooks memoryHook;

    memoryHook.malloc_fn = malloc;
    memoryHook.free_fn   = free;
    cJSON_InitHooks(&memoryHook);

    ret = HAL_GetDevInfo(&devinfo);
    if (ret) {
        Log_e("load dev info failed: %d", ret);
        app_send_error_log(peer, CUR_ERR, ERR_APP_CMD, ret);
        return -1;
    }

    cJSON *reply_json = cJSON_CreateObject();
    cJSON_AddNumberToObject(reply_json, "cmdType", (int)CMD_DEVICE_REPLY);
    cJSON_AddStringToObject(reply_json, "productId", devinfo.product_id);
    cJSON_AddStringToObject(reply_json, "deviceName", devinfo.device_name);
    cJSON_AddStringToObject(reply_json, "protoVersion", SOFTAP_BOARDING_VERSION);

    char *json_str = cJSON_Print(reply_json);
    if (!json_str) {
        Log_e("cJSON_PrintPreallocated failed!");
        cJSON_Delete(reply_json);
        app_send_error_log(peer, CUR_ERR, ERR_APP_CMD, ERR_JSON_PRINT);
        return -1;
    }
    /* append msg delimiter */
//    strcat(json_str, "\r\n");
    cJSON_Delete(reply_json);
    HAL_Printf("Report dev info(%d): %s", strlen(json_str), json_str);

    int udp_resend_cnt = 3;
udp_resend:
    ret = sendto(peer->socket_id, json_str, strlen(json_str), 0, peer->socket_addr, peer->addr_len);
    if (ret < 0) {
        free(json_str);
        Log_e("send error: %s", strerror(errno));
        push_error_log(ERR_SOCKET_SEND, errno);
        return -1;
    }
    /* UDP packet could be lost, send it again */
    /* NOT necessary for TCP */
    if (peer->socket_addr != NULL && --udp_resend_cnt) {
        HAL_SleepMs(1000);
        goto udp_resend;
    }

    HAL_Printf("Report dev info: %s", json_str);
    free(json_str);
    return 0;
}

static int _app_reply_auth_reqinfo(comm_peer_t *peer)
{
    int ret;
    cJSON_Hooks memoryHook;

    memoryHook.malloc_fn = malloc;
    memoryHook.free_fn   = free;
    cJSON_InitHooks(&memoryHook);

    char tvs_productId[MAX_SIZE_OF_TVS_PRODUCT_ID + 1] = {0};
    char tvs_dsn[MAX_SIZE_OF_TVS_DEVICE_NAME + 1] = {0};
    HAL_GetTvsInfo(tvs_productId, tvs_dsn);

    cJSON *reply_json = cJSON_CreateObject();
    cJSON_AddNumberToObject(reply_json, "cmdType", (int)CMD_AUTHINFO_REPLY);
    cJSON_AddStringToObject(reply_json, "pid", tvs_productId);
    cJSON_AddStringToObject(reply_json, "dsn", tvs_dsn);

    char authReqInfos[150] = {0};
    tvs_authorize_manager_build_auth_req(authReqInfos, sizeof(authReqInfos));
    cJSON_AddStringToObject(reply_json, "authReqInfo", authReqInfos);

    char *json_str = cJSON_Print(reply_json);
    if (!json_str) {
        Log_e("cJSON_PrintPreallocated failed!");
        cJSON_Delete(reply_json);
        app_send_error_log(peer, CUR_ERR, ERR_APP_CMD, ERR_JSON_PRINT);
        return -1;
    }
    /* append msg delimiter */
//    strcat(json_str, "\r\n");
    cJSON_Delete(reply_json);
    HAL_Printf("Reply auth reqinfo(%d): %s", strlen(json_str), json_str);

    int udp_resend_cnt = 3;
udp_resend:
    ret = sendto(peer->socket_id, json_str, strlen(json_str), 0, peer->socket_addr, peer->addr_len);
    if (ret < 0) {
        free(json_str);
        Log_e("send error: %s", strerror(errno));
        push_error_log(ERR_SOCKET_SEND, errno);
        return -1;
    }
    /* UDP packet could be lost, send it again */
    /* NOT necessary for TCP */
    if (peer->socket_addr != NULL && --udp_resend_cnt) {
        HAL_SleepMs(1000);
        goto udp_resend;
    }

    HAL_Printf("Reply auth reqinfo: %s", json_str);
    free(json_str);
    return 0;
}

static int _app_handle_recv_data(comm_peer_t *peer, char *pdata, int len)
{
    int ret;
    cJSON *root = cJSON_Parse(pdata);
    if (!root) {
        Log_e("Parsing JSON Error: [%s]", cJSON_GetErrorPtr());
        app_send_error_log(peer, CUR_ERR, ERR_APP_CMD, ERR_APP_CMD_JSON_FORMAT);
        return -1;
    }

    cJSON *cmd_json = cJSON_GetObjectItem(root, "cmdType");
    if (cmd_json == NULL) {
        Log_e("Invalid cmd JSON: %s", pdata);
        cJSON_Delete(root);
        app_send_error_log(peer, CUR_ERR, ERR_APP_CMD, ERR_APP_CMD_JSON_FORMAT);
        return -1;
    }

    int cmd = cmd_json->valueint;
    switch (cmd) {
    /* Token only for simple config  */
    case CMD_TOKEN_ONLY: {
        cJSON *token_json = cJSON_GetObjectItem(root, "token");
        if (token_json) {
            // set device bind token
            qiot_device_bind_set_token(token_json->valuestring);
            ret = _app_reply_dev_info(peer);

            // sleep a while before exit
            HAL_SleepMs(1000);

            /* 0: need to wait for next cmd
             * 1: Everything OK and we've finished the job */
            return (ret == 0);
        } else {
            cJSON_Delete(root);
            Log_e("invlaid token!");
            app_send_error_log(peer, CUR_ERR, ERR_APP_CMD, ERR_APP_CMD_AP_INFO);
            return -1;
        }
    }
    break;

    /* SSID/PW/TOKEN for softAP */
    case CMD_SSID_PW_TOKEN: {
#if WIFI_PROV_SOFT_AP_ENABLE
        cJSON *ssid_json  = cJSON_GetObjectItem(root, "ssid");
        cJSON *psw_json   = cJSON_GetObjectItem(root, "password");
        cJSON *token_json = cJSON_GetObjectItem(root, "token");

        if (ssid_json && psw_json && token_json) {

            //parse token and connect to ap
            qiot_device_bind_set_token(token_json->valuestring);
            _app_reply_dev_info(peer);
            // sleep a while before changing to STA mode
            HAL_SleepMs(3000);

            Log_i("STA to connect SSID:%s PASSWORD:%s", ssid_json->valuestring, psw_json->valuestring);
            PUSH_LOG("SSID:%s|PSW:%s|TOKEN:%s", ssid_json->valuestring, psw_json->valuestring,
                     token_json->valuestring);
            ret = tvs_wifi_sta_connect(ssid_json->valuestring, psw_json->valuestring, 0);
            if (ret) {
                Log_e("tvs_wifi_sta_connect failed: %d", ret);
                PUSH_LOG("tvs_wifi_sta_connect failed: %d", ret);
                app_send_error_log(peer, CUR_ERR, ERR_WIFI_AP_STA, ret);
                cJSON_Delete(root);
                set_soft_ap_config_result(WIFI_CONFIG_FAIL);
                return -1;
            } else {
                Log_d("tvs_wifi_sta_connect success");
                set_soft_ap_config_result(WIFI_CONFIG_SUCCESS);
            }
            cJSON_Delete(root);

            /* return 1 as device alreay switch to STA mode and unable to recv cmd anymore
             * 1: Everything OK and we've finished the job */
            return 1;
        } else {
            cJSON_Delete(root);
            Log_e("invlaid ssid/password/token!");
            app_send_error_log(peer, CUR_ERR, ERR_APP_CMD, ERR_APP_CMD_AP_INFO);
            return -1;
        }
#endif
    }
    break;

    case CMD_LOG_QUERY:
        ret = app_send_dev_log(peer);
        Log_i("app_send_dev_log ret: %d", ret);
        return 1;

    case CMD_AUTHINFO_QUERY:
        ret = _app_reply_auth_reqinfo(peer);
        Log_i("_app_reply_auth_reqinfo ret: %d", ret);
        return ret;

    default:
        cJSON_Delete(root);
        Log_e("Unknown cmd: %d", cmd);
        app_send_error_log(peer, CUR_ERR, ERR_APP_CMD, ERR_APP_CMD_JSON_FORMAT);
        break;
    }

    return -1;
}

static void _qiot_comm_service_task(void *pvParameters)
{
    int ret, server_socket = -1;
    char addr_str[128] = {0};

    /* stay longer than 5 minutes to handle error log */
    uint32_t server_count = WAIT_CNT_5MIN_SECONDS / SELECT_WAIT_TIME_SECONDS + 5;

    struct sockaddr_in server_addr;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(APP_SERVER_PORT);
    inet_ntoa_r(server_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

    server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (server_socket < 0) {
        Log_e("Unable to create socket: errno %d", errno);
        push_error_log(ERR_SOCKET_OPEN, errno);
        goto end_of_task;
    }

    ret = bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        Log_e("Socket unable to bind: errno %d", errno);
        /*TODO: probably another udp task is still running */
        push_error_log(ERR_SOCKET_BIND, errno);
        goto end_of_task;
    }

    Log_i("UDP server socket listening...");
    fd_set sets;
    comm_peer_t peer_client = {
        .socket_id   = server_socket,
        .socket_addr = NULL,
        .addr_len    = 0,
    };

    int select_err_cnt = 0;
    int recv_err_cnt   = 0;
    while (sg_comm_task_run && --server_count) {
        FD_ZERO(&sets);
        FD_SET(server_socket, &sets);
        struct timeval timeout;
        timeout.tv_sec  = SELECT_WAIT_TIME_SECONDS;
        timeout.tv_usec = 0;

        int ret = select(server_socket + 1, &sets, NULL, NULL, &timeout);
        if (ret > 0) {
            select_err_cnt = 0;
            struct sockaddr_in source_addr;
            unsigned int addrLen = sizeof(source_addr);
            char rx_buffer[1024] = {0};

            int len = recvfrom(server_socket, rx_buffer, sizeof(rx_buffer) - 1, MSG_DONTWAIT,
                               (struct sockaddr *)&source_addr, &addrLen);

            // Error occured during receiving
            if (len < 0) {
                recv_err_cnt++;
                Log_w("recvfrom error: %d, cnt: %d", errno, recv_err_cnt);
                if (recv_err_cnt > 3) {
                    Log_e("recvfrom error: %d, cnt: %d", errno, recv_err_cnt);
                    push_error_log(ERR_SOCKET_RECV, errno);
                    break;
                }
                continue;
            }
            // Connection closed
            else if (len == 0) {
                recv_err_cnt = 0;
                Log_w("Connection is closed by peer");
                continue;
            }
            // Data received
            else {
                recv_err_cnt = 0;
                // Get the sender's ip address as string
                inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                rx_buffer[len] = 0;
                Log_i("Received %d bytes from <%s:%u> msg: %s", len, addr_str, source_addr.sin_port, rx_buffer);

                peer_client.socket_addr = (struct sockaddr *)&source_addr;
                peer_client.addr_len    = sizeof(source_addr);

                // send error log here, otherwise no chance for previous error
                get_and_post_error_log(&peer_client);

                ret = _app_handle_recv_data(&peer_client, rx_buffer, len);
                if (ret == 1) {
                    Log_w("Finish app cmd handling.");
                    break;
                }

                get_and_post_error_log(&peer_client);
                continue;
            }
        } else if (0 == ret) {
            select_err_cnt = 0;
            Log_d("wait for read...");
            if (peer_client.socket_addr != NULL) {
                get_and_post_error_log(&peer_client);
            }
            continue;
        } else {
            select_err_cnt++;
            Log_w("select-recv error: %d, cnt: %d", errno, select_err_cnt);
            if (select_err_cnt > 3) {
                Log_e("select-recv error: %d, cnt: %d", errno, select_err_cnt);
                push_error_log(ERR_SOCKET_SELECT, errno);
                break;
            }
            HAL_SleepMs(500);
        }
    }

end_of_task:
    if (server_socket != -1) {
        Log_w("Shutting down UDP server socket");
        shutdown(server_socket, 0);
        closesocket(server_socket);
    }

    sg_comm_task_run = false;
    Log_i("UDP server task quit");
    HAL_TaskDelete(NULL);
}

int qiot_comm_service_start(void)
{
    sg_comm_task_run = true;

    int ret = HAL_TaskCreate(_qiot_comm_service_task, COMM_SERVER_TASK_NAME, COMM_SERVER_TASK_STACK_BYTES, NULL,
                             COMM_SERVER_TASK_PRIO, NULL);

    if (ret != true) {
        Log_e("create comm_server_task failed: %d", ret);
        PUSH_LOG("create comm_server_task failed: %d", ret);
        push_error_log(ERR_OS_TASK, ret);
        return -1;
    }

    return 0;
}

void qiot_comm_service_stop(void)
{
    sg_comm_task_run = false;
}

