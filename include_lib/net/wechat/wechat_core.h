#ifndef __JIELI_NET_H
#define __JIELI_NET_H

enum {
    NET_ERROR_OK = 0,
    NET_ERROR_ONLINE,

    OAUTH_ERROR_USER,
    OAUTH_ERROR_MALLOC,
    OAUTH_ERROR_SOCK_CONNECT,
    OAUTH_ERROR_SOCK_WRITE,
    OAUTH_ERROR_SOCK_READ,
    OAUTH_ERROR_BIND,
    OAUTH_ERROR_TOKEN,
    OAUTH_ERROR_GET_WAN_IP,
    OAUTH_ERROR_SEND,
    OAUTH_ERROR_FILE,
    OAUTH_ERROR_JSON,
    OAUTH_ERROR_VOICE_INFO,

    MQTT_ERR_MALLOC,
    MQTT_ERR_PTR,
    MQTT_ERR_STU,
    MQTT_ERR_CLOSE,
    MQTT_ERR_SEND,
    MQTT_ERR_USER,
    MQTT_ERR_JSON,
};

char app_net_online(void);
char app_net_ready(void);

int wechat_core_open(void);
void wechat_core_close(void);

#endif /*__JIELI_NET_H*/

