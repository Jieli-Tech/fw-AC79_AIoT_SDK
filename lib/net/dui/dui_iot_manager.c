#include <string.h>
#include "os/os_api.h"
#include "iot.h"
#include "iot_ota.h"
#include "iot_parse.h"
#ifdef DUI_MEMHEAP_DEBUG
#include "mem_leak_test.h"
#endif

#define xerror printf
#define xinfo  printf
#define xdebug printf

#define MQTT_MAX_RECONNECT_DLY_MS           16000
#define MQTT_DEFAULT_RECONNECT_DLY_MS       500
#define MQTT_HEART_SEND_WND                 (100000 / OS_TICKS_PER_SEC)
#define MQTT_VERSION_SEND_WND               (36000000 / OS_TICKS_PER_SEC)

static iot_proc_t *g_iot_proc = NULL;
static char *g_iot_mqtt_apikey = NULL;

static void iot_msg_callback(void *param, void *payload, int payloadlen)
{
    iot_proc_t *iot_proc = (iot_proc_t *)param;
    msg_queue_item_t *item;

    if (NULL == payload || 0 == payloadlen) {
        xerror("Input parameters are invalid !\n");
        return;
    }

    xinfo("iot recv msg : %s\n", (char *)payload);

    item = (msg_queue_item_t *)malloc(sizeof(msg_queue_item_t) + payloadlen + 1);
    if (NULL == item) {
        return;
    }
    item->payload = sizeof(msg_queue_item_t) + (char *)item;
    item->payload_len = payloadlen;

    strcpy(item->payload, payload);

    if (os_q_post(&g_iot_proc->recv_queue, item)) {
        xerror("xQueueSend mqtt recv msg failed !\n");
        free(item);
    }
}

static void iot_ota_msg_callback(void *param, void *payload, int payloadlen)
{
    iot_proc_t *iot_proc = (iot_proc_t *)param;
    msg_queue_item_t *item;

    if (NULL == payload || 0 == payloadlen) {
        xerror("Input parameters are invalid !\n");
        return;
    }

    xinfo("ota msg : %s\n", (char *)payload);

    item = (msg_queue_item_t *)malloc(sizeof(msg_queue_item_t) + payloadlen + 1);
    if (NULL == item) {
        return;
    }
    item->payload = sizeof(msg_queue_item_t) + (char *)item;
    item->payload_len = payloadlen;

    strcpy(item->payload, payload);

    if (os_q_post(&g_iot_proc->recv_queue, item)) {
        xerror("xQueueSend mqtt recv msg failed !\n");
        free(item);
    }
}

int iot_send(char *buf)
{
    msg_queue_item_t *send_item;

    if ((NULL == buf)) {
        return -1;
    }

    if (IOT_STA_CONNECTED != g_iot_proc->status) {
        xerror("iot_proc not connected !\n");
        return -1;
    }

    send_item = (msg_queue_item_t *)malloc(sizeof(msg_queue_item_t) + strlen(buf) + 1);
    if (NULL == send_item) {
        return -1;
    }
    send_item->payload = sizeof(msg_queue_item_t) + (char *)send_item;
    send_item->payload_len = strlen(buf);

    strcpy(send_item->payload, buf);

    xinfo("iot send msg : %s\n", buf);

    if (os_q_post(&g_iot_proc->send_queue, send_item)) {
        xerror("xQueueSend failed !\n");
        free(send_item);
        return -1;
    }

    return 0;
}

int iot_ota_send(char *buf, int size)
{
    return dui_iot_ota_mqtt_send(&g_iot_proc->ota_mqtt, buf, size);
}

static void iot_poll_and_online_loop(void *param)
{
    iot_proc_t *iot_proc = (iot_proc_t *)param;

    uint32_t heart_send_tick = os_time_get();

    while (IOT_STA_CONNECTED == iot_proc->status && 0 == iot_proc->exit_flag) {
        if (iot_proc->iot_send_failed > 0) {
            break;
        }

        if ((os_time_get() - heart_send_tick) > MQTT_HEART_SEND_WND) {
            heart_send_tick = os_time_get();
            iot_ctl_online();
        }

        if (0 != dui_iot_mqtt_yield(&iot_proc->mqtt)) {
            break;
        }
        //vTaskDelay(2000/portTICK_RATE_MS);
    }

    iot_proc->iot_send_failed++;
    xdebug("=== iot_poll_and_online_loop exit  ====\n");
}

