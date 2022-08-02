#include "aligenie_audio.h"
#include "aligenie_sdk.h"
#include "aligenie_os.h"
#include "server/server_core.h"
#include "server/ai_server.h"
#include "server/audio_server.h"
#include "generic/circular_buf.h"
#include "network_download/net_audio_buf.h"
#include "os/os_api.h"

#define AG_REC_CBUF_SIZE (1024 * 4)
#define AG_DEC_CBUF_SIZE (1024 * 30)
#define AG_SEND_FRAME_NUM 10

extern const struct ai_sdk_api ag_sdk_api;

static const char *s_sample_source = "mic";
static u8 enable_vad = 1;
static u8 vad_status;
static u8 s_channel_bit_map;
static u8 ag_message_flag;
static OS_SEM message_sem;
AG_AUDIO_INFO_T ag_audio_info;

static struct ag_enc_t {
    void *enc_server;
    OS_SEM enc_sem;
    cbuffer_t rec_cbuf;
    u8 rec_buf[AG_REC_CBUF_SIZE];
    u8 rec_state;
    u16 frame_size;
} *ag_enc_hdl;

static struct ag_dec_t {
    void *dec_server;
    OS_SEM end_sem;
    void *net_buf;
    u32 state;
} s_ag_dec_hdl;

#define ag_dec_hdl (&s_ag_dec_hdl)

enum {
    RECORDER_START,
    RECORDER_STOP,
};

__attribute__((weak)) int get_app_music_prompt_dec_status(void)
{
    return 0;
}

__attribute__((weak)) void *get_asr_read_input_cb(void)
{
    return NULL;
}

__attribute__((weak)) int get_app_music_playtime(void)
{
    return 0;
}

__attribute__((weak)) int get_app_music_total_time(void)
{
    return 0;
}

__attribute__((weak)) int get_app_music_volume(void)
{
    return 100;
}

__attribute__((weak)) int get_app_music_net_dec_status(void)
{
    return AUDIO_DEC_STOP;
}

void ag_message_sem_post(void)
{
    if (ag_message_flag) {
        os_sem_set(&message_sem, 0);
        os_sem_post(&message_sem);
        os_sem_post(&message_sem);
    }
}

void clear_ag_message_flag(void)
{
    ag_message_flag = 0;
}

void set_ag_rec_sample_source(const char *sample_source)
{
    s_sample_source = sample_source;
}

void set_ag_rec_channel_bit_map(u8 channel_bit_map)
{
    s_channel_bit_map = channel_bit_map;
}

/*device control*/
int ag_os_device_standby(void)
{
    return ai_server_event_notify(&ag_sdk_api, NULL, AI_SERVER_EVENT_SHUTDOWN);
}
int ag_os_device_shutdown(void)
{
    return ai_server_event_notify(&ag_sdk_api, NULL, AI_SERVER_EVENT_SHUTDOWN);
}

void ag_audio_idle(void)
{
    printf("ag_audio_idle\n");
}

int ag_audio_url_play(const AG_AUDIO_INFO_T *const info)
{
    int ret = ai_server_event_url(&ag_sdk_api, info->url, AI_SERVER_EVENT_URL);

    memcpy(&ag_audio_info, info, sizeof(AG_AUDIO_INFO_T));
    ag_audio_info.status = AG_PLAYER_URL_PLAYING;
    ag_sdk_notify_player_status_change(&ag_audio_info);

    return ret;
}

int ag_audio_url_pause(void)
{
    int ret = ai_server_event_url(&ag_sdk_api, NULL, AI_SERVER_EVENT_PAUSE);

    ag_audio_info.status = AG_PLAYER_URL_PAUSED;
    ag_sdk_notify_player_status_change(&ag_audio_info);

    return ret;
}

int ag_audio_url_resume(void)
{
    int ret = ai_server_event_url(&ag_sdk_api, NULL, AI_SERVER_EVENT_RESUME_PLAY);

    ag_audio_info.status = AG_PLAYER_URL_PLAYING;
    ag_sdk_notify_player_status_change(&ag_audio_info);

    return ret;
}

int ag_audio_url_stop(void)
{
    int ret = ai_server_event_url(&ag_sdk_api, NULL, AI_SERVER_EVENT_STOP);

    ag_audio_info.status = AG_PLAYER_URL_STOPPED;
    ag_sdk_notify_player_status_change(&ag_audio_info);

    return ret;
}

int ag_audio_get_url_play_progress(int *outval)
{
    if (!ag_message_flag && get_app_music_net_dec_status() != AUDIO_DEC_STOP) {
        *outval = get_app_music_playtime();
        return 0;
    }

    return -1;
}

