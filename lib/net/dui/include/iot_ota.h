#ifndef __IOT_OTA_H__
#define __IOT_OTA_H__

#include "iot_mqtt.h"

#ifdef __cplusplus
extern "C" {
#endif

int dui_iot_ota_mqtt_connect(iot_mqtt_t *iot_mqtt);
void dui_iot_ota_mqtt_disconnect(iot_mqtt_t *iot_mqtt);
int dui_iot_ota_mqtt_send(iot_mqtt_t *iot_mqtt, char *buf, int size);
int dui_iot_ota_mqtt_yield(iot_mqtt_t *iot_mqtt);
int dui_iot_ota_register_callback(iot_mqtt_t *iot_mqtt, void (*msg_callback)(void *, void *, int), void *param);
int dui_iot_ota_mqtt_init(iot_mqtt_t *iot_mqtt, const char *srv, int port, const char *topic, const char *deviceid);
void dui_iot_ota_mqtt_deinit(iot_mqtt_t *iot_mqtt);

#ifdef __cplusplus
}
#endif

#endif /* !__IOT_OTA_H__ */

