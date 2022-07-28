#include "server/audio_server.h"
#include "server/server_core.h"
#include "generic/circular_buf.h"
#include "fs/fs.h"
#include "os/os_api.h"
#include "event.h"
#include "app_config.h"

#if (defined CONFIG_ASR_ALGORITHM) && (CONFIG_ASR_ALGORITHM == JLASP_ALGORITHM)

#if defined CONFIG_VIDEO_ENABLE || defined CONFIG_NO_SDRAM_ENABLE
#define AISP_DUAL_MIC_ALGORITHM    0   //0选择单mic/1选择双mic算法
#else
#define AISP_DUAL_MIC_ALGORITHM    0   //0选择单mic/1选择双mic算法
#endif

#if CONFIG_AUDIO_ENC_SAMPLE_SOURCE != AUDIO_ENC_SAMPLE_SOURCE_MIC
#ifdef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
#define CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN_OTHER
#endif
#undef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
#endif

extern u32 timer_get_ms(void);

#if AISP_DUAL_MIC_ALGORITHM
#define ONCE_SR_POINTS	160
#define AISP_BUF_SIZE	(48 * ONCE_SR_POINTS)	//跑不过来时适当加大倍数
#else
#define ONCE_SR_POINTS	256
#if defined CONFIG_NO_SDRAM_ENABLE
#define AISP_BUF_SIZE	(2  * ONCE_SR_POINTS)	//跑不过来时适当加大倍数
#else
#define AISP_BUF_SIZE	(4  * ONCE_SR_POINTS)	//跑不过来时适当加大倍数
#endif
#endif

#define PCM_TEST_MODE_0                0   //测试模式0，会发送当前音频给电脑
#define PCM_TEST_MODE_1                1   //测试模式1，会发唤醒时当前缓存音频给电脑

#define WIFI_PCM_STREAN_TEST       PCM_TEST_MODE_0

#ifdef WIFI_PCM_STREAN_SOCKET_ENABLE

#if (WIFI_PCM_STREAN_TEST == PCM_TEST_MODE_0)
#define AEC_BUF_SIZE AISP_BUF_SIZE
#else
#define AEC_BUF_SIZE  16 * 1024
#endif
#endif

#define WTK_BUF_SIZE  32 * 1024

#define CONFIG_NO_SDRAM_ENABLE

static struct {
    int pid;
    u32 sample_rate;
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
#if !defined CONFIG_NO_SDRAM_ENABLE
    cbuffer_t wtk_cbuf;
    s16 wtk_buf[WTK_BUF_SIZE];

    cbuffer_t asr_cbuf;
    s16 asr_buf[AISP_BUF_SIZE];
    OS_SEM asr_sem;
#endif
} aisp_server;

#define __this (&aisp_server)

typedef struct {
    int key_aec;
    int key_ns;
    int key_agc;
    int key_kws;
} JL_disp_param;

int JL_Spp_Get_memSize(JL_disp_param *param);
void *ESGW_init(void *mem_buf, int buf_len, void *forward_handle, void *wt_handle, JL_disp_param *param);
int ESGW_rest(void *ENHSK_Feng, JL_disp_param *param);
int ESGW_RUN(void *ENHSK_Feng, short *near_buf, short *far_buf, JL_disp_param *param);
int ESGW_free(void *ptr);
int WUTONG_memsize(void);
void *WUTONG_RUN(void *pvData_mem, void *file_ptr, int mem_len, int online, float confidence);
int WUTONG_feed(void *anwFinit, char *pdata, int len);

