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
#include "fn_asr.h"

extern u32 timer_get_ms(void);

__attribute__((always_inline))
static uint32_t Stereo2Mono(void *audio_buf, uint32_t len, int LR)
{
    if (!audio_buf || !len || len % 4) {
        printf("%s arg err\n", __func__);
        return 0;
    }
    int16_t *buf = audio_buf;
    u32 i = 0;
    LR = LR ? 1 : 0;

    for (i = 0; i < len / 4; i++) {
        buf[i] = buf[i * 2 + LR];
    }
    return len / 2;
}

static void AudioMicTask(void *p)
{


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



#define N 480
    uint16_t PcmBuf1[N];

    int i, rs;
    const char *text;
    float score;

    FILE *fp;
    u32 total_time_cnt = 0, timer_cnt, RealLen, total_len = 0;

    while (!storage_device_ready()) {
        os_time_dly(10);
        puts("wait storage_device_ready ");
    }

    fp = fopen(CONFIG_ROOT_PATH"chan1-1-ref.wav", "r");
    total_len += 44;
    if (!fp) {
        printf("open file fail \n");
        return;
    }

    Wanson_ASR_Init();
    printf("Wanson_ASR_Init=0x%x \n", Wanson_ASR_Init);

    while (1) {
        fseek(fp, total_len, SEEK_SET);
        RealLen = fread(fp, PcmBuf1, N * 2);
        if (RealLen == N * 2) {
            total_len += RealLen;
            timer_cnt = timer_get_ms();
            JL_TIMER4->CNT = 0;
            rs = Wanson_ASR_Recog((short *)PcmBuf1, N, &text, &score); //320M双核,消耗142M
            total_time_cnt += JL_TIMER4->CNT;
//            total_time_cnt += (timer_get_ms()- timer_cnt);
            if (rs == 1) {
                printf("wAudioMicTask_ %s\n", text);   //识别结果打印
                put_buf(text, 8);
            } else {
                putchar('#');
            }
        } else {
            printf("wAudioMicTask_ fread end total_len = %d\n", total_len);
            fclose(fp);
            Wanson_ASR_Release();
            break;
        }
    }

    printf("total_time_cnt = %d  \r\n", total_time_cnt);

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
