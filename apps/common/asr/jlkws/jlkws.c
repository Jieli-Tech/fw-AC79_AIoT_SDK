#include "server/audio_server.h"
#include "server/server_core.h"
#include "generic/circular_buf.h"
#include "os/os_api.h"
#include "event.h"
#include "app_config.h"
#include "jlsp_far_keyword.h"
#include "jlsp_kws_aec.h"

#if (defined CONFIG_ASR_ALGORITHM) && (CONFIG_ASR_ALGORITHM == JLKWS_ALGORITHM)

#define AISP_DUAL_MIC_ALGORITHM    0   //0选择单mic/1选择双mic算法

#if CONFIG_AUDIO_ENC_SAMPLE_SOURCE != AUDIO_ENC_SAMPLE_SOURCE_MIC
#ifdef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
#define CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN_OTHER
#endif
#undef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
#endif

#define PCM_TEST_MODE_0                0   //测试模式0，会发送当前音频给电脑
#define PCM_TEST_MODE_1                1   //测试模式1，会发送每条唤醒词的录音

#define WIFI_PCM_STREAN_TEST       PCM_TEST_MODE_0

#define ONCE_SR_POINTS	256

#ifndef WIFI_PCM_STREAN_SOCKET_ENABLE
#define AISP_BUF_SIZE	(ONCE_SR_POINTS * (AISP_DUAL_MIC_ALGORITHM + 1) * 2)	//跑不过来时适当加大倍数
#else
#if (WIFI_PCM_STREAN_TEST == PCM_TEST_MODE_0)
#define AISP_BUF_SIZE	(ONCE_SR_POINTS * (AISP_DUAL_MIC_ALGORITHM + 1) * 8)	//跑不过来时适当加大倍数
#else
#define AISP_BUF_SIZE	(ONCE_SR_POINTS * (AISP_DUAL_MIC_ALGORITHM + 1) * 1000)	//跑不过来时适当加大倍数
#endif
#endif

static struct {
    int pid;
    u16 sample_rate;
    u8 volatile exit_flag;
    int volatile run_flag;
    OS_SEM sem;
#ifndef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
    void *linein_enc;
    cbuffer_t linein_cbuf;
    s16 linein_buf[AISP_BUF_SIZE];
    s16 mic_buf[AISP_BUF_SIZE * (1 + AISP_DUAL_MIC_ALGORITHM)];
#else
    s16 mic_buf[AISP_BUF_SIZE * (2 + AISP_DUAL_MIC_ALGORITHM)];
#endif
    void *mic_enc;
    cbuffer_t mic_cbuf;
#ifdef WIFI_PCM_STREAN_SOCKET_ENABLE
    short send_buf[ONCE_SR_POINTS * 4];
#endif
} aisp_server;

#define __this (&aisp_server)

static const float kws_multi_confidence[] = {
    0.4, 0.4, 0.4, 0.4, //小杰小杰，小杰同学，播放音乐，停止播放
    0.4, 0.4, 0.4, 0.4, //暂停播放，增大音量，减小音量，上一首
    0.4, 0.4, 0.4, 0.4, //下一首，打开降噪，关闭降噪，打开通透
};

static const float kws_call_confidence[] = {
    0.4, 0.4, //接听电话，挂断电话
};

const int CONFIG_KWS_MULTI_CMD_MODEL_ENABLE = 1;
const int CONFIG_KWS_CALL_CMD_MODEL_ENABLE  = 0;
const int CONFIG_KWS_RAM_USE_ENABLE = 0;

extern void aisp_resume(void);

