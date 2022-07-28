#include "generic/circular_buf.h"
#include "generic/version.h"
#include "server/server_core.h"
#include "server/ai_server.h"
#include "dui.h"
#ifdef DUI_MEMHEAP_DEBUG
#include "mem_leak_test.h"
#endif


//通过调用版本检测函数使的模块的代码能够被链接
#ifdef DUI_SDK_VERSION
MODULE_VERSION_EXPORT(dui_sdk, DUI_SDK_VERSION);
#endif

const struct ai_sdk_api dui_sdk_api;

static const int events[][2] = {
    { AI_EVENT_SPEAK_END, DUI_SPEAK_END },
    { AI_EVENT_MEDIA_END, DUI_MEDIA_END },
    { AI_EVENT_MEDIA_STOP, DUI_MEDIA_STOP},
    { AI_EVENT_PREVIOUS_SONG, DUI_PREVIOUS_SONG },
    { AI_EVENT_NEXT_SONG, DUI_NEXT_SONG },
    { AI_EVENT_RECORD_START, DUI_RECORD_START },
    { AI_EVENT_RECORD_BREAK, DUI_RECORD_BREAK },
    { AI_EVENT_RECORD_STOP, DUI_RECORD_STOP },
    { AI_EVENT_VOICE_MODE, DUI_VOICE_MODE },
    { AI_EVENT_COLLECT_RES, DUI_COLLECT_RESOURCE },
    { AI_EVENT_SPEAK_START, DUI_SPEAK_START},
    { AI_EVENT_MEDIA_START, DUI_MEDIA_START},
    { AI_EVENT_PLAY_PAUSE, DUI_PLAY_PAUSE},
    { AI_EVENT_QUIT, DUI_QUIT },
    { AI_EVENT_VOLUME_CHANGE, DUI_VOLUME_CHANGE},
};

enum {
    DUI_SPEEK_URL,
    DUI_MEDIA_URL,
    DUI_PICTURE_URL,
};

typedef enum {
    RECORDER_START,
    RECORDER_STOP
} dui_rec_state_t;

typedef struct {
    u8 vad_stop_flag;
    u8 vad_status;
    u8 vad_enable;
} VAD_INFO;



static u8 curr_url_type;
static VAD_INFO vad_info;
static dui_rec_state_t s_dui_rec_state = RECORDER_STOP;
static OS_MUTEX mutex;
static struct server *enc_server = NULL;
static DUI_AUDIO_INFO *info = NULL;
static cbuffer_t vad_cbuf;
static u8 vad_cache_buf[ONE_CACHE_BUF_LEN];

struct list_head dui_audio_info_list_head = LIST_HEAD_INIT(dui_audio_info_list_head);

static int event2dui(int event)
{
    for (int i = 0; i < ARRAY_SIZE(events); i++) {
        if (events[i][0] == event) {
            return events[i][1];
        }
    }
    return -1;
}

static int dui_sdk_do_event(int event, int arg)
{
    int err;
    int argc = 2;
    int argv[4];
    int dui_event;

    dui_event = event2dui(event);
    if (dui_event == -1) {
        log_e("dui_event %s %d", __func__, __LINE__);
        return 0;
    }
    printf("dui_event: 0x%x\n", dui_event);
    argv[0] = dui_event;
    argv[1] = arg;
    if (dui_event == DUI_RECORD_START) {
        if (s_dui_rec_state == RECORDER_START) {
            return 0;
        }
    } else if (dui_event == DUI_MEDIA_END) {
        if (curr_url_type == DUI_SPEEK_URL) {
        } else if (curr_url_type == DUI_PICTURE_URL) {
            return 0;
        }
    }
    if (dui_event == DUI_RECORD_START ||
        dui_event == DUI_PREVIOUS_SONG ||
        dui_event == DUI_COLLECT_RESOURCE ||
        dui_event == DUI_NEXT_SONG) {
        if (get_dui_msg_notify()) {
            return 0;
        }
    }
    err = os_taskq_post_type("dui_app_task", Q_USER, argc, argv);
    if (err != OS_NO_ERR) {
        printf("send msg to dui_app_task fail, event : %d\n", dui_event);
    }

    return 0;
}

