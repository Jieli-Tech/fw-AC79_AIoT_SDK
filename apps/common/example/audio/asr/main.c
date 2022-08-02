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
#include "event/device_event.h"
#include "system/wait.h"
#include "system/app_core.h"
#include "syscfg/syscfg_id.h"
#include "storage_device.h"


#if (defined CONFIG_ASR_ALGORITHM_ENABLE)

#define AISP_DUAL_MIC_ALGORITHM    0   //0选择单mic算法

//使用差分mic代替linein回采
#define CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN

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
extern void asr_resume(void);

#if AISP_DUAL_MIC_ALGORITHM
#define ONCE_SR_POINTS	160
#define AISP_BUF_SIZE	(12 * ONCE_SR_POINTS)	//跑不过来时适当加大倍数
#else
#define ONCE_SR_POINTS	512
#define AISP_BUF_SIZE	(2  * ONCE_SR_POINTS)	//跑不过来时适当加大倍数
#endif

//打断唤醒运行标志
u8 asr_wakeup_flag;

//cpu测试标志
u8 asr_cpu_test_flag;

int cpu_test_pid;

typedef struct {
    int len;
    char name[64];
} FILE_INFO;

static struct {
    int pid;
    u16 sample_rate;
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
    //2.获取第三方算法所需堆空间大小
    u32 memPoolLen = LDEEW_memSize();  //获取算法需要的heap大小
    u32 mic_len, linein_len;

    printf("aisp_main run 0x%x,%s\r\n", (u32)LDEEW_version(), LDEEW_version());
    printf("memPoolLen is:%d\n", memPoolLen);

    //3.获取第三方授权信息,并且授权启动第三方算法程序
    dui_auth_info_t auth_info;
    /* 此处信息请根据dui信息修改 https://www.duiopen.com/ */
    auth_info.productKey = "58f1aeeb54fadab27a6ce70fd222ec46";
    auth_info.productId = "279594353";
    auth_info.ProductSecret = "89249b0fb48d7c12454a079fc97aee72";
    extern int app_dui_auth_second(dui_auth_info_t *auth_info, u8 mark);
    if (0 != app_dui_auth_second(&auth_info, 0)) {
        printf("dui auth fail, please contact aispeech!!!\n");
        return;
    }
    puts("app_dui_auth_second ok\n");

    //4.申请算法运行所需内存
    void *pcMemPool = calloc(1, memPoolLen);  //申请内存
    if (NULL == pcMemPool) {
        return;
    }

    /* start engine and pass auth cfg*/
    //pstLfespdEng = LDEEW_RUN(pcMemPool,memPoolLen,auth_string, "words=xiao ting xiao ting,xiao ti xiao ti,xiao ding xiao ding,xiao di xiao di,bo fang yin yue,zan ting bo fang,shang yi shou,xia yi shou,zeng da yin liang,jian xiao yin liang,jie ting dian hua,gua duan dian hua,que ren que ren,qv xiao qv xiao,bi xin bi xin;thresh=0.42,0.29,0.26,0.42,0.26,0.24,0.59,0.49,0.30,0.22,0.24,0.26,0.23,0.30,0.28;" ,wtk_handler, asr_handler);

    //5.运行第三方算法程序,处理数据后通过回调进行通知
    pstLfespdEng = LDEEW_RUN(pcMemPool, memPoolLen, "words=xiao ai tong xue,da sheng yi dian,xiao sheng yi dian,zan ting bo fang,xia yi shou;thresh=0.60,0.32,0.32,0.33,0.33;", wtk_handler, aec_handler);
    if (NULL == pstLfespdEng || pstLfespdEng == (void *)0xffffffff) {
        printf("LDEEW_Start auth fail \n");
        free(pcMemPool);
        return;
    }
    printf("LDEEW_Start auth OK \n");

    __this->auth_flag = 1;

    //6.打开mic，获取语音数据
    asr_resume();

    //7.while环喂数据给第三方算法程序
    while (asr_wakeup_flag) {
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
#if 0	//如果回采通道采用AD1需要打开此宏
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

        //8.此处是将整理好的数据喂给第三方算法程序
        LDEEW_feed(pstLfespdEng, (char *)buf, sizeof(buf));	//单MIC时TODO

        time_cnt += timer_get_ms() - time;
        if (++cnt == 100) {
            /* printf("aec time :%d \n", time_cnt); */
            time_cnt = cnt = 0;
        }
    }