static void aisp_task(void *priv)
{
    u32 mic_len, linein_len;
    int aec_len;
    u32 tmp_offset = 0;
    s16 tmp_aec_buf[160];

    int ret;
    int model = 0;
    int model_size, private_heap_size, share_heap_size;
    void *kws = NULL;
    void *aec = NULL;
    u8 *kws_memory = NULL;
    u8 *aec_memory = NULL;

    jl_far_keyword_model_get_heap_size(model, &model_size, &private_heap_size, &share_heap_size);

    kws_memory = malloc(private_heap_size + share_heap_size);
    if (!kws_memory) {
        goto __exit;
    }

    aec_memory = malloc(asp2_aec_get_heap_size());
    if (!aec_memory) {
        goto __exit;
    }

    kws = jl_far_keyword_model_init(model, kws_memory, private_heap_size, kws_memory + private_heap_size, share_heap_size, model_size, model ? kws_call_confidence : kws_multi_confidence, 1);
    if (!kws) {
        goto __exit;
    }

    aec = asp2_aec_init(aec_memory, asp2_aec_get_heap_size());
    if (!aec) {
        goto __exit;
    }

    aisp_resume();

    while (1) {
        if (__this->exit_flag) {
            break;
        }

        if (!__this->run_flag) {
            os_sem_pend(&__this->sem, 0);
            continue;
        }

        if (__this->exit_flag) {
            break;
        }

#ifndef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
        if ((cbuf_get_data_size(&__this->mic_cbuf) < ONCE_SR_POINTS * 2 * (AISP_DUAL_MIC_ALGORITHM + 1))
            || (cbuf_get_data_size(&__this->linein_cbuf) < ONCE_SR_POINTS * 2)) {
            os_sem_pend(&__this->sem, 0);
            continue;
        }
        short far_data_buf[ONCE_SR_POINTS];
        short near_data_buf[ONCE_SR_POINTS * (1 + AISP_DUAL_MIC_ALGORITHM)];
        mic_len = cbuf_read(&__this->mic_cbuf, near_data_buf, ONCE_SR_POINTS * 2 * (AISP_DUAL_MIC_ALGORITHM + 1));
        if (!mic_len) {
            continue;
        }
        linein_len = cbuf_read(&__this->linein_cbuf, far_data_buf, ONCE_SR_POINTS * 2);
        if (!linein_len) {
            continue;
        }
#else
        short buf[ONCE_SR_POINTS * (2 + AISP_DUAL_MIC_ALGORITHM)];
        mic_len = cbuf_read(&__this->mic_cbuf, buf, sizeof(buf));
        if (!mic_len) {
            os_sem_pend(&__this->sem, 0);
            continue;
        }

        short far_data_buf[ONCE_SR_POINTS];
        short near_data_buf[ONCE_SR_POINTS * (1 + AISP_DUAL_MIC_ALGORITHM)];

        for (int i = 0; i < ONCE_SR_POINTS; i++) {
#if AISP_DUAL_MIC_ALGORITHM
            near_data_buf[2 * i] = buf[3 * i];
            near_data_buf[2 * i + 1] = buf[3 * i + 1];
            far_data_buf[i]  = buf[3 * i + 2];
#else
            near_data_buf[i] = buf[2 * i];
            far_data_buf[i]  = buf[2 * i + 1];
#endif
        }
#endif

#ifdef WIFI_PCM_STREAN_SOCKET_ENABLE
#if (WIFI_PCM_STREAN_TEST == PCM_TEST_MODE_0)
        for (u32 i = 0; i < ONCE_SR_POINTS; ++i) {
#if AISP_DUAL_MIC_ALGORITHM
            __this->send_buf[4 * i] = near_data_buf[i * 2];
            __this->send_buf[4 * i + 1] = near_data_buf[i * 2 + 1];
#else
            __this->send_buf[4 * i] = near_data_buf[i];
#endif
        }
#endif

#if (WIFI_PCM_STREAN_TEST == PCM_TEST_MODE_1)
        extern void wifi_pcm_stream_socket_send(u8 * buf, u32 len);
        wifi_pcm_stream_socket_send(near_data_buf, sizeof(near_data_buf));
#endif

#endif

#if 0
        /*Feed data to engine*/
        ret = jl_far_keyword_model_process(kws, model, near_data_buf, sizeof(near_data_buf));
        if (ret > 1) {
            printf("++++++++++++++++++ %d ++++++++++++++++++\n", ret);
            jl_far_keyword_model_reset(kws);
        }

#else
        asp2_aec_process(aec, near_data_buf, far_data_buf, near_data_buf, &aec_len);

        u8 *p = (u8 *)near_data_buf;
        u32 remain_len = ONCE_SR_POINTS * 2;

#if 0
        if (tmp_offset + ONCE_SR_POINTS > 160) {
#else
        while (remain_len > 160 * 2) {
#endif
            memcpy(&tmp_aec_buf[tmp_offset / 2], p, sizeof(tmp_aec_buf) - tmp_offset);

            ret = jl_far_keyword_model_process(kws, model, tmp_aec_buf, sizeof(tmp_aec_buf));
            if (ret > 1) {
                printf("++++++++++++++++++ %d ++++++++++++++++++\n", ret);
                jl_far_keyword_model_reset(kws);
            }

            p += sizeof(tmp_aec_buf) - tmp_offset;
            remain_len -= sizeof(tmp_aec_buf) - tmp_offset;
            tmp_offset = 0;
        }

        if (remain_len > 0) {
            memcpy(tmp_aec_buf, p, remain_len);
            tmp_offset += remain_len;
            remain_len = 0;
        }

#ifdef WIFI_PCM_STREAN_SOCKET_ENABLE
#if (WIFI_PCM_STREAN_TEST == PCM_TEST_MODE_0)
        extern void wifi_pcm_stream_socket_send(u8 * buf, u32 len);
        for (u32 i = 0; i < ONCE_SR_POINTS; ++i) {
            __this->send_buf[4 * i + 2] = far_data_buf[i];
            __this->send_buf[4 * i + 3] = near_data_buf[i];
        }
        wifi_pcm_stream_socket_send((u8 *)__this->send_buf, sizeof(__this->send_buf));
#endif
#endif
#endif
    }

__exit:

    if (kws) {
        jl_far_keyword_model_free(kws);
    }
    if (aec) {
        asp2_aec_free(aec);
    }
    if (kws_memory) {
        free(kws_memory);
    }
    if (aec_memory) {
        free(aec_memory);
    }

    __this->run_flag = 0;
}

static void enc_server_event_handler(void *priv, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_SERVER_EVENT_ERR:
    case AUDIO_SERVER_EVENT_END:
        break;
    default:
        break;
    }
}

