#ifndef _IOT_MQTT_H_
#define _IOT_MQTT_H_

#include "MQTTClient.h"

#define MQTT_BUF_SIZE               (1024 * 10)    /*MQTT buf size*/
#define MQTT_PROC_BUF_SIZE          MQTT_BUF_SIZE

typedef struct {
    Network     network;
    Client      client;
    char       *server;
    int         port;
    char       *deviceid;
    char       *clientid;
    char       *topic;
    uint8_t     send_buffer[MQTT_PROC_BUF_SIZE];
    uint8_t     recv_buffer[MQTT_PROC_BUF_SIZE];
    bool        network_connected;
    bool        client_connected;
    bool        exit_flag;
    void        *param;
    void (*msg_callback)(void *, void *, int);
} iot_mqtt_t;


int dui_iot_mqtt_connect(iot_mqtt_t *iot_mqtt);
void dui_iot_mqtt_disconnect(iot_mqtt_t *iot_mqtt);
int dui_iot_mqtt_send(iot_mqtt_t *iot_mqtt, void *buf, int size);
int dui_iot_mqtt_yield(iot_mqtt_t *iot_mqtt);
int dui_iot_register_callback(iot_mqtt_t *iot_mqtt, void (*msg_callback)(void *, void *, int), void *param);
int dui_iot_mqtt_init(iot_mqtt_t *iot_mqtt, const char *srv, int port, const char *topic, const char *deviceid);
void dui_iot_mqtt_deinit(iot_mqtt_t *iot_mqtt);

#endif

