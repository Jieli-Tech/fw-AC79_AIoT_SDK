#include "server/ai_server.h"
#include "os/os_api.h"
#include "server/server_core.h"
#include "version.h"
#include "turing.h"
#include "generic/circular_buf.h"
#include "turing_iot.h"

#ifdef TURING_SDK_VERSION
MODULE_VERSION_EXPORT(turing_sdk, TURING_SDK_VERSION);
#endif


const struct ai_sdk_api turing_sdk_api;

static const int events[][2] = {
    { AI_EVENT_SPEAK_END, TURING_SPEAK_END      },
    { AI_EVENT_MEDIA_END, TURING_MEDIA_END      },
    { AI_EVENT_PREVIOUS_SONG, TURING_PREVIOUS_SONG },
    { AI_EVENT_NEXT_SONG, TURING_NEXT_SONG      },
    { AI_EVENT_RECORD_BREAK, TURING_RECORD_ERR  },
    { AI_EVENT_RECORD_START, TURING_RECORD_START},
    { AI_EVENT_RECORD_STOP, TURING_RECORD_STOP  },
    { AI_EVENT_VOICE_MODE, TURING_VOICE_MODE    },
    { AI_EVENT_COLLECT_RES, TURING_COLLECT_RESOURCE   },
    { AI_EVENT_QUIT    	, TURING_QUIT    	    },
};

enum {
    TURING_SPEEK_URL,
    TURING_MEDIA_URL,
    TURING_PICTURE_URL,
};

typedef enum {
    RECORDER_START,
    RECORDER_STOP
} turing_rec_state_t;


#define TURING_SAMPLE_RATE	16000
#define ONE_CACHE_BUF_LEN	1344

static OS_SEM sem;
static u8 cache_buf[ONE_CACHE_BUF_LEN];
static u16 curr_offset;
static u8 curr_url_type;
static u8 vad_status;
static turing_rec_state_t s_turing_rec_state = RECORDER_STOP;
static struct server *enc_server = NULL;

static int event2turing(int event)
{
    for (int i = 0; i < ARRAY_SIZE(events); i++) {
        if (events[i][0] == event) {
            return events[i][1];
        }
    }

    return -1;
}

static int turing_sdk_do_event(int event, int arg)
{
    int err;
    int argc = 2;
    int argv[4];
    int turing_event;

    turing_event = event2turing(event);
    if (turing_event == -1) {
        return 0;
    }

    argv[0] = turing_event;
    argv[1] = arg;

    printf("turing_event: %x\n", turing_event);

    if (!turing_app_get_connect_status()) {
        return 0;
    }

    if (turing_event == TURING_RECORD_START && s_turing_rec_state == RECORDER_START) {
        return 0;
    } else if (turing_event == TURING_MEDIA_END && curr_url_type == TURING_PICTURE_URL) {
        argv[0] = TURING_PICTURE_PLAY_END;
    }

    if (turing_event == TURING_RECORD_START ||
        turing_event == TURING_PREVIOUS_SONG ||
        turing_event == TURING_COLLECT_RESOURCE ||
        turing_event == TURING_NEXT_SONG) {
        if (get_turing_msg_notify()) {
            return 0;
        }
    }

    err = os_taskq_post_type("turing_app_task", Q_USER, argc, argv);
    if (err != OS_NO_ERR) {
        printf("send msg to turing_app_task fail, event : %d\n", turing_event);
    }

    return 0;
}

