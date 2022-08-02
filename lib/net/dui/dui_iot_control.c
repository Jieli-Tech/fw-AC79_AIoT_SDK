#include <string.h>
#include "iot.h"
#include "dui.h"
#ifdef DUI_MEMHEAP_DEBUG
#include "mem_leak_test.h"
#endif

#define  IOT_SEND_MSG_MAX_SIZE   (1024)

static int iot_msg_send(const char *do_name, const char *do_data)
{
    const char *userid = dui_get_userid();
    const char *apikey = iot_mgr_get_mqtt_apikey();
    const char *deviceid = dui_get_device_id();
    const char *send_msg_format = "{\"name\":\"toy\", \"apikey\":\"%s\", \"userid\":\"%s\", \"deviceid\":\"%s\", \"data\":%s, \"do\":\"%s\"}";
    char *send_msg = NULL;
    int send_msg_len = 0;
    int ret = -1;

    if (do_name == NULL || do_data == NULL || apikey == NULL || deviceid == NULL || userid == NULL) {
        return ret;
    }

    send_msg_len = strlen(send_msg_format) + strlen(apikey) + strlen(userid) + strlen(deviceid) + strlen(do_data) + strlen(do_name) + 1;
    send_msg = (char *)calloc(send_msg_len, sizeof(char));
    if (send_msg == NULL) {
        return ret;
    }

    snprintf(send_msg, send_msg_len, send_msg_format, apikey, userid, deviceid, do_data, do_name);
    ret = iot_send(send_msg);
    if (ret) {
        printf("msg send fail : %s\n", send_msg);
    }
    free(send_msg);

    return ret;
}

int iot_ctl_play_dui(const media_item_t *item)
{
    const char *do_name = "play_dui";
    char *do_data = NULL;
    const char *do_data_format = "{\"url\":\"%s\", \"img\":\"%s\", \"song\":\"%s\", \"duration\":%d, \"album\":\"%s\", \"artist\":\"%s\"}";
    int do_data_len = strlen(do_data_format) + 16;

    if (item->linkUrl) {
        do_data_len += strlen(item->linkUrl);
    }
    if (item->imageUrl) {
        do_data_len += strlen(item->imageUrl);
    }
    if (item->title) {
        do_data_len += strlen(item->title);
    }
    if (item->album) {
        do_data_len += strlen(item->album);
    }
    if (item->subTitle) {
        do_data_len += strlen(item->subTitle);
    }

    int ret = -1;

    do_data = (char *)calloc(do_data_len, sizeof(char));
    if (!do_data) {
        return ret;
    }

    snprintf(do_data, do_data_len, do_data_format, item->linkUrl,
             item->imageUrl ? item->imageUrl : "",
             item->title ? item->title : "",
             item->duration,
             item->album ? item->album : "",
             item->subTitle ? item->subTitle : "");

    ret = iot_msg_send(do_name, do_data);
    free(do_data);

    return ret;
}

int iot_ctl_play_done(void)
{
    const char *do_name = "play_done";
    const char *do_data = "{}";
    iot_ctl_play_pause();
    return iot_msg_send(do_name, do_data);
}

int iot_ctl_play_prev(void)
{
    const char *do_name = "prev";
    const char *do_data = "{}";
    return iot_msg_send(do_name, do_data);
}

int iot_ctl_play_next(void)
{
    const char *do_name = "next";
    const char *do_data = "{}";
    return iot_msg_send(do_name, do_data);
}

int iot_ctl_play_pause(void)
{
    const char *do_name = "suspend";
    const char *do_data = "{}";
    return iot_msg_send(do_name, do_data);
}

int iot_ctl_play_resume(void)
{
    const char *do_name = "continue_play";
    const char *do_data = "{}";
    return iot_msg_send(do_name, do_data);
}

int iot_ctl_play_mode(int mode)
{
    const char *do_name = "play_mode";
    char *do_data = NULL;
    const char *do_data_format = "{\"mode\":%d}";
    int do_data_len = strlen(do_data_format) + 5;
    int ret = -1;

    do_data = (char *)calloc(do_data_len, sizeof(char));
    if (!do_data) {
        return ret;
    }

    snprintf(do_data, do_data_len, do_data_format, mode);
    ret = iot_msg_send(do_name, do_data);
    free(do_data);

    return ret;
}

int iot_ctl_play_progress(int progress)
{
    const char *do_name = "drag";
    char *do_data = NULL;
    const char *do_data_format = "{\"progress\":%d}";
    int do_data_len = strlen(do_data_format) + 32;
    int ret = -1;

    do_data = (char *)calloc(do_data_len, sizeof(char));
    if (!do_data) {
        return ret;
    }

    snprintf(do_data, do_data_len, do_data_format, progress);
    ret = iot_msg_send(do_name, do_data);
    free(do_data);

    return ret;
}

