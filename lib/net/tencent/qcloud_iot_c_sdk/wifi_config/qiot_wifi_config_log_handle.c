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

#include <string.h>
#include <time.h>

#include "cJSON_common/cJSON.h"
#include "lwip/sockets.h"
#include "qcloud_iot_export_log.h"
#include "qcloud_iot_import.h"
#include "qiot_internal.h"

/************** WiFi config error msg collect and post feature ******************/

#ifdef WIFI_LOG_UPLOAD

#define LOG_QUEUE_SIZE 10
#define LOG_ITEM_SIZE  128

/* FreeRTOS msg queue */
static void *sg_dev_log_queue = NULL;
static bool sg_log_task_run = false;

#endif

int init_dev_log_queue(void)
{
#ifdef WIFI_LOG_UPLOAD
    if (sg_dev_log_queue) {
        Log_d("re-enter, reset queue");
        HAL_QueueReset(sg_dev_log_queue);
        return 0;
    }

    sg_dev_log_queue = HAL_QueueCreate(LOG_QUEUE_SIZE, LOG_ITEM_SIZE);
    if (sg_dev_log_queue == NULL) {
        Log_e("HAL_QueueCreate failed");
        return ERR_OS_QUEUE;
    }
#endif
    return 0;
}

void delete_dev_log_queue(void)
{
#ifdef WIFI_LOG_UPLOAD
    HAL_QueueDestory(sg_dev_log_queue);
    sg_dev_log_queue = NULL;
#endif
}

int push_dev_log(const char *func, const int line, const char *fmt, ...)
{
#ifdef WIFI_LOG_UPLOAD
    if (sg_dev_log_queue == NULL) {
        Log_e("log queue not initialized!");
        return ERR_OS_QUEUE;
    }

    char log_buf[LOG_ITEM_SIZE];
    memset(log_buf, 0, LOG_ITEM_SIZE);

    // only keep the latest LOG_QUEUE_SIZE log
    uint32_t log_cnt = (uint32_t)HAL_QueueItemWaitingCount(sg_dev_log_queue);
    if (log_cnt == LOG_QUEUE_SIZE) {
        // pop the oldest one
        HAL_QueueItemPop(sg_dev_log_queue, log_buf, 0);
        HAL_Printf("<<< POP LOG: %s", log_buf);
    }

    char *o = log_buf;
    memset(log_buf, 0, LOG_ITEM_SIZE);
    o += HAL_Snprintf(o, LOG_ITEM_SIZE, "%u|%s(%d): ", HAL_GetTimeMs(), func, line);

    va_list ap;
    va_start(ap, fmt);
    HAL_Vsnprintf(o, LOG_ITEM_SIZE - 3 - strlen(log_buf), fmt, ap);
    va_end(ap);

    strcat(log_buf, "\r\n");

    /* unblocking send */
    // int ret = xQueueGenericSend(sg_dev_log_queue, log_buf, 0, queueSEND_TO_BACK);
    int ret = HAL_QueueItemPush(sg_dev_log_queue, log_buf, 0);
    if (ret != true) {
        Log_e("HAL_QueueItemPush failed: %d", ret);
        return ERR_OS_QUEUE;
    }

    // HAL_Printf(">>> PUSH LOG: %s\n", log_buf);
#endif
    return 0;
}

int app_send_dev_log(comm_peer_t *peer)
{
    int ret = 0;
#ifdef WIFI_LOG_UPLOAD

    if (sg_dev_log_queue == NULL) {
        Log_e("log queue not initialized!");
        return ERR_OS_QUEUE;
    }

    uint32_t log_cnt = (uint32_t)HAL_QueueItemWaitingCount(sg_dev_log_queue);
    if (log_cnt == 0) {
        return 0;
    }

    size_t max_len = (log_cnt * LOG_ITEM_SIZE) + 32;
    char *json_buf = HAL_Malloc(max_len);
    if (json_buf == NULL) {
        Log_e("malloc failed!");
        return -1;
    }

    memset(json_buf, 0, max_len);

    char log_buf[LOG_ITEM_SIZE];
    unsigned long rc;
    do {
        memset(log_buf, 0, LOG_ITEM_SIZE);
        rc = HAL_QueueItemPop(sg_dev_log_queue, log_buf, 0);
        if (rc == true) {
            strcat(json_buf, log_buf);
        }
    } while (rc == true);

    HAL_Printf("to reply: %s\r\n", json_buf);


    int i = 0;
    for (i = 0; i < 2; i++) {
        ret = sendto(peer->socket_id, json_buf, strlen(json_buf), 0, peer->socket_addr, peer->addr_len);
        if (ret < 0) {
            Log_e("send error: %s", strerror(errno));
            break;
        }
        HAL_SleepMs(500);
    }
    HAL_Free(json_buf);

    // HAL_QueueDestory(sg_dev_log_queue);
#endif
    return ret;
}