static int aisp_vfs_fwrite(void *file, void *data, u32 len)
{
    cbuffer_t *cbuf = (cbuffer_t *)file;

    u32 wlen = cbuf_write(cbuf, data, len);
    if (wlen != len) {
        cbuf_clear(&__this->mic_cbuf);
#ifndef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
        cbuf_clear(&__this->linein_cbuf);
#endif
        puts("busy!\n");
    }
    if (file == (void *)&__this->mic_cbuf) {
        os_sem_set(&__this->sem, 0);
        os_sem_post(&__this->sem);
    }

    return len;
}

static int aisp_vfs_fclose(void *file)
{
    return 0;
}

static const struct audio_vfs_ops aisp_vfs_ops = {
    .fwrite = aisp_vfs_fwrite,
    .fclose = aisp_vfs_fclose,
};

int aisp_open(u16 sample_rate)
{
    __this->exit_flag = 0;
    __this->mic_enc = server_open("audio_server", "enc");
#ifndef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
    __this->linein_enc = server_open("audio_server", "enc");
#endif
    server_register_event_handler(__this->mic_enc, NULL, enc_server_event_handler);
    cbuf_init(&__this->mic_cbuf, __this->mic_buf, sizeof(__this->mic_buf));
#ifndef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
    server_register_event_handler(__this->linein_enc, NULL, enc_server_event_handler);
    cbuf_init(&__this->linein_cbuf, __this->linein_buf, sizeof(__this->linein_buf));
#endif
    os_sem_create(&__this->sem, 0);
    __this->sample_rate = sample_rate;

    return thread_fork("aisp", 3, 2048, 0, &__this->pid, aisp_task, __this);
}

void aisp_suspend(void)
{
    union audio_req req = {0};

    if (!__this->run_flag) {
        return;
    }

    __this->run_flag = 0;

    req.enc.cmd = AUDIO_ENC_STOP;
    server_request(__this->mic_enc, AUDIO_REQ_ENC, &req);
    cbuf_clear(&__this->mic_cbuf);
#ifndef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
    server_request(__this->linein_enc, AUDIO_REQ_ENC, &req);
    cbuf_clear(&__this->linein_cbuf);
#endif
}