DUI_AUDIO_INFO *get_dui_audio_info(void)
{
    DUI_AUDIO_INFO *info  = NULL;
    os_mutex_pend(&mutex, 0);
    if (!list_empty(&dui_audio_info_list_head)) {
        info = list_first_entry(&dui_audio_info_list_head, DUI_AUDIO_INFO, entry); //需要加互斥
        if (info) {
            list_del(&info->entry);
        }
    }
    os_mutex_post(&mutex);
    return info;
}

void put_dui_audio_info(DUI_AUDIO_INFO *info)
{
    os_mutex_pend(&mutex, 0);
    if (info) {
        free(info);
    }
    os_mutex_post(&mutex);
}

static int dui_vfs_fwrite(void *file, void *data, u32 len)
{
    int ret = 0;
    int remain = 0;
    int data_len = len;
    u32 header = 0;
    u32 rlen = 0;
    u32 ret_len;
    u16 curr_offset = 0;
    u8  cache_buf[ONE_CACHE_BUF_LEN];

    if (s_dui_rec_state == RECORDER_STOP) {
        goto exit1;
    }

    if (vad_info.vad_enable) {

#if 0
        if (0 == vad_info.vad_status) {
            while (0 == cbuf_is_write_able(&vad_cbuf, len)) {
                if (4 != cbuf_read(&vad_cbuf, &header, 4)) {
                    goto exit1;
                }
                rlen = ntohl(header) + 4; //帧长度+帧编码参数
                if (rlen != cbuf_read(&vad_cbuf, cache_buf, rlen)) {
                    goto exit1;
                }
            }
            ret = cbuf_write(&vad_cbuf, data, len);
            goto exit1;
        }
#endif
        //只使能vad停止功能
        if (2 == vad_info.vad_status && !vad_info.vad_stop_flag) {

            DUI_OS_TASKQ_POST("dui_app_task", 1, DUI_RECORD_STOP);
            vad_info.vad_stop_flag = 1;
        }
#if 0
        if (cbuf_get_data_size(&vad_cbuf) > 0) {
            curr_offset += cbuf_read(&vad_cbuf, cache_buf, cbuf_get_data_size(&vad_cbuf));//唤醒之后就读出来
        }
#endif
    }

    os_mutex_pend(&mutex, 0);
writing:
    if (!info) {
        info = calloc(1, sizeof(DUI_AUDIO_INFO));
        if (!info) {
            log_e("\n calloc audio buf err\n");
            goto exit2;
        }
        //当且仅当VAD唤醒时拷贝一次
        if (curr_offset) {
            memcpy(info->buf, cache_buf, curr_offset);
            info->len += curr_offset;
            curr_offset = 0;
        }
        info->sessionid = get_record_sessionid();
    }
    if ((info->len + data_len) < ONE_CACHE_BUF_LEN) {
        /*当前长度加上数据长度 小于 缓冲区的数据 直接拷贝即可*/
        memcpy(info->buf + info->len, data, data_len);
        info->len += data_len;
    } else {
        remain = ONE_CACHE_BUF_LEN - info->len;
        memcpy(info->buf + info->len, data, remain);
        info->len += remain;
        data = (u8 *)data + remain;
        data_len = data_len - remain;
        list_add_tail(&info->entry, &dui_audio_info_list_head);
        info = NULL;
        if (s_dui_rec_state != RECORDER_START) {
            goto exit2;
        }

        DUI_OS_TASKQ_POST("dui_app_task", 1, DUI_RECORD_SEND);

        if (data_len != 0) {
            goto writing;
        }
    }
    ret = len;
exit2:
    os_mutex_post(&mutex);
exit1:
    return ret;
}

static int dui_vfs_fclose(void *file)
{

    os_mutex_pend(&mutex, 0);
    if (info) {
        if (info->len) {
            list_add_tail(&info->entry, &dui_audio_info_list_head);//需要加互斥
        } else {
            free(info);
        }
    }
    info = NULL;
    os_mutex_post(&mutex);

    return 0;
}

