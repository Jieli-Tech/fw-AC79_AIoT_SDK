#include <string.h>
#include "MQTTClient.h"
#include "iot_ota.h"
#ifdef DUI_MEMHEAP_DEBUG
#include "mem_leak_test.h"
#endif

#define xerror printf
#define xinfo  printf

static iot_mqtt_t *g_ota_mqtt = NULL;

static int sock_cb_func(enum sock_api_msg_type type, void *priv)
{
    iot_mqtt_t *iot_mqtt = (iot_mqtt_t *)priv;
    if (iot_mqtt->exit_flag) {
        return -1;
    }
    return 0;
}

static void mqtt_ota_msg_callback(MessageData *md)
{
    if ((NULL == md)
        || (NULL == md->message)
        || (NULL == md->message->payload)
        || (0 == md->message->payloadlen)) {
        xerror("Input parameters are invalid !\n");
        return;
    }

    if (g_ota_mqtt->msg_callback != NULL && g_ota_mqtt->param != NULL) {
        g_ota_mqtt->msg_callback(g_ota_mqtt->param, md->message->payload, md->message->payloadlen);
    }
}

int dui_iot_ota_mqtt_connect(iot_mqtt_t *iot_mqtt)
{
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

    NewNetwork(&iot_mqtt->network);
    SetNetworkCb(&iot_mqtt->network, sock_cb_func, iot_mqtt);

    if (0 != ConnectNetwork(&iot_mqtt->network, iot_mqtt->server, iot_mqtt->port)) {
        xerror("ConnectNetwork failed\n");
        return -1;
    }

    SetNetworkRecvTimeout(&iot_mqtt->network, 2000);

    iot_mqtt->network_connected = true;

    MQTTClient(
        &iot_mqtt->client,
        &iot_mqtt->network,
        2000,
        iot_mqtt->send_buffer,
        MQTT_PROC_BUF_SIZE,
        iot_mqtt->recv_buffer,
        MQTT_PROC_BUF_SIZE);

    data.MQTTVersion       = 4;
    data.clientID.cstring  = iot_mqtt->clientid;
    data.username.cstring  = "admin";
    data.password.cstring  = "password";
    data.keepAliveInterval = 30;
    data.cleansession      = 1;
    data.willFlag          = 0;

    xinfo("clientID: %s\n", data.clientID.cstring);
    xinfo("deviceid: %s\n", iot_mqtt->deviceid);
    xinfo("username: %s\n", data.username.cstring);
    xinfo("password: %s\n", data.password.cstring);

    if (0 != MQTTConnect(&iot_mqtt->client, &data)) {
        xerror("MQTTConnect failed\n");
        return -1;
    }

    iot_mqtt->client_connected = true;

    if (0 != MQTTSubscribe(&iot_mqtt->client, iot_mqtt->deviceid, QOS0, mqtt_ota_msg_callback)) {
        xerror("MQTTSubscribe failed\n");
        return -1;
    }

    xinfo("OTA MQTTConnect success\n");

    return 0;
}

void dui_iot_ota_mqtt_disconnect(iot_mqtt_t *iot_mqtt)
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

int dui_iot_ota_mqtt_send(iot_mqtt_t *iot_mqtt, char *buf, int size)
{
    MQTTMessage message;

    if (NULL == iot_mqtt || NULL == buf || 0 == size) {
        xerror("Input parameters are invalid !\n");
        return 0;
    }

    message.id         = 0;
    message.qos        = QOS0;
    message.retained   = false;
    message.dup        = false;
    message.payload    = buf;
    message.payloadlen = size;

    if (0 != MQTTPublish(&iot_mqtt->client, iot_mqtt->topic, &message)) {
        xerror("MQTT msg publish failed!\n");
        return -1;
    }

    return 0;
}

int dui_iot_ota_mqtt_yield(iot_mqtt_t *iot_mqtt)
{
    return MQTTYield(&iot_mqtt->client, 1000);
}

int dui_iot_ota_register_callback(iot_mqtt_t *iot_mqtt,
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

int dui_iot_ota_mqtt_init(iot_mqtt_t *iot_mqtt, const char *srv, int port, const char *topic, const char *deviceid)
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

    g_ota_mqtt = iot_mqtt;

    return 0;

err_exit:
    dui_iot_ota_mqtt_deinit(iot_mqtt);
    return -1;
}

void dui_iot_ota_mqtt_deinit(iot_mqtt_t *iot_mqtt)
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

    g_ota_mqtt = NULL;
}