int iot_ctl_play_volume(int volume)
{
    const char *do_name = "sound";
    char *do_data = NULL;
    const char *do_data_format = "{\"volume\":%d}";
    int do_data_len = strlen(do_data_format) + 5;
    int ret = -1;

    do_data = (char *)calloc(do_data_len, sizeof(char));
    if (!do_data) {
        return ret;
    }

    snprintf(do_data, do_data_len, do_data_format, volume);
    ret = iot_msg_send(do_name, do_data);
    free(do_data);

    return ret;
}

int iot_ctl_play_collect(void)
{
    const char *do_name = "like_play";
    const char *do_data = "{}";
    return iot_msg_send(do_name, do_data);
}

int iot_ctl_operate_collect(unsigned char add, const media_item_t *item)
{
    const char *do_name = add ? "box_like" : "box_cancel_like";
    char *do_data = NULL;
    const char *do_data_format = "{\"url\":\"%s\", \"song\":\"%s\", \"artist\":\"%s\", \"img\":\"%s\", \"album\":\"%s\"}";

    int do_data_len = strlen(do_data_format) + 1;

    if (item->linkUrl) {
        do_data_len += strlen(item->linkUrl);
    }
    if (item->imageUrl) {
        do_data_len += strlen(item->imageUrl);
    }
    if (item->title) {
        do_data_len += strlen(item->title);
    }
    if (item->album) {
        do_data_len += strlen(item->album);
    }
    if (item->subTitle) {
        do_data_len += strlen(item->subTitle);
    }

    int ret = -1;

    do_data = (char *)calloc(do_data_len, sizeof(char));
    if (!do_data) {
        return ret;
    }

    snprintf(do_data, do_data_len, do_data_format, item->linkUrl,
             item->title ? item->title : "",
             item->subTitle ? item->subTitle : "",
             item->imageUrl ? item->imageUrl : "",
             item->album ? item->album : "");

    ret = iot_msg_send(do_name, do_data);
    free(do_data);

    return ret;
}

int iot_ctl_first_online(void)
{
    const char *version = dui_get_version();
    const char *do_name = "first_online";//第一个上线指令
    char *do_data = NULL;
    const char *do_data_format = "{\"device_is_lock\":\"1\", \"battery\":100, \"volume\":%d, \"version\":\"%s\"}";
    int do_data_len = strlen(do_data_format) + strlen(version) + 16;
    int ret = -1;

    do_data = (char *)calloc(do_data_len, sizeof(char));
    if (!do_data) {
        return ret;
    }

    snprintf(do_data, do_data_len, do_data_format, get_app_music_volume(), version);
    ret = iot_msg_send(do_name, do_data);
    free(do_data);

    return ret;
}

int iot_ctl_online(void)
{
    const char *do_name = "online";//心跳
    const char *do_data = "{}";
    return iot_msg_send(do_name, do_data);
}

int iot_ctl_network_config(void)
{
    const char *uid = dui_get_net_cfg_uid();
    const char *ssid = wifi_module_get_sta_ssid();
    const char *deviceid = dui_get_device_id();
    const char *product_id = dui_get_product_id();
    const char *do_name = "network_config";//设备配网成功，上报状态
    char *do_data = NULL;
    const char *do_data_format = "{\"ssid\":\"%s\", \"uid\":\"%s\", \"product_id\":\"%s\", \"client_id\":\"CID%s\"}";
    int ret = -1;
    if (!uid || !ssid || !deviceid || !product_id) {
        return ret;
    }

    int do_data_len = strlen(do_data_format) + strlen(ssid) + strlen(uid) + strlen(product_id) + strlen(deviceid) + 1;

    do_data = (char *)calloc(do_data_len, sizeof(char));
    if (!do_data) {
        return ret;
    }

    snprintf(do_data, do_data_len, do_data_format, ssid, uid, product_id, deviceid);
    ret = iot_msg_send(do_name, do_data);
    free(do_data);

    return ret;
}

int iot_ctl_child_identification(int on_off)
{
    const char *do_name = "child_identification";
    char *do_data = NULL;
    const char *do_data_format = "{\"state\":%d}";
    int do_data_len = strlen(do_data_format) + 4;
    int ret = -1;

    do_data = (char *)calloc(do_data_len, sizeof(char));
    if (!do_data) {
        return ret;
    }

    snprintf(do_data, do_data_len, do_data_format, on_off);
    ret = iot_msg_send(do_name, do_data);
    free(do_data);

    return ret;
}

