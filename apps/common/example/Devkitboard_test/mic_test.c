#include "server/audio_server.h"
#include "server/server_core.h"
#include "generic/circular_buf.h"
#include "os/os_api.h"
#include "app_config.h"
#include "fs/fs.h"


#define CACHE_BUF_LEN (8 * 1024)

struct {
    FILE *fp;
    struct server *enc_server;
    struct server *dec_server;
    u8 *cache_buf;
    cbuffer_t save_cbuf;
    OS_SEM w_sem;
    OS_SEM r_sem;
    volatile u8 run_flag;
    u8 dec_volume;
    u8 enc_volume;
} reverberation_hdl = {
    .dec_volume = 80,
    .enc_volume = 80,
};

#define __this (&reverberation_hdl)

static u8 rate_tab_num = 0;
static u8 source_tab_num = 0;

static const char *const sample_source_table[] = {
    "plnk0",
    "plnk1",
    "mic",
    "linein",
    "iis0",
    "iis1",
};

static const u32 sample_rate_table[] = {
    8000,
    11025,
    12000,
    16000,
    22050,
    24000,
    32000,
    44100,
    48000,
};

static int reverberation_vfs_fwrite(void *file, void *data, u32 len)
{
    cbuf_write(&__this->save_cbuf, data, len);
    os_sem_set(&__this->r_sem, 0);
    os_sem_post(&__this->r_sem);
    return len;
}

static int reverberation_vfs_fread(void *file, void *data, u32 len)
{
    u32 rlen;

    do {
        rlen = cbuf_read(&__this->save_cbuf, data, len);
        if (rlen == len) {
            break;
        }
        /* log_i("cbuf_read fail !\n"); */
        os_sem_pend(&__this->r_sem, 0);
        if (!__this->run_flag) {
            return 0;
        }
    } while (__this->run_flag);

    return len;
}

static int reverberation_vfs_fclose(void *file)
{
    return 0;
}

static int reverberation_vfs_flen(void *file)
{
    return 0;
}

static const struct audio_vfs_ops reverberation_vfs_ops = {
    .fwrite = reverberation_vfs_fwrite,
    .fread  = reverberation_vfs_fread,
    .fclose = reverberation_vfs_fclose,
    .flen   = reverberation_vfs_flen,
};

int reverberation_test_close(void)
{
    union audio_req req = {0};

    puts("----------reverberation test stop----------\n");

    if (!__this->run_flag) {
        return 0;
    }

    __this->run_flag = 0;

    os_sem_post(&__this->w_sem);
    os_sem_post(&__this->r_sem);

    if (__this->enc_server) {
        req.enc.cmd = AUDIO_ENC_CLOSE;
        server_request(__this->enc_server, AUDIO_REQ_ENC, &req);
        server_close(__this->enc_server);
        __this->enc_server = NULL;
    }

    if (__this->dec_server) {
        req.dec.cmd = AUDIO_DEC_STOP;
        server_request(__this->dec_server, AUDIO_REQ_DEC, &req);
        server_close(__this->dec_server);
        __this->dec_server = NULL;
    }

    if (__this->cache_buf) {
        free(__this->cache_buf);
        __this->cache_buf = NULL;
    }

    if (__this->fp) {
        fclose(__this->fp);
        __this->fp = NULL;
    }

    return 0;
}

static void enc_server_event_handler(void *priv, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_SERVER_EVENT_ERR:
    case AUDIO_SERVER_EVENT_END:
        reverberation_test_close();
        break;
    case AUDIO_SERVER_EVENT_CURR_TIME:
        printf("play time : %d\n", argv[1]);
        break;
    default:
        break;
    }
}

