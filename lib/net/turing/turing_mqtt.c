#include "mqtt/MQTTClient.h"
#include "device/device.h"
#include "wifi/wifi_connect.h"
#include "turing_iot.h"
#include "turing_mqtt.h"
#include "turing.h"
#include "syscfg/syscfg_id.h"
#include <time.h>
#include <stdlib.h>

#define MQTT_DEBUG_EN          1
#if MQTT_DEBUG_EN
#define MQTT_DEBUG              log_d
#define MQTT_LOGV               log_v
#define MQTT_LOGD               log_d
#define MQTT_LOGI               log_i
#define MQTT_LOGW               log_w
#define MQTT_LOGE               log_e
#define MQTT_LOGWTF             log_i
#else
#define MQTT_DEBUG(_l,...)
#define MQTT_LOGV(...)
#define MQTT_LOGD(...)
#define MQTT_LOGI(...)
#define MQTT_LOGW(...)
#define MQTT_LOGE(...)
#define MQTT_LOGWTF(...)
#endif


#define HOST                            "mqtt.ai.tuling123.com"
#define PORT                            10883

#define DEV_MQTT_QOS					QOS1

#define MQTT_SEND_BUF_LEN               1024
#define MQTT_READ_BUF_LEN               (1024 * 2)

#define DEV_ENT_RECV_TIMEOUT          	1000
#define DEV_ENT_SEND_TIMEOUT          	1000

#define MQTT_PUT_TO_MS                  (1 * 1000)
#define DEV_MQTT_KEEPALIVE_TIME       	(60 * 1000)

struct dev_mqtt_hdl {
    Client c;
    Network n;
    u8 stu;

    unsigned char sendbuf[MQTT_SEND_BUF_LEN];
    unsigned char recvbuf[MQTT_READ_BUF_LEN];

    const char *username;
    const char *password;
    const char *clientid;
    const char *topic;
    u32 timeout;
};

/*********************************************/
static struct {
    u8 close;
    u8 connected;
    char username[65];
    char password[33];
    char device_id[33];
    char product_id[10];
    char wechat_id[33];
    char api_key[65];
    int WechatTaskPid;
    int MqttTaskPid;
    void *iot_hdl;
    struct dev_mqtt_hdl *mqtt;
} turing_net_ctx;

#define __this (&turing_net_ctx)

char turing_net_ready(void)
{
    return __this->connected;
}

__attribute__((weak)) const char *get_qrcode_result_openid(void)
{
    return NULL;
}

__attribute__((weak)) const char *get_voiceprint_result_random(void)
{
    return NULL;
}

__attribute__((weak)) const char *turing_ble_cfg_net_get_openid(int *len)
{
    *len = 0;
    return NULL;
}

__attribute__((weak)) void turing_ble_cfg_net_result_notify(int succeeded, const char *mac, const char *api_key, const char *deviceid)
{

}

///////////////////////////////////////////////////////////////
static void dev_mqtt_user_msg(MessageData *md)
{
    MQTTMessage *message = md->message;

    MQTT_LOGI("payload len:%d, msg:%s\n", (u32)message->payloadlen, message->payload);

    dev_mqtt_cb_user_msg(message->payload, message->payloadlen);
}

static int sock_cb_func(enum sock_api_msg_type type, void *priv)
{
    struct dev_mqtt_hdl *mqtt = (struct dev_mqtt_hdl *)priv;
    if (__this->close) {
        return -1;
    }
    return 0;
}

