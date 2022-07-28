#include "server/audio_server.h"
#include "server/server_core.h"
#include "generic/circular_buf.h"
#include "json_c/json_tokener.h"
#include "fs/fs.h"
#include "asm/sfc_norflash_api.h"
#include "os/os_api.h"
#include "event.h"
#include "event/key_event.h"
#include "app_config.h"
#include <time.h>

#if (defined CONFIG_ASR_ALGORITHM) && (CONFIG_ASR_ALGORITHM == AISP_ALGORITHM)

#if defined CONFIG_VIDEO_ENABLE || defined CONFIG_NO_SDRAM_ENABLE
#define AISP_DUAL_MIC_ALGORITHM    0   //0选择单mic/1选择双mic算法
#else
#define AISP_DUAL_MIC_ALGORITHM    1   //0选择单mic/1选择双mic算法
#endif

#if CONFIG_AUDIO_ENC_SAMPLE_SOURCE != AUDIO_ENC_SAMPLE_SOURCE_MIC
#ifdef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
#define CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN_OTHER
#endif
#undef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
#endif

typedef struct LDEEW_ENGINE LDEEW_ENGINE_S;

char *LDEEW_Get_DeviceName(void);
char *LDEEW_version(void);
int LDEEW_memSize(void);
LDEEW_ENGINE_S *LDEEW_RUN(void *pvMemBase, int len, void *pvWkpEnv, void *pvWkpHandler, void *pvAsrHandler);
int LDEEW_feed(LDEEW_ENGINE_S *pstLfespdEng, char *pcData, int iLen);

extern u32 timer_get_ms(void);
extern void aisp_resume(void);

#if AISP_DUAL_MIC_ALGORITHM
#define ONCE_SR_POINTS	160
#ifdef WIFI_PCM_STREAN_SOCKET_ENABLE
#define AISP_BUF_SIZE	(48 * ONCE_SR_POINTS)	//跑不过来时适当加大倍数
#else
#define AISP_BUF_SIZE	(12 * ONCE_SR_POINTS)	//跑不过来时适当加大倍数
#endif
#else
#define ONCE_SR_POINTS	512
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

typedef struct {
    int len;
    char name[64];
} FILE_INFO;