int ag_audio_get_url_play_duration(int *outval)
{
    if (!ag_message_flag && get_app_music_net_dec_status() != AUDIO_DEC_STOP) {
        *outval = get_app_music_total_time();
        return 0;
    }

    return -1;
}

int ag_audio_url_seek_progress(int seconds)
{
    return ai_server_event_notify(&ag_sdk_api, (void *)seconds, AI_SERVER_EVENT_SEEK);
}

AG_PLAYER_STATUS_E ag_audio_get_player_status(void)
{
#if 0
    int dec_status = get_app_music_net_dec_status();

    if (dec_status == AUDIO_DEC_START) {
        return AG_PLAYER_URL_PLAYING;
    } else if (dec_status == AUDIO_DEC_PAUSE) {
        return AG_PLAYER_URL_PAUSED;
    }

    return AG_PLAYER_URL_STOPPED;
#else
    return ag_audio_info.status ? ag_audio_info.status : AG_PLAYER_URL_FINISHED;
#endif
}

int ag_audio_play_prompt(char *url)
{
    int ret = 0;

    os_sem_create(&message_sem, 0);
    ag_message_flag = 1;
    ret = ai_server_event_url(&ag_sdk_api, url, AI_SERVER_EVENT_URL_TTS);
    os_sem_pend(&message_sem, 6000);
    ag_message_flag = 0;
    return ret;
}

int ag_audio_play_local_prompt(AG_LOCAL_PROMPT_TYPE_E type)
{
#if 0
    return ai_server_event_notify(&ag_sdk_api, (void *)type, AI_SERVER_EVENT_PLAY_BEEP);
#else
    printf("ag_audio_play_local_prompt:%d\n", type);	//TODO实现堵塞
    return 0;
#endif
}

int ag_audio_stop_prompt(void)
{
    printf("--------- %s  %d\n", __func__, __LINE__);

    if (ag_message_flag) {
#if 0
        ai_server_event_url(&ag_sdk_api, NULL, AI_SERVER_EVENT_STOP);
        os_sem_set(&message_sem, 0);
        os_sem_post(&message_sem);
#else
        os_sem_pend(&message_sem, 6000);
#endif
    }

    return 0;
}

static int ag_vfs_flen(void *priv)
{
    return -1;
}

static int ag_vfs_fread(void *file, void *data, u32 len)
{
    int rlen = net_buf_read(data, len, file);
    if (rlen == -2) {
        return -2;	//暂时读不到数
    } else if (rlen < 0) {
        return 0;
    }

    return rlen;
}

static int ag_vfs_fseek(void *priv, u32 offset, int orig)
{
    int ret = net_buf_seek(offset, orig, priv);

    if (ret == -2) {	//暂时读不到数
        return ret;
    }

    return 0;
}

static int ag_vfs_fwrite(void *file, void *data, u32 len)
{
    if (ag_enc_hdl->rec_state == RECORDER_STOP) {
        return 0;
    }

    ag_enc_hdl->frame_size = len;
    u8 *pdata = (u8 *)data;

    pdata[0] = len - 4; //长度
    pdata[1] = 0x0;
    pdata[2] = 0x54; //16000K
    pdata[3] = 0x0;

    cbuf_write(&ag_enc_hdl->rec_cbuf, data, len);

    if (cbuf_get_data_size(&ag_enc_hdl->rec_cbuf) >= len * AG_SEND_FRAME_NUM) {
        os_sem_set(&ag_enc_hdl->enc_sem, 0);
        os_sem_post(&ag_enc_hdl->enc_sem);
    }

    return len;
}

static int ag_vfs_fclose(void *file)
{
    return 0;
}

static const struct audio_vfs_ops ag_vfs_ops = {
    .fread  = ag_vfs_fread,
    .fwrite = ag_vfs_fwrite,
    .fclose = ag_vfs_fclose,
    .fseek  = ag_vfs_fseek,
    .flen   = ag_vfs_flen,
};

static void dec_server_event_handler(void *priv, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_SERVER_EVENT_ERR:
        puts("ag_tts: AUDIO_SERVER_EVENT_ERR\n");
    case AUDIO_SERVER_EVENT_END:
        puts("ag_tts: AUDIO_SERVER_EVENT_END\n");
        os_sem_post(&((struct ag_dec_t *)priv)->end_sem);
        break;
    default:
        break;
    }
}

