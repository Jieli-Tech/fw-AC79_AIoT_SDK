#include "server/ai_server.h"
#include "system/os/os_api.h"
#include "server/server_core.h"
#include "version.h"
#include "echo_cloud.h"
#include "generic/circular_buf.h"
#include "server/upgrade_server.h"
#include "sock_api/sock_api.h"

#ifdef ECHO_CLOUD_SDK_VERSION
MODULE_VERSION_EXPORT(echo_cloud_sdk, ECHO_CLOUD_SDK_VERSION);
#endif


const struct ai_sdk_api echo_cloud_sdk_api;

static const int events[][2] = {
    { AI_EVENT_SPEAK_END, ECHO_CLOUD_SPEAK_END     },
    { AI_EVENT_MEDIA_END, ECHO_CLOUD_MEDIA_END     },
    { AI_EVENT_MEDIA_STOP, ECHO_CLOUD_MEDIA_STOP     },
    { AI_EVENT_PREVIOUS_SONG, ECHO_CLOUD_PREVIOUS_SONG },
    { AI_EVENT_NEXT_SONG, ECHO_CLOUD_NEXT_SONG     },
    { AI_EVENT_RECORD_START, ECHO_CLOUD_RECORD_START  },
    { AI_EVENT_RECORD_STOP, ECHO_CLOUD_RECORD_STOP   },
    { AI_EVENT_VOLUME_CHANGE, ECHO_CLOUD_VOLUME_CHANGE  },
    { AI_EVENT_PLAY_PAUSE, ECHO_CLOUD_PLAY_PAUSE  },
    { AI_EVENT_COLLECT_RES, ECHO_CLOUD_COLLECT_RESOURCE   },
    { AI_EVENT_CHILD_LOCK, ECHO_CLOUD_CHILD_LOCK_CHANGE   },
    { AI_EVENT_CUSTOM_FUN, ECHO_CLOUD_DEVICE_CODE_REPORT  },
    { AI_EVENT_QUIT    	, ECHO_CLOUD_QUIT    	     },
};

enum {
    ECHO_CLOUD_SPEEK_URL,
    ECHO_CLOUD_MEDIA_URL,
};

typedef enum {
    RECORDER_STOP = 0,
    RECORDER_START,
} echo_cloud_rec_state_t;


#define WECHAT_ENC_MAX_DURATION		(60)	//s
#define WECHAT_ENC_BUFFER_MIN_LEN	(1600 + 6)


#if 1
#define ONE_CACHE_BUF_LEN	800	//MSS=1300
#define CACHE_BUF_LEN		(4 * ONE_CACHE_BUF_LEN)		//vad_buf 占用一半
#else
#define ONE_CACHE_BUF_LEN	1200	//MSS=1300
#define CACHE_BUF_LEN		ONE_CACHE_BUF_LEN
#endif


static struct {
    struct server *enc_server;
    struct echo_cloud_hdl hdl;
    echo_cloud_rec_state_t rec_state;
    OS_SEM sem;
    OS_MUTEX mutex;
    cbuffer_t vad_cbuf;
    int app_task_pid;
    volatile u8 exit_flag;
    u8 voice_mode;
    u8 connect_status;
    u8 vad_status;
    u8 vad_enable;
    u8 curr_url_type;
    u8 msg_notify_disable;
    u16 curr_offset;
    u32 rec_total_len;
    u8 enc_buffer[CACHE_BUF_LEN + 6];
} echo_cloud_api_hdl;

#define __this	(&echo_cloud_api_hdl)


static int event2echo_cloud(int event)
{
    for (int i = 0; i < ARRAY_SIZE(events); i++) {
        if (events[i][0] == event) {
            return events[i][1];
        }
    }

    return -1;
}

static int echo_cloud_sdk_do_event(int event, int arg)
{
    int err;
    int argc = 2;
    int argv[4];
    int echo_cloud_event;

    echo_cloud_event = event2echo_cloud(event);
    if (echo_cloud_event == -1) {
        return 0;
    }

    printf("echo_cloud_event: %x\n", echo_cloud_event);

    if (!__this->connect_status) {
        return 0;
    }

    if (echo_cloud_event == ECHO_CLOUD_RECORD_START) {
        if (__this->rec_state == RECORDER_START) {
            return 0;
        }
    } else if (echo_cloud_event == ECHO_CLOUD_MEDIA_END) {
        if (__this->curr_url_type == ECHO_CLOUD_SPEEK_URL) {
            echo_cloud_event = ECHO_CLOUD_SPEAK_END;
        }
    }

    if (echo_cloud_event != ECHO_CLOUD_RECORD_STOP &&
        echo_cloud_event != ECHO_CLOUD_CHILD_LOCK_CHANGE &&
        __this->msg_notify_disable) {
        return 0;
    }

    argv[0] = echo_cloud_event;
    argv[1] = arg;

    err = os_taskq_post_type("echo_cloud_app_task", Q_USER, argc, argv);
    if (err != OS_NO_ERR) {
        printf("send msg to echo_cloud_app_task fail, event : %d\n", echo_cloud_event);
    }

    return 0;
}