int reverberation_test_open(int sample_rate, u32 msec, const char *sample_source)
{
    int err;
    union audio_req req = {0};

    if (!sample_source || __this->run_flag) {
        return -1;
    }

    puts("----------reverberation test start----------\n");
    printf("sample_rate : %d    time_ms : %d    sample_source : %s \n", sample_rate, msec, sample_source);

    if (!strcmp(sample_source, "linein")) {
        __this->enc_volume = 34;
    }

#if 1
    if (!__this->enc_server) {
        __this->enc_server = server_open("audio_server", "enc");
        if (!__this->enc_server) {
            goto __err;
        }
        server_register_event_handler(__this->enc_server, NULL, enc_server_event_handler);
    }

    if (!__this->dec_server) {
        __this->dec_server = server_open("audio_server", "dec");
        if (!__this->dec_server) {
            goto __err;
        }
    }

    __this->cache_buf = (u8 *)malloc(CACHE_BUF_LEN);
    if (__this->cache_buf == NULL) {
        goto __err;
    }
    cbuf_init(&__this->save_cbuf, __this->cache_buf, CACHE_BUF_LEN);

    os_sem_create(&__this->w_sem, 0);
    os_sem_create(&__this->r_sem, 0);

    __this->run_flag = 1;

    req.enc.cmd = AUDIO_ENC_OPEN;
#ifdef CONFIG_ALL_ADC_CHANNEL_OPEN_ENABLE
    req.enc.channel = 2;
    req.enc.channel_bit_map = BIT(CONFIG_AISP_MIC0_ADC_CHANNEL) | BIT(CONFIG_AISP_MIC1_ADC_CHANNEL);
    req.enc.volume = CONFIG_AISP_MIC_ADC_GAIN;
#else
    req.enc.channel = 1;
    req.enc.volume = __this->enc_volume;
#endif
    req.enc.output_buf = NULL;
    req.enc.output_buf_len = 8 * 1024;
    req.enc.sample_rate = sample_rate;
    req.enc.format = "pcm";
    req.enc.frame_size = sample_rate / 100 * 4;
    req.enc.sample_source = sample_source;
    req.enc.vfs_ops = &reverberation_vfs_ops;
    req.enc.msec = msec;

    err = server_request(__this->enc_server, AUDIO_REQ_ENC, &req);
    if (err) {
        goto __err;
    }

    memset(&req, 0, sizeof(union audio_req));

    req.dec.cmd             = AUDIO_DEC_OPEN;
    req.dec.volume          = __this->dec_volume;
    req.dec.output_buf      = NULL;
    req.dec.output_buf_len  = 4 * 1024;
#ifdef CONFIG_ALL_ADC_CHANNEL_OPEN_ENABLE
    req.dec.channel         = 2;
#else
    req.dec.channel         = 1;
#endif
    req.dec.sample_rate     = sample_rate;
    req.dec.priority        = 1;
    req.dec.vfs_ops         = &reverberation_vfs_ops;
    req.dec.dec_type 		= "pcm";
    req.dec.sample_source   = "dac";

    err = server_request(__this->dec_server, AUDIO_REQ_DEC, &req);
    if (err) {
        goto __err1;
    }

    req.dec.cmd = AUDIO_DEC_START;
    err = server_request(__this->dec_server, AUDIO_REQ_DEC, &req);
    if (err) {
        goto __err1;
    }

    return 0;

__err1:
    req.dec.cmd = AUDIO_ENC_CLOSE;
    server_request(__this->enc_server, AUDIO_REQ_ENC, &req);

__err:
    if (__this->enc_server) {
        server_close(__this->enc_server);
        __this->enc_server = NULL;
    }
    if (__this->dec_server) {
        server_close(__this->dec_server);
        __this->dec_server = NULL;
    }
    if (__this->cache_buf) {
        free(__this->cache_buf);
        __this->cache_buf = NULL;
    }

    __this->run_flag = 0;

    return -1;
#endif

#if 1
    if (!__this->enc_server) {
        __this->enc_server = server_open("audio_server", "enc");
        server_register_event_handler(__this->enc_server, NULL, enc_server_event_handler);
    }

    os_sem_create(&__this->w_sem, 0);
    os_sem_create(&__this->r_sem, 0);

    __this->run_flag = 1;

    req.enc.cmd = AUDIO_ENC_OPEN;
    req.enc.channel = 1;
    req.enc.volume = __this->enc_volume;
    req.enc.output_buf = NULL;
    req.enc.output_buf_len = 30 * 1024;
    req.enc.sample_rate = sample_rate;
    req.enc.format = "wav";
    req.enc.frame_size = sample_rate / 10;
    req.enc.sample_source = sample_source;
    req.enc.msec = 10000;
    req.enc.file = __this->fp = fopen(CONFIG_ROOT_PATH"src.wav", "w+");

    return server_request(__this->enc_server, AUDIO_REQ_ENC, &req);
#endif

#if 1
    if (!__this->dec_server) {
        __this->dec_server = server_open("audio_server", "dec");
        if (!__this->dec_server) {
            return -1;
        }
        server_register_event_handler(__this->dec_server, NULL, enc_server_event_handler);
    }

    req.dec.cmd             = AUDIO_DEC_OPEN;
    req.dec.volume          = 100;
    req.dec.output_buf      = NULL;
    req.dec.output_buf_len  = 12 * 1024;
    req.dec.channel         = 1;
    req.dec.sample_rate     = 96000;
    req.dec.priority        = 1;
    /* req.dec.file            = fopen(CONFIG_ROOT_PATH"dac.wav", "r"); */
    req.dec.file            = fopen(CONFIG_ROOT_PATH"dac.pcm", "r");
    req.dec.dec_type 		= "pcm";

    server_request(__this->dec_server, AUDIO_REQ_DEC, &req);

    req.dec.cmd = AUDIO_DEC_START;
    server_request(__this->dec_server, AUDIO_REQ_DEC, &req);
#endif

    return 0;
}