static const u8 stop_cache_buf[] = {
    0x00, 0x00, 0x00, 0x08, 0x07, 0xBE, 0x5D, 0x3C, 0x48, 0x0B, 0xE4, 0xC1, 0x22, 0x23, 0x61, 0xF0,
    0x00, 0x00, 0x00, 0x07, 0x02, 0xD5, 0x6C, 0x75, 0x48, 0x07, 0xC9, 0x79, 0xC1, 0x24, 0x58,
    0x00, 0x00, 0x00, 0x07, 0x02, 0xD5, 0x6C, 0x75, 0x48, 0x07, 0xC9, 0x79, 0xC1, 0x24, 0x58,
    0x00, 0x00, 0x00, 0x08, 0x18, 0xE4, 0x9E, 0xE1, 0x48, 0x07, 0xC9, 0x72, 0x27, 0xDC, 0x06, 0x40,
    0x00, 0x00, 0x00, 0x08, 0x18, 0xE4, 0x9E, 0xE1, 0x48, 0x07, 0xC9, 0x72, 0x27, 0xDC, 0x06, 0x40,
};

static int turing_vfs_fwrite(void *file, void *data, u32 len)
{
    int remain = 0;
    int cplen = 0;
    int err = 0;
    u32 ret_len = len;
    u8 *p = (u8 *)data;

    if (s_turing_rec_state == RECORDER_STOP) {
        return 0;
    }

    if (vad_status == 4) {
        return len;
    }

    if (2 == vad_status) {
        if (OS_NO_ERR != os_taskq_post("turing_app_task", 1, TURING_RECORD_STOP)) {
            puts("turing_app_task send msg TURING_RECORD_STOP timeout ");
        } else {
            vad_status = 3;	//通知结束录音，最后还需要发送一个结束包
        }
    }

    do {
        remain = ONE_CACHE_BUF_LEN - curr_offset;
        cplen = remain < len ? remain : len;
        if (cplen == 0 || remain < len) {
            if (s_turing_rec_state != RECORDER_START) {
                return 0;
            }
            err = os_taskq_post("turing_app_task", 4, TURING_RECORD_SEND, (int)cache_buf,
                                curr_offset, &sem);
            if (err != OS_NO_ERR) {
                puts("turing_app_task msg full\n");
                return 0;
            }

            if (OS_TIMEOUT == os_sem_pend(&sem, 400)) {
                return 0;
            }
            curr_offset = 0;
        } else {
            memcpy(cache_buf + curr_offset, p, cplen);
            curr_offset += cplen;
            p += cplen;
            len -= cplen;
        }
    } while (len);

    if (vad_status != 4 && get_turing_rec_is_stopping() && curr_offset > 0) {
        err = os_taskq_post("turing_app_task", 4, TURING_RECORD_SEND, (int)cache_buf,
                            curr_offset, &sem);
        if (err != OS_NO_ERR) {
            puts("turing_app_task msg full\n");
            return 0;
        }
        curr_offset = 0;
        vad_status = 4;	//发送最后一包
    }

    return ret_len;
}

static int turing_vfs_fclose(void *file)
{
    return 0;
}

static const struct audio_vfs_ops turing_vfs_ops = {
    .fwrite = turing_vfs_fwrite,
    .fclose = turing_vfs_fclose,
};

#define WECHAT_BUFFER_LEN	(50 * 1024)

static u8 *wechat_buffer = NULL;

static int wechat_vfs_fwrite(void *file, void *data, u32 len)
{
    if (curr_offset + len > WECHAT_BUFFER_LEN) {
        log_w("%s  curr_offset > %d \n", __func__, WECHAT_BUFFER_LEN);
        return 0;
    }

    memcpy(wechat_buffer + curr_offset, data, len);
    curr_offset += len;
    return len;
}

static const struct audio_vfs_ops wechat_vfs_ops = {
    .fwrite = wechat_vfs_fwrite,
    .fclose = turing_vfs_fclose,
};

static void wechat_enc_server_event_handler(void *priv, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_SERVER_EVENT_ERR:
        puts("turing wechat : AUDIO_SERVER_EVENT_EER\n");
    case AUDIO_SERVER_EVENT_END:
        puts("turing_wechat : AUDIO_SERVER_EVENT_END\n");
        if (s_turing_rec_state == RECORDER_START) {
            if (OS_NO_ERR != os_taskq_post("turing_app_task", 1, TURING_RECORD_STOP)) {
                puts("turing_app_task send msg TURING_RECORD_STOP timeout \n");
            }
        }
        break;
    default:
        break;
    }
}