static int echo_cloud_vfs_fwrite(void *file, void *data, u32 len)
{
    u32 remain = 0;
    u32 cplen = 0;
    int err = 0;
    u32 ret_len = len;
    u8 *p = (u8 *)data;
    u8 vad_status = __this->vad_status;

    if (len == 6) {
        return len;		//跳过头部
    }

    if (__this->rec_state == RECORDER_STOP) {
        return 0;
    }

    __this->rec_total_len += len;

    if (__this->vad_enable) {
        if (0 == vad_status) {
            if (0 == cbuf_is_write_able(&__this->vad_cbuf, len)) {
                if (len != cbuf_read(&__this->vad_cbuf, __this->enc_buffer + 6, len)) {
                    return 0;
                }
            }
            cbuf_write(&__this->vad_cbuf, data, len);
            return len;
        }

        u32 cbuf_data_len = cbuf_get_data_size(&__this->vad_cbuf);

        while (cbuf_data_len > 0) {
            if (cbuf_data_len > ONE_CACHE_BUF_LEN) {
                __this->curr_offset = cbuf_read(&__this->vad_cbuf, __this->enc_buffer + 6, ONE_CACHE_BUF_LEN);
            } else {
                __this->curr_offset = cbuf_read(&__this->vad_cbuf, __this->enc_buffer + 6, cbuf_data_len);
            }
            cbuf_data_len -= __this->curr_offset;
            //echo_cloud的服务器限制了每次发800个字节,加上头部是806个字节
            if (__this->curr_offset == ONE_CACHE_BUF_LEN) {
                if (__this->rec_state != RECORDER_START) {
                    return 0;
                }
                if (__this->enc_buffer[0]) {
                    err = os_taskq_post("echo_cloud_app_task", 4, ECHO_CLOUD_RECORD_SEND, (int)__this->enc_buffer,
                                        ONE_CACHE_BUF_LEN + 6, &__this->sem);
                } else {
                    err = os_taskq_post("echo_cloud_app_task", 4, ECHO_CLOUD_RECORD_SEND, (int)(__this->enc_buffer + 6),
                                        ONE_CACHE_BUF_LEN, &__this->sem);
                }
                if (err != OS_NO_ERR) {
                    puts("echo_cloud_app_task msg full\n");
                    return 0;
                }

                if (OS_TIMEOUT == os_sem_pend(&__this->sem, 450)) {
                    return 0;
                }
                __this->enc_buffer[0] = 0;

                __this->curr_offset = 0;
            }
        }

        if (2 == vad_status) {
            if (OS_NO_ERR != os_taskq_post("echo_cloud_app_task", 1, ECHO_CLOUD_RECORD_STOP)) {
                puts("echo_cloud_app_task send msg ECHO_CLOUD_RECORD_STOP timeout ");
            }
            return ret_len;
        }
    }

    do {
        remain = ONE_CACHE_BUF_LEN - __this->curr_offset;
        cplen = remain < len ? remain : len;
        if (cplen == 0) {
            if (__this->rec_state != RECORDER_START) {
                return 0;
            }
            if (__this->enc_buffer[0]) {
                err = os_taskq_post("echo_cloud_app_task", 4, ECHO_CLOUD_RECORD_SEND, (int)__this->enc_buffer,
                                    ONE_CACHE_BUF_LEN + 6, &__this->sem);
            } else {
                err = os_taskq_post("echo_cloud_app_task", 4, ECHO_CLOUD_RECORD_SEND, (int)(__this->enc_buffer + 6),
                                    ONE_CACHE_BUF_LEN, &__this->sem);
            }
            if (err != OS_NO_ERR) {
                puts("echo_cloud_app_task msg full\n");
                return 0;
            }

            if (OS_TIMEOUT == os_sem_pend(&__this->sem, 450)) {
                return 0;
            }

            __this->enc_buffer[0] = 0;
            __this->curr_offset = 0;
        } else {
            memcpy(__this->enc_buffer + __this->curr_offset + 6, p, cplen);
            __this->curr_offset += cplen;
            p += cplen;
            len -= cplen;
        }

    } while (len);

    return ret_len;
}

