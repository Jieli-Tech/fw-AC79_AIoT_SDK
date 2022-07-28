#include <string.h>
#include <time.h>
#include "mbedtls/base64.h"
#include "iot_rsa.h"
#include "iot_mqtt.h"
#ifdef DUI_MEMHEAP_DEBUG
#include "mem_leak_test.h"
#endif
#define xerror printf
#define xinfo  printf

#define DUI_MQTT_QOS QOS0
static iot_mqtt_t *g_iot_mqtt = NULL;

static void mqtt_msg_callback(MessageData *md)
{
    if ((NULL == md)
        || (NULL == md->message)
        || (NULL == md->message->payload)
        || (0 == md->message->payloadlen)) {
        xerror("Input parameters are invalid !\n");
        return;
    }

    if (g_iot_mqtt->msg_callback != NULL && g_iot_mqtt->param != NULL) {
        g_iot_mqtt->msg_callback(g_iot_mqtt->param, md->message->payload, md->message->payloadlen);
        memset(md->message->payload, 0, md->message->payloadlen);
    }
}

static char *rsa_encrypt(const char *src)
{
    char *ciphertext = (char *)malloc(256);
    if (ciphertext == NULL) {
        return NULL;
    }

    memset(ciphertext, 0, 256);
    speech_rsa_encrypt(src, strlen(src), ciphertext);
    return ciphertext;
}

static time_t get_timestamp(void)
{
#if 1
    return time(NULL);
#else
    struct timeval ntp_time;
    struct tm *time = NULL;

    if (sntp_get_time(NULL, &ntp_time) == 0) {
        time = localtime(&ntp_time.tv_sec);
        LOG_D("ntp timer tv_sec %d\n", ntp_time.tv_sec);
        LOG_D("<sntp> %u-%02u-%02u %02u:%02u:%02u %02u\n",
              time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
              time->tm_hour, time->tm_min, time->tm_sec, time->tm_wday);
    }

    return ntp_time.tv_sec;
#endif
}

static int get_RSA_base64_password(const char *username, char *password)
{
    char temp[256];
    size_t len = 0;

    time_t timestamp = get_timestamp();
    sprintf(temp, "%s###%ld", username, timestamp);
    xinfo("%s", temp);

    char *rsa = rsa_encrypt(temp);
    if (rsa != NULL) {
        if (mbedtls_base64_encode((unsigned char *)password, 256, &len,
                                  (const unsigned char *)rsa, 128) != 0) {
            xerror("mbedtls_base64_encode failed\n");
            free(rsa);
            rsa = NULL;
            return -1;
        } else {
            free(rsa);
            rsa = NULL;
            return 0;
        }
    } else {
        xerror("rsa_encrypt_test failed\n");
        return -1;
    }
}

static int sock_cb_func(enum sock_api_msg_type type, void *priv)
{
    iot_mqtt_t *iot_mqtt = (iot_mqtt_t *)priv;
    if (iot_mqtt->exit_flag) {
        return -1;
    }
    return 0;
}

int dui_iot_mqtt_connect(iot_mqtt_t *iot_mqtt)
{
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

    NewNetwork(&iot_mqtt->network);
    SetNetworkCb(&iot_mqtt->network, sock_cb_func, iot_mqtt);

    if (0 != ConnectNetwork(&iot_mqtt->network, iot_mqtt->server, iot_mqtt->port)) {
        iot_mqtt->network.disconnect(&iot_mqtt->network);
        xerror("ConnectNetwork failed\n");
        return -1;
    }

    SetNetworkRecvTimeout(&iot_mqtt->network, 1000);

    iot_mqtt->network_connected = true;

    MQTTClient(
        &iot_mqtt->client,
        &iot_mqtt->network,
        2000,
        iot_mqtt->send_buffer,
        MQTT_PROC_BUF_SIZE,
        iot_mqtt->recv_buffer,
        MQTT_PROC_BUF_SIZE);

    char password[256] = {0};
    get_RSA_base64_password(iot_mqtt->deviceid, password);

    data.MQTTVersion       = 4;
    data.clientID.cstring  = iot_mqtt->clientid;
    data.username.cstring  = iot_mqtt->deviceid;
    data.password.cstring  = password;
    data.keepAliveInterval = 30;
    data.cleansession      = 1;
    data.willFlag          = 1;
    char will_topic[128] = {0};
    sprintf(will_topic, "%s_will", iot_mqtt->topic);
    char will_message[256] = {0};
    sprintf(will_message, "{\"name\":\"toy\",\"userid\":\"%s\",\"deviceid\":\"%s\",\"data\":{},\"do\":\"will\"}",
            "", iot_mqtt->deviceid);
    data.will.topicName.cstring = will_topic;
    data.will.message.cstring = will_message;

    xinfo("clientID: %s\n", data.clientID.cstring);
    xinfo("username: %s\n", data.username.cstring);
    xinfo("password: %s\n", data.password.cstring);

    if (0 != MQTTConnect(&iot_mqtt->client, &data)) {
        xerror("MQTTConnect failed\n");
        return -1;
    }

    iot_mqtt->client_connected = true;

    if (0 != MQTTSubscribe(&iot_mqtt->client, iot_mqtt->deviceid, DUI_MQTT_QOS, mqtt_msg_callback)) {
        xerror("MQTTSubscribe failed\n");
        return -1;
    }

    xinfo("MQTTConnect success\n");

    return 0;
}