int reverberation_test_enc_volume_change(int step)
{
    union audio_req req;

    int volume = __this->enc_volume + step;
    if (volume < 0) {
        volume = 0;
    } else if (volume > 100) {
        volume = 100;
    }
    if (volume == __this->enc_volume || !__this->enc_server || !__this->run_flag) {
        return -EINVAL;
    }
    __this->enc_volume = volume;

    log_d("set_enc_volume: %d\n", volume);

    req.enc.cmd     = AUDIO_ENC_SET_VOLUME;
    req.enc.volume  = volume;
    return server_request(__this->enc_server, AUDIO_REQ_ENC, &req);
}

int reverberation_test_dec_volume_change(int step)
{
    union audio_req req;

    int volume = __this->dec_volume + step;
    if (volume < 0) {
        volume = 0;
    } else if (volume > 100) {
        volume = 100;
    }
    if (volume == __this->dec_volume || !__this->dec_server || !__this->run_flag) {
        return -EINVAL;
    }
    __this->dec_volume = volume;

    log_d("set_dec_volume: %d\n", volume);

    req.dec.cmd     = AUDIO_DEC_SET_VOLUME;
    req.dec.volume  = volume;
    return server_request(__this->dec_server, AUDIO_REQ_DEC, &req);
}

int reverberation_test_sample_source_change(void)
{
    reverberation_test_close();
    source_tab_num++;
    if (source_tab_num >= ARRAY_SIZE(sample_source_table)) {
        source_tab_num = 0;
    }
    return reverberation_test_open(sample_rate_table[rate_tab_num], 0, sample_source_table[source_tab_num]);
}

int reverberation_test_sample_rate_change(void)
{
    reverberation_test_close();
    rate_tab_num++;
    if (rate_tab_num >= ARRAY_SIZE(sample_rate_table)) {
        rate_tab_num = 0;
    }
    return reverberation_test_open(sample_rate_table[rate_tab_num], 0, sample_source_table[source_tab_num]);
}