static int echo_cloud_vfs_fclose(void *file)
{
    return 0;
}

static const struct audio_vfs_ops echo_cloud_vfs_ops = {
    .fwrite = echo_cloud_vfs_fwrite,
    .fclose = echo_cloud_vfs_fclose,
};

static void enc_server_event_handler(void *priv, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_SERVER_EVENT_ERR:
    case AUDIO_SERVER_EVENT_END:
        puts("echo_cloud rec: AUDIO_SERVER_EVENT_END\n");
        if (__this->rec_state == RECORDER_START) {
            if (OS_NO_ERR != os_taskq_post("echo_cloud_app_task", 1, ECHO_CLOUD_RECORD_ERR)) {
                puts("echo_cloud_app_task send msg ECHO_CLOUD_RECORD_ERR timeout ");
            }
        }
        break;
    case AUDIO_SERVER_EVENT_SPEAK_START:
        __this->vad_status = 1;
        puts("speak start ! \n");
        break;
    case AUDIO_SERVER_EVENT_SPEAK_STOP:
        __this->vad_status = 2;
        puts("speak stop ! \n");
        break;
    default:
        break;
    }
}

static int echo_cloud_send_last_packet(struct echo_cloud_http_hdl *p)
{
    int err = 0;

    if ((__this->vad_enable && !__this->vad_status) || (WECHAT_MODE != __this->voice_mode && !__this->rec_total_len)) {
        ai_server_event_notify(&echo_cloud_sdk_api, NULL, AI_SERVER_EVENT_REC_ERR);
        return 0;
    }

    if (__this->curr_offset > 0) {
        if (__this->enc_buffer[0]) {
            err = echo_cloud_enc_http_send(p, __this->enc_buffer, __this->curr_offset + 6);
            __this->enc_buffer[0] = 0;
        } else {
            err = echo_cloud_enc_http_send(p, __this->enc_buffer + 6, __this->curr_offset);
        }
        __this->curr_offset = 0;
    }

    if (!err) {
        err = echo_cloud_enc_http_send(p, NULL, 0);
    }

    return err;
}

static int echo_cloud_recorder_start(u16 sample_rate, u8 __voice_mode, u8 enable_vad)
{
    int err;
    union audio_req req = {0};
    OS_SEM mic_sem;

    puts("echo_cloud_recorder_start\n");

    if (__this->rec_state == RECORDER_START) {
        return -1;
    }

    if (!__this->enc_server) {
        __this->enc_server = server_open("audio_server", "enc");
    }

    server_register_event_handler(__this->enc_server, NULL, enc_server_event_handler);

    u8 voice_mode = __voice_mode & 0x3;
    __this->voice_mode = voice_mode;
    __this->vad_enable = enable_vad;
    __this->vad_status = 0;
    __this->curr_offset = 0;
    __this->rec_total_len = 0;

    memcpy(__this->enc_buffer, "#!AMR\n", 6);
    cbuf_init(&__this->vad_cbuf, __this->enc_buffer + 6 + 2 * ONE_CACHE_BUF_LEN, 2 * ONE_CACHE_BUF_LEN);
    os_sem_set(&__this->sem, 0);

    req.enc.cmd = AUDIO_ENC_OPEN;
    req.enc.channel = 1;
    req.enc.volume = 100;
    req.enc.priority = 0;
    req.enc.output_buf = NULL;
    req.enc.output_buf_len = 6 * 1024;
    if (voice_mode == WECHAT_MODE) {
        req.enc.msec = WECHAT_ENC_MAX_DURATION * 1000;
    } else {
        req.enc.msec = 15 * 1000;
    }
    req.enc.sample_rate = sample_rate;
    req.enc.format = "amr";
    req.enc.frame_size = 4 * 1024;
    req.enc.sample_source = "mic";
    req.enc.vfs_ops = &echo_cloud_vfs_ops;
    req.enc.channel_bit_map = (__voice_mode & 0x70) ? BIT(((__voice_mode & 0x70) >> 4) - 1) : 0;
    if (voice_mode != WECHAT_MODE && enable_vad) {
        req.enc.use_vad = 1;
    }

    os_sem_create(&mic_sem, 0);
    ai_server_event_notify(&echo_cloud_sdk_api, &mic_sem, AI_SERVER_EVENT_MIC_OPEN);
    os_sem_pend(&mic_sem, 0);
    os_sem_del(&mic_sem, 1);

    __this->rec_state = RECORDER_START;

    err = server_request(__this->enc_server, AUDIO_REQ_ENC, &req);
    if (err == 0) {
        puts("--------echo_cloud_start_rec: exit\n");
        return 0;
    }

    return -1;
}