static int dev_mqtt_start(struct dev_mqtt_hdl *mqtt)
{
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

    if (!mqtt->username || !mqtt->password || !mqtt->clientid || !mqtt->topic) {
        return -1;
    }

    NewNetwork(&mqtt->n);
    SetNetworkCb(&mqtt->n, sock_cb_func, mqtt);

    if (0 != ConnectNetwork(&mqtt->n, HOST, PORT)) {
        MQTT_LOGE("connectNetwork error\n");
        mqtt->n.disconnect(&mqtt->n);
        return -1;
    }

    SetNetworkRecvTimeout(&mqtt->n, 1000);

    MQTTClient(&mqtt->c, &mqtt->n, 2000, mqtt->sendbuf, MQTT_SEND_BUF_LEN, mqtt->recvbuf, MQTT_READ_BUF_LEN);

    data.willFlag = 0;
    data.MQTTVersion = 4;
    data.clientID.cstring = (char *)mqtt->clientid;
    data.username.cstring = (char *)mqtt->username;
    data.password.cstring = (char *)mqtt->password;
    data.keepAliveInterval = DEV_MQTT_KEEPALIVE_TIME / 1000;
    data.cleansession = 1;

    if (0 != MQTTConnect(&mqtt->c, &data)) {
        MQTT_LOGE("MQTTConnect failed\n");
        mqtt->n.disconnect(&mqtt->n);
        return -1;
    }

    if (0 != MQTTSubscribe(&mqtt->c, mqtt->topic, DEV_MQTT_QOS, dev_mqtt_user_msg)) {
        MQTT_LOGE("MQTTSubscribe failed \n");
        MQTTDisconnect(&mqtt->c);
        mqtt->n.disconnect(&mqtt->n);
        return -1;
    }

    mqtt->stu = 1;
    MQTT_LOGI("mqtt start ok \n");

    return 0;
}

static void dev_mqtt_stop(struct dev_mqtt_hdl *mqtt)
{
    if (!mqtt->stu) {
        return;
    }
    MQTTDisconnect(&mqtt->c);
    mqtt->n.disconnect(&mqtt->n);
    mqtt->stu = 0;
    MQTT_LOGI("mqtt stop\n");
}

static int dev_mqtt_opr(struct dev_mqtt_hdl *mqtt)
{
    return MQTTYield(&mqtt->c, MQTT_PUT_TO_MS);
}

static void dev_mqtt_set_user(struct dev_mqtt_hdl *mqtt, const char *username, const char *password, const char *clientid, const char *topic)
{
    mqtt->username = username;
    mqtt->password = password;
    mqtt->clientid = clientid;
    mqtt->topic = topic;
    mqtt->timeout = time(NULL) + 10 * 60;
}

static int get_dev_profile(void)
{
    int ret = 0;

    ret = get_turing_device_id(__this->device_id, sizeof(__this->device_id));
    if (ret) {
        return ret;
    }

    ret = get_turing_product_id(__this->product_id, sizeof(__this->product_id));
    if (ret) {
        return ret;
    }

    ret = get_turing_api_key(__this->api_key, sizeof(__this->api_key));
    if (ret) {
        return ret;
    }

    ret = get_turing_wechat_id(__this->wechat_id, sizeof(__this->wechat_id));
    if (ret) {
        return ret;
    }

    ret = get_turing_mqtt_username(__this->username, sizeof(__this->username));
    if (ret) {
        return ret;
    }

    ret = get_turing_mqtt_password(__this->password, sizeof(__this->password));
    if (ret) {
        return ret;
    }

    return ret;
}

