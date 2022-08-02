#ifndef _IOT_MANAGER_H_
#define _IOT_MANAGER_H_

#include "iot_mqtt.h"

#define IOT_MSG_QUEUE_LENGTH        (20)

typedef void (*iot_on_connected)(void);

typedef enum {
    IOT_STA_INIT = 0,
    IOT_STA_CONNECTING,
    IOT_STA_CONNECTED,
    IOT_STA_UNKOWN
} iot_state_t;

typedef struct {
    void *payload;
    uint32_t payload_len;
} msg_queue_item_t;

typedef struct {
    iot_mqtt_t          mqtt;
    iot_mqtt_t          ota_mqtt;

    OS_QUEUE       recv_queue;
    OS_QUEUE       send_queue;

    iot_state_t         status;
    iot_state_t         ota_status;

    unsigned int        iot_send_failed;
    unsigned int        ota_send_failed;
    unsigned int        heart_tick;

    iot_on_connected    conn_cb;

    int iot_ctl_proc_pid;
    int iot_ota_proc_pid;
    int iot_parse_proc_pid;

    volatile int exit_flag;
} iot_proc_t;

typedef struct iot_config_para {
    const char   *device_id;

    const char   *iot_mqtt_srv;
    const char   *iot_mqtt_topic;
    const char   *iot_mqtt_apikey;
    int           iot_mqtt_port;

    const char   *ota_mqtt_srv;
    const char   *ota_mqtt_topic;
    int           ota_mqtt_port;
} iot_config_para_t;

extern int iot_send(char *buf);
extern int iot_ota_send(char *buf, int size);
extern int iot_mgr_init(const char *device_id);
extern void iot_mgr_deinit(void);
extern const char *iot_mgr_get_mqtt_apikey(void);

#endif