static int echo_cloud_recorder_stop(void)
{
    int err;
    union audio_req req;

    if (!__this->enc_server || __this->rec_state == RECORDER_STOP) {
        return 0;
    }
    __this->rec_state = RECORDER_STOP;
    os_sem_post(&__this->sem);

    req.enc.cmd = AUDIO_ENC_CLOSE;

    err = server_request(__this->enc_server, AUDIO_REQ_ENC, &req);
    if (err == 0) {
        ai_server_event_notify(&echo_cloud_sdk_api, NULL, AI_SERVER_EVENT_MIC_CLOSE);
    }

    if (__this->voice_mode == WECHAT_MODE && __this->rec_total_len < WECHAT_ENC_BUFFER_MIN_LEN) {
        ai_server_event_notify(&echo_cloud_sdk_api, NULL, AI_SERVER_EVENT_REC_TOO_SHORT);
    }

    return 0;
}

void JL_echo_cloud_media_speak_play(const char *url)
{
    __this->curr_url_type = ECHO_CLOUD_SPEEK_URL;
    ai_server_event_url(&echo_cloud_sdk_api, url, AI_SERVER_EVENT_URL);
}

void JL_echo_cloud_wechat_speak_play(const char *url)
{
    char *save_url = NULL;

    if (url && strlen(url) > 0) {
        save_url = (char *)malloc(strlen(url) + 1);
        if (!save_url) {
            return;
        }
        strcpy(save_url, url);
        if (ai_server_event_notify(&echo_cloud_sdk_api, save_url, AI_SERVER_EVENT_RECV_CHAT)) {
            free(save_url);
        }
    }
}

void JL_echo_cloud_media_audio_play(const char *url)
{
    __this->curr_url_type = ECHO_CLOUD_MEDIA_URL;
    ai_server_event_url(&echo_cloud_sdk_api, url, AI_SERVER_EVENT_URL);
}

void JL_echo_cloud_media_audio_continue(const char *url)
{
    ai_server_event_url(&echo_cloud_sdk_api, url, AI_SERVER_EVENT_CONTINUE);
}

void JL_echo_cloud_media_audio_pause(const char *url)
{
    ai_server_event_url(&echo_cloud_sdk_api, url, AI_SERVER_EVENT_PAUSE);
}

void JL_echo_cloud_upgrade_notify(int event, void *arg)
{
    ai_server_event_notify(&echo_cloud_sdk_api, arg, event);
}

void JL_echo_cloud_volume_change_notify(int volume)
{
    ai_server_event_notify(&echo_cloud_sdk_api, (void *)volume, AI_SERVER_EVENT_VOLUME_CHANGE);
}

void JL_echo_cloud_child_lock_change_notify(int value)
{
    ai_server_event_notify(&echo_cloud_sdk_api, (void *)value, AI_SERVER_EVENT_CHILD_LOCK_CHANGE);
}

void JL_echo_cloud_light_change_notify(int value)
{
    ai_server_event_notify(&echo_cloud_sdk_api, (void *)value, AI_SERVER_EVENT_LIGHT_CHANGE);
}

__attribute__((weak)) int get_echo_cloud_device_id(char *device_id, u32 len)
{
    return -1;
}

__attribute__((weak)) int get_echo_cloud_hash_key(char *hash_key, u32 len)
{
    return -1;
}

__attribute__((weak)) int get_echo_cloud_uuid(char *uuid, u32 len)
{
    return -1;
}

__attribute__((weak)) const char *get_echo_cloud_firmware_version(void)
{
    return "0.0.0.0";
}

__attribute__((weak)) int check_echo_cloud_firmware_version(const char *url, const char *new_ver, const char *checksum)
{
    return 0;
}

__attribute__((weak)) int get_app_music_volume(void)
{
    return 100;
}

__attribute__((weak)) int get_app_music_playtime(void)
{
    return 0;
}

