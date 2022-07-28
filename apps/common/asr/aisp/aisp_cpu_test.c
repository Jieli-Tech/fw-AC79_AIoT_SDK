#include "server/audio_server.h"
#include "server/server_core.h"
#include "system/init.h"
#include "generic/circular_buf.h"
#include "json_c/json_tokener.h"
#include "storage_device.h"
#include "fs/fs.h"
#include "os/os_api.h"
#include "aispeech_alg.h"
#include "event.h"
#include "app_config.h"

extern u32 timer_get_ms(void);

static int wtk_handler(void *pvUsrData, int iIdx, char *pcJson)
{
    printf("Channel %d wakeuped; Details: %s\n", iIdx, pcJson);
    return 0;
}

static int aec_handler(void *pvUsrData, s8 *pcData, s32 iLen, s8 iStatus)
{
    //aisp_write2sd(pcData,iLen);
    //printf("asr: \n");
    return 0;
}
typedef struct dui_auth_info {
    char *productKey;
    char *productId;
    char *ProductSecret;
} dui_auth_info_t;
static void AudioMicTask(void *p)
{
    while (!storage_device_ready()) {
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
    auth_info.productKey = 请向思必驰购买lisence;
    auth_info.productId = 请向思必驰购买lisence;
    auth_info.ProductSecret = 请向思必驰购买lisence;
    extern int app_dui_auth_second(dui_auth_info_t *auth_info, u8 mark);
    if (0 != app_dui_auth_second(&auth_info, 0)) {
        printf("dui auth fail, please contact aispeech!!!\n");
        return;
    }
    puts("app_dui_auth_second ok\n");


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
#if 1
    fp = fopen(CONFIG_ROOT_PATH"chan3-1-ref.wav", "r");
    total_len += 44;
#else
    fp = fopen(CONFIG_ROOT_PATH"test_3.pcm", "r");
#endif

    if (!fp) {
        printf("open file fail.\n");
        return;
    }

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

    while (1) {
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

    thread_fork("AudioMicTask", 25, 4000, 0, 0, AudioMicTask, NULL);
    while (1) {
        msleep(1000);
    }
}
late_initcall(AudioMicTask_init);




void aisp_open(void) {}
void aisp_resume(void) {}
void aisp_suspend(void) {}