static void iot_send_queue_clear(iot_proc_t *iot_proc)
{
    msg_queue_item_t *item;

    while (1) {
        if (os_q_pend(&iot_proc->send_queue, -1, &item)) {
            return;
        }

        free(item);
    }
}

static void iot_ctl_proc(iot_proc_t *iot_proc)
{
    msg_queue_item_t *item;

    while (0 == iot_proc->exit_flag) {
        if (iot_proc->iot_send_failed > 0) {
            iot_proc->status = IOT_STA_INIT;
            break;
        }

        if (os_q_pend(&iot_proc->send_queue, 100, &item)) {
            continue;
        }

        if (0 != dui_iot_mqtt_send(&iot_proc->mqtt, item->payload, item->payload_len)) {
            iot_proc->iot_send_failed++;
        }

        free(item);
    }
}

static void iot_ctl_proc_loop(void *param)
{
    int iot_status = 0;
    iot_proc_t *iot_proc = (iot_proc_t *)param;
    int send_cfg_netword_cnt = 0;
    int iot_loop_pid = 0;
    u32 dly_cnt = 0;
    u32 delay_ms = MQTT_DEFAULT_RECONNECT_DLY_MS;

    while (0 == iot_proc->exit_flag) {
        iot_status = iot_proc->status;

        switch (iot_status) {
        case IOT_STA_INIT:
            /* xdebug("==>IOT_STA_INIT\n"); */
            iot_proc->iot_send_failed = 0;
            dui_iot_mqtt_disconnect(&iot_proc->mqtt);
            thread_kill(&iot_loop_pid, KILL_WAIT);
            iot_loop_pid = 0;
            if (0 == dly_cnt) {
                iot_proc->status++;
                break;
            }
            --dly_cnt;
            os_time_dly(10);
            break;

        case IOT_STA_CONNECTING:
            xdebug("==>IOT_STA_CONNECTING\n");
            if (0 != dui_iot_mqtt_connect(&iot_proc->mqtt)) {
                iot_proc->status = IOT_STA_INIT;
                delay_ms = (delay_ms << 1) > MQTT_MAX_RECONNECT_DLY_MS ? MQTT_MAX_RECONNECT_DLY_MS : delay_ms << 1;
                dly_cnt = delay_ms / 100;
                break;
            }
            delay_ms = MQTT_DEFAULT_RECONNECT_DLY_MS;
            dly_cnt = delay_ms / 100;
            iot_proc->status++;
            if (IOT_STA_CONNECTED == iot_proc->status) {
                iot_ctl_first_online();
            }
            break;

        case IOT_STA_CONNECTED:
            xdebug("==>IOT_STA_CONNECTED\n");
            if (iot_proc->conn_cb) {
                iot_proc->conn_cb();
            }
            if (0 == send_cfg_netword_cnt) {
                if (0 != iot_ctl_network_config()) {
                    iot_proc->status = IOT_STA_INIT;
                    break;
                }
                ++send_cfg_netword_cnt;
            }
            thread_fork("dui_iot_poll_online", 21, 1024, 0, &iot_loop_pid, iot_poll_and_online_loop, iot_proc);
            iot_ctl_proc(iot_proc);
            break;

        default:
            break;
        }
    }

    dui_iot_mqtt_disconnect(&iot_proc->mqtt);
    thread_kill(&iot_loop_pid, KILL_WAIT);
}

static void iot_parse_proc_loop(void *param)
{
    iot_proc_t *iot_proc = (iot_proc_t *)param;
    msg_queue_item_t *recv_item;

    while (1) {
        if (os_q_pend(&iot_proc->recv_queue, 0, &recv_item)) {
            xerror("Get queue item failed !\n");
            continue;
        }

        if (recv_item == (msg_queue_item_t *) - 1) {
            break;
        }

        if (!iot_proc->exit_flag) {
            dui_iot_parse_process((const char *)recv_item->payload);
        }

        free(recv_item);
    }
}