__attribute__((weak)) int get_app_music_light_intensity(void)
{
    return 100;
}

__attribute__((weak)) int get_app_music_battery_power(void)
{
    return 100;
}

__attribute__((weak)) int ai_platform_if_support_poweron_recommend(void)
{
    return 1;
}

static int get_echo_cloud_profile(struct echo_cloud_hdl *echo)
{
    int ret = 0;

    ret = get_echo_cloud_device_id(echo->auth.device_id, DEVICE_ID_LEN + 1);
    if (ret) {
        return ret;
    }

    ret = get_echo_cloud_hash_key(echo->auth.hash, HASH_LEN + 1);
    if (ret) {
        return ret;
    }

    ret = get_echo_cloud_uuid(echo->auth.channel_uuid, UUID_LEN + 1);
    if (ret) {
        return ret;
    }

    return ret;
}

static void echo_cloud_heart_beat(void *priv)
{
    struct echo_cloud_hdl *p = (struct echo_cloud_hdl *)priv;
    json_object *cmd_data = NULL;

    if (p->media_play_state != ECHO_CLOUD_MEDIA_STATE_STOP) {
        cmd_data = json_object_new_object();
        json_object_object_add(cmd_data, "token", json_object_new_string(p->token.media_play_token));
        json_object_object_add(cmd_data, "offset", json_object_new_int(get_app_music_playtime() * 1000));
        echo_cloud_msg_http_req(&p->http, MEDIA_PLAYER_PROGRESS_CHANGER, cmd_data);
        return;
    }
    if (!p->is_record) {
        cmd_data = json_object_new_object();
        echo_cloud_msg_http_req(&p->http, DEVICE_HEARTBEAT, cmd_data);
    }
}

static void echo_cloud_device_status_sync(void *priv)
{
    struct echo_cloud_hdl *p = (struct echo_cloud_hdl *)priv;
    json_object *cmd_data = NULL;

    if (!p->is_record) {
        cmd_data = json_object_new_object();
        json_object_object_add(cmd_data, "light", json_object_new_int(get_app_music_light_intensity()));
        echo_cloud_msg_http_req(&p->http, LIGHT_CHANGED, cmd_data);
        cmd_data = json_object_new_object();
        json_object_object_add(cmd_data, "battery", json_object_new_int(get_app_music_battery_power()));
        echo_cloud_msg_http_req(&p->http, BATTERY_CHANGED, cmd_data);
    }
}

static void get_device_mac_address(char *mac_address, u32 str_len)
{
    char *p = mac_address;
    char mac_addr[6];

    extern int wifi_get_mac(char *mac_addr);
    wifi_get_mac(mac_addr);

    for (int i = 0; i < 6 && i < str_len / 3; i++) {
        *(p++) = *((char *)"0123456789ABCDEF" + ((mac_addr[i] >> 4) & 0x0f));
        *(p++) = *((char *)"0123456789ABCDEF" + (mac_addr[i] & 0x0f));
        *(p++) = ':';
    }
    *(--p) = '\0';
}

static void echo_cloud_media_stop_notify(struct echo_cloud_hdl *p, int offset)
{
    if (p->media_play_state != ECHO_CLOUD_MEDIA_STATE_STOP) {
        json_object *cmd_data = json_object_new_object();
        p->media_play_state = ECHO_CLOUD_MEDIA_STATE_STOP;
        json_object_object_add(cmd_data, "token", json_object_new_string(p->token.media_play_token));
        json_object_object_add(cmd_data, "offset", json_object_new_int(offset * 1000));
        echo_cloud_msg_http_req(&p->http, MEDIA_PLAYER_STOPPED, cmd_data);
    }
}

