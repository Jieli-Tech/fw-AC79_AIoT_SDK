#include "server/ai_server.h"
#include "duerapp_config.h"
#include "duerapp_recorder.h"
#include "duerapp_event.h"
#include "lightduer_voice.h"
#include "system/os/os_api.h"
#include "server/server_core.h"
#include "version.h"
#include "generic/circular_buf.h"

#ifdef DUER_SDK_VERSION
MODULE_VERSION_EXPORT(duer_sdk, DUER_SDK_VERSION);
#endif

extern int duer_app_init(void);
extern bool duer_app_get_connect_state(void);
extern void duer_app_uninit(void);


const struct ai_sdk_api duer_sdk_api;

static const int events[][2] = {
    { AI_EVENT_SPEAK_END, DUER_SPEAK_END     },
    { AI_EVENT_MEDIA_END, DUER_MEDIA_END     },
    { AI_EVENT_MEDIA_STOP, DUER_MEDIA_STOP	 },
    { AI_EVENT_PLAY_PAUSE, DUER_PLAY_PAUSE    },
    { AI_EVENT_PREVIOUS_SONG, DUER_PREVIOUS_SONG },
    { AI_EVENT_NEXT_SONG, DUER_NEXT_SONG     },
    { AI_EVENT_VOLUME_CHANGE, DUER_VOLUME_CHANGE },
    { AI_EVENT_VOLUME_INCR, DUER_VOLUME_INCR   },
    { AI_EVENT_VOLUME_DECR, DUER_VOLUME_DECR   },
    { AI_EVENT_VOLUME_MUTE, DUER_VOLUME_MUTE   },
    { AI_EVENT_RECORD_START, DUER_RECORD_START },
    { AI_EVENT_RECORD_STOP, DUER_RECORD_STOP   },
    { AI_EVENT_VOICE_MODE, DUER_VOICE_MODE     },
    { AI_EVENT_COLLECT_RES, DUER_COLLECT_RES   },
    { AI_EVENT_QUIT    	, DUER_QUIT    	     },
};

enum {
    DUER_SPEEK_URL,
    DUER_MEDIA_URL,
};

#if DUER_SAMPLE_RATE == 8000
static u8 cache_buf[576];
#else
static u8 cache_buf[552];
#endif


#define VAD_CACHE_BUF_LEN (4 * sizeof(cache_buf))

static u32 total_size;
static OS_SEM sem;
static u8 s_vad_status;
static u16 curr_offset = 0;
#ifdef DUER_USE_VAD
static cbuffer_t vad_cbuf;
static u8 *s_vad_cache_buf;
#endif
static struct server *enc_server = NULL;
static duer_rec_state_t s_duer_rec_state = RECORDER_STOP;

static int event2duer(int event)
{
    for (int i = 0; i < ARRAY_SIZE(events); i++) {
        if (events[i][0] == event) {
            return events[i][1];
        }
    }

    return -1;
}

static int duer_sdk_do_event(int event, int arg)
{
    int err;
    int argc = 2;
    int argv[4];
    int duer_event;

    duer_event = event2duer(event);
    if (duer_event == -1) {
        return 0;
    }

    printf("duer_event: %x\n", duer_event);

    argv[1] = arg;
    argv[0] = duer_event;

    if (duer_event == DUER_RECORD_START) {
        if (s_duer_rec_state == RECORDER_START) {
            return 0;
        }
        os_sem_set(&sem, 0);
    } else if (duer_event == DUER_RECORD_STOP) {
        argv[1] = 0;
    }

    do {
        err = os_taskq_post_type("duer_app_task", Q_USER, argc, argv);
        if (err != OS_Q_FULL) {
            break;
        }
        os_time_dly(10);
    } while (1);

    return err;
}