static void iot_ota_proc(iot_proc_t *iot_proc)
{
    uint32_t heart_send_tick = os_time_get();
    uint32_t version_check_tick = 0;
    bool version_first_send = false;

    while (0 == iot_proc->exit_flag) {
        if (iot_proc->ota_send_failed > 0) {
            iot_proc->ota_status = IOT_STA_INIT;
            break;
        }

        if ((os_time_get() - heart_send_tick) > MQTT_HEART_SEND_WND) {
            heart_send_tick = os_time_get();
            if (0 != iot_ctl_ota_online()) {
                iot_proc->ota_send_failed++;
                continue;
            }
        }

        if (version_first_send != true || (os_time_get() - version_check_tick) > MQTT_VERSION_SEND_WND) {
            if (version_first_send != true) {
                version_first_send = true;
            }
            version_check_tick = os_time_get();
            if (0 != iot_ctl_ota_query()) {
                iot_proc->ota_send_failed++;
                continue;
            }
        }

        if (0 != dui_iot_ota_mqtt_yield(&iot_proc->ota_mqtt)) {
            iot_proc->ota_send_failed++;
            continue;
        }
    }
}

static void iot_ota_proc_loop(void *param)
{
    int iot_ota_status = 0;
    iot_proc_t *iot_proc = (iot_proc_t *)param;
    u32 dly_cnt = 0;
    u32 delay_ms = MQTT_DEFAULT_RECONNECT_DLY_MS;

    while (0 == iot_proc->exit_flag) {
        iot_ota_status = iot_proc->ota_status;

        switch (iot_ota_status) {
        case IOT_STA_INIT:
            /* xdebug("==>IOT_OTA_INIT\n"); */
            iot_proc->ota_send_failed = 0;
            dui_iot_ota_mqtt_disconnect(&iot_proc->ota_mqtt);
            if (0 == dly_cnt) {
                iot_proc->ota_status++;
                break;
            }
            --dly_cnt;
            os_time_dly(10);
            break;

        case IOT_STA_CONNECTING:
            xdebug("==>IOT_OTA_CONNECTING\n");
            if (0 != dui_iot_ota_mqtt_connect(&iot_proc->ota_mqtt)) {
                iot_proc->ota_status = IOT_STA_INIT;
                delay_ms = (delay_ms << 1) > MQTT_MAX_RECONNECT_DLY_MS ? MQTT_MAX_RECONNECT_DLY_MS : delay_ms << 1;
                dly_cnt = delay_ms / 100;
                break;
            }
            delay_ms = MQTT_DEFAULT_RECONNECT_DLY_MS;
            dly_cnt = delay_ms / 100;
            iot_proc->ota_status++;
            if (IOT_STA_CONNECTED == iot_proc->ota_status) {
                iot_ctl_ota_first_online();
            }
            break;

        case IOT_STA_CONNECTED:
            xdebug("==>IOT_OTA_CONNECTED\n");
            iot_ota_proc(iot_proc);
            break;

        default:
            break;
        }
    }

    dui_iot_ota_mqtt_disconnect(&iot_proc->ota_mqtt);
}