static void echo_cloud_update_info(struct echo_cloud_hdl *p)
{
    json_object *cmd_data = NULL;

    char mac_address[18];
    get_device_mac_address(mac_address, 18);
    cmd_data = json_object_new_object();
    json_object_object_add(cmd_data, "firmware_version", json_object_new_string(get_echo_cloud_firmware_version()));
    json_object_object_add(cmd_data, "mac_address", json_object_new_string(mac_address));
    json_object_object_add(cmd_data, "reconnect", json_object_new_boolean(true));
    echo_cloud_msg_http_req(&p->http, DEVICE_ONLINE, cmd_data);

    cmd_data = json_object_new_object();
    json_object_object_add(cmd_data, "volume", json_object_new_int(get_app_music_volume()));
    echo_cloud_msg_http_req(&p->http, VOLUME_CHANGED, cmd_data);

    static const char input_text[] = {
        0xE6, 0x92, 0xAD, 0xE6, 0x94, 0xBE, 0xE5, 0x84, 0xBF, 0xE6, 0xAD, 0x8C, 0x00
    };
    if (ai_platform_if_support_poweron_recommend()) {
        cmd_data = json_object_new_object();
        /* json_object_object_add(cmd_data, "text", json_object_new_string("播放黄凯芹的歌")); */
        json_object_object_add(cmd_data, "text", json_object_new_string(input_text));
        p->media_play_opt = MEDIA_IGNORE_SPEEK;
        echo_cloud_msg_http_req(&p->http, AI_DIALOG_INPUT, cmd_data);
        p->media_play_opt = MEDIA_HAS_PALY;
    }

    p->heartbeat_timer = sys_timer_add(p, echo_cloud_heart_beat, 5000);
    p->sync_timer = sys_timer_add(p, echo_cloud_device_status_sync, 90000);
    echo_cloud_device_status_sync(p);
}