static const struct audio_vfs_ops dui_vfs_ops = {
    .fwrite = dui_vfs_fwrite,
    .fclose = dui_vfs_fclose,
};

static void enc_server_event_handler(void *priv, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_SERVER_EVENT_ERR:
        puts("dui rec: AUDIO_SERVER_EVENT_ERR\n");
        if (s_dui_rec_state == RECORDER_START) {
            DUI_OS_TASKQ_POST("dui_app_task", 1, DUI_RECORD_ERR);
        }
        break;
    case AUDIO_SERVER_EVENT_END:
        puts("dui rec: AUDIO_SERVER_EVENT_END\n");
        if (s_dui_rec_state == RECORDER_START) {
            DUI_OS_TASKQ_POST("dui_app_task", 1, DUI_RECORD_STOP);
        }
        break;
    case AUDIO_SERVER_EVENT_SPEAK_START:
        vad_info.vad_status = 1;
        puts("speak start ! \n");
        break;
    case AUDIO_SERVER_EVENT_SPEAK_STOP:
        vad_info.vad_status = 2;
        puts("speak stop ! \n");
        break;
    default:
        break;
    }
}

u8 dui_get_vad_status(void)
{
    return vad_info.vad_status;
}

__attribute__((weak)) void *get_asr_read_input_cb(void)
{
    return NULL;
}

__attribute__((weak)) u8 last_audio_dec_is_start(void)
{
    return 0;
}

int dui_recorder_start(u16 sample_rate, u8 voice_mode, u8 use_vad)
{
    int err;
    union audio_req req = {0};
    OS_SEM mic_sem;

    ASSERT(s_dui_rec_state != RECORDER_START, "s_dui_rec_state == RECORDER_START")

    if (!enc_server) {
        enc_server = server_open("audio_server", "enc");
        ASSERT(enc_server != NULL, "audio_server open err");
    }

    vad_info.vad_stop_flag = 0;
    vad_info.vad_status = 0;
    vad_info.vad_enable = use_vad;
    server_register_event_handler(enc_server, NULL, enc_server_event_handler);

    cbuf_init(&vad_cbuf, vad_cache_buf, ONE_CACHE_BUF_LEN);

    req.enc.cmd = AUDIO_ENC_OPEN;
    req.enc.channel = 1;
    req.enc.volume = 100;
    req.enc.priority = 0;
    req.enc.output_buf = NULL;
    req.enc.sample_rate = sample_rate;
    req.enc.sample_source = "mic";
    req.enc.output_buf_len = 8 * 1024;
    //req.enc.vad_start_threshold = 320;
    req.enc.msec = 15 * 1000;//录音时间
    //还可以支持amr，采样率8000
    req.enc.format = "opus";
    req.enc.vfs_ops = &dui_vfs_ops;
    //当且仅当CONFIG_ALL_ADC_CHANNEL_OPEN_ENABLE打开时，才可赋值这个参数
    if (voice_mode & 0x78) {
        u8 source = (voice_mode & 0x78) >> 3;
        if (source <= 4) {
            req.enc.channel_bit_map = BIT(source - 1);
        } else if (source == 5) {
            req.enc.sample_source = "plnk0";
        } else if (source == 6) {
            req.enc.sample_source = "plnk1";
        } else if (source == 7) {
            req.enc.sample_source = "iis0";
        } else if (source == 8) {
            req.enc.sample_source = "iis1";
        }
    }
    printf("\n  req.enc.channel_bit_map = %d\n", req.enc.channel_bit_map);
    if (use_vad) {
#if !CONFIG_CLOUD_VAD_ENABLE
        req.enc.use_vad = 1;
#endif
    }

    os_sem_create(&mic_sem, 0);
    ai_server_event_notify(&dui_sdk_api, &mic_sem, AI_SERVER_EVENT_MIC_OPEN);
    os_sem_pend(&mic_sem, 0);
    os_sem_del(&mic_sem, 1);

    u32(*read_input)(u8 *, u32) = get_asr_read_input_cb();
    if (read_input) {
        req.enc.sample_source = "virtual";
        req.enc.read_input = read_input;
        req.enc.frame_size = sample_rate == 16000 ? 640 : 320;
        req.enc.channel_bit_map = 0;
    }

    s_dui_rec_state = RECORDER_START;
    err = server_request(enc_server, AUDIO_REQ_ENC, &req);
    if (err) {
        goto exit;
    }

    return 0;
exit:
    return -1;
}

