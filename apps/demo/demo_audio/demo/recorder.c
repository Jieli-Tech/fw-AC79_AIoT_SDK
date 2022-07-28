#include "server/audio_server.h"
#include "server/server_core.h"
#include "system/app_core.h"
#include "generic/circular_buf.h"
#include "os/os_api.h"
#include "app_config.h"
#include "syscfg/syscfg_id.h"
#include "event/key_event.h"
#include "storage_device.h"
#include "fs/fs.h"
#include <time.h>

#ifdef CONFIG_RECORDER_MODE_ENABLE

#define CONFIG_STORE_VOLUME
#define VOLUME_STEP         5
#define GAIN_STEP           5
#define MIN_VOLUME_VALUE	5
#define MAX_VOLUME_VALUE	100
#define INIT_VOLUME_VALUE   50

struct recorder_hdl {
    FILE *fp;
    struct server *enc_server;
    struct server *dec_server;
    void *cache_buf;
    cbuffer_t save_cbuf;
    OS_SEM w_sem;
    OS_SEM r_sem;
    volatile u8 run_flag;
    u8 volume;
    u8 gain;
    u8 channel;
    u8 direct;
    const char *sample_source;
    int sample_rate;
};

static struct recorder_hdl recorder_handler;

#define __this (&recorder_handler)