void dui_iot_mqtt_disconnect(iot_mqtt_t *iot_mqtt)
{
    if (true == iot_mqtt->client_connected) {
        iot_mqtt->client_connected = false;
        MQTTDisconnect(&iot_mqtt->client);
    }

    if (true == iot_mqtt->network_connected) {
        iot_mqtt->network_connected = false;
        iot_mqtt->network.disconnect(&iot_mqtt->network);
    }
}

int dui_iot_mqtt_send(iot_mqtt_t *iot_mqtt, void *buf, int size)
{
    MQTTMessage message;

    if (NULL == iot_mqtt || NULL == buf || size == 0) {
        return 0;
    }

    message.id         = 0;
    message.qos        = DUI_MQTT_QOS;
    message.retained   = false;
    message.dup        = false;
    message.payload    = buf;
    message.payloadlen = size;

    if (0 != MQTTPublish(&iot_mqtt->client, iot_mqtt->topic, &message))   {
        xerror("MQTT msg publish failed!\n");
        return -1;
    }

    return 0;
}

int dui_iot_mqtt_yield(iot_mqtt_t *iot_mqtt)
{
    return MQTTYield(&iot_mqtt->client, 1000);
}

int dui_iot_register_callback(iot_mqtt_t *iot_mqtt,
                              void (*msg_callback)(void *, void *, int),
                              void *param)
{
    if (iot_mqtt == NULL || msg_callback == NULL || param == NULL) {
        return -1;
    }

    iot_mqtt->param = param;
    iot_mqtt->msg_callback = msg_callback;

    return 0;
}

int dui_iot_mqtt_init(iot_mqtt_t *iot_mqtt, const char *srv, int port, const char *topic, const char *deviceid)
{
    if (iot_mqtt == NULL || srv == NULL || topic == NULL || deviceid == NULL) {
        return -1;
    }

    iot_mqtt->server = (char *)malloc(strlen(srv) + 1);
    if (iot_mqtt->server == NULL) {
        goto err_exit;
    }
    strcpy(iot_mqtt->server, srv);
    iot_mqtt->port = port;

    iot_mqtt->topic = (char *)malloc(strlen(topic) + 1);
    if (iot_mqtt->topic == NULL) {
        goto err_exit;
    }
    strcpy(iot_mqtt->topic, topic);

    iot_mqtt->deviceid = (char *)malloc(strlen(deviceid) + 1);
    if (iot_mqtt->deviceid == NULL) {
        goto err_exit;
    }
    strcpy(iot_mqtt->deviceid, deviceid);

    iot_mqtt->clientid = (char *)malloc(strlen(deviceid) + 4);
    if (iot_mqtt->clientid == NULL) {
        goto err_exit;
    }
    sprintf(iot_mqtt->clientid, "CID%s", deviceid);

    iot_mqtt->network_connected = false;
    iot_mqtt->client_connected = false;
    iot_mqtt->param = NULL;
    iot_mqtt->msg_callback = NULL;

    g_iot_mqtt = iot_mqtt;

    return 0;

err_exit:
    dui_iot_mqtt_deinit(iot_mqtt);
    return -1;
}

void dui_iot_mqtt_deinit(iot_mqtt_t *iot_mqtt)
{
    if (iot_mqtt->server) {
        free(iot_mqtt->server);
        iot_mqtt->server = NULL;
    }

    if (iot_mqtt->topic) {
        free(iot_mqtt->topic);
        iot_mqtt->topic = NULL;
    }

    if (iot_mqtt->deviceid) {
        free(iot_mqtt->deviceid);
        iot_mqtt->deviceid = NULL;
    }

    if (iot_mqtt->clientid) {
        free(iot_mqtt->clientid);
        iot_mqtt->clientid = NULL;
    }

    iot_mqtt->param = NULL;
    iot_mqtt->msg_callback = NULL;

    g_iot_mqtt = NULL;
}