static void enc_server_event_handler(void *priv, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_SERVER_EVENT_ERR:
        puts("turing rec: AUDIO_SERVER_EVENT_ERR\n");
        if (s_turing_rec_state == RECORDER_START) {
            if (OS_NO_ERR != os_taskq_post("turing_app_task", 1, TURING_RECORD_ERR)) {
                puts("turing_app_task send msg TURING_RECORD_ERR timeout \n");
            }
        }
        break;
    case AUDIO_SERVER_EVENT_END:
        puts("turing rec: AUDIO_SERVER_EVENT_END\n");
        if (s_turing_rec_state == RECORDER_START) {
            if (OS_NO_ERR != os_taskq_post("turing_app_task", 1, TURING_RECORD_STOP)) {
                puts("turing_app_task send msg TURING_RECORD_STOP timeout \n");
            }
            if (OS_NO_ERR != os_taskq_post("turing_app_task", 4, TURING_RECORD_SEND, (int)stop_cache_buf, sizeof(stop_cache_buf), &sem)) {
                puts("turing_app_task send msg TURING_RECORD_SEND timeout \n");
            }
        }
        break;
    case AUDIO_SERVER_EVENT_SPEAK_START:
        vad_status = 1;		//开始说话
        puts("speak start ! \n");
        break;
    case AUDIO_SERVER_EVENT_SPEAK_STOP:
        vad_status = 2;		//停止说话
        puts("speak stop ! \n");
        break;
    default:
        break;
    }
}

__attribute__((weak)) void *get_asr_read_input_cb(void)
{
    return NULL;
}

int turing_recorder_start(u16 sample_rate, u8 voice_mode)
{
    int err;
    union audio_req req = {0};
    OS_SEM mic_sem;

    puts("turing_recorder_start\n");

    if (!enc_server) {
        enc_server = server_open("audio_server", "enc");
    }

    os_sem_set(&sem, 0);

    vad_status = 0;
    curr_offset = 0;

    req.enc.cmd = AUDIO_ENC_OPEN;
    req.enc.channel = 1;
    req.enc.volume = 100;
    req.enc.sample_rate = sample_rate;
    req.enc.sample_source = "mic";
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

    if (WECHAT_MODE != (voice_mode & 0x3)) {
        req.enc.output_buf_len = 8 * 1024;
        req.enc.msec = 15 * 1000;
#if TURING_ENC_USE_OPUS == 0
        req.enc.format = "speex";
        req.enc.frame_head_reserve_len = 1;
#else
        req.enc.format = "opus";
#endif
        req.enc.vfs_ops = &turing_vfs_ops;
        req.enc.use_vad = voice_mode & VAD_ENABLE ? 1 : 0;
        server_register_event_handler(enc_server, NULL, enc_server_event_handler);
    } else {
        req.enc.output_buf_len = 4 * 1024;
        req.enc.format = "amr";
        req.enc.vfs_ops = &wechat_vfs_ops;
        req.enc.msec = 30 * 1000;
        ASSERT(wechat_buffer == NULL);
        wechat_buffer = (u8 *)malloc(WECHAT_BUFFER_LEN);
        if (!wechat_buffer) {
            return -1;
        }
        server_register_event_handler(enc_server, NULL, wechat_enc_server_event_handler);
    }

    os_sem_create(&mic_sem, 0);
    ai_server_event_notify(&turing_sdk_api, &mic_sem, AI_SERVER_EVENT_MIC_OPEN);
    os_sem_pend(&mic_sem, 0);
    os_sem_del(&mic_sem, 1);

    if ((voice_mode & WECHAT_MODE) == 0) {
        u32(*read_input)(u8 *, u32) = get_asr_read_input_cb();
        if (read_input) {
            req.enc.sample_source = "virtual";
            req.enc.read_input = read_input;
            req.enc.frame_size = sample_rate == 16000 ? 640 : 320;
            req.enc.channel_bit_map = 0;
        }
    } else {
        if (get_asr_read_input_cb()) {
            req.enc.sample_rate = 16000;
            req.enc.amr_src = 1;
        }
    }

    s_turing_rec_state = RECORDER_START;

    return server_request(enc_server, AUDIO_REQ_ENC, &req);
}

