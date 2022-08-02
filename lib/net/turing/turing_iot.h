#ifndef _TURING_IOT_H_
#define _TURING_IOT_H_

#include "generic/typedef.h"

#define IOT_ADDR  "http://iot-ai.tuling123.com"
#define IOT_PORT  80


enum {
    CUSTOM_DEFINE = -1,
    DEVICE_STATE = 0,
    MUSIC_STATE,
    URGENT_STATE,
    LOCAL_MUSIC_RESOURCE_LIST,
    LOCAL_STORY_RESOURCE_LIST,
};

void *turing_iot_init(void);

int turing_iot_uninit(void *priv);

void *get_turing_iot_hdl(void);

int turing_iot_qiut_notify(void *priv);

int turing_iot_set_key(void *priv, const char *api_key, const char *device_id);

int turing_iot_authorize(void *priv, int product_id, const char *mac);

int turing_iot_check_authorize(void *priv);

int turing_iot_get_topic(void *priv);

int turing_iot_notify_status(void *priv, u8 type, const char *status_buffer);

char *turing_iot_get_client_id(void *priv);

char *turing_iot_get_topic_uuid(void *priv);

int turing_iot_get_music_res(void *priv, u8 type);

char *turing_iot_get_url(void *priv);

int turing_iot_get_song_title(void *priv, char *title, int *id);

int turing_iot_post_res(u8 *buf, u32 len);

int turing_iot_report_version(void *priv);

int turing_iot_check_update(void *priv);

int turing_iot_collect_resource(void *priv, int album_id, u8 is_play);

int turing_iot_bt_cfg_bind(void *priv, const char *openid);

int turing_iot_sinvoice_cfg_bind(void *priv, const char *openid);

#endif