int ag_audio_tts_play_start(AG_TTS_FORMAT_E format)
{
    int err = 0;
    union audio_req req = {0};

    ag_message_flag = 0;
    /* ai_server_event_url(&ag_sdk_api, NULL, AI_SERVER_EVENT_STOP); */

    while (get_app_music_prompt_dec_status()) {
        ag_os_task_mdelay(100);
    }

    ag_dec_hdl->dec_server = server_open("audio_server", "dec");
    if (!ag_dec_hdl->dec_server) {
        return -1;
    }
    server_register_event_handler_to_task(ag_dec_hdl->dec_server, ag_dec_hdl, dec_server_event_handler, "app_core");

    os_sem_create(&ag_dec_hdl->end_sem, 0);
    ag_dec_hdl->state = 0;

    u32 bufsize = AG_DEC_CBUF_SIZE;
    ag_dec_hdl->net_buf = net_buf_init(&bufsize, NULL);
    if (!ag_dec_hdl->net_buf) {
        server_close(ag_dec_hdl->dec_server);
        ag_dec_hdl->dec_server = NULL;
    }
    net_buf_set_time_out(0, ag_dec_hdl->net_buf);
    net_buf_active(ag_dec_hdl->net_buf);

    return 0;
}

int ag_audio_tts_play_put_data(const void *const inData, int dataLen)
{
    int err = 0;
    union audio_req req = {0};

    net_buf_write((u8 *)inData, dataLen, ag_dec_hdl->net_buf);

    if (ag_dec_hdl->state == 0) {
        ag_dec_hdl->state = 1;
        req.dec.cmd             = AUDIO_DEC_OPEN;
        req.dec.volume          = get_app_music_volume();
        req.dec.output_buf_len  = 4 * 1024;
        req.dec.sample_source   = "dac";
        req.dec.dec_type        = "mp3";
        req.dec.file            = (FILE *)ag_dec_hdl->net_buf;
        req.dec.vfs_ops 		= &ag_vfs_ops;

        err = server_request(ag_dec_hdl->dec_server, AUDIO_REQ_DEC, &req);
        if (err) {
            server_close(ag_dec_hdl->dec_server);
            ag_dec_hdl->dec_server = NULL;
            net_buf_uninit(ag_dec_hdl->net_buf);
            ag_dec_hdl->net_buf = NULL;
            return err;
        }

        req.dec.cmd = AUDIO_DEC_START;

        err = server_request(ag_dec_hdl->dec_server, AUDIO_REQ_DEC, &req);
    }

    return err;
}

int ag_audio_tts_play_wait_finish(void)
{
    union audio_req req = {0};

    net_buf_set_file_end(ag_dec_hdl->net_buf);
    os_sem_pend(&ag_dec_hdl->end_sem, 0);
    req.dec.cmd = AUDIO_DEC_STOP;
    server_request(ag_dec_hdl->dec_server, AUDIO_REQ_DEC, &req);
    server_close(ag_dec_hdl->dec_server);
    ag_dec_hdl->dec_server = NULL;
    net_buf_uninit(ag_dec_hdl->net_buf);
    ag_dec_hdl->net_buf = NULL;

    return 0;
}

int ag_audio_tts_play_abort(void)
{
    union audio_req req = {0};

#if 0
    net_buf_inactive(ag_dec_hdl->net_buf);
    req.dec.cmd = AUDIO_DEC_STOP;
    server_request(ag_dec_hdl->dec_server, AUDIO_REQ_DEC, &req);
    int argv[2];
    argv[0] = AUDIO_SERVER_EVENT_END;
    argv[1] = (int)ag_dec_hdl;
    server_event_handler_del(ag_dec_hdl->dec_server, 2, argv);
    server_close(ag_dec_hdl->dec_server);
    ag_dec_hdl->dec_server = NULL;
    net_buf_uninit(ag_dec_hdl->net_buf);
    ag_dec_hdl->net_buf = NULL;
#else
    os_sem_post(&ag_dec_hdl->end_sem);
#endif

    return 0;
}

AG_RECORDER_DATA_TYPE_E ag_audio_get_record_data_type(void)
{
    return AG_RECORDER_DATA_TYPE_SPEEX;
}

static void enc_server_event_handler(void *priv, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_SERVER_EVENT_ERR:
        puts("ag rec: AUDIO_SERVER_EVENT_ERR\n");
        ag_sdk_notify_keyevent(AG_KEYEVENT_DISCARD_SPEECH);
        break;
    case AUDIO_SERVER_EVENT_END:
        puts("ag rec: AUDIO_SERVER_EVENT_END\n");
        ag_sdk_notify_keyevent(AG_KEYEVENT_DISCARD_SPEECH);
        break;
    case AUDIO_SERVER_EVENT_SPEAK_START:
        puts("speak start ! \n");
        vad_status = 1;
        ag_sdk_notify_keyevent(AG_KEYEVENT_VAD_START);
        break;
    case AUDIO_SERVER_EVENT_SPEAK_STOP:
        vad_status = 2;
        puts("speak stop ! \n");
        ag_sdk_notify_keyevent(AG_KEYEVENT_VAD_STOP);
        break;
    default:
        break;
    }
}