static void echo_cloud_app_task(void *priv)
{
    struct echo_cloud_hdl *p = (struct echo_cloud_hdl *)priv;
    int msg[32];
    int err;
    json_object *cmd_data = NULL;

    memset(p, 0, sizeof(struct echo_cloud_hdl));

#if 0
    strcpy(p->auth.device_id, "AI000000000000000000000000000000");
    strcpy(p->auth.channel_uuid, "2900a1f8-d1d0-41ad-90af-b59779210b28");
    strcpy(p->auth.hash, "BKsY0nCiD6tpng4sldALH1kjWuV3BnfysuVzypbC");
#else
    while (0 != get_echo_cloud_profile(p)) {
        puts("echo_cloud get profile fail ! \n");
        os_time_dly(50);
        if (__this->exit_flag) {
            __this->exit_flag = 0;
            return;
        }
    }
#endif

    p->http.auth = &p->auth;
    p->http.priv = p;
    p->http.timeout_ms = 5000;
    p->http.out_cb = echo_cloud_rcv_json_handler;
    p->media_play_opt = MEDIA_HAS_PALY;
    p->media_play_state = ECHO_CLOUD_MEDIA_STATE_STOP;

    os_mutex_pend(&__this->mutex, 0);
    if (__this->exit_flag || echo_cloud_mqtt_init(&p->auth)) {
        os_mutex_post(&__this->mutex);
        __this->exit_flag = 0;
        return;
    }
    os_mutex_post(&__this->mutex);

    echo_cloud_update_info(p);

    __this->connect_status = 1;

    while (1) {
        err = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (err != OS_TASKQ || msg[0] != Q_USER) {
            continue;
        }

        if (__this->exit_flag) {
            if (msg[1] == ECHO_CLOUD_MQTT_MSG_DEAL) {
                free((void *)msg[2]);
                continue;
            } else if (msg[1] != ECHO_CLOUD_QUIT) {
                continue;
            }
        }

        switch (msg[1]) {
        case ECHO_CLOUD_RECORD_START:
            if (!p->is_record) {
                u8 voice_mode = msg[2] & 0x3;
                if (0 == echo_cloud_enc_http_start(&p->http, voice_mode)) {
                    echo_cloud_recorder_start(8000, msg[2], msg[2] & VAD_ENABLE);
                    __this->msg_notify_disable = 1;
                    p->is_record = true;
                }
            }
            break;
        case ECHO_CLOUD_RECORD_SEND:
            if (p->is_record) {
                err = echo_cloud_enc_http_send(&p->http, (void *)msg[2], msg[3]);
                os_sem_post((OS_SEM *)msg[4]);
                if (err) {
                    echo_cloud_recorder_stop();
                    echo_cloud_enc_http_close(&p->http);
                    p->is_record = false;
                    __this->msg_notify_disable = 0;
                }
            }
            break;
        case ECHO_CLOUD_RECORD_STOP:
            if (p->is_record) {
                echo_cloud_recorder_stop();
                echo_cloud_send_last_packet(&p->http);
                echo_cloud_enc_http_close(&p->http);
                p->is_record = false;
                __this->msg_notify_disable = 0;
            }
            break;
        case ECHO_CLOUD_RECORD_ERR:
            if (p->is_record) {
                echo_cloud_recorder_stop();
                echo_cloud_enc_http_close(&p->http);
                p->is_record = false;
                __this->msg_notify_disable = 0;
                if (__this->voice_mode != WECHAT_MODE) {
                    ai_server_event_notify(&echo_cloud_sdk_api, NULL, AI_SERVER_EVENT_REC_ERR);
                }
            }
            break;
        case ECHO_CLOUD_PREVIOUS_SONG:
            puts("===========ECHO_CLOUD_PREVIOUS_SONG\n");
            p->media_play_state = ECHO_CLOUD_MEDIA_STATE_STOP;
            p->media_play_opt = MEDIA_PLAY_IMMEDIATELY;
            cmd_data = json_object_new_object();
            json_object_object_add(cmd_data, "token", json_object_new_string(p->token.media_play_token));
            echo_cloud_msg_http_req(&p->http, MEDIA_PLAYER_PREVIOUS_DEVICE, cmd_data);
            p->media_play_opt = MEDIA_HAS_PALY;
            break;
        case ECHO_CLOUD_MEDIA_END:
        case ECHO_CLOUD_NEXT_SONG:
            puts("===========ECHO_CLOUD_NEXT_SONG\n");
            p->media_play_state = ECHO_CLOUD_MEDIA_STATE_STOP;
            p->media_play_opt = MEDIA_PLAY_IMMEDIATELY;
            cmd_data = json_object_new_object();
            json_object_object_add(cmd_data, "token", json_object_new_string(p->token.media_play_token));
            echo_cloud_msg_http_req(&p->http, MEDIA_PLAYER_NEXT_DEVICE, cmd_data);
            p->media_play_opt = MEDIA_HAS_PALY;
            break;
        case ECHO_CLOUD_MEDIA_STOP:
            echo_cloud_media_stop_notify(p, msg[2]);
            break;
        case ECHO_CLOUD_SPEAK_END:
            puts("===========ECHO_CLOUD_SPEAK_END\n");
            /* cmd_data = json_object_new_object(); */
            /* json_object_object_add(cmd_data, "token", json_object_new_string(p->token.ai_dialog_token)); */
            /* echo_cloud_msg_http_req(&p->http, AI_DIALOG_FINISHED, cmd_data); */
            if (MEDIA_WAIT_PLAY == p->media_play_opt && p->media_url) {
                p->media_play_opt = MEDIA_HAS_PALY;
                JL_echo_cloud_media_audio_play(p->media_url);
                p->media_play_state = ECHO_CLOUD_MEDIA_STATE_START;
                cmd_data = json_object_new_object();
                json_object_object_add(cmd_data, "token", json_object_new_string(p->token.media_play_token));
                echo_cloud_msg_http_req(&p->http, MEDIA_PLAYER_STARTED, cmd_data);
            }
            break;
        case ECHO_CLOUD_PLAY_PAUSE:
            if (p->media_play_state != ECHO_CLOUD_MEDIA_STATE_STOP) {
                int is_pause = msg[2];
                cmd_data = json_object_new_object();
                if (is_pause) {
                    p->media_play_state = ECHO_CLOUD_MEDIA_STATE_PAUSE;
                    json_object_object_add(cmd_data, "token", json_object_new_string(p->token.media_play_token));
                    json_object_object_add(cmd_data, "offset", json_object_new_int(1000 * get_app_music_playtime()));
                    echo_cloud_msg_http_req(&p->http, MEDIA_PLAYER_PAUSED, cmd_data);
                } else {
                    p->media_play_state = ECHO_CLOUD_MEDIA_STATE_START;
                    echo_cloud_msg_http_req(&p->http, MEDIA_PLAYER_RESUMED, cmd_data);
                }
            }
            break;
        case ECHO_CLOUD_VOLUME_CHANGE:
            cmd_data = json_object_new_object();
            json_object_object_add(cmd_data, "volume", json_object_new_int(msg[2]));
            echo_cloud_msg_http_req(&p->http, VOLUME_CHANGED, cmd_data);
            break;
        case ECHO_CLOUD_COLLECT_RESOURCE:
            ;
            int is_play_collect = msg[2];
            if (!is_play_collect && p->media_url) {
                cmd_data = json_object_new_object();
                json_object_object_add(cmd_data, "token", json_object_new_string(p->token.media_play_token));
                echo_cloud_msg_http_req(&p->http, MEDIA_PLAYER_FAVORITE_DEVICE, cmd_data);
            }
            if (is_play_collect) {
                p->media_play_state = ECHO_CLOUD_MEDIA_STATE_STOP;
                p->media_play_opt = MEDIA_PLAY_IMMEDIATELY;
                cmd_data = json_object_new_object();
                echo_cloud_msg_http_req(&p->http, MEDIA_PLAYER_PLAY_FAVORITE_DEVICE, cmd_data);
                p->media_play_opt = MEDIA_HAS_PALY;
            }
            break;
        case ECHO_CLOUD_CHILD_LOCK_CHANGE:
            cmd_data = json_object_new_object();
            json_object_object_add(cmd_data, "child_lock", json_object_new_boolean(msg[2]));
            echo_cloud_msg_http_req(&p->http, CHILD_LOCK_CHANGED, cmd_data);
            break;
        case ECHO_CLOUD_DEVICE_CODE_REPORT:
            echo_cloud_media_stop_notify(p, msg[2]);
            cmd_data = json_object_new_object();
            echo_cloud_msg_http_req(&p->http, DEVICE_REQUEST_CODE, cmd_data);
            break;
        case ECHO_CLOUD_MQTT_MSG_DEAL:
            if (p->media_play_opt != MEDIA_WAIT_PLAY) {
                p->media_play_opt = MEDIA_PLAY_IMMEDIATELY;
                p->http.out_cb(p, (const char *)msg[2]);
                p->media_play_opt = MEDIA_HAS_PALY;
            } else {
                p->http.out_cb(p, (const char *)msg[2]);
            }
            free((void *)msg[2]);
            break;
        case ECHO_CLOUD_QUIT:
            sys_timer_del(p->heartbeat_timer);
            sys_timer_del(p->sync_timer);
            if (p->is_record) {
                echo_cloud_recorder_stop();
                echo_cloud_enc_http_close(&p->http);
                p->is_record = false;
            }
            echo_cloud_msg_http_close(&p->http);
            /* p->http.timeout_ms = 1000; */
            /* cmd_data = json_object_new_object(); */
            /* echo_cloud_msg_http_req(&p->http, DEVICE_OFFLINE, cmd_data); */
            free(p->media_url);
            __this->msg_notify_disable = 0;
            __this->exit_flag = 0;
            return;
        default:
            break;
        }
    }
}