static void log_server_task(void *pvParameters)
{
#ifdef WIFI_LOG_UPLOAD
    int  ret, server_socket = -1;
    char addr_str[128] = {0};

    /* stay 6 minutes to handle log */
    uint32_t server_count = 360 / SELECT_WAIT_TIME_SECONDS;

    struct sockaddr_in server_addr;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(LOG_SERVER_PORT);
    inet_ntoa_r(server_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

    server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (server_socket < 0) {
        Log_e("socket failed: errno %d", errno);
        goto end_of_task;
    }

    ret = bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        Log_e("bind failed: errno %d", errno);
        goto end_of_task;
    }

    Log_i("LOG server socket listening...");
    fd_set      sets;
    comm_peer_t peer_client = {
        .socket_id   = server_socket,
        .socket_addr = NULL,
        .addr_len    = 0,
    };

    int select_err_cnt = 0;
    int recv_err_cnt   = 0;
    while (sg_log_task_run && --server_count) {
        FD_ZERO(&sets);
        FD_SET(server_socket, &sets);
        struct timeval timeout;
        timeout.tv_sec  = SELECT_WAIT_TIME_SECONDS;
        timeout.tv_usec = 0;

        int ret = select(server_socket + 1, &sets, NULL, NULL, &timeout);
        if (ret > 0) {
            select_err_cnt = 0;
            struct sockaddr_in source_addr;
            unsigned int       addrLen        = sizeof(source_addr);
            char               rx_buffer[256] = {0};

            int len = recvfrom(server_socket, rx_buffer, sizeof(rx_buffer) - 1, MSG_DONTWAIT,
                               (struct sockaddr *)&source_addr, &addrLen);

            // Error occured during receiving
            if (len < 0) {
                recv_err_cnt++;
                Log_w("recvfrom error: %d, cnt: %d", errno, recv_err_cnt);
                if (recv_err_cnt > 3) {
                    Log_e("recvfrom error: %d, cnt: %d", errno, recv_err_cnt);
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

                if (strncmp(rx_buffer, "{\"cmdType\":3}", 12) == 0) {
                    ret = app_send_dev_log(&peer_client);
                    Log_i("app_send_dev_log ret: %d", ret);
                    break;
                }

                continue;
            }
        } else if (0 == ret) {
            select_err_cnt = 0;
            Log_d("wait for read...");
            continue;
        } else {
            select_err_cnt++;
            Log_w("select-recv error: %d, cnt: %d", errno, select_err_cnt);
            if (select_err_cnt > 3) {
                Log_e("select-recv error: %d, cnt: %d", errno, select_err_cnt);
                break;
            }
            HAL_SleepMs(500);
        }
    }

end_of_task:
    if (server_socket != -1) {
        Log_w("Shutting down LOG server socket");
        shutdown(server_socket, 0);
        close(server_socket);
    }

    stop_softAP();
//	delete_dev_log_queue();
    sg_log_task_run = false;
    Log_i("LOG server task quit");

#ifdef CONFIG_FREERTOS_USE_TRACE_FACILITY
    TaskStatus_t task_status;
    vTaskGetInfo(NULL, &task_status, pdTRUE, eRunning);
    Log_i(">>>>> task %s stack left: %u, free heap: %u", task_status.pcTaskName, task_status.usStackHighWaterMark,
          xPortGetFreeHeapSize());
#endif

    HAL_TaskDelete(NULL);
#endif
}

int qiot_log_service_start(void)
{
#ifdef WIFI_LOG_UPLOAD
    sg_log_task_run = true;
    int ret = HAL_TaskCreate(log_server_task, COMM_SERVER_TASK_NAME, COMM_SERVER_TASK_STACK_BYTES, NULL,
                             COMM_SERVER_TASK_PRIO, NULL);

    if (ret != true) {
        Log_e("create comm_server_task failed: %d", ret);
        PUSH_LOG("create comm_server_task failed: %d", ret);
        push_error_log(ERR_OS_TASK, ret);
        sg_log_task_run = false;
        return -1;
    }
#endif
    return 0;
}

void qiot_log_service_stop(void)
{
#ifdef WIFI_LOG_UPLOAD
    sg_log_task_run = false;
#endif
}