//AUDIO ADC支持的采样率
static const u16 sample_rate_table[] = {
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

//编码器输出PCM数据
static int recorder_vfs_fwrite(void *file, void *data, u32 len)
{
    cbuffer_t *cbuf = (cbuffer_t *)file;
    if (0 == cbuf_write(cbuf, data, len)) {
        //上层buf写不进去时清空一下，避免出现声音滞后的情况
        cbuf_clear(cbuf);
    }
    os_sem_set(&__this->r_sem, 0);
    os_sem_post(&__this->r_sem);
    //此回调返回0录音就会自动停止
    return len;
}

//解码器读取PCM数据
static int recorder_vfs_fread(void *file, void *data, u32 len)
{
    cbuffer_t *cbuf = (cbuffer_t *)file;
    u32 rlen;

    do {
        rlen = cbuf_get_data_size(cbuf);
        rlen = rlen > len ? len : rlen;
        if (cbuf_read(cbuf, data, rlen) > 0) {
            len = rlen;
            break;
        }
        //此处等待信号量是为了防止解码器因为读不到数而一直空转
        os_sem_pend(&__this->r_sem, 0);
        if (!__this->run_flag) {
            return 0;
        }
    } while (__this->run_flag);

    //返回成功读取的字节数
    return len;
}

static int recorder_vfs_fclose(void *file)
{
    return 0;
}

static int recorder_vfs_flen(void *file)
{
    return 0;
}

static const struct audio_vfs_ops recorder_vfs_ops = {
    .fwrite = recorder_vfs_fwrite,
    .fread  = recorder_vfs_fread,
    .fclose = recorder_vfs_fclose,
    .flen   = recorder_vfs_flen,
};

static int recorder_close(void)
{
    union audio_req req = {0};

    if (!__this->run_flag) {
        return 0;
    }

    log_d("----------recorder close----------\n");

    __this->run_flag = 0;

    os_sem_post(&__this->w_sem);
    os_sem_post(&__this->r_sem);

    if (__this->enc_server) {
        req.enc.cmd = AUDIO_ENC_CLOSE;
        server_request(__this->enc_server, AUDIO_REQ_ENC, &req);
    }

    if (__this->dec_server) {
        req.dec.cmd = AUDIO_DEC_STOP;
        server_request(__this->dec_server, AUDIO_REQ_DEC, &req);
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
        recorder_close();
        break;
    case AUDIO_SERVER_EVENT_SPEAK_START:
        log_i("speak start ! \n");
        break;
    case AUDIO_SERVER_EVENT_SPEAK_STOP:
        log_i("speak stop ! \n");
        break;
    default:
        break;
    }
}

//将MIC的数字信号采集后推到DAC播放
//注意：如果需要播放两路MIC，DAC分别对应的是DACL和DACR，要留意芯片封装是否有DACR引脚出来，
//      而且要使能DAC的双通道输出，DAC如果采用差分输出方式也只会听到第一路MIC的声音
static int recorder_play_to_dac(int sample_rate, u8 channel)
{
    int err;
    union audio_req req = {0};

    log_d("----------recorder_play_to_dac----------\n");

    if (channel > 2) {
        channel = 2;
    }
    __this->cache_buf = malloc(sample_rate * channel); //上层缓冲buf缓冲0.5秒的数据，缓冲太大听感上会有延迟
    if (__this->cache_buf == NULL) {
        return -1;
    }
    cbuf_init(&__this->save_cbuf, __this->cache_buf, sample_rate * channel);

    os_sem_create(&__this->w_sem, 0);
    os_sem_create(&__this->r_sem, 0);

    __this->run_flag = 1;

    /****************打开解码DAC器*******************/
    req.dec.cmd             = AUDIO_DEC_OPEN;
    req.dec.volume          = __this->volume;
    req.dec.output_buf_len  = 4 * 1024;
    req.dec.channel         = channel;
    req.dec.sample_rate     = sample_rate;
    req.dec.vfs_ops         = &recorder_vfs_ops;
    req.dec.dec_type 		= "pcm";
    req.dec.sample_source   = CONFIG_AUDIO_DEC_PLAY_SOURCE;
    req.dec.file            = (FILE *)&__this->save_cbuf;
    /* req.dec.attr            = AUDIO_ATTR_LR_ADD; */          //左右声道数据合在一起,封装只有DACL但需要测试两个MIC时可以打开此功能

    err = server_request(__this->dec_server, AUDIO_REQ_DEC, &req);
    if (err) {
        goto __err;
    }

    req.dec.cmd = AUDIO_DEC_START;
    server_request(__this->dec_server, AUDIO_REQ_DEC, &req);

    /****************打开编码器*******************/
    memset(&req, 0, sizeof(union audio_req));

    //BIT(x)用来区分上层需要获取哪个通道的数据
    if (channel == 2) {
        req.enc.channel_bit_map = BIT(CONFIG_AUDIO_ADC_CHANNEL_L) | BIT(CONFIG_AUDIO_ADC_CHANNEL_R);
    } else {
        req.enc.channel_bit_map = BIT(CONFIG_AUDIO_ADC_CHANNEL_L);
    }
    req.enc.frame_size = sample_rate / 100 * 4 * channel;	//收集够多少字节PCM数据就回调一次fwrite
    req.enc.output_buf_len = req.enc.frame_size * 3; //底层缓冲buf至少设成3倍frame_size
    req.enc.cmd = AUDIO_ENC_OPEN;
    req.enc.channel = channel;
    req.enc.volume = __this->gain;
    req.enc.sample_rate = sample_rate;
    req.enc.format = "pcm";
    req.enc.sample_source = __this->sample_source;
    req.enc.vfs_ops = &recorder_vfs_ops;
    req.enc.file = (FILE *)&__this->save_cbuf;
    if (channel == 1 && !strcmp(__this->sample_source, "mic") && (sample_rate == 8000 || sample_rate == 16000)) {
        req.enc.use_vad = 1; //打开VAD断句功能
        req.enc.dns_enable = 1; //打开降噪功能
        req.enc.vad_auto_refresh = 1; //VAD自动刷新
    }

    err = server_request(__this->enc_server, AUDIO_REQ_ENC, &req);
    if (err) {
        goto __err1;
    }

    return 0;

__err1:
    req.dec.cmd = AUDIO_DEC_STOP;
    server_request(__this->dec_server, AUDIO_REQ_DEC, &req);

__err:
    if (__this->cache_buf) {
        free(__this->cache_buf);
        __this->cache_buf = NULL;
    }

    __this->run_flag = 0;

    return -1;
}

//MIC或者LINEIN模拟直通到DAC，不需要软件参与
static int audio_adc_analog_direct_to_dac(int sample_rate, u8 channel)
{
    union audio_req req = {0};

    log_d("----------audio_adc_analog_direct_to_dac----------\n");

    __this->run_flag = 1;

    req.enc.cmd = AUDIO_ENC_OPEN;
    req.enc.channel = channel;
    req.enc.volume = __this->gain;
    req.enc.format = "pcm";
    req.enc.sample_source = __this->sample_source;
    req.enc.sample_rate = sample_rate;
    req.enc.direct2dac = 1;
    req.enc.high_gain = 1;
    if (channel == 4) {
        req.enc.channel_bit_map = 0x0f;
    } else if (channel == 2) {
        req.enc.channel_bit_map = BIT(CONFIG_AUDIO_ADC_CHANNEL_L) | BIT(CONFIG_AUDIO_ADC_CHANNEL_R);
    } else {
        req.enc.channel_bit_map = BIT(CONFIG_AUDIO_ADC_CHANNEL_L);
    }

    return server_request(__this->enc_server, AUDIO_REQ_ENC, &req);
}

//录音文件到SD卡
static int recorder_to_file(int sample_rate, u8 channel)
{
    union audio_req req = {0};

    __this->run_flag = 1;
    __this->direct = 0;

    char time_str[64] = {0};
    char file_name[100] = {0};
    u8 dir_len = 0;
    struct tm timeinfo = {0};
    time_t timestamp = time(NULL) + 28800;
    localtime_r(&timestamp, &timeinfo);
    strcpy(time_str, CONFIG_ROOT_PATH"RECORDER/\\U");
    dir_len = strlen(time_str);
    strftime(time_str + dir_len, sizeof(time_str) - dir_len, "%Y-%m-%dT%H-%M-%S.wav", &timeinfo);
    log_i("recorder file name : %s\n", time_str);

    memcpy(file_name, time_str, dir_len);

    for (u8 i = 0; i < strlen(time_str) - dir_len; ++i) {
        file_name[dir_len + i * 2] = time_str[dir_len + i];
    }

    req.enc.cmd = AUDIO_ENC_OPEN;
    req.enc.channel = channel;
    req.enc.volume = __this->gain;
    req.enc.frame_size = 8192;
    req.enc.output_buf_len = req.enc.frame_size * 10;
    req.enc.sample_rate = sample_rate;
    req.enc.format = "wav";
    req.enc.sample_source = __this->sample_source;
    req.enc.msec = CONFIG_AUDIO_RECORDER_DURATION;
    req.enc.file = __this->fp = fopen(file_name, "w+");
    /* req.enc.sample_depth = 24; //IIS支持采集24bit深度 */
    if (channel == 4) {
        req.enc.channel_bit_map = 0x0f;
    } else if (channel == 2) {
        req.enc.channel_bit_map = BIT(CONFIG_AUDIO_ADC_CHANNEL_L) | BIT(CONFIG_AUDIO_ADC_CHANNEL_R);
    } else {
        req.enc.channel_bit_map = BIT(CONFIG_AUDIO_ADC_CHANNEL_L);
    }

    return server_request(__this->enc_server, AUDIO_REQ_ENC, &req);
}

static void recorder_play_pause(void)
{
    union audio_req req = {0};

    req.dec.cmd = AUDIO_DEC_PP;
    server_request(__this->dec_server, AUDIO_REQ_DEC, &req);

    req.enc.cmd = AUDIO_ENC_PP;
    server_request(__this->enc_server, AUDIO_REQ_ENC, &req);

    if (__this->cache_buf) {
        cbuf_clear(&__this->save_cbuf);
    }
}

//调整ADC的模拟增益
static int recorder_enc_gain_change(int step)
{
    union audio_req req = {0};

    int gain = __this->gain + step;
    if (gain < 0) {
        gain = 0;
    } else if (gain > 100) {
        gain = 100;
    }
    if (gain == __this->gain) {
        return -1;
    }
    __this->gain = gain;

    if (!__this->enc_server) {
        return -1;
    }

    log_d("set_enc_gain: %d\n", gain);

    req.enc.cmd     = AUDIO_ENC_SET_VOLUME;
    req.enc.volume  = gain;
    return server_request(__this->enc_server, AUDIO_REQ_ENC, &req);
}

//调整DAC的数字音量和模拟音量
static int recorder_dec_volume_change(int step)
{
    union audio_req req = {0};

    int volume = __this->volume + step;
    if (volume < MIN_VOLUME_VALUE) {
        volume = MIN_VOLUME_VALUE;
    } else if (volume > MAX_VOLUME_VALUE) {
        volume = MAX_VOLUME_VALUE;
    }
    if (volume == __this->volume) {
        return -1;
    }
    __this->volume = volume;

    if (!__this->dec_server) {
        return -1;
    }

    log_d("set_dec_volume: %d\n", volume);

#ifdef CONFIG_STORE_VOLUME
    syscfg_write(CFG_MUSIC_VOL, &__this->volume, sizeof(__this->volume));
#endif

    req.dec.cmd     = AUDIO_DEC_SET_VOLUME;
    req.dec.volume  = volume;
    return server_request(__this->dec_server, AUDIO_REQ_DEC, &req);
}

static int recorder_mode_init(void)
{
    log_i("recorder_play_main\n");

    memset(__this, 0, sizeof(struct recorder_hdl));

#ifdef CONFIG_STORE_VOLUME
    if (syscfg_read(CFG_MUSIC_VOL, &__this->volume, sizeof(__this->volume)) < 0 ||
        __this->volume < MIN_VOLUME_VALUE || __this->volume > MAX_VOLUME_VALUE) {
        __this->volume = INIT_VOLUME_VALUE;
    }
#else
    __this->volume = INIT_VOLUME_VALUE;
#endif
    if (__this->volume < 0 || __this->volume > MAX_VOLUME_VALUE) {
        __this->volume = INIT_VOLUME_VALUE;
    }

#if CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_PLNK0
    __this->sample_source = "plnk0";
#elif CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_PLNK1
    __this->sample_source = "plnk1";
#elif CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_IIS0
    __this->sample_source = "iis0";
#elif CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_IIS1
    __this->sample_source = "iis1";
#elif CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_LINEIN
    __this->sample_source = "linein";
#else
    __this->sample_source = "mic";
#endif

    __this->channel = CONFIG_AUDIO_RECORDER_CHANNEL;
    __this->gain = CONFIG_AUDIO_ADC_GAIN;
    __this->sample_rate = CONFIG_AUDIO_RECORDER_SAMPLERATE;

    __this->enc_server = server_open("audio_server", "enc");
    server_register_event_handler_to_task(__this->enc_server, NULL, enc_server_event_handler, "app_core");

    __this->dec_server = server_open("audio_server", "dec");

    return recorder_play_to_dac(__this->sample_rate, __this->channel);
}

static void recorder_mode_exit(void)
{
    recorder_close();
    server_close(__this->dec_server);
    __this->dec_server = NULL;
    server_close(__this->enc_server);
    __this->enc_server = NULL;
}

static int recorder_key_click(struct key_event *key)
{
    int ret = true;

    switch (key->value) {
    case KEY_OK:
#if CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_LINEIN || CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_MIC
        if (__this->direct) {
            if (__this->run_flag) {
                recorder_close();
            } else {
                audio_adc_analog_direct_to_dac(__this->sample_rate, __this->channel);
            }
        } else {
            recorder_play_pause();
        }
#else
        recorder_play_pause();
#endif
        break;
    case KEY_VOLUME_DEC:
        recorder_dec_volume_change(-VOLUME_STEP);
        break;
    case KEY_VOLUME_INC:
        recorder_dec_volume_change(VOLUME_STEP);
        break;
    default:
        ret = false;
        break;
    }

    return ret;
}

static int recorder_key_long(struct key_event *key)
{
    switch (key->value) {
#if CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_LINEIN || CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_MIC
    case KEY_OK:
        recorder_close();
        if (__this->direct) {
            recorder_play_to_dac(__this->sample_rate, __this->channel);
        } else {
            audio_adc_analog_direct_to_dac(__this->sample_rate, __this->channel);
        }
        __this->direct = !__this->direct;
        break;
    case KEY_VOLUME_DEC:
        recorder_enc_gain_change(-GAIN_STEP);
        break;
    case KEY_VOLUME_INC:
        recorder_enc_gain_change(GAIN_STEP);
        break;
#endif
    case KEY_MODE:
        if (storage_device_ready()) {
            recorder_close();
            recorder_to_file(__this->sample_rate, __this->channel);
        }
        break;
    default:
        break;
    }

    return true;
}

static int recorder_key_event_handler(struct key_event *key)
{
    switch (key->action) {
    case KEY_EVENT_CLICK:
        return recorder_key_click(key);
    case KEY_EVENT_LONG:
        return recorder_key_long(key);
    default:
        break;
    }

    return true;
}

static int recorder_event_handler(struct application *app, struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        return recorder_key_event_handler((struct key_event *)event->payload);
    default:
        return false;
    }
}

static int recorder_state_machine(struct application *app, enum app_state state, struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        break;
    case APP_STA_START:
        recorder_mode_init();
        break;
    case APP_STA_PAUSE:
        break;
    case APP_STA_RESUME:
        break;
    case APP_STA_STOP:
        recorder_mode_exit();
        break;
    case APP_STA_DESTROY:
        break;
    }

    return 0;
}

static const struct application_operation recorder_ops = {
    .state_machine  = recorder_state_machine,
    .event_handler 	= recorder_event_handler,
};

REGISTER_APPLICATION(recorder) = {
    .name 	= "recorder",
    .ops 	= &recorder_ops,
    .state  = APP_STA_DESTROY,
};

#endif
