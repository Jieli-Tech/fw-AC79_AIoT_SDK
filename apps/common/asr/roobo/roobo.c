#include "app_config.h"
#include "server/audio_server.h"
#include "server/server_core.h"
#include "generic/circular_buf.h"
#include "os/os_api.h"
#include "os/os_api.h"
#include "app_config.h"
#include <time.h>
#include "event/key_event.h"
#include "asr.h"
#include "ivPolos.h"
#include "fig_asr_api.h"
#include "fig_asr_type.h"


#if (defined CONFIG_ASR_ALGORITHM) && (CONFIG_ASR_ALGORITHM == ROOBO_ALGORITHM)

#include "resmapping_table.h"

#define ROOBO_DUAL_MIC_ALGORITHM    0   //选择单mic/双mic算法

/*每次AEC输入的数据量大小, 匹配的每个大小是short*/
#define ROOBO_AEC_INPUT_SHORT_LEN      512
#define ROOBO_ASR_INPUT_SHORT_LEN      160   /* 10ms */
//#define ROOBO_ASR_INPUT_SHORT_LEN      480   /* 30ms */

#define CACHE_BUF_SIZE	(32 * 1024)

typedef struct {
    int wordId;
    void *text;
    float score;
} roobo_asr_result_t;

typedef struct {
    short *left_mic;                 /* 16bit第一路麦克风 */
    short *right_mic;                /* 16bit第二路麦克风，没有填NULL */
    short *left_aec;                 /* 16bit第一路参考信号 */
    short *right_aec;                /* 16bit第二路参考信号, 没有填NULL */
    int len;                         /* 对应short类型长度, 必须= 512*sizeof(short) */
} roobo_aec_stream_t;

static struct {
    FIG_INST pFigAsr;
    char fig_aec_buf[300 * 1024];
    u16 volatile run_flag;
    u16 sample_rate;
    struct server *mic_enc;
    struct server *linein_enc;
    cbuffer_t mic_cbuf;
    cbuffer_t asr_cbuf;
    cbuffer_t aec_cbuf;
#ifndef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
    cbuffer_t linein_cbuf;
    char linein_buf[CACHE_BUF_SIZE];
#if ROOBO_DUAL_MIC_ALGORITHM
    char mic_buf[CACHE_BUF_SIZE * 2];
#else
    char mic_buf[CACHE_BUF_SIZE];
#endif

#else

#if ROOBO_DUAL_MIC_ALGORITHM
    char mic_buf[CACHE_BUF_SIZE * 3];
#else
    char mic_buf[CACHE_BUF_SIZE * 2];
#endif

#endif
    char asr_buf[CACHE_BUF_SIZE];
    char aec_buf[CACHE_BUF_SIZE];
    OS_SEM sem;
    OS_SEM asr_sem;
    OS_SEM aec_sem;
} roobo_server;
#define __this (&roobo_server)