static int iot_proc_init(const iot_config_para_t *cfg, iot_on_connected cb)
{
    if (!cfg->device_id
        || !cfg->iot_mqtt_srv
        || !cfg->iot_mqtt_topic
        || !cfg->iot_mqtt_apikey
        || !cfg->ota_mqtt_srv
        || !cfg->ota_mqtt_topic) {
        return -1;
    }

    g_iot_proc = (iot_proc_t *)malloc(sizeof(iot_proc_t));
    if (g_iot_proc == NULL) {
        return -1;
    }

    g_iot_mqtt_apikey = (char *)calloc(strlen(cfg->iot_mqtt_apikey) + 1, sizeof(char));
    if (!g_iot_mqtt_apikey) {
        free(g_iot_proc);
        g_iot_proc = NULL;
        return -1;
    }
    strcpy(g_iot_mqtt_apikey, cfg->iot_mqtt_apikey);

    memset(g_iot_proc, 0, sizeof(iot_proc_t));
    g_iot_proc->conn_cb = cb;

    g_iot_proc->status = IOT_STA_INIT;
    dui_iot_mqtt_init(&g_iot_proc->mqtt,
                      cfg->iot_mqtt_srv,
                      cfg->iot_mqtt_port,
                      cfg->iot_mqtt_topic,
                      cfg->device_id);
    dui_iot_register_callback(&g_iot_proc->mqtt, iot_msg_callback, g_iot_proc);

    g_iot_proc->ota_status = IOT_STA_INIT;
    dui_iot_ota_mqtt_init(&g_iot_proc->ota_mqtt,
                          cfg->ota_mqtt_srv,
                          cfg->ota_mqtt_port,
                          cfg->ota_mqtt_topic,
                          cfg->device_id);
    dui_iot_ota_register_callback(&g_iot_proc->ota_mqtt, iot_ota_msg_callback, g_iot_proc);

    os_q_create(&g_iot_proc->send_queue, IOT_MSG_QUEUE_LENGTH);
    os_q_create(&g_iot_proc->recv_queue, IOT_MSG_QUEUE_LENGTH);

    thread_fork("dui_iot_ctl_proc", 20, 1024, 0, &g_iot_proc->iot_ctl_proc_pid, iot_ctl_proc_loop, g_iot_proc);
    /* thread_fork("dui_iot_ota_proc", 21, 1024, 0, &g_iot_proc->iot_ota_proc_pid, iot_ota_proc_loop, g_iot_proc); */ //暂不启用OTA，需要时，开启该线程，实现相关逻辑
    thread_fork("dui_iot_parse_proc", 22, 768, 0, &g_iot_proc->iot_parse_proc_pid, iot_parse_proc_loop, g_iot_proc);

    return 0;
}

int iot_mgr_init(const char *device_id)
{
    iot_config_para_t cfg = {0};

    cfg.device_id = device_id;
    cfg.iot_mqtt_srv = "iot-hub.duiopen.com";
    /* cfg.iot_mqtt_topic = "2a9e40accda84cada7791c19ebec81ee"; */
    cfg.iot_mqtt_topic = "6752919c7c8745339b385bc9cc16f79a";
    cfg.iot_mqtt_apikey = "59676e4d6fa54c5b94b69d5ff1ff426a";
    cfg.iot_mqtt_port = 1883;
    cfg.ota_mqtt_srv = "mqtt.iot.aispeech.com";
    cfg.ota_mqtt_topic = "2a9e40accda84cada7791c19ebec81ee";
    cfg.ota_mqtt_port = 1883;

    return iot_proc_init(&cfg, NULL);
}

void iot_mgr_deinit(void)
{
    if (g_iot_proc) {
        g_iot_proc->exit_flag = 1;
        g_iot_proc->ota_mqtt.exit_flag = 1;
        g_iot_proc->mqtt.exit_flag = 1;
        thread_kill(&g_iot_proc->iot_ota_proc_pid, KILL_WAIT);
        thread_kill(&g_iot_proc->iot_ctl_proc_pid, KILL_WAIT);

        if (os_q_post(&g_iot_proc->recv_queue, (void *) - 1)) {
            xerror("iot_mgr_deinit post err full \n");
        }
        thread_kill(&g_iot_proc->iot_parse_proc_pid, KILL_WAIT);
        iot_send_queue_clear(g_iot_proc);
        dui_iot_mqtt_deinit(&g_iot_proc->mqtt);
        dui_iot_ota_mqtt_deinit(&g_iot_proc->ota_mqtt);
        dui_free_media_item();
        os_q_del(&g_iot_proc->send_queue, 1);
        os_q_del(&g_iot_proc->recv_queue, 1);
        free(g_iot_mqtt_apikey);
        g_iot_mqtt_apikey = NULL;
        free(g_iot_proc);
        g_iot_proc = NULL;
    }
}

const char *iot_mgr_get_mqtt_apikey(void)
{
    return g_iot_mqtt_apikey ? g_iot_mqtt_apikey : "";
}