    free(pcMemPool);
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


void *get_asr_read_input_cb(void)
{
    if (!__this->run_flag) {
        return NULL;
    }

    return NULL;
}

//1.打开audio服务，初始化cbuf，注册audio服务处理函数，信号量创建，创建aisp_task线程
int asr_open(u16 sample_rate)
{
    log_i("asr_wakeup_main\n");
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

#if AISP_DUAL_MIC_ALGORITHM
    return thread_fork("aisp", 3, 3328, 0, &__this->pid, aisp_task, __this);
#else
    return thread_fork("aisp", 3, 3840, 0, &__this->pid, aisp_task, __this);
#endif
}

void asr_suspend(void)
{
    union audio_req req = {0};

    if (!__this->auth_flag || !__this->run_flag) {
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

void asr_uninit(void)
{
    os_sem_post(&__this->sem);

    server_close(__this->mic_enc);
    __this->mic_enc = NULL;
#ifndef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
    server_close(__this->linein_enc);
    __this->linein_enc = NULL;
#endif
}

void asr_resume(void)
{
    union audio_req req = {0};

    if (!__this->auth_flag || __this->run_flag) {
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


















//测试cpu占用率
static void AudioMicTask(void *p)
{
    while (!storage_device_ready()) {
        if (!asr_wakeup_flag) {
            return ;
        }
        os_time_dly(10);
        puts("wait storage_device_ready ");
    }

    FILE *fp;
    u32  total_len = 0, len, memPoolLen;
    char buf[1024 * 3] = {0};
    char *pcMemPool;
    LDEEW_ENGINE_S *pstLfespdEng;
    memPoolLen = LDEEW_memSize();  //获取算法需要的heap大小

    printf("aisp_main run 0x%x,%s\\n", (u32)LDEEW_version(), LDEEW_version());
    printf("memPoolLen is:%d\n", memPoolLen);

    dui_auth_info_t auth_info;
    /*
    此处信息请根据dui信息修改,参考 思必驰单双mic打断唤醒算法选择_思必驰DUI平台账号创建.doc
    注册网址：https://www.duiopen.com/
    快速接入详细流程地址：https://cloud.aispeech.com/docs/2044
    */
    auth_info.productKey = "58f1aeeb54fadab27a6ce70fd222ec46";//请向思必驰购买lisence;
    auth_info.productId = "279594353";//请向思必驰购买lisence;
    auth_info.ProductSecret = "89249b0fb48d7c12454a079fc97aee72";//请向思必驰购买lisence;
    extern int app_dui_auth_second(dui_auth_info_t *auth_info, u8 mark);
    if (0 != app_dui_auth_second(&auth_info, 0)) {
        printf("dui auth fail, please contact aispeech!!!\n");
        return;
    }
    puts("app_dui_auth_second ok\n");

#if 1
    fp = fopen(CONFIG_ROOT_PATH"chan3-1.wav", "r");
    total_len += 44;
#else
    fp = fopen(CONFIG_ROOT_PATH"test_3.pcm", "r");
#endif

    if (!fp) {
        printf("open file fail.\n");
        return;
    }

    pcMemPool = calloc(1, memPoolLen);  //申请内存
    if (NULL == pcMemPool) {
        printf("calloc fail  \n");
        return;
    }
    /* start engine and pass auth cfg*/
    pstLfespdEng = LDEEW_RUN(pcMemPool, memPoolLen, "words=xiao ai tong xue,da sheng yi dian,xiao sheng yi dian,zan ting bo fang,xia yi shou;thresh=0.60,0.32,0.32,0.33,0.33;", wtk_handler, aec_handler);
    if (NULL == pstLfespdEng || pstLfespdEng == (void *)0xffffffff) {
        puts("LDEEW_Start auth fail \n");
        free(pcMemPool);
        return;
    }
    msleep(1000);
    puts("LDEEW_Start auth OK! \n");
    /*read simulation file*/

    JL_TIMER3->CON = BIT(14);
    JL_TIMER3->PRD = -1;
    JL_TIMER3->CON = BIT(0); //sys clk 320, 1->0.018747us

    JL_TIMER4->CON = BIT(14);
    JL_TIMER4->PRD = -1;
    JL_TIMER4->CON = BIT(0); //sys clk 320, 1->0.018747us


//    JL_TIMER5->CON = BIT(14);
//    JL_TIMER5->PRD = -1;
//    JL_TIMER5->CON = BIT(0); //sys clk 320, 1->0.018747us

//    u32 now_time_cnt,start_time_cnt=timer_get_ms();
//    JL_TIMER4->CNT=0;
//    while(1)
//    {
//        if(timer_get_ms()-start_time_cnt>1000*60)
//        {
//             now_time_cnt = JL_TIMER4->CNT;
//             printf("now_time_cnt = 0x%x",now_time_cnt);
//             while(1);
//        }
//    }

    u32 time_sd_total, time_sd;
    u32 time_math_total, time_math;
    u32 ReadCnt ;

AGAIN:
    total_len = time_sd_total = time_sd = time_math_total = time_math = ReadCnt = 0;

    while (asr_wakeup_flag) {
        time_sd = timer_get_ms();
        JL_TIMER3->CNT = 0;
        fseek(fp, total_len, SEEK_SET);
        len = fread(buf, 1, sizeof(buf), fp);
//        time_sd_total += (timer_get_ms() - time_sd);
        time_sd_total += JL_TIMER3->CNT;
        if (len == sizeof(buf)) {
            total_len += len;
            ReadCnt++;

            JL_TIMER4->CNT = 0;
//            time_math = timer_get_ms();
            LDEEW_feed(pstLfespdEng, buf, len);
            time_math_total += JL_TIMER4->CNT;
//            time_math_total += (timer_get_ms() - time_math);

        } else {
            printf("total_len=%d, ReadCnt=%d, time_math_total= %u ,time_sd_total=%u\r\n", total_len, ReadCnt, time_math_total, time_sd_total);//双核消耗 205M CPU,单核255M,计算方法 320*time_math_total*0.018747us/1000000/文件总时间秒
            break;
//            goto AGAIN;
        }
    }
    free(pcMemPool);

//    void cal_AISP(void);
//    cal_AISP();
}
static void A222(void *p)
{
    while (1);
}
static void AudioMicTask_init(void)
{
//    thread_fork("A222", 26, 2000, 0, 0, A222, NULL);msleep(20);
    log_i("asr_cpu_main\n");

    thread_fork("AudioMicTask", 25, 4000, 0, &cpu_test_pid, AudioMicTask, NULL);
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










/*****************************************************/
/*********************APP框架*************************/
/***********与本例子无关，只供演示功能使用************/
/*****************************************************/

static int ai_speaker_key_click(struct key_event *key)
{
    int ret = true;

    switch (key->value) {
    case KEY_OK:
        if (__this->run_flag == 0) {
            asr_resume();     //打开MIC
        } else {
            asr_suspend();    //关闭MIC
        }
        break;
    default:
        ret = false;
        break;
    }

    return ret;
}

static int ai_speaker_key_long(struct key_event *key)
{
    int ret = true;
    switch (key->value) {
    case KEY_OK:
        break;
    case KEY_VOLUME_DEC:
        break;
    case KEY_VOLUME_INC:
        break;
    case KEY_MODE:
        //长按MODE切换 测试cpu占用率(需有对应文件) 与 打断唤醒例子
        if (!asr_cpu_test_flag) {
            asr_wakeup_flag = 0;
            thread_kill(&__this->pid, KILL_WAIT);
            asr_suspend();
            asr_uninit();
            asr_wakeup_flag = 1;
            asr_cpu_test_flag = 1;
            AudioMicTask_init();
        } else {
            asr_wakeup_flag = 0;
            thread_kill(&cpu_test_pid, KILL_WAIT);	//这句是等待线程完全退出再继续跑下去，不是用作杀死线程
            asr_wakeup_flag = 1;
            asr_open(CONFIG_AUDIO_RECORDER_SAMPLERATE);
            asr_cpu_test_flag = 0;

        }


        break;
    default:
        ret = false;
        break;
    }
    return ret;
}

static int ai_speaker_key_event_handler(struct key_event *key)
{
    switch (key->action) {
    case KEY_EVENT_CLICK:
        return ai_speaker_key_click(key);
    case KEY_EVENT_LONG:
        return ai_speaker_key_long(key);
    default:
        break;
    }

    return true;
}

static int ai_speaker_event_handler(struct application *app, struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        return ai_speaker_key_event_handler((struct key_event *)event->payload);
    default:
        return false;
    }
}

static int ai_speaker_state_machine(struct application *app, enum app_state state, struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        break;
    case APP_STA_START:
        asr_wakeup_flag = 1;
        asr_cpu_test_flag = 0;	//默认进入打断唤醒模式
        asr_open(CONFIG_AUDIO_RECORDER_SAMPLERATE);
        break;
    case APP_STA_PAUSE:
        break;
    case APP_STA_RESUME:
        break;
    case APP_STA_STOP:
        if (!asr_cpu_test_flag) {
            asr_wakeup_flag = 0;
            thread_kill(&__this->pid, KILL_WAIT);
            asr_suspend();
            asr_uninit();
        } else {
            asr_wakeup_flag = 0;
            thread_kill(&cpu_test_pid, KILL_WAIT);	//这句是等待线程完全退出再继续跑下去，不是用作杀死线程
        }
        break;
    case APP_STA_DESTROY:
        break;
    }

    return 0;
}

static const struct application_operation ai_speaker_ops = {
    .state_machine  = ai_speaker_state_machine,
    .event_handler 	= ai_speaker_event_handler,
};


REGISTER_APPLICATION(ai_speaker) = {
    .name 	= "ai_speaker",
    .ops 	= &ai_speaker_ops,
    .state  = APP_STA_DESTROY,
};


#endif