static int duer_vfs_fwrite(void *file, void *data, u32 len)
{
    int err;
    u32 ret_len = len;
    u32 rlen = 0;
    u8 *p = (u8 *)data;

    p[0] = (u8)len - 4;
#if DUER_SAMPLE_RATE == 8000
    p[1] = 0x53;
#else
    p[1] = 0x54;
#endif
    p[2] = 0x50;
    p[3] = 0x58;

#ifdef DUER_USE_VAD
    if ((DUER_VOICE_MODE_DEFAULT == duer_voice_get_mode() ||
         DUER_VOICE_MODE_C2E_BOT == duer_voice_get_mode()) && s_vad_status == 0) {
        if (0 == cbuf_is_write_able(&vad_cbuf, len)) {
            rlen = len;
            if (rlen != cbuf_read(&vad_cbuf, cache_buf, rlen)) {
                return 0;
            }
        }
        cbuf_write(&vad_cbuf, data, len);
        return ret_len;
    }
#endif

    if (s_duer_rec_state == RECORDER_STOP) {
        return 0;
    }
    /* printf("spx_len: %d\n", len); */

    total_size += len;

#ifdef DUER_USE_VAD
    if (cbuf_get_data_size(&vad_cbuf) > 0) {
        while (cbuf_get_data_size(&vad_cbuf) > sizeof(cache_buf)) {
            cbuf_read(&vad_cbuf, cache_buf, sizeof(cache_buf));
            do {
                if (s_duer_rec_state != RECORDER_START) {
                    return 0;
                }
                err = os_taskq_post("duer_app_task", 4, DUER_RECORD_SEND, cache_buf,
                                    sizeof(cache_buf), &sem);
                if (err != OS_Q_FULL) {
                    break;
                }
                os_time_dly(10);
            } while (1);
            os_sem_pend(&sem, 0);
        }
        if (cbuf_get_data_size(&vad_cbuf) > 0) {
            curr_offset += cbuf_read(&vad_cbuf, cache_buf, cbuf_get_data_size(&vad_cbuf));
        }
    }
#endif

    do {
        int remain = sizeof(cache_buf) - curr_offset;
        int cplen = remain < len ? remain : len;
        if (cplen == 0) {
            do {
                if (s_duer_rec_state != RECORDER_START) {
                    return 0;
                }
                err = os_taskq_post("duer_app_task", 4, DUER_RECORD_SEND, cache_buf,
                                    sizeof(cache_buf), &sem);
                if (err != OS_Q_FULL) {
                    break;
                }
                os_time_dly(10);
            } while (1);

            os_sem_pend(&sem, 0);
            if (s_duer_rec_state != RECORDER_START) {
                break;
            }
            curr_offset = 0;
        } else {
            memcpy(cache_buf + curr_offset, p, cplen);
            curr_offset += cplen;
            p += cplen;
            len -= cplen;
        }

    } while (len);

    return ret_len;
}

static int duer_vfs_fclose(void *file)
{
    return 0;
}

static const struct audio_vfs_ops duer_vfs_ops = {
    .fwrite = duer_vfs_fwrite,
    .fclose = duer_vfs_fclose,
};

static void enc_server_event_handler(void *priv, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_SERVER_EVENT_ERR:
    case AUDIO_SERVER_EVENT_END:
        if (s_duer_rec_state == RECORDER_START) {
            if (OS_NO_ERR != os_taskq_post("duer_app_task", 2, DUER_RECORD_STOP, 0)) {
                puts("duer_app_task send msg DUER_RECORD_STOP fail ");
            }
        }
        break;
    case AUDIO_SERVER_EVENT_SPEAK_START:
        s_vad_status = 1;
        puts("speak start ! \n");
#ifdef DUER_USE_VAD
        if (s_duer_rec_state == RECORDER_START) {
            duer_voice_start(DUER_SAMPLE_RATE);
        }
#endif
        break;
    case AUDIO_SERVER_EVENT_SPEAK_STOP:
        s_vad_status = 2;
        puts("speak stop ! \n");
        if (s_duer_rec_state == RECORDER_START) {
            if (OS_NO_ERR != os_taskq_post("duer_app_task", 2, DUER_RECORD_STOP, 0)) {
                puts("duer_app_task send msg DUER_RECORD_STOP fail ");
            }
        }
        break;
    default:
        break;
    }
}

__attribute__((weak)) void *get_asr_read_input_cb(void)
{
    return NULL;
}

int duer_recorder_start(u16 sample_rate, u8 voice_mode)
{
    int err;
    union audio_req req = {0};
    OS_SEM mic_sem;

    puts("duer_recorder_start\n");

    if (!enc_server) {
        enc_server = server_open("audio_server", "enc");
        if (!enc_server) {
            return DUER_ERR_FAILED;
        }
        server_register_event_handler(enc_server, NULL, enc_server_event_handler);
    }

    s_vad_status = 0;
    curr_offset = 0;
    total_size = 0;

#ifdef DUER_USE_VAD
    s_vad_cache_buf = (u8 *)malloc(VAD_CACHE_BUF_LEN);
    if (s_vad_cache_buf == NULL) {
        return DUER_ERR_FAILED;
    }
    cbuf_init(&vad_cbuf, s_vad_cache_buf, VAD_CACHE_BUF_LEN);
#endif

    req.enc.cmd = AUDIO_ENC_OPEN;
    req.enc.channel = 1;
    req.enc.volume = 100;
    req.enc.priority = 0;
    req.enc.output_buf = NULL;
    req.enc.output_buf_len = 2 * 1024;
    req.enc.sample_rate = sample_rate;
    req.enc.format = "speex";
    req.enc.frame_size = 4480;
    req.enc.sample_source = "mic";
    req.enc.frame_head_reserve_len = 4;
    req.enc.vfs_ops = &duer_vfs_ops;
    req.enc.file = NULL;
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
    req.enc.use_vad = (voice_mode & VAD_ENABLE) ? 1 : 0;
    if (voice_mode & VAD_ENABLE) {
        /* req.enc.vad_start_threshold = 320; */
        req.enc.msec = 15 * 1000;
    }

    os_sem_create(&mic_sem, 0);
    ai_server_event_notify(&duer_sdk_api, &mic_sem, AI_SERVER_EVENT_MIC_OPEN);
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
    }

    s_duer_rec_state = RECORDER_START;

    err = server_request(enc_server, AUDIO_REQ_ENC, &req);
    if (err == 0) {
        return DUER_OK;
    }

    return DUER_ERR_FAILED;
}