void aisp_resume(void)
{
    union audio_req req = {0};

    if (__this->run_flag) {
        return;
    }

    __this->run_flag = 1;
    os_sem_set(&__this->sem, 0);
    os_sem_post(&__this->sem);

    req.enc.cmd = AUDIO_ENC_OPEN;
#ifndef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
#if AISP_DUAL_MIC_ALGORITHM
    req.enc.channel = 2;
    req.enc.channel_bit_map = BIT(CONFIG_AISP_MIC0_ADC_CHANNEL) | BIT(CONFIG_AISP_MIC1_ADC_CHANNEL);
#else
    req.enc.channel = 1;
    req.enc.channel_bit_map = BIT(CONFIG_AISP_MIC0_ADC_CHANNEL);
#endif
#else
#if AISP_DUAL_MIC_ALGORITHM
    req.enc.channel = 3;
    req.enc.channel_bit_map = BIT(CONFIG_AISP_MIC0_ADC_CHANNEL) | BIT(CONFIG_AISP_MIC1_ADC_CHANNEL) | BIT(CONFIG_AISP_LINEIN_ADC_CHANNEL);
#else
    req.enc.channel = 2;
    req.enc.channel_bit_map = BIT(CONFIG_AISP_MIC0_ADC_CHANNEL) | BIT(CONFIG_AISP_LINEIN_ADC_CHANNEL);
#endif
#endif
    req.enc.frame_size = ONCE_SR_POINTS * 2 * req.enc.channel;
    req.enc.volume = CONFIG_AISP_MIC_ADC_GAIN;
    req.enc.sample_rate = __this->sample_rate;
    req.enc.format = "pcm";
#if CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_PLNK0
    req.enc.sample_source = "plnk0";
#elif CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_PLNK1
    req.enc.sample_source = "plnk1";
#elif CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_IIS0
    req.enc.sample_source = "iis0";
#elif CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_IIS1
    req.enc.sample_source = "iis1";
#else
    req.enc.sample_source = "mic";
#endif
    req.enc.vfs_ops = &aisp_vfs_ops;
    req.enc.output_buf_len = req.enc.frame_size * 3;
    req.enc.file = (FILE *)&__this->mic_cbuf;

    server_request(__this->mic_enc, AUDIO_REQ_ENC, &req);

#ifdef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
    extern void adc_multiplex_set_gain(const char *source, u8 channel_bit_map, u8 gain);
    adc_multiplex_set_gain("mic", BIT(CONFIG_AISP_LINEIN_ADC_CHANNEL), CONFIG_AISP_LINEIN_ADC_GAIN);
#else
    memset(&req, 0, sizeof(req));

    req.enc.cmd = AUDIO_ENC_OPEN;
    req.enc.channel = 1;
    req.enc.volume = CONFIG_AISP_LINEIN_ADC_GAIN;
    req.enc.sample_rate = __this->sample_rate;
    req.enc.format = "pcm";
    req.enc.frame_size = ONCE_SR_POINTS * 2;
#ifdef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN_OTHER
    req.enc.sample_source = "mic";	//使用数字MIC且用差分MIC做回采时需要打开这个
#else
    req.enc.sample_source = "linein";
#endif
    req.enc.vfs_ops = &aisp_vfs_ops;
    req.enc.output_buf_len = req.enc.frame_size * 3;
    req.enc.channel_bit_map = BIT(CONFIG_AISP_LINEIN_ADC_CHANNEL);
    req.enc.file = (FILE *)&__this->linein_cbuf;

    server_request(__this->linein_enc, AUDIO_REQ_ENC, &req);
#endif
}

void aisp_close(void)
{
    if (__this->exit_flag) {
        return;
    }

    aisp_suspend();

    __this->exit_flag = 1;

    os_sem_post(&__this->sem);

    if (__this->mic_enc) {
        server_close(__this->mic_enc);
        __this->mic_enc = NULL;
    }
#ifndef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
    if (__this->linein_enc) {
        server_close(__this->linein_enc);
        __this->linein_enc = NULL;
    }
#endif

    thread_kill(&__this->pid, KILL_WAIT);
}

#endif