static int wtk_handler(const char *str_keyword)
{
    struct key_event evt = {0};

    char time_str[128] = {0};
    struct tm timeinfo = {0};
    time_t timestamp = time(NULL) + 28800;
    localtime_r(&timestamp, &timeinfo);
    strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%S", &timeinfo);
    printf("{ wakeup_word: \"小爱小爱\", awaken_time: %s}\n", time_str);;

    static const char xiao_ai_xiao_ai[] = {0xd0, 0xa1, 0xb0, 0xac, 0xd0, 0xa1, 0xb0, 0xac}; //小艾小艾
    static const char bo_fang_yin_yue[] = {0xb2, 0xa5, 0xb7, 0xc5, 0xd2, 0xf4, 0xc0, 0xd6 };// 播放音乐
    static const char zan_ting_yin_yue[] = {0xd4, 0xdd, 0xcd, 0xa3, 0xd2, 0xf4, 0xc0, 0xd6}; // 暂停音乐
    static const char zeng_da_yin_liang[] = {0xd4, 0xf6, 0xb4, 0xf3, 0xd2, 0xf4, 0xc1, 0xbf}; //增大音量
    static const char jian_xiao_yin_liang[] = {0xbc, 0xf5, 0xd0, 0xa1, 0xd2, 0xf4, 0xc1, 0xbf}; //减小音量
    static const char shang_yi_qu[] = {0xc9, 0xcf, 0xd2, 0xbb, 0xc7, 0xfa}; //上一曲
    static const char xia_yi_qu[] = {0xcf, 0xc2, 0xd2, 0xbb, 0xc7, 0xfa}; //下一曲

    evt.type = KEY_EVENT_USER;

    if (!memcmp(str_keyword, zan_ting_yin_yue, sizeof(zan_ting_yin_yue))) {
        evt.action = KEY_EVENT_HOLD;
        evt.value = KEY_LOCAL;
    } else if (!memcmp(str_keyword, bo_fang_yin_yue, sizeof(bo_fang_yin_yue))) {
        evt.action = KEY_EVENT_UP;
        evt.value = KEY_LOCAL;
    } else if (!memcmp(str_keyword, shang_yi_qu, sizeof(shang_yi_qu))) {
        evt.action = KEY_EVENT_LONG;
        evt.value = KEY_UP;
    } else if (!memcmp(str_keyword, xia_yi_qu, sizeof(xia_yi_qu))) {
        evt.action = KEY_EVENT_LONG;
        evt.value = KEY_DOWN;
    } else if (!memcmp(str_keyword, zeng_da_yin_liang, sizeof(zeng_da_yin_liang))) {
        evt.action = KEY_EVENT_CLICK;
        evt.value = KEY_DOWN;
    } else if (!memcmp(str_keyword, jian_xiao_yin_liang, sizeof(jian_xiao_yin_liang))) {
        evt.action = KEY_EVENT_CLICK;
        evt.value = KEY_UP;
    } else {
        evt.action = KEY_EVENT_CLICK;
        evt.value = KEY_ENC;
    }

    key_event_notify(KEY_EVENT_FROM_USER, &evt);

    return 0;
}

/*
 * 函数名称: roobo_aec_process
 * 函数功能: roobo aec 处理
 * 输入参数:roobo_aec_stream_t input_stream 输入的audio数据信息, 16K 16bit音频,每个音频的长度为ROOBO_AEC_INPUT_SHORT_LEN*sizeof(short)
 * 输入参数: void* output_data 输出音频的16K 16bit输出音频   512 * sizeof(short)
 * 输出参数: int* out_size 输出音频长度, 12 * sizeof(short)
 * 返回值: int 0 成功，非0 失败
 */
static int roobo_aec_process(roobo_aec_stream_t input_st, void *output_data, int *out_size)
{
    short input_data[ROOBO_AEC_INPUT_SHORT_LEN * 4] = {0};

    if (input_st.len != ROOBO_AEC_INPUT_SHORT_LEN) {
        printf("input data len（short) %d != required %d\n", input_st.len, ROOBO_AEC_INPUT_SHORT_LEN);
        return -1;
    } else if (!input_st.left_mic || !input_st.left_aec) {
        printf("input mic or aec buf is NULL, reject\n");
        return -2;
    }

    short *left_mic = input_st.left_mic;
    short *left_aec = input_st.left_aec;
    short *right_mic = input_st.right_mic;
    short *right_aec = input_st.right_aec;

    if (right_mic) {
        if (right_aec) {
            for (int i = 0; i < ROOBO_AEC_INPUT_SHORT_LEN; i++) {
                input_data[4 * i] = *left_mic++;
                input_data[4 * i + 1] = *right_mic++;
                input_data[4 * i + 2] = *left_aec++;
                input_data[4 * i + 3] = *right_aec++;
            }
        } else {
            for (int i = 0; i < ROOBO_AEC_INPUT_SHORT_LEN; i++) {
                input_data[4 * i] = *left_mic++;
                input_data[4 * i + 1] = *right_mic++;
                input_data[4 * i + 2] = *left_aec++;
//                input_data[4 * i + 3] = *left_aec++;
            }
        }
    } else {
        for (int i = 0; i < ROOBO_AEC_INPUT_SHORT_LEN; i++) {
            input_data[4 * i] = *left_mic++;
            input_data[4 * i + 2] = *left_aec++;
        }
    }

    int ret = POLOS_AUDIOWRITE(input_data, sizeof(input_data), output_data, out_size);

#ifdef WIFI_PCM_STREAN_SOCKET_ENABLE
    s16 *output = (s16 *)output_data;
    for (int i = 0; i < ROOBO_AEC_INPUT_SHORT_LEN; i++) {
        input_data[4 * i + 3] = *output++;  //left_mic right_mic left_linein aec
    }
    extern void wifi_pcm_stream_socket_send(u8 * buf, u32 len);
    wifi_pcm_stream_socket_send((u8 *)input_data, sizeof(input_data));
#endif

    return ret;
}