int duer_recorder_stop(void)
{
    int err;
    union audio_req req;

    if (!enc_server || s_duer_rec_state == RECORDER_STOP) {
        return DUER_OK;
    }
    s_duer_rec_state = RECORDER_STOP;
    os_sem_post(&sem);

    /* duer_voice_stop(NULL, 0); */

    req.enc.cmd = AUDIO_ENC_CLOSE;
    err = server_request(enc_server, AUDIO_REQ_ENC, &req);
    if (err == 0) {
        ai_server_event_notify(&duer_sdk_api, NULL, AI_SERVER_EVENT_MIC_CLOSE);
    }

#ifdef DUER_USE_VAD
    if (s_vad_cache_buf) {
        free(s_vad_cache_buf);
        s_vad_cache_buf = NULL;
    }
#endif

    server_close(enc_server);
    enc_server = NULL;

    if (DUER_VOICE_MODE_WCHAT == duer_voice_get_mode() &&
        ((total_size < 32 * 54 && DUER_SAMPLE_RATE == 8000) ||
         (total_size < 46 * 52 && DUER_SAMPLE_RATE == 16000))
       ) {
        ai_server_event_notify(&duer_sdk_api, NULL, AI_SERVER_EVENT_REC_TOO_SHORT);
    }

    return DUER_OK;
}

unsigned char duer_get_recorder_vad_status(void)
{
    return s_vad_status;
}

duer_rec_state_t duer_get_recorder_state(void)
{
    return s_duer_rec_state;
}

void JL_duer_media_speak_play(const char *url)
{
    ai_server_event_url(&duer_sdk_api, url, AI_SERVER_EVENT_URL_TTS);
}

void duer_wechat_speak_play(const char *url)
{
    char *save_url = NULL;

    if (url && strlen(url) > 0) {
        save_url = (char *)malloc(strlen(url) + 1);
        if (!save_url) {
            return;
        }
        strcpy(save_url, url);
        if (0 != ai_server_event_notify(&duer_sdk_api, save_url, AI_SERVER_EVENT_RECV_CHAT)) {
            free(save_url);
        }
    }
}

void JL_duer_media_audio_play(const char *url)
{
    ai_server_event_url(&duer_sdk_api, url, AI_SERVER_EVENT_URL_MEDIA);
}

void JL_duer_media_audio_continue(const char *url)
{
    ai_server_event_url(&duer_sdk_api, url, AI_SERVER_EVENT_CONTINUE);
}

void JL_duer_media_audio_pause(const char *url)
{
    ai_server_event_url(&duer_sdk_api, url, AI_SERVER_EVENT_PAUSE);
}

void JL_duer_upgrade_notify(int event, void *arg)
{
    ai_server_event_notify(&duer_sdk_api, arg, event);
}

void JL_duer_volume_change_notify(int volume)
{
    ai_server_event_notify(&duer_sdk_api, (void *)volume, AI_SERVER_EVENT_VOLUME_CHANGE);
}

static int duer_sdk_open()
{
    os_sem_create(&sem, 0);
    return duer_app_init();
}

static int duer_sdk_check()
{
    if (duer_app_get_connect_state()) {
        return AI_STAT_CONNECTED;
    }

    return AI_STAT_DISCONNECTED;
}

static int duer_sdk_disconnect()
{
    union audio_req req = {0};

    duer_app_uninit();

    if (enc_server) {
        if (s_duer_rec_state != RECORDER_STOP) {
            s_duer_rec_state = RECORDER_STOP;
            req.enc.cmd = AUDIO_ENC_CLOSE;
            server_request(enc_server, AUDIO_REQ_ENC, &req);
        }
        server_close(enc_server);
        enc_server = NULL;
    }

    os_sem_del(&sem, 1);
    return 0;
}

REGISTER_AI_SDK(duer_sdk_api) = {
    .name           = "duer",
    .connect        = duer_sdk_open,
    .state_check    = duer_sdk_check,
    .do_event       = duer_sdk_do_event,
    .disconnect     = duer_sdk_disconnect,
};
