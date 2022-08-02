#ifndef _IOT_H_
#define _IOT_H_

#include "iot_mqtt.h"
#include "iot_manager.h"

enum tts_type {
    TTS_COLLECT_OK,
    TTS_COLLECT_CANCEL,
    TTS_COLLECT_NONE,
};

enum media_source_type {
    MEDIA_SOURCE_AI,
    MEDIA_SOURCE_IOT,
    MEDIA_SOURCE_BT,
    MEDIA_SOURCE_DLNA,
};

typedef struct {
    char *linkUrl;
    char *imageUrl;
    char *title;
    char *album;
    char *subTitle;
    int duration;
} media_item_t;

extern void dui_media_set_source(enum media_source_type type);
extern enum media_source_type dui_media_get_source(void);
extern int dui_iot_music_play(const char *url, media_item_t *media);
extern int dui_media_music_item(media_item_t *item);
extern void dui_free_media_item(void);

extern int iot_ctl_play_dui(const media_item_t *item);
extern int iot_ctl_play_done(void);
extern int iot_ctl_play_prev(void);
extern int iot_ctl_play_next(void);
extern int iot_ctl_play_pause(void);
extern int iot_ctl_play_resume(void);
extern int iot_ctl_play_mode(int mode);
extern int iot_ctl_play_progress(int progress);
extern int iot_ctl_play_volume(int volume);
extern int iot_ctl_play_collect(void);
extern int iot_ctl_box_like(const media_item_t *item);
extern int iot_ctl_box_cancle_like(const media_item_t *item);
extern int iot_ctl_operate_collect(unsigned char add, const media_item_t *item);
extern int iot_ctl_first_online(void);
extern int iot_ctl_online(void);
extern int iot_ctl_child_identification(int on_off);
extern int iot_ctl_wakeup(int on_off);
extern int iot_ctl_network_config(void);
extern int iot_ctl_state(const media_item_t *item);
extern int iot_ctl_ota_upgrade(void);
extern int iot_ctl_ota_first_online(void);
extern int iot_ctl_ota_online(void);
extern int iot_ctl_ota_query(void);
extern int iot_add_dui_remind(const char *data);
extern int iot_del_dui_remind(const char *data);
extern int iot_speech_play(void);

#endif