/*
 * 函数名称: roobo_aec_init
 * 函数功能: roobo aec 初始化
 * 输入参数: aec sdk内部使用的内存大小
 * 输出参数: void
 * 返回值: int 0 成功，非0 失败
 */
static int roobo_aec_init(void *mem, void *userdata)
{
    printf("aec version = %d\n",  GetAECVersion());
    return POLOS_INIT(mem);
}

static int roobo_asr_process(short *buf, int len,  char *text, float *score)
{
//    static int process_index = 0;
//	if(++process_index % 10 == 0) {
//		printf("[%d]figasr_process %d", process_index, len);
//	}

    bool bAudioEnd = 0;
    PAsrResult pResult = NULL;
    int ret = FigWriteAudio(__this->pFigAsr, (char *)buf, len * sizeof(short), bAudioEnd, &pResult);
    if (ret != 0) {
        printf("Error: write audio fail with error code = %d\n", ret);
        return ret;
    }

    // process result
    if (pResult != NULL) {
        printf("asr result:word %d: [%lu] %s!!!!\n", pResult->nWordId, strlen(pResult->szText), pResult->szText);
        printf("asr score %d, form %d to %d", pResult->nCmScore, pResult->nBegin, pResult->nEnd);
        sprintf(text, "%s", pResult->szText);
        *score = pResult->nWordId;
        return 1;
    }

    return 0;
}

static int roobo_asr_init(void *userdata)
{
    int ret = 0;

    ret = FigCreateInst(&__this->pFigAsr, (const char *)nnet, (const char *)graph_cmd);
    if (ret != 0) {
        printf("Error: FigCreateInst fail with error code = %d\n", ret);
        goto exit;
    }

    ret = FigStartProcess(__this->pFigAsr);
    if (ret != 0) {
        printf("Error: asr start fail with error code = %d\n", ret);
        goto exit;
    }
    /* printf("roobo_asr_init ok,,,0x%x,0x%x\n", nnet, graph_cmd); */

exit:
    return ret;
}

static int roobo_asr_deinit(void)
{
    int ret = FigStopProcess(__this->pFigAsr);
    if (ret != 0) {
        printf("Error: asr stop fail with error code = %d\n", ret);
        return ret;
    }

    ret = FigDestroyInst(__this->pFigAsr);
    if (ret != 0) {
        printf("Error: uninitialize fail with error code = %d\n", ret);
        return ret;
    }
    __this->pFigAsr = NULL;

    return 0;
}

static void roobo_asr_reset(void)
{
    asr_reset();
}

static int roobo_vfs_fwrite(void *file, void *data, u32 len)
{
    cbuffer_t *cbuf = (cbuffer_t *)file;
    u32 wlen;

    wlen = cbuf_write(cbuf, data, len);
    if (wlen != len) {
        cbuf_clear(&__this->mic_cbuf);
#ifndef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
        cbuf_clear(&__this->linein_cbuf);
#endif
        puts("aec busy!\n");
    }

    if (file == (void *)&__this->mic_cbuf) {
        os_sem_set(&__this->sem, 0);
        os_sem_post(&__this->sem);
    }
    return len;
}

static int roobo_vfs_fclose(void *file)
{
    return 0;
}

//录音文件操作方法集。传递给MIC底层驱动调用
static const struct audio_vfs_ops roobo_vfs_ops = {
    .fwrite = roobo_vfs_fwrite,
    .fclose = roobo_vfs_fclose,
};