static void turing_mqtt_task(void *priv)
{
    struct dev_mqtt_hdl *mqtt = (struct dev_mqtt_hdl *)priv;
    u32 delay = 500;
    u32 delay_cnt = 0;
    int argv[4];
    int err;
    int first = 1;
    const char *openid;

    while (1) {
        if (__this->close) {
            dev_mqtt_stop(mqtt);
            return;
        }

        if (!mqtt->stu) {
            if (delay_cnt) {
                --delay_cnt;
                os_time_dly(50);
                continue;
            } else {
                if (delay < 16 * 1000) {
                    delay <<= 1;
                }
                delay_cnt = delay / 500;
            }

            // mqtt每次连接都要判定mqtt的token是否超时
            if (time(NULL) + 3 * 60 >= mqtt->timeout) {
                // 获取mqtt的用户信息
                MQTT_LOGI("dev get mqtt user...\n");

                //自定义服务认证
                if (get_dev_profile()) {
                    continue;
                }

                u8 mac_addr[6];
                char mac[13];
                wifi_get_mac(mac_addr);
                snprintf(mac, sizeof(mac), "%02X%02X%02X%02X%02X%02X", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

                turing_iot_set_key(__this->iot_hdl, __this->api_key, __this->device_id);

                if (first) {
                    openid = turing_ble_cfg_net_get_openid(&err);
                    if (err > 0) {
                        turing_ble_cfg_net_result_notify(1, (const char *)mac_addr, __this->api_key, __this->device_id);
                        turing_iot_bt_cfg_bind(__this->iot_hdl, openid);
                    } else if (get_qrcode_result_openid()) {
                        turing_iot_sinvoice_cfg_bind(__this->iot_hdl, get_qrcode_result_openid());
                    } else if (get_voiceprint_result_random()) {
                        turing_iot_sinvoice_cfg_bind(__this->iot_hdl, get_voiceprint_result_random());
                    }
                    first = 0;
                }

                turing_iot_check_update(__this->iot_hdl);

                if (0 != turing_iot_authorize(__this->iot_hdl, atoi(__this->product_id), mac)) {
                    continue;
                }
                if (0 != turing_iot_check_authorize(__this->iot_hdl)) {
                    continue;
                }
                if (0 != turing_iot_get_topic(__this->iot_hdl)) {
                    continue;
                }
                u8 first = 1;
                if (syscfg_read(VM_FIR_INDEX, &first, sizeof(first)) > 0 || first == 1) {
                    if (0 == turing_iot_report_version(__this->iot_hdl)) {
                        first = 0;
                        syscfg_write(VM_FIR_INDEX, &first, sizeof(first));
                    }
                }

                if (first) {
                    turing_set_airkiss_para(__this->wechat_id, __this->api_key, __this->device_id);
                    turing_start_bind_device(0x7fffffff);
                    first = 0;
                }

                argv[0] = WECHAT_SEND_INIT_STATE;
                argv[1] = (int)__this->iot_hdl;

                do {
                    err = os_taskq_post_type("wechat_api_task", Q_USER, 2, argv);
                    if (err != OS_Q_FULL) {
                        break;
                    }
                    os_time_dly(20);
                } while (1);

                dev_mqtt_set_user(mqtt, __this->username, __this->password, turing_iot_get_client_id(__this->iot_hdl), turing_iot_get_topic_uuid(__this->iot_hdl));

                printf("username:%s  password:%s  client_id:%s  topoc:%s\n"
                       , mqtt->username
                       , mqtt->password
                       , mqtt->clientid
                       , mqtt->topic);
            }

            if (0 == dev_mqtt_start(mqtt)) {
                delay = 500;
                delay_cnt = 1;
                __this->connected = 1;
            }
        } else {
            if (0 != dev_mqtt_opr(mqtt)) {
                // mqtt 有错误，重新连接
                dev_mqtt_stop(mqtt);
                __this->connected = 0;
            }
        }
    }
}

void *get_turing_iot_hdl(void)
{
    return __this->iot_hdl;
}

void *turing_mqtt_init(void)
{
    __this->mqtt = calloc(1, sizeof(struct dev_mqtt_hdl));
    if (!__this->mqtt) {
        return NULL;
    }

    __this->iot_hdl = turing_iot_init();
    if (!__this->iot_hdl) {
        free(__this->mqtt);
        __this->mqtt = NULL;
        return NULL;
    }

    __this->close = 0;

    thread_fork("turing_mqtt_task", 21, 1536, 0, &__this->MqttTaskPid, turing_mqtt_task, __this->mqtt);
    thread_fork("wechat_api_task",  14, 1536, 64, &__this->WechatTaskPid, wechat_api_task, __this->iot_hdl);

    return __this->iot_hdl;
}

void turing_mqtt_uninit(void)
{
    __this->close = 1;
    __this->connected = 0;

    turing_iot_qiut_notify(__this->iot_hdl);

    int argv[4];
    int err;
    argv[0] = WECHAT_KILL_SELF_TASK;

    do {
        err = os_taskq_post_type("wechat_api_task", Q_USER, 1, argv);
        if (err != OS_Q_FULL) {
            break;
        }
        os_time_dly(10);
    } while (1);

    thread_kill(&__this->MqttTaskPid, KILL_WAIT);

    thread_kill(&__this->WechatTaskPid, KILL_WAIT);

    turing_stop_bind_device_task();

    free(__this->mqtt);
    __this->mqtt = NULL;

    turing_iot_uninit(__this->iot_hdl);

    __this->iot_hdl = NULL;
}
