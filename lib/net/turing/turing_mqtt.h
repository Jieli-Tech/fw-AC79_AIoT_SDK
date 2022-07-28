#ifndef __TURING_MQTT_H
#define __TURING_MQTT_H

#include "generic/typedef.h"

void *turing_mqtt_init(void);

void turing_mqtt_uninit(void);

char turing_net_ready(void);

void dev_mqtt_cb_user_msg(const char *data, u32 len);

void wechat_api_task(void *arg);

int get_app_music_volume(void);
int turing_wechat_state_noitfy(u8 type, const char *status_buffer);
void turing_volume_change_notify(int volume);
void turing_wechat_play_promt(const char *fname);
void turing_wechat_media_audio_play(const char *url);
void turing_wechat_media_audio_pause(const char *url);
void turing_wechat_media_audio_continue(const char *url);
void turing_wechat_speak_play(const char *url);
int turing_wechat_next_song(char *title, int *id);
int turing_wechat_pre_song(char *title, int *id);
int turing_wechat_pause_song(char *title, int id, u8 status);
int turing_wechat_collect_resource(char *title, int *id, u8 is_play);

int get_turing_device_id(char *device_id, u32 len);
int get_turing_product_id(char *product_id, u32 len);
int get_turing_api_key(char *api_key, u32 len);
int get_turing_wechat_id(char *wechat_id, u32 len);
int get_turing_mqtt_username(char *mqtt_username, u32 len);
int get_turing_mqtt_password(char *mqtt_password, u32 len);

int turing_set_airkiss_para(const char *device_type, const char *device_id_prefix, const char *uuid);
int turing_start_bind_device(int lifecycle);
void turing_stop_bind_device_task(void);

void turing_ble_cfg_net_result_notify(int succeeded, const char *mac, const char *api_key, const char *deviceid);
const char *turing_ble_cfg_net_get_openid(int *len);
const char *get_voiceprint_result_random(void);
const char *get_qrcode_result_openid(void);

#endif