//功能：底层驱动回调函数
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

static int mic_record_start(int sample_rate)
{
    int err = -1;
    union audio_req req = {0};

    //打开enc_server
    if (!__this->mic_enc) {
        __this->mic_enc = server_open("audio_server", "enc");
        if (!__this->mic_enc) {
            goto _mic_err_;
        }
        server_register_event_handler(__this->mic_enc, NULL, enc_server_event_handler);
    }

    //初始化cycle_buf
    cbuf_init(&__this->mic_cbuf, __this->mic_buf, sizeof(__this->mic_buf));

    //MIC数据采集配置
    req.enc.cmd = AUDIO_ENC_OPEN;          //命令
    req.enc.volume = CONFIG_AISP_MIC_ADC_GAIN;   //录音音量
    req.enc.output_buf = NULL;             //缓存buf
    req.enc.sample_rate = sample_rate;     //采样率
    req.enc.format = "pcm";                //录音数据格式
#ifdef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
#if ROOBO_DUAL_MIC_ALGORITHM
    req.enc.channel = 3;
    req.enc.channel_bit_map = BIT(CONFIG_AISP_MIC0_ADC_CHANNEL) | BIT(CONFIG_AISP_MIC1_ADC_CHANNEL) | BIT(CONFIG_AISP_LINEIN_ADC_CHANNEL);
#else
    req.enc.channel = 2;
    req.enc.channel_bit_map = BIT(CONFIG_AISP_MIC0_ADC_CHANNEL) | BIT(CONFIG_AISP_LINEIN_ADC_CHANNEL);
#endif // ROOBO_DUAL_MIC_ALGORITHM
#else
#if ROOBO_DUAL_MIC_ALGORITHM
    req.enc.channel = 2;                   //采样通道数目
    req.enc.channel_bit_map = BIT(CONFIG_AISP_MIC0_ADC_CHANNEL) | BIT(CONFIG_AISP_MIC1_ADC_CHANNEL);
#else
    req.enc.channel = 1;                   //采样通道数目
    req.enc.channel_bit_map = BIT(CONFIG_AISP_MIC0_ADC_CHANNEL);
#endif // ROOBO_DUAL_MIC_ALGORITHM
#endif
    req.enc.frame_size = ROOBO_AEC_INPUT_SHORT_LEN * 2 * req.enc.channel;   //数据帧大小
    req.enc.output_buf_len = req.enc.frame_size * 3;      //缓存buf大小
    req.enc.sample_source = "mic";         //采样源
    req.enc.vfs_ops = &roobo_vfs_ops;     //文件操作方法集
    req.enc.msec = 0;                   //采样时间
    req.enc.file = (FILE *)&__this->mic_cbuf;

    //发送录音请求
    err = server_request(__this->mic_enc, AUDIO_REQ_ENC, &req);
    if (err) {
        goto _mic_err_;
    }

#ifdef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
    extern void adc_multiplex_set_gain(const char *source, u8 channel_bit_map, u8 gain);
    adc_multiplex_set_gain("mic", BIT(CONFIG_AISP_LINEIN_ADC_CHANNEL), CONFIG_AISP_LINEIN_ADC_GAIN);
#endif

    return 0;

_mic_err_:
    if (__this->mic_enc) {
        server_close(__this->mic_enc);
        __this->mic_enc = NULL;
    }

    return -1;
}