static int echo_cloud_app_init(void)
{
    __this->exit_flag = 0;
    __this->connect_status = 0;

    return thread_fork("echo_cloud_app_task", 22, 1024, 128, &__this->app_task_pid, echo_cloud_app_task, &__this->hdl);
}

static void echo_cloud_app_uninit(void)
{
    __this->exit_flag = 1;
    __this->connect_status = 0;

    os_mutex_pend(&__this->mutex, 0);
    echo_cloud_mqtt_uninit();
    os_mutex_post(&__this->mutex);

    do {
        if (OS_Q_FULL != os_taskq_post("echo_cloud_app_task", 1, ECHO_CLOUD_QUIT)) {
            break;
        }
        puts("echo_cloud_app_task send msg QUIT timeout \n");
        http_cancel_dns(&__this->hdl.http.ctx);
        http_cancel_dns(&__this->hdl.http.rec_ctx);
        os_time_dly(2);
    } while (1);

    while (__this->exit_flag) {
        http_cancel_dns(&__this->hdl.http.ctx);
        http_cancel_dns(&__this->hdl.http.rec_ctx);
    }

    thread_kill(&__this->app_task_pid, KILL_WAIT);
}

static int echo_cloud_sdk_open(void)
{
    os_sem_create(&__this->sem, 0);
    os_mutex_create(&__this->mutex);
    return echo_cloud_app_init();
}

static int echo_cloud_sdk_check(void)
{
    if (__this->connect_status) {
        return AI_STAT_CONNECTED;
    }

    return AI_STAT_DISCONNECTED;
}

static int echo_cloud_sdk_disconnect(void)
{
    echo_cloud_recorder_stop();

    echo_cloud_app_uninit();

    if (__this->enc_server) {
        server_close(__this->enc_server);
        __this->enc_server = NULL;
    }
    os_sem_del(&__this->sem, 1);
    os_mutex_del(&__this->mutex, 1);

    return 0;
}

REGISTER_AI_SDK(echo_cloud_sdk_api) = {
    .name           = "echo_cloud",
    .connect        = echo_cloud_sdk_open,
    .state_check    = echo_cloud_sdk_check,
    .do_event       = echo_cloud_sdk_do_event,
    .disconnect     = echo_cloud_sdk_disconnect,
};
