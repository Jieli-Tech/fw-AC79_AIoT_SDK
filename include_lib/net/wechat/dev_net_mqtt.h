#ifndef __DEV_NET_MQTT_H
#define __DEV_NET_MQTT_H

#include "typedef.h"

typedef void (*jl_mqtt_func)(const char *cmd, const char *parm);
void dev_mqtt_msg_cb_reg(jl_mqtt_func sys, jl_mqtt_func user);

int dev_mqtt_push(char *msg_name, char *buf, int buf_len);
int dev_mqtt_push_status(char *status_cmd, char *status_parm);
int dev_mqtt_push_playlist(char *status_cmd, char *status_parm);

#endif /*__DEV_NET_MQTT_H*/