#ifndef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
static int linein_record_start(int sample_rate)
{
    int err = -1;
    union audio_req req = {0};

    //打开enc_server
    if (!__this->linein_enc) {
        __this->linein_enc = server_open("audio_server", "enc");
        if (!__this->linein_enc) {
            goto _mic_err_;
        }
        server_register_event_handler(__this->linein_enc, NULL, enc_server_event_handler);
    }

    //初始化cycle_buf
    cbuf_init(&__this->linein_cbuf, __this->linein_buf, sizeof(__this->linein_buf));

    //MIC数据采集配置
    req.enc.cmd = AUDIO_ENC_OPEN;          //命令
    req.enc.channel = 1;                   //采样通道数目
    req.enc.volume = CONFIG_AISP_LINEIN_ADC_GAIN;   //录音音量
    req.enc.output_buf = NULL;             //缓存buf
    req.enc.sample_rate = sample_rate;     //采样率
    req.enc.format = "pcm";                //录音数据格式
    req.enc.frame_size = ROOBO_AEC_INPUT_SHORT_LEN * 2;   //数据帧大小
    req.enc.output_buf_len = req.enc.frame_size * 3;      //缓存buf大小
    req.enc.sample_source = "linein";      //采样源
    req.enc.vfs_ops = &roobo_vfs_ops;     //文件操作方法集
    req.enc.msec = 0;                   //采样时间
    req.enc.file = (FILE *)&__this->linein_cbuf;
    req.enc.channel_bit_map = BIT(CONFIG_AISP_LINEIN_ADC_CHANNEL);

    //发送录音请求
    err = server_request(__this->linein_enc, AUDIO_REQ_ENC, &req);
    if (err) {
        goto _mic_err_;
    }

    return 0;

_mic_err_:
    printf("linin_start failed\n");

    if (__this->linein_enc) {
        server_close(__this->linein_enc);
        __this->linein_enc = NULL;
    }

    return -1;
}
#endif

static void roobo_audiolib_handler(short *left_mic_buf, short *right_mic_buf, short *left_linein_buf, short *right_linein_buf, int len)
{
    int wlen;
    short aec_out_buf[ROOBO_AEC_INPUT_SHORT_LEN];
    int out_len = 0;

    roobo_aec_stream_t input_stream = {
        .left_mic = left_mic_buf,
        .right_mic = right_mic_buf,
        .left_aec = left_linein_buf,
        .right_aec = right_linein_buf,
        .len = len,
    };

    roobo_aec_process(input_stream, aec_out_buf, &out_len);
    out_len *= sizeof(short);
    wlen = cbuf_write(&__this->asr_cbuf, aec_out_buf, out_len);
    if (wlen != out_len) {
        cbuf_clear(&__this->asr_cbuf);
        puts("asr busy!\n");
    }
    os_sem_set(&__this->asr_sem, 0);
    os_sem_post(&__this->asr_sem);

    wlen = cbuf_write(&__this->aec_cbuf, aec_out_buf, out_len);
    if (wlen != out_len) {
        /* cbuf_clear(&__this->aec_cbuf); */
        /* puts("ai busy!\n"); */
    }
    os_sem_set(&__this->aec_sem, 0);
    os_sem_post(&__this->aec_sem);
}