int iot_ctl_wakeup(int on_off)
{
    const char *do_name = "wake_up";
    char *do_data = NULL;
    const char *do_data_format = "{\"state\":%d}";
    int do_data_len = strlen(do_data_format) + 4;
    int ret = -1;

    do_data = (char *)calloc(do_data_len, sizeof(char));
    if (!do_data) {
        return ret;
    }

    snprintf(do_data, do_data_len, do_data_format, on_off);
    ret = iot_msg_send(do_name, do_data);
    free(do_data);

    return ret;
}

int iot_ctl_state(const media_item_t *item)
{
    const char *state = NULL;
    const char *wifiSSID = wifi_module_get_sta_ssid();
    const char *version =  dui_get_version();
    const char *do_name = "state";
    char *do_data = NULL;
    const char *do_data_format = "{\"state\":\"%s\", \"wifiSsid\":\"%s\", \"version\":\"%s\", \"url\":\"%s\", \"song\":\"%s\", \"artist\":\"%s\", \"img\":\"%s\", \"album\":\"%s\", \"wifiState\":true, \"volume\":%d, \"battery\":100, \"duration\":%d, \"source\":%d}";

    if (dui_media_get_source() == MEDIA_SOURCE_AI) {
        media_item_t *dui_get_ai_music_item(void);
        item = dui_get_ai_music_item();
    }

    switch (dui_media_get_playing_status()) {
    case STOP:
        state = "free";
        break;
    case PAUSE:
        state = "suspend";
        break;
    case PLAY:
        state = "play";
        break;
    default:
        break;
    }

    int do_data_len = strlen(do_data_format) + strlen(state) + strlen(wifiSSID) + strlen(version) + 16;

    if (item->linkUrl) {
        do_data_len += strlen(item->linkUrl);
    }
    if (item->imageUrl) {
        do_data_len += strlen(item->imageUrl);
    }
    if (item->title) {
        do_data_len += strlen(item->title);
    }
    if (item->album) {
        do_data_len += strlen(item->album);
    }
    if (item->subTitle) {
        do_data_len += strlen(item->subTitle);
    }

    int ret = -1;

    do_data = (char *)calloc(do_data_len, sizeof(char));
    if (!do_data) {
        return ret;
    }

    snprintf(do_data, do_data_len, do_data_format, state, wifiSSID, version, item->linkUrl ? item->linkUrl : "",
             item->title ? item->title : "",
             item->subTitle ? item->subTitle : "",
             item->imageUrl ? item->imageUrl : "",
             item->album ? item->album : "",
             get_app_music_volume(), item->duration, (MEDIA_SOURCE_IOT == dui_media_get_source()) ? 2 : 1);
    ret = iot_msg_send(do_name, do_data);
    free(do_data);

    return ret;
}

int iot_ctl_box_like(const media_item_t *item)
{
    const char *do_name = "box_like";
    char *do_data = NULL;
    const char *do_data_format = "{\"url\":\"%s\", \"img\":\"%s\", \"song\":\"%s\", \"album\":\"%s\", \"artist\":\"%s\",\"musicType\":%d}";
    int do_data_len = strlen(do_data_format) + 16;

    if (item->linkUrl) {
        do_data_len += strlen(item->linkUrl);
    }
    if (item->imageUrl) {
        do_data_len += strlen(item->imageUrl);
    }
    if (item->title) {
        do_data_len += strlen(item->title);
    }
    if (item->album) {
        do_data_len += strlen(item->album);
    }
    if (item->subTitle) {
        do_data_len += strlen(item->subTitle);
    }

    int ret = -1;

    do_data = (char *)calloc(do_data_len, sizeof(char));
    if (!do_data) {
        return ret;
    }

    snprintf(do_data, do_data_len, do_data_format, item->linkUrl,
             item->imageUrl ? item->imageUrl : "",
             item->title ? item->title : "",
             item->album ? item->album : "",
             item->subTitle ? item->subTitle : "",
             0);

    ret = iot_msg_send(do_name, do_data);
    free(do_data);

    return ret;

}

int iot_ctl_box_cancle_like(const media_item_t *item)
{
    const char *do_name = "box_cancel_like";
    char *do_data = NULL;
    const char *do_data_format = "{\"url\":\"%s\", \"img\":\"%s\", \"song\":\"%s\", \"album\":\"%s\", \"artist\":\"%s\"}";
    int do_data_len = strlen(do_data_format) + 16;

    if (item->linkUrl) {
        do_data_len += strlen(item->linkUrl);
    }
    if (item->imageUrl) {
        do_data_len += strlen(item->imageUrl);
    }
    if (item->title) {
        do_data_len += strlen(item->title);
    }
    if (item->album) {
        do_data_len += strlen(item->album);
    }
    if (item->subTitle) {
        do_data_len += strlen(item->subTitle);
    }

    int ret = -1;

    do_data = (char *)calloc(do_data_len, sizeof(char));
    if (!do_data) {
        return ret;
    }

    snprintf(do_data, do_data_len, do_data_format, item->linkUrl,
             item->imageUrl ? item->imageUrl : "",
             item->title ? item->title : "",
             item->album ? item->album : "",
             item->subTitle ? item->subTitle : "");

    ret = iot_msg_send(do_name, do_data);
    free(do_data);

    return ret;

}