static struct {
    int pid;
    u16 sample_rate;
    u8 volatile exit_flag;
    u8 volatile run_flag;
    u8 auth_flag;
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
    cbuffer_t aec_cbuf;
    s16 aec_buf[AEC_BUF_SIZE];
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

static int wtk_handler(void *pvUsrData, int iIdx, char *pcJson)
{
    struct key_event key = {0};
    json_object *obj = NULL;
    json_object *keyword = NULL;
    const char *str_keyword = NULL;

    /* printf("Channel %d wakeuped; Details: %s\n", iIdx, pcJson); */
#if 0
    char time_str[128] = {0};
    struct tm timeinfo = {0};
    time_t timestamp = time(NULL) + 28800;
    localtime_r(&timestamp, &timeinfo);
    strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%S", &timeinfo);
    printf("{ wakeup_word: \"小爱同学\", awaken_time: %s}\n", time_str);;
#endif

    obj = json_tokener_parse(pcJson);
    if (obj) {
        keyword = json_object_object_get(obj, "wakeupWord");
        if (keyword) {
            str_keyword = json_object_get_string(keyword);
            if (str_keyword) {
                printf("{ wakeup_word: %s}\n", str_keyword);
            }
            if (!strcmp(str_keyword, "zan ting bo fang")) {
                key.action = KEY_EVENT_HOLD;
                key.value = KEY_LOCAL;
            } else if (!strcmp(str_keyword, "bo fang yin yue")) {
                key.action = KEY_EVENT_UP;
                key.value = KEY_LOCAL;
            } else if (!strcmp(str_keyword, "shang yi shou")) {
                key.action = KEY_EVENT_LONG;
                key.value = KEY_UP;
            } else if (!strcmp(str_keyword, "xia yi shou")) {
                key.action = KEY_EVENT_LONG;
                key.value = KEY_DOWN;
            } else if (!strcmp(str_keyword, "da sheng yi dian")) {
                key.action = KEY_EVENT_CLICK;
                key.value = KEY_DOWN;
            } else if (!strcmp(str_keyword, "xiao sheng yi dian")) {
                key.action = KEY_EVENT_CLICK;
                key.value = KEY_UP;
            } else if (!strcmp(str_keyword, "jie ting dian hua")) {
                key.action = KEY_EVENT_CLICK;
                key.value = KEY_PHONE;
            } else {
                key.action = KEY_EVENT_CLICK;
                key.value = KEY_ENC;
            }

            key.type = KEY_EVENT_USER;
            key_event_notify(KEY_EVENT_FROM_USER, &key);

#ifdef WIFI_PCM_STREAN_SOCKET_ENABLE
#if (WIFI_PCM_STREAN_TEST == PCM_TEST_MODE_1)
            {
                int len, total_len = 0;
                //在我这里发送信息
                FILE_INFO info = {0};
                info.len = strlen(str_keyword);
                strcpy(info.name, str_keyword);
                extern void wifi_pcm_stream_socket_send(u8 * buf, u32 len);
                wifi_pcm_stream_socket_send((u8 *)&info, sizeof(info));

                //在这里发送音频
                while (cbuf_get_data_size(&__this->aec_cbuf)) {
                    len = cbuf_read(&__this->aec_cbuf, __this->send_buf, sizeof(__this->send_buf));
                    total_len += len;
                    wifi_pcm_stream_socket_send(__this->send_buf, len);
                    if (total_len >= AEC_BUF_SIZE * sizeof(s16)) {
                        break;
                    }
                }
                extern  void wifi_pcm_stream_socket_close(void);
                wifi_pcm_stream_socket_close();
            }
#endif
#endif

#if !defined CONFIG_NO_SDRAM_ENABLE
            {
                extern int check_asr_format(cbuffer_t *cbuf, int sample_rate, const char *text, u8 digital, u8 dual);
                check_asr_format(&__this->wtk_cbuf, __this->sample_rate, str_keyword,
                                 CONFIG_AUDIO_ENC_SAMPLE_SOURCE != AUDIO_ENC_SAMPLE_SOURCE_MIC, AISP_DUAL_MIC_ALGORITHM);
            }
#endif
        }
        json_object_put(obj);
    }

    return 0;
}

#if AISP_DUAL_MIC_ALGORITHM
static int aec_handler(void *pvUsrData, s8 *pcData, s32 iLen, s8 iStatus)
#else
//typedef int (*LDEEW_handler_t)(void *user_data, int status, char *json, int bytes);
static int aec_handler(void *pcData, int iStatus, char *json, int iLen)
#endif
{
    u32 wlen;
#ifdef WIFI_PCM_STREAN_SOCKET_ENABLE
#if (WIFI_PCM_STREAN_TEST == PCM_TEST_MODE_1)
    if (!cbuf_is_write_able(&__this->aec_cbuf, iLen)) {
        cbuf_read_updata(&__this->aec_cbuf, iLen);
    }
#endif
    cbuf_write(&__this->aec_cbuf, pcData, iLen);
#endif

#if !defined CONFIG_NO_SDRAM_ENABLE
    extern u8 get_asr_data_used(void);
    if (!get_asr_data_used()) {
        if (!cbuf_is_write_able(&__this->wtk_cbuf, iLen)) {
            cbuf_read_updata(&__this->wtk_cbuf, iLen);
        }
        cbuf_write(&__this->wtk_cbuf, pcData, iLen);
    }

    wlen = cbuf_write(&__this->asr_cbuf, pcData, iLen);
    if (wlen != iLen) {
//        puts("ai busy!\n");
    }
    os_sem_set(&__this->asr_sem, 0);
    os_sem_post(&__this->asr_sem);
#endif
    return 0;
}

typedef struct dui_auth_info {
    char *productKey;
    char *productId;
    char *ProductSecret;
} dui_auth_info_t;

static void aisp_task(void *priv)
{
    short buf[ONCE_SR_POINTS * (2 + AISP_DUAL_MIC_ALGORITHM)];
    u32 time = 0, time_cnt = 0, cnt = 0;
    LDEEW_ENGINE_S *pstLfespdEng = NULL;
    u32 memPoolLen = LDEEW_memSize();  //获取算法需要的heap大小
    u32 mic_len, linein_len;

    printf("aisp_main run 0x%x,%s\r\n", (u32)LDEEW_version(), LDEEW_version());
    printf("memPoolLen is:%d\n", memPoolLen);

    dui_auth_info_t auth_info;
    /* 此处信息请根据dui信息修改 https://www.duiopen.com/ */
    auth_info.productKey = "请向思必驰购买lisence";
    auth_info.productId = "请向思必驰购买lisence";
    auth_info.ProductSecret = "请向思必驰购买lisence";
    extern int app_dui_auth_second(dui_auth_info_t *auth_info, u8 mark);
    if (0 != app_dui_auth_second(&auth_info, 0)) {
        printf("dui auth fail, please contact aispeech!!!\n");
        return;
    }
    puts("app_dui_auth_second ok\n");

    void *pcMemPool = calloc(1, memPoolLen);  //申请内存
    if (NULL == pcMemPool) {
        return;
    }

    /* start engine and pass auth cfg*/
    pstLfespdEng = LDEEW_RUN(pcMemPool, memPoolLen, "words=xiao ai tong xue,da sheng yi dian,xiao sheng yi dian,zan ting bo fang,xia yi shou;thresh=0.60,0.32,0.32,0.33,0.33;", wtk_handler, aec_handler); //唤醒词：小爱同学
    /* pstLfespdEng = LDEEW_RUN(pcMemPool, memPoolLen, "words=ni hao xiao le,da sheng yi dian,xiao sheng yi dian,zan ting bo fang,xia yi shou;thresh=0.60,0.32,0.32,0.33,0.33;", wtk_handler, aec_handler); //唤醒词：你好小乐 */
    if (NULL == pstLfespdEng || pstLfespdEng == (void *)0xffffffff) {
        printf("LDEEW_Start auth fail \n");
        free(pcMemPool);
        return;
    }
    printf("LDEEW_Start auth OK \n");

    __this->auth_flag = 1;

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
        short tempbuf[ONCE_SR_POINTS * (2 + AISP_DUAL_MIC_ALGORITHM)];
        mic_len = cbuf_read(&__this->mic_cbuf, tempbuf, ONCE_SR_POINTS * 2 * (AISP_DUAL_MIC_ALGORITHM + 1));
        if (!mic_len) {
            continue;
        }
        linein_len = cbuf_read(&__this->linein_cbuf, (u8 *)tempbuf + mic_len, ONCE_SR_POINTS * 2);
        if (!linein_len) {
            continue;
        }

        mic_len /= 2;
        //重组数据
        for (u32 i = 0, j = 0; j < ONCE_SR_POINTS; ++j) {
#if AISP_DUAL_MIC_ALGORITHM
            buf[i++] = tempbuf[j * 2];
            buf[i++] = tempbuf[j * 2 + 1];
#else
            buf[i++] = tempbuf[j];
#endif
            buf[i++] = tempbuf[mic_len + j];
        }

#else

        mic_len = cbuf_read(&__this->mic_cbuf, buf, sizeof(buf));
        if (!mic_len) {
            os_sem_pend(&__this->sem, 0);
            continue;
        }
#if CONFIG_AISP_LINEIN_ADC_CHANNEL == 1 && (CONFIG_AISP_LINEIN_ADC_CHANNEL < CONFIG_AISP_MIC0_ADC_CHANNEL || CONFIG_AISP_LINEIN_ADC_CHANNEL < CONFIG_AISP_MIC1_ADC_CHANNEL)
#if AISP_DUAL_MIC_ALGORITHM
        s16 temp = 0;
        for (u32 i = 0; i < ONCE_SR_POINTS; ++i) {
            temp = buf[3 * i + 1];
            buf[3 * i + 1] = buf[3 * i + 2];
            buf[3 * i + 2] = temp;
        }
#else
        s16 temp = 0;
        for (u32 i = 0; i < ONCE_SR_POINTS; ++i) {
            temp = buf[2 * i + 1];
            buf[2 * i + 1] = buf[2 * i];
            buf[2 * i] = temp;
        }
#endif
#endif

#endif
        time = timer_get_ms();
        /*Feed data to engine*/

        LDEEW_feed(pstLfespdEng, (char *)buf, sizeof(buf));	//单MIC时TODO

#ifdef WIFI_PCM_STREAN_SOCKET_ENABLE
#if (WIFI_PCM_STREAN_TEST == PCM_TEST_MODE_0)
        short temp_buf[ONCE_SR_POINTS];
        extern void wifi_pcm_stream_socket_send(u8 * buf, u32 len);
        cbuf_read(&__this->aec_cbuf, temp_buf, sizeof(temp_buf));
        for (u32 i = 0; i < ONCE_SR_POINTS; ++i) {
#if AISP_DUAL_MIC_ALGORITHM
            __this->send_buf[4 * i] = buf[3 * i];
            __this->send_buf[4 * i + 1] = buf[3 * i + 1];
            __this->send_buf[4 * i + 2] = buf[3 * i + 2];
            __this->send_buf[4 * i + 3] = temp_buf[i];
#else
            __this->send_buf[4 * i] = buf[2 * i];
            __this->send_buf[4 * i + 2] = buf[2 * i + 1];
            __this->send_buf[4 * i + 3] = temp_buf[i];
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

    task_kill("uda_main");

    free(pcMemPool);

    __this->auth_flag = 0;
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
#ifdef WIFI_PCM_STREAN_SOCKET_ENABLE
        cbuf_clear(&__this->aec_cbuf);
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
#ifdef WIFI_PCM_STREAN_SOCKET_ENABLE
    cbuf_init(&__this->aec_cbuf, __this->aec_buf, sizeof(__this->aec_buf));
#endif
#if !defined CONFIG_NO_SDRAM_ENABLE
    cbuf_init(&__this->wtk_cbuf, __this->wtk_buf, sizeof(__this->wtk_buf));
    cbuf_init(&__this->asr_cbuf, __this->asr_buf, sizeof(__this->asr_buf));
    os_sem_create(&__this->asr_sem, 0);
#endif

    os_sem_create(&__this->sem, 0);
    __this->sample_rate = sample_rate;

#if AISP_DUAL_MIC_ALGORITHM
    return thread_fork("aisp", 3, 3328, 0, &__this->pid, aisp_task, __this);
#else
    return thread_fork("aisp", 3, 3840, 0, &__this->pid, aisp_task, __this);
#endif
}

void aisp_suspend(void)
{
    union audio_req req = {0};

    if (!__this->auth_flag || !__this->run_flag) {
        return;
    }

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

    if (!__this->auth_flag || __this->run_flag) {
        return;
    }

#ifdef WIFI_PCM_STREAN_SOCKET_ENABLE
    cbuf_clear(&__this->aec_cbuf);
#endif
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

extern int wifi_get_mac(u8 *mac_addr);
#define PROFILE_PATH "mnt/sdfile/app/aisp"

u32 aisp_get_auth_second_flash_addr(void)
{
    u32 profile_addr;
    FILE *profile_fp = fopen(PROFILE_PATH, "r");
    if (profile_fp == NULL) {
        puts("aisp_get_auth_second_flash_addr ERROR!!!\r\n");
        ASSERT(0);
    }
    struct vfs_attr file_attr;
    fget_attrs(profile_fp, &file_attr);
    profile_addr = sdfile_cpu_addr2flash_addr(file_attr.sclust);
    fclose(profile_fp);
    return profile_addr;
}

void aisp_auth_flash_erase_sector(u32 addr)
{
    norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, addr);
}

void aisp_auth_flash_write(void *data, u32 addr, u32 len)
{
    norflash_write(NULL, data, len, addr);
}

void aisp_auth_flash_read(void *data, u32 addr, u32 len)
{
    norflash_read(NULL, data, len, addr);
}

void get_wifi_mac_addr(u8 *mac_addr)
{
    wifi_get_mac(mac_addr);
}
#endif