int turing_recorder_stop(u8 voice_mode)
{
    int err;
    union audio_req req = {0};

    if (!enc_server || s_turing_rec_state == RECORDER_STOP) {
        return 0;
    }
    s_turing_rec_state = RECORDER_STOP;
    os_sem_post(&sem);

    req.enc.cmd = AUDIO_ENC_CLOSE;
    err = server_request(enc_server, AUDIO_REQ_ENC, &req);
    if (err == 0) {
        ai_server_event_notify(&turing_sdk_api, NULL, AI_SERVER_EVENT_MIC_CLOSE);
    }

    if (WECHAT_MODE == voice_mode) {
        if (curr_offset < 1600 + 6) {
            ai_server_event_notify(&turing_sdk_api, NULL, AI_SERVER_EVENT_REC_TOO_SHORT);
        } else {
            turing_iot_post_res(wechat_buffer, curr_offset);
        }
    }

    if (wechat_buffer) {
        free(wechat_buffer);
        wechat_buffer = NULL;
    }

    curr_offset = 0;

    return err;
}

void JL_turing_media_speak_play(const char *url)
{
    curr_url_type = TURING_SPEEK_URL;
    ai_server_event_url(&turing_sdk_api, url, AI_SERVER_EVENT_URL_TTS);
}

void JL_turing_media_audio_play(const char *url)
{
    curr_url_type = TURING_MEDIA_URL;
    ai_server_event_url(&turing_sdk_api, url, AI_SERVER_EVENT_URL_MEDIA);
}

void JL_turing_picture_audio_play(const char *url)
{
    curr_url_type = TURING_PICTURE_URL;
    ai_server_event_url(&turing_sdk_api, url, AI_SERVER_EVENT_URL);
}

void JL_turing_media_audio_continue(const char *url)
{
    ai_server_event_url(&turing_sdk_api, url, AI_SERVER_EVENT_CONTINUE);
}

void JL_turing_media_audio_pause(const char *url)
{
    ai_server_event_url(&turing_sdk_api, url, AI_SERVER_EVENT_PAUSE);
}

void JL_turing_upgrade_notify(int event, void *arg)
{
    ai_server_event_notify(&turing_sdk_api, arg, event);
}

void JL_turing_rec_err_notify(void *arg)
{
    ai_server_event_notify(&turing_sdk_api, arg, AI_SERVER_EVENT_REC_ERR);
}

void JL_turing_volume_change_notify(int volume)
{
    ai_server_event_notify(&turing_sdk_api, (void *)volume, AI_SERVER_EVENT_VOLUME_CHANGE);
}

static int turing_sdk_open()
{
    os_sem_create(&sem, 0);

    return turing_app_init();
}

static int turing_sdk_check()
{
    if (turing_app_get_connect_status()) {
        return AI_STAT_CONNECTED;
    }

    return AI_STAT_DISCONNECTED;
}

static int turing_sdk_disconnect()
{
    turing_recorder_stop(AI_MODE);
    turing_app_uninit();
    if (enc_server) {
        server_close(enc_server);
        enc_server = NULL;
    }
    os_sem_del(&sem, 1);
    return 0;
}

REGISTER_AI_SDK(turing_sdk_api) = {
    .name           = "turing",
    .connect        = turing_sdk_open,
    .state_check    = turing_sdk_check,
    .do_event       = turing_sdk_do_event,
    .disconnect     = turing_sdk_disconnect,
};