//设备查询语音消息
int iot_speech_play(void)
{
    const char *do_name = "speech_play";
    const char *do_data = "{}";
    return iot_msg_send(do_name, do_data);
}

//通知app闹钟,提醒创建
int iot_add_dui_remind(const char *data)
{
    const char *do_name = "add_dui_remind";
    const char *do_data = data;
    return iot_msg_send(do_name, do_data);
}

//通知app闹钟,提醒删除
int iot_del_dui_remind(const char *data)
{
    const char *do_name = "del_dui_remind";
    const char *do_data = data;
    return iot_msg_send(do_name, do_data);
}

int iot_ctl_ota_upgrade(void)
{
    const char *deviceid = dui_get_device_id();
    const char *product_code = dui_get_product_code();
    const char *version = dui_get_version();
    char *msg_data = NULL;
    const char *msg_data_format = "{\"name\":\"toy\", \"deviceid\":\"%s\", \"data\":{\"product_code\":\"%s\",\"publish_id\":1,\"version\":\"%s\"}, \"do\":\"upgrade\"}";
    int msg_data_len = strlen(msg_data_format) + strlen(deviceid) + strlen(product_code) + strlen(version) + 1;
    int ret = -1;

    msg_data = (char *)calloc(msg_data_len, sizeof(char));
    if (!msg_data) {
        return ret;
    }

    snprintf(msg_data, msg_data_len, msg_data_format, deviceid, product_code, version);
    ret = iot_ota_send(msg_data, strlen(msg_data));
    free(msg_data);

    return ret;
}

int iot_ctl_ota_first_online(void)
{
    const char *deviceid = dui_get_device_id();
    const char *version = dui_get_version();
    char *msg_data = NULL;
    const char *msg_data_format = "{\"name\":\"toy\",\"deviceid\":\"%s\",\"data\":{\"device_is_lock\":\"1\",\"volume\":%d,\"battery\":100,\"version\":\"%s\"},\"do\":\"first_online\"}";
    int msg_data_len = strlen(msg_data_format) + strlen(deviceid) + strlen(version) + 5;
    int ret = -1;

    msg_data = (char *)calloc(msg_data_len, sizeof(char));
    if (!msg_data) {
        return ret;
    }

    snprintf(msg_data, msg_data_len, msg_data_format, deviceid, get_app_music_volume(), version);
    ret = iot_ota_send(msg_data, strlen(msg_data));
    free(msg_data);

    return ret;
}

int iot_ctl_ota_online(void)
{
    const char *deviceid = dui_get_device_id();
    char *msg_data = NULL;
    const char *msg_data_format = "{\"name\":\"toy\",\"deviceid\":\"%s\",\"data\":{},\"do\":\"online\"}";
    int msg_data_len = strlen(msg_data_format) + strlen(deviceid) + 1;
    int ret = -1;

    msg_data = (char *)calloc(msg_data_len, sizeof(char));
    if (!msg_data) {
        return ret;
    }

    snprintf(msg_data, msg_data_len, msg_data_format, deviceid);
    ret = iot_ota_send(msg_data, strlen(msg_data));
    free(msg_data);

    return ret;
}

int iot_ctl_ota_query(void)
{
    const char *deviceid = dui_get_device_id();
    const char *product_code = dui_get_product_code();
    const char *version = dui_get_version();
    char *msg_data = NULL;
    const char *msg_data_format = "{\"name\":\"toy\",\"deviceid\":\"%s\",\"data\":{\"product_code\":\"%s\",\"version\":\"%s\"},\"do\":\"box_silence_version\"}";
    int msg_data_len = strlen(msg_data_format) + strlen(deviceid) + strlen(product_code) + strlen(version) + 1;
    int ret = -1;

    msg_data = (char *)calloc(msg_data_len, sizeof(char));
    if (!msg_data) {
        return ret;
    }

    snprintf(msg_data, msg_data_len, msg_data_format, deviceid, product_code, version);
    ret = iot_ota_send(msg_data, strlen(msg_data));
    free(msg_data);

    return ret;
}