static void aisp_task(void *priv)
{
    u32 time = 0, time_cnt = 0, cnt = 0;
    u32 mic_len, linein_len;
    u32 tmp_offset = 0;
    s16 tmp_aec_buf[160];

    FILE *fp = fopen("mnt/spiflash/res/param.bin", "r");
    if (fp == NULL) {
        return;
    }
    void *file_ptr = malloc(1024 * 83);
    fread(fp, file_ptr, 1024 * 83);//64
    fclose(fp);

    //aec ns agc init
    JL_disp_param param = {0};
    JL_disp_param *param_s = &param;

    param_s->key_aec = 0;
    param_s->key_agc = 0;
    param_s->key_ns = 0;
    param_s->key_kws = 0;

    u32 mem_len = JL_Spp_Get_memSize(param_s);

    void *mem_buf = zalloc(mem_len);
    if (NULL == mem_buf) {
        free(file_ptr);
        return;
    }

    void *ENHSK_Feng = ESGW_init(mem_buf, mem_len, 0, 0, param_s);
    ESGW_rest(ENHSK_Feng, param_s);

    void *buf_ptr = zalloc(WUTONG_memsize());
    if (!buf_ptr) {
        free(mem_buf);
        free(file_ptr);
        return;
    }

    int online_cmvn = 0;
    float confidence = 0.4;
    void *anwFinitEng = WUTONG_RUN(buf_ptr, file_ptr, WUTONG_memsize(), online_cmvn, confidence);

    while (1) {
        if (!__this->run_flag) {
            os_sem_pend(&__this->sem, 0);
            continue;
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
            __this->send_buf[4 * i] = near_data_buf[i];
        }
#endif
#endif

        time = timer_get_ms();
        /*Feed data to engine*/
        /* ESGW_RUN(ENHSK_Feng, near_data_buf, far_data_buf, param_s); */

#if !defined CONFIG_NO_SDRAM_ENABLE
        extern u8 get_asr_data_used(void);
        if (!get_asr_data_used()) {
            if (!cbuf_is_write_able(&__this->wtk_cbuf, sizeof(near_data_buf))) {
                cbuf_read_updata(&__this->wtk_cbuf, sizeof(near_data_buf));
            }
            cbuf_write(&__this->wtk_cbuf, near_data_buf, sizeof(near_data_buf));
        }

        cbuf_write(&__this->asr_cbuf, near_data_buf, sizeof(near_data_buf));
        os_sem_set(&__this->asr_sem, 0);
        os_sem_post(&__this->asr_sem);
#endif

        u8 *p = (u8 *)near_data_buf;
        u32 remain_len = ONCE_SR_POINTS * 2;

        while (remain_len > 160 * 2) {
            memcpy(&tmp_aec_buf[tmp_offset / 2], p, sizeof(tmp_aec_buf) - tmp_offset);
            time = timer_get_ms();
            WUTONG_feed(anwFinitEng, (char *)tmp_aec_buf, sizeof(tmp_aec_buf));
            printf("use : %d\n", timer_get_ms() - time);
            tmp_offset = 0;
            remain_len -= sizeof(tmp_aec_buf) - tmp_offset;
            p += sizeof(tmp_aec_buf) - tmp_offset;
        }

        if (remain_len > 0) {
            memcpy(tmp_aec_buf, p, remain_len);
            tmp_offset += remain_len;
        }

#ifdef WIFI_PCM_STREAN_SOCKET_ENABLE
#if (WIFI_PCM_STREAN_TEST == PCM_TEST_MODE_0)
        extern void wifi_pcm_stream_socket_send(u8 * buf, u32 len);
        for (u32 i = 0; i < ONCE_SR_POINTS; ++i) {
#if AISP_DUAL_MIC_ALGORITHM

#else
            __this->send_buf[4 * i + 1] = far_data_buf[i];
            __this->send_buf[4 * i + 3] = near_data_buf[i];
#endif
        }
        wifi_pcm_stream_socket_send((u8 *)__this->send_buf, sizeof(__this->send_buf));
#endif
#endif

        time_cnt += timer_get_ms() - time;
        if (++cnt == 100) {
            /* printf("aec time :%d \n", time_cnt); */
            time_cnt = cnt = 0;
        }
    }

    free(mem_buf);
    free(file_ptr);
    free(buf_ptr);
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
#if !defined CONFIG_NO_SDRAM_ENABLE
        extern u8 get_asr_data_used(void);
        if (!get_asr_data_used()) {
            cbuf_clear(&__this->wtk_cbuf);
        }
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

#if !defined CONFIG_NO_SDRAM_ENABLE
u32 asr_read_input(u8 *buf, u32 len)
{
    u32 rlen = 0;

    do {
        if (!__this->run_flag) {
            return 0;
        }
        rlen = cbuf_read(&__this->asr_cbuf, buf, len);
        if (rlen == len) {
            break;
        }
        os_sem_pend(&__this->asr_sem, 0);
    } while (!rlen);

    return rlen;
}
#endif

void *get_asr_read_input_cb(void)
{
    if (!__this->run_flag) {
        return NULL;
    }

#if !defined CONFIG_NO_SDRAM_ENABLE
    os_sem_set(&__this->asr_sem, 0);
    cbuf_clear(&__this->asr_cbuf);
    return (void *)&asr_read_input;
#else
    return NULL;
#endif
}

int aisp_open(u16 sample_rate)
{
    union audio_req req = {0};

    __this->mic_enc = server_open("audio_server", "enc");
    if (!__this->mic_enc) {
        goto __err;
    }

#ifndef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
    __this->linein_enc = server_open("audio_server", "enc");
    if (!__this->linein_enc) {
        goto __err;
    }
#endif

    server_register_event_handler(__this->mic_enc, NULL, enc_server_event_handler);
    cbuf_init(&__this->mic_cbuf, __this->mic_buf, sizeof(__this->mic_buf));
#ifndef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
    server_register_event_handler(__this->linein_enc, NULL, enc_server_event_handler);
    cbuf_init(&__this->linein_cbuf, __this->linein_buf, sizeof(__this->linein_buf));
#endif
#if !defined CONFIG_NO_SDRAM_ENABLE
    cbuf_init(&__this->wtk_cbuf, __this->wtk_buf, sizeof(__this->wtk_buf));
    cbuf_init(&__this->asr_cbuf, __this->asr_buf, sizeof(__this->asr_buf));
    os_sem_create(&__this->asr_sem, 0);
#endif

    os_sem_create(&__this->sem, 0);

#if AISP_DUAL_MIC_ALGORITHM
    thread_fork("aisp", 3, 2532, 0, &__this->pid, aisp_task, __this);
#else
    thread_fork("aisp", 3, 3300, 0, &__this->pid, aisp_task, __this);
#endif

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
    req.enc.sample_rate = sample_rate;
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
    req.enc.sample_rate = sample_rate;
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

    __this->sample_rate = sample_rate;
    __this->run_flag = 1;
    os_sem_post(&__this->sem);

    return 0;

__err:
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

    return -1;
}

void aisp_suspend(void)
{
    union audio_req req = {0};

    __this->run_flag = 0;

#if !defined CONFIG_NO_SDRAM_ENABLE
    os_sem_post(&__this->asr_sem);
#endif

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

#if !defined CONFIG_NO_SDRAM_ENABLE
    cbuf_clear(&__this->wtk_cbuf);
#endif

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

#endif