int dui_recorder_stop(u8 voice_mode)
{
    int err;
    union audio_req req = {0};

    if (!enc_server || s_dui_rec_state == RECORDER_STOP) {
        return 0;
    }

    printf("\n dui_recorder_stop\n");
    s_dui_rec_state = RECORDER_STOP;
    req.enc.cmd = AUDIO_ENC_CLOSE;
    err = server_request(enc_server, AUDIO_REQ_ENC, &req);
    if (err == 0) {
        ai_server_event_notify(&dui_sdk_api, NULL, AI_SERVER_EVENT_MIC_CLOSE);
    }

    return err;
}

void dui_media_speak_play(const char *url)
{
    curr_url_type = DUI_SPEEK_URL;
    ai_server_event_url(&dui_sdk_api, url, AI_SERVER_EVENT_URL_TTS);
}

void dui_media_audio_play(const char *url)
{
    curr_url_type = DUI_MEDIA_URL;
    ai_server_event_url(&dui_sdk_api, url, AI_SERVER_EVENT_URL);
    dui_media_set_playing_status(PLAY);
}

void dui_media_audio_continue_play(const char *url)
{
    ai_server_event_url(&dui_sdk_api, url, AI_SERVER_EVENT_CONTINUE);
    dui_media_set_playing_status(PLAY);
}

void dui_media_audio_pause_play(const char *url)
{
    ai_server_event_url(&dui_sdk_api, url, AI_SERVER_EVENT_PAUSE);
    dui_media_set_playing_status(PAUSE);
}

void dui_media_audio_resume_play(void)
{
    extern u8 last_audio_dec_is_start(void);
    if (last_audio_dec_is_start()) {
        ai_server_event_url(&dui_sdk_api, NULL, AI_SERVER_EVENT_RESUME_PLAY);
    }
}

void dui_media_audio_prev_play(void)
{
    ai_server_event_url(&dui_sdk_api, NULL, AI_SERVER_EVENT_PREV_PLAY);
}

void dui_media_audio_next_play(void)
{
    ai_server_event_url(&dui_sdk_api, NULL, AI_SERVER_EVENT_NEXT_PLAY);
}

void dui_media_audio_stop_play(const char *url)
{
    ai_server_event_url(&dui_sdk_api, url, AI_SERVER_EVENT_STOP);
}

void dui_volume_change_notify(int volume)
{
    ai_server_event_notify(&dui_sdk_api, (void *)volume, AI_SERVER_EVENT_VOLUME_CHANGE);
}

int dui_media_audio_play_seek(u32 time_elapse)
{
    return ai_server_event_notify(&dui_sdk_api, (void *)time_elapse, AI_SERVER_EVENT_SEEK);
}

int dui_tone_tts_play(int type)
{
    return 0;
}

void dui_event_notify(int event, void *arg)
{
    ai_server_event_notify(&dui_sdk_api, arg, event);
}

static int dui_sdk_open()
{
    os_mutex_create(&mutex);
    return dui_app_init();
}

static int dui_sdk_check()
{
    if (dui_app_get_connect_status()) {
        return AI_STAT_CONNECTED;
    }
    return AI_STAT_DISCONNECTED;
}

static int dui_sdk_disconnect()
{
    dui_app_uninit();
    if (enc_server) {
        server_close(enc_server);
        enc_server = NULL;
    }
    os_mutex_del(&mutex, 1);
    return 0;
}

REGISTER_AI_SDK(dui_sdk_api) = {
    .name           = "dui",
    .connect        = dui_sdk_open,
    .state_check    = dui_sdk_check,
    .do_event       = dui_sdk_do_event,
    .disconnect     = dui_sdk_disconnect,
};