int ag_audio_record_start(AG_AUDIO_RECORD_TYPE_E type)
{
    int err;
    union audio_req req = {0};
    OS_SEM mic_sem;

    ASSERT(ag_enc_hdl == NULL);

    ag_enc_hdl = (struct ag_enc_t *)zalloc(sizeof(struct ag_enc_t));
    if (!ag_enc_hdl) {
        return -1;
    }

    ag_enc_hdl->enc_server = server_open("audio_server", "enc");
    if (!ag_enc_hdl->enc_server) {
        free(ag_enc_hdl);
        ag_enc_hdl = NULL;
        return -1;
    }

    server_register_event_handler_to_task(ag_enc_hdl->enc_server, ag_enc_hdl, enc_server_event_handler, "app_core");

    cbuf_init(&ag_enc_hdl->rec_cbuf, ag_enc_hdl->rec_buf, AG_REC_CBUF_SIZE);

    req.enc.cmd = AUDIO_ENC_OPEN;
    req.enc.channel = 1;
    req.enc.volume = 100;
    req.enc.sample_rate = 16000;
    req.enc.output_buf_len = 2 * 1024;
    //req.enc.vad_start_threshold = 320;
    req.enc.msec = 15 * 1000;//录音时间
    req.enc.format = "speex";
    req.enc.vfs_ops = &ag_vfs_ops;
    req.enc.use_vad = enable_vad;
    req.enc.channel_bit_map = s_channel_bit_map;
    req.enc.sample_source = s_sample_source;
    req.enc.frame_head_reserve_len = 4;

    os_sem_create(&ag_enc_hdl->enc_sem, 0);
    os_sem_create(&mic_sem, 0);
    ai_server_event_notify(&ag_sdk_api, &mic_sem, AI_SERVER_EVENT_MIC_OPEN);
    os_sem_pend(&mic_sem, 0);
    os_sem_del(&mic_sem, 1);

#if 0
    u32(*read_input)(u8 *, u32) = get_asr_read_input_cb();
    if (read_input) {
        req.enc.sample_source = "virtual";
        req.enc.read_input = read_input;
        req.enc.frame_size = 640;
        req.enc.channel_bit_map = 0;
    }
#endif

    ag_enc_hdl->rec_state = RECORDER_START;
    return server_request(ag_enc_hdl->enc_server, AUDIO_REQ_ENC, &req);
}

int ag_audio_record_get_data(const void *outData, int maxLen)
{
    if (!ag_enc_hdl->frame_size) {
        os_sem_pend(&ag_enc_hdl->enc_sem, 0);
    }

    u32 read_len = ag_enc_hdl->frame_size * AG_SEND_FRAME_NUM;

    while (cbuf_get_data_size(&ag_enc_hdl->rec_cbuf) < read_len && ag_enc_hdl->rec_state == RECORDER_START) {
        os_sem_pend(&ag_enc_hdl->enc_sem, 0);
    }

    //保证读整数帧
    return cbuf_read(&ag_enc_hdl->rec_cbuf, (void *)outData, maxLen > read_len ? read_len : ((volatile u32)maxLen / ag_enc_hdl->frame_size) * ag_enc_hdl->frame_size);
}

int ag_audio_record_stop(void)
{
    int err;
    union audio_req req = {0};

    if (!ag_enc_hdl) {
        return 0;
    }

    ag_enc_hdl->rec_state = RECORDER_STOP;

    os_sem_post(&ag_enc_hdl->enc_sem);

    req.enc.cmd = AUDIO_ENC_CLOSE;
    err = server_request(ag_enc_hdl->enc_server, AUDIO_REQ_ENC, &req);
    if (err == 0) {
        ai_server_event_notify(&ag_sdk_api, NULL, AI_SERVER_EVENT_MIC_CLOSE);
    }
    server_close(ag_enc_hdl->enc_server);
    ag_enc_hdl->enc_server = NULL;
    ag_enc_hdl->frame_size = 0;
    free(ag_enc_hdl);
    ag_enc_hdl = NULL;

    /* enable_vad = 0; */

    return err;
}

int ag_audio_set_volume(int volume)
{
    return ai_server_event_notify(&ag_sdk_api, (void *)volume, AI_SERVER_EVENT_VOLUME_CHANGE);
}

int ag_audio_get_volume(void)
{
    return get_app_music_volume();
}

int ag_audio_vad_start(void)
{
    enable_vad = 1;
    /* ag_sdk_notify_keyevent(AG_KEYEVENT_VAD_START); */
    puts("enable_vad\n");
    return 0;
}
