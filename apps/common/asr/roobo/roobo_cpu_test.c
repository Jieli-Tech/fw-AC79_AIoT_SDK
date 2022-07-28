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
#include "asr.h"
#include "ivPolos.h"
#include "fig_asr_api.h"
#include "fig_asr_type.h"
#include "resmapping_table.h"

extern u32 timer_get_ms(void);


#define AEC_SPEED_TEST 0
#define ASR_SPEED_TEST 1




/*每次AEC输入的数据量大小, 匹配的每个大小是short*/
#define ROOBO_AEC_INPUT_SHORT_LEN      512
#define ROOBO_ASR_INPUT_SHORT_LEN      160   /* 10ms */
//#define ROOBO_ASR_INPUT_SHORT_LEN      480   /* 30ms */


typedef struct {
    int wordId;
    void *text;
    float score;
} roobo_asr_result_t;


typedef struct {
    void *left_mic;                 /* 16bit第一路麦克风 */
    void *right_mic;                /* 16bit第二路麦克风，没有填NULL */
    void *left_aec;					/* 16bit第一路参考信号 */
    void *right_aec;				/* 16bit第二路参考信号, 没有填NULL */
    int len;                        /* 对应short类型长度, 必须= 512*sizeof(short) */
} roobo_aec_stream_t;



static struct {
    FIG_INST pFigAsr;
    char fig_aec_buf[300 * 1024];
} roobo_server;
#define __this (&roobo_server)


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
        printf("Error:  write audio fail with error code = %d\n", ret);
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

    ret = FigCreateInst(&__this->pFigAsr, nnet, graph_cmd);
    if (ret != 0) {
        printf("Error: FigCreateInst fail with error code = %d\n", ret);
        goto exit;
    }

    ret = FigStartProcess(__this->pFigAsr);
    if (ret != 0) {
        printf("Error: asr start fail with error code =  %d\n", ret);
        goto exit;
    }

    printf("roobo_asr_init ok\n");
    return ret;

exit:
    return ret;
}



static void figasr_test_handler(void *priv)
{
    int total_len = 0;
    int rlen = 0;
    short read_buf[ROOBO_ASR_INPUT_SHORT_LEN + 1] = {0};
    int index = 0;
    int result = 0;
    char text[32];
    float score = 0;


    printf("figasr addr = 0x%x\n", (u32)FigWriteAudio);


    JL_TIMER4->CON = BIT(14);
    JL_TIMER4->PRD = -1;
    JL_TIMER4->CON = BIT(0); //sys clk 240, 1->0.025us


    u32 time_math_total, time_math;
    time_math_total = time_math = 0;



#if 0
    FILE *r_fd = fopen("storage/sd0/C/xiaoaixiaoai.pcm", "r");
#else
    FILE *r_fd = fopen("storage/sd0/C/chan1-ref.wav", "r");
    fseek(r_fd, 44, SEEK_SET);
#endif

    if (!r_fd) {
        printf("open fffile.pcm fail\n");
        return;
    }

    do {
        ++index;
        int rlen = fread(r_fd, read_buf, ROOBO_ASR_INPUT_SHORT_LEN * sizeof(short));
        if (ROOBO_ASR_INPUT_SHORT_LEN * sizeof(short) != rlen) {
            printf("[%d]fread_end: %x, total = %d\n", index, rlen, total_len);
            fclose(r_fd);
            break;
        }

        if ((index % 100) == 0) {
            printf("asr read len %d\n", total_len);
        }


//            JL_TIMER4->CNT=0;
        time_math = timer_get_ms();

        result = roobo_asr_process((short *)read_buf, ROOBO_ASR_INPUT_SHORT_LEN, text, &score);

//            time_math_total += JL_TIMER4->CNT;
        time_math_total += (timer_get_ms() - time_math);






        if (result == 1) {
            printf("[%d] asr ok score = %f, text = %s", index, score, text);
        }
        total_len += rlen;

    } while (1);

    printf("figasr test %d bytes cost %d ms\n", total_len, time_math_total);
}




static void aec_speed_test_handler(void *priv)
{
    JL_TIMER5->CON = BIT(14);
    JL_TIMER5->PRD = -1;
    JL_TIMER5->CON = BIT(0); //sys clk 240, 1->0.025us


    u32 time_math_total, time_math;
    time_math_total = time_math = 0;


#define AEC_N   512*8
#define AEC_OUT_N   512*4

    short read_buf[AEC_N] = {0}; //buf大小与 frame_size 相等
    short aec_out_buf[AEC_OUT_N] = {0};

    FILE *r_fd = fopen("storage/sd0/C/jeli_aec_demo_4_input_music.pcm", "r");
    if (!r_fd) {
        printf("open jieli_aec_demo.pcm fail\n");
        return;
    }

    int total_in_len = 0;
    int total_out_len = 0;

    int index = 0;
    int out_len = 0;

    do {
        ++index;
        int rlen = fread(r_fd, read_buf, AEC_N);
        if (AEC_N != rlen) {
            printf("[%d]fread_end: %x, total = %d\n", index, rlen, total_in_len);
            fclose(r_fd);
            break;
        }

        if ((index % 100) == 0) {
            printf("[%d]aec read len %d, AEC_N = %d\n", index, rlen, AEC_N);
        }


//            JL_TIMER5->CNT=0;
        time_math = timer_get_ms();



        POLOS_AUDIOWRITE((void *)read_buf, AEC_N, aec_out_buf, &out_len);

//            time_math_total += JL_TIMER5->CNT;
        time_math_total += (timer_get_ms() - time_math);



        total_in_len += rlen;
        total_out_len += out_len * sizeof(short);
    } while (1);

    printf("[%d]audio aec done read_in len %d, out_len %d, cost time %d ms\n", index, total_in_len, total_out_len, time_math_total);
}

static void aec_speed_test(void)
{
    printf("aec version = 0x%x, %d\n", (u32)GetAECVersion, GetAECVersion());

    int len = 300 * 1024;
    char *aec_buf = malloc(len);
    if (!aec_buf) {
        printf("malloc %d bytes aec_buf failed\n", len);
        return;
    } else {
        printf("malloc %d bytes aec_buf ok %p\n", len, aec_buf);
    }
    int ret = POLOS_INIT(aec_buf);
    if (ret) {
        printf("init aec failed\n");
    } else {
        printf("aec init ok\n");
    }

    printf("create aec_sd_test start\n");
    int tret = thread_fork("aec_speed_test", 10, 1024 * 10, 0, NULL, aec_speed_test_handler, NULL);
    if (tret != 0) {
        printf("create aec_sd_test failed\n");
    } else {
        printf("create aec_sd_test ok\n");
    }

    printf("-------- asr speed test end ------------\n");
}

static void figasr_sdcard_test()
{
    int ret = roobo_asr_init(NULL);
    if (ret < 0) {
        printf("roobo asr init failed\n");
        return;
    }

    int tret = thread_fork("figasr_test", 8, 1024, 0, NULL, figasr_test_handler, NULL);
    if (tret != 0) {
        printf("create figasr_test failed\n");
    } else {
        printf("create figasr_test ok\n");
    }
}


void roobo_test()
{
    printf("run into figasr speed test  \n");
    while (!storage_device_ready()) {
        os_time_dly(10);
        puts("wait storage_device_ready ");
    }

#if  ASR_SPEED_TEST
    figasr_sdcard_test();
#endif // ASR_SPEED_TEST

#if  AEC_SPEED_TEST
    aec_speed_test();
#endif // AEC_SPEED_TEST

}
module_initcall(roobo_test);