static void roobo_aec_handler(void *priv)
{
    for (;;) {
        os_sem_pend(&__this->sem, 0);

#ifdef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
#if ROOBO_DUAL_MIC_ALGORITHM
        short temp_buf[ROOBO_AEC_INPUT_SHORT_LEN * 3];
        short left_mic_buf[ROOBO_AEC_INPUT_SHORT_LEN];
        short right_mic_buf[ROOBO_AEC_INPUT_SHORT_LEN];
        short left_linein_buf[ROOBO_AEC_INPUT_SHORT_LEN];
        while ((cbuf_get_data_size(&__this->mic_cbuf) >= ROOBO_AEC_INPUT_SHORT_LEN * 6)) {
            if (0 == cbuf_read(&__this->mic_cbuf, (void *)temp_buf, sizeof(temp_buf))) {
                break;
            }
            for (u32 i = 0; i < ROOBO_AEC_INPUT_SHORT_LEN; ++i) {
                left_mic_buf[i]	= temp_buf[3 * i] ;
                right_mic_buf[i]	= temp_buf[3 * i + 1] ;
                left_linein_buf[i]	= temp_buf[3 * i + 2];
            }
            roobo_audiolib_handler(left_mic_buf, right_mic_buf, left_linein_buf, NULL, ROOBO_AEC_INPUT_SHORT_LEN);
        }
#else
        short temp_buf[ROOBO_AEC_INPUT_SHORT_LEN * 2];
        short left_mic_buf[ROOBO_AEC_INPUT_SHORT_LEN];
        short left_linein_buf[ROOBO_AEC_INPUT_SHORT_LEN];
        while ((cbuf_get_data_size(&__this->mic_cbuf) >= ROOBO_AEC_INPUT_SHORT_LEN * 4)) {
            if (0 == cbuf_read(&__this->mic_cbuf, (void *)temp_buf, sizeof(temp_buf))) {
                break;
            }
            for (u32 i = 0; i < ROOBO_AEC_INPUT_SHORT_LEN; ++i) {
                left_mic_buf[i]	= temp_buf[2 * i] ;
                left_linein_buf[i]	= temp_buf[2 * i + 1];
            }
            roobo_audiolib_handler(left_mic_buf, NULL, left_linein_buf, NULL, ROOBO_AEC_INPUT_SHORT_LEN);
        }
#endif // ROOBO_DUAL_MIC_ALGORITHM

#else

#if  ROOBO_DUAL_MIC_ALGORITHM
        short temp_buf[ROOBO_AEC_INPUT_SHORT_LEN * 2];
        short left_mic_buf[ROOBO_AEC_INPUT_SHORT_LEN];
        short right_mic_buf[ROOBO_AEC_INPUT_SHORT_LEN];
        short left_linein_buf[ROOBO_AEC_INPUT_SHORT_LEN];
        while ((cbuf_get_data_size(&__this->mic_cbuf) >= ROOBO_AEC_INPUT_SHORT_LEN * 4) && (cbuf_get_data_size(&__this->linein_cbuf) >= ROOBO_AEC_INPUT_SHORT_LEN * 2)) {
            if (0 == cbuf_read(&__this->mic_cbuf, (void *)temp_buf, sizeof(temp_buf))) {
                break;
            }
            if (0 == cbuf_read(&__this->linein_cbuf, (void *)left_linein_buf, sizeof(left_linein_buf))) {
                break;
            }

            for (u32 i = 0; i < ROOBO_AEC_INPUT_SHORT_LEN; ++i) {
                left_mic_buf[i]	= temp_buf[2 * i] ;
                right_mic_buf[i]	= temp_buf[2 * i + 1];
            }
            roobo_audiolib_handler(left_mic_buf, right_mic_buf, left_linein_buf, NULL, ROOBO_AEC_INPUT_SHORT_LEN);
        }
#else
        short left_mic_buf[ROOBO_AEC_INPUT_SHORT_LEN];
        short left_linein_buf[ROOBO_AEC_INPUT_SHORT_LEN];
        while ((cbuf_get_data_size(&__this->mic_cbuf) >= ROOBO_AEC_INPUT_SHORT_LEN * 2)
               && (cbuf_get_data_size(&__this->linein_cbuf) >= ROOBO_AEC_INPUT_SHORT_LEN * 2)) {
            if (0 == cbuf_read(&__this->mic_cbuf, (void *)left_mic_buf, sizeof(left_mic_buf))) {
                break;
            }
            if (0 == cbuf_read(&__this->linein_cbuf, (void *)left_linein_buf, sizeof(left_linein_buf))) {
                break;
            }
            roobo_audiolib_handler(left_mic_buf, NULL, left_linein_buf, NULL, ROOBO_AEC_INPUT_SHORT_LEN);
        }
#endif// ROOBO_DUAL_MIC_ALGORITHM
#endif
    }
}

static void roobo_asr_handler(void *priv)
{
    short read_buf[ROOBO_ASR_INPUT_SHORT_LEN]; //buf大小与 frame_size 相等
    int result = 0;
    char text[32];
    float score = 0;

    for (;;) {
        os_sem_pend(&__this->asr_sem, 0);

        while (cbuf_get_data_size(&__this->asr_cbuf) >= ROOBO_ASR_INPUT_SHORT_LEN * 2) {
            if (0 == cbuf_read(&__this->asr_cbuf, (void *)read_buf, sizeof(read_buf))) {
                break;
            }

            result = roobo_asr_process(read_buf, ROOBO_ASR_INPUT_SHORT_LEN, text, &score);
            if (result == 1) {
                printf("2asr ok score = %f, text = %s", score, text);
                wtk_handler(text);
            }
        }
    }
}

static bool roobo_audiolib_init(void)
{
    int ret = 0;

    do {
        ret = roobo_aec_init(__this->fig_aec_buf, NULL);
        if (ret) {
            printf("init aec failed\n");
            ret = -2;
            break;
        } else {
            printf("aec init ok\n");
        }

        ret = roobo_asr_init(NULL);
        if (ret) {
            printf("init asr failed\n");
            ret = -3;
            break;
        } else {
            printf("init asr ok\n");
        }
    } while (0);

    return ret;
}

int aisp_open(u16 sample_rate)
{
    if (roobo_audiolib_init()) {
        printf("roobo_audiolib init failed\n");
        return -1;
    }

    os_sem_create(&__this->sem, 0);
    os_sem_create(&__this->asr_sem, 0);
    cbuf_init(&__this->asr_cbuf, __this->asr_buf, sizeof(__this->asr_buf));

    os_sem_create(&__this->aec_sem, 0);
    cbuf_init(&__this->aec_cbuf, __this->aec_buf, sizeof(__this->aec_buf));

    thread_fork("roobo_asr_handler", 3, 1024, 0, NULL, roobo_asr_handler, NULL);
#if ROOBO_DUAL_MIC_ALGORITHM
    thread_fork("roobo_aec_handler", 3, 5632, 0, NULL, roobo_aec_handler, NULL);
#else
    thread_fork("roobo_aec_handler", 3, 5120, 0, NULL, roobo_aec_handler, NULL);
#endif

#ifndef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
    linein_record_start(sample_rate);
#endif
    mic_record_start(sample_rate);

    __this->sample_rate = sample_rate;
    __this->run_flag = 1;

    return 0;
}

void aisp_suspend(void)
{
    union audio_req req = {0};

    __this->run_flag = 0;

    os_sem_post(&__this->aec_sem);

#ifndef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
    //关闭enc_server
    if (__this->linein_enc) {
        req.enc.cmd = AUDIO_ENC_CLOSE;
        server_request(__this->linein_enc, AUDIO_REQ_ENC, &req);
        server_close(__this->linein_enc);
        __this->linein_enc = NULL;
    }
    cbuf_clear(&__this->linein_cbuf);
#endif
    //关闭enc_server
    if (__this->mic_enc) {
        req.enc.cmd = AUDIO_ENC_CLOSE;
        server_request(__this->mic_enc, AUDIO_REQ_ENC, &req);
        server_close(__this->mic_enc);
        __this->mic_enc = NULL;
    }

    cbuf_clear(&__this->mic_cbuf);
    cbuf_clear(&__this->aec_cbuf);
    cbuf_clear(&__this->asr_cbuf);
}

void aisp_resume(void)
{
    cbuf_clear(&__this->mic_cbuf);
    cbuf_clear(&__this->aec_cbuf);
    cbuf_clear(&__this->asr_cbuf);

    os_sem_set(&__this->sem, 0);
    os_sem_set(&__this->asr_sem, 0);
    os_sem_set(&__this->aec_sem, 0);

#ifndef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
    cbuf_clear(&__this->linein_cbuf);
    linein_record_start(__this->sample_rate);
#endif
    mic_record_start(__this->sample_rate);

    __this->run_flag = 1;
}

u32 asr_read_input(u8 *buf, u32 len)
{
    u32 rlen = 0;

    do {
        if (!__this->run_flag) {
            return 0;
        }
        rlen = cbuf_read(&__this->aec_cbuf, buf, len);
        if (rlen == len) {
            break;
        }
        os_sem_pend(&__this->aec_sem, 0);
    } while (!rlen);

    return rlen;
}

void *get_asr_read_input_cb(void)
{
    if (!__this->run_flag) {
        return NULL;
    }

    os_sem_set(&__this->aec_sem, 0);
    cbuf_clear(&__this->aec_cbuf);
    return (void *)&asr_read_input;
}

#endif

