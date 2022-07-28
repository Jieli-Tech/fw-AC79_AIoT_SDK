#include "app_config.h"
#include "system/includes.h"
#include "device/device.h"
#include "lcd_drive.h"
#include "os/os_api.h"
#include "asm/includes.h"
#include "system/timer.h"
#include "sys_common.h"
#include "system/includes.h"
#include "simple_avi_unpkg.h"
#include "server/server_core.h"
#include "server/audio_server.h"
#include "server/video_dec_server.h"
#include "lcd_te_driver.h"
#include "lcd_config.h"
#include "yuv_soft_scalling.h"//YUV420p_Soft_Scaling
#include "storage_device.h"

//依赖文件 jpeg2yuv.c simple_avi_unpkg.c

#ifdef USE_SD_PLAY_AVI_DEMO //测试OK

#define AVI_ENTER_CRITICAL() \
    spin_lock(&avi->lock)


#define AVI_EXIT_CRITICAL() \
    spin_unlock(&avi->lock)

extern int jpeg2yuv_open(void);
extern void jpeg2yuv_yuv_callback_register(void (*cb)(u8 *data, u32 len, int width, int height));
extern int jpeg2yuv_jpeg_frame_write(u8 *buf, u32 len);
extern void jpeg2yuv_close(void);

struct avi_avi {
    int jpg_num;        //需要播放的JPG数量
    int jpg_run_num;
    int audio_num;
    int audio_run_num;
    int jpg_time_id;
    int audio_time_id;
    int play_time;
    u8 avi_play_status;
    char *jpg_buf;
    char *audio_buf;
    FILE *fd;
    OS_SEM audio_run;
    OS_SEM jpg_run;

    struct server *dec_server;  //解码服务
    cbuffer_t cbuf;        //音频数据cycle buffer
    char *data_cbuf;             //音频数据buffer
    OS_SEM rd_sem;               //写音频数据信号量
    u8 dec_volume;              //播放音量
    u8 dec_status;
    u8 audio_play_size;
    char *yuv_buf;
    spinlock_t lock;	/*!< cbuffer硬件锁 */
    u8 dec_ready_flag;
};

/*static struct avi_avi avi_play_handler;*/

#define AVI_PATH 	CONFIG_ROOT_PATH"bad.avi"

#define IDX_00DC   	ntohl(0x30306463)
#define IDX_01WB   	ntohl(0x30317762)
#define IDX_00WB   	ntohl(0x30307762)
#define DEC_JPG_LEN     50*1024

static void Calculation_frame(void)
{
    static u32 tstart = 0, tdiff = 0;
    static u8 fps_cnt = 0;

    fps_cnt++ ;

    if (!tstart) {
        tstart = timer_get_ms();
    } else {
        tdiff = timer_get_ms() - tstart;

        if (tdiff >= 1000) {
            printf("\n [data_to_te]fps_count = %d\n", fps_cnt *  1000 / tdiff);
            tstart = 0;
            fps_cnt = 0;
        }
    }
}
/*************************音频播放部分*********************************/
#define CBUF_LEN      12 * 1024  //文件系统写数据比较快网络会被干扰所以这个buf可以小一点
#define DEC_AUDIO_LEN      1024

static int open_audio_dec(void *priv);

/****解码器会从CBUF里面取数取到数据就会有声音*****/
static int audio_vfs_fread(void *priv, void *data, u32 len)
{
    int rlen = 0;
    struct avi_avi *avi = (struct avi_avi *)priv;
    if (cbuf_get_data_size(&avi->cbuf)) {
        rlen = cbuf_read(&avi->cbuf, data, len);
    } else {
        rlen = 0;
    }
    return len;
}

static int audio_vfs_flen(void *priv)
{
    struct avi_avi *avi = (struct avi_avi *)priv;

    return avi->audio_play_size; //一定要告知解码器MP3格式的歌曲总长度 不然会报格式错误 导致播不出
}

static const struct audio_vfs_ops audio_vfs_ops = {
    .fread  = audio_vfs_fread,
    .flen   = audio_vfs_flen,
};

static int open_audio_dec(void *priv)
{
    union audio_req req = {0};
    struct avi_avi *avi = (struct avi_avi *)priv;

    memset(&req, 0, sizeof(union audio_req));

    avi->dec_volume = 40;

    req.dec.output_buf      = NULL;
    req.dec.volume          = avi->dec_volume;
    req.dec.output_buf_len  = CBUF_LEN;
    req.dec.channel         = 1;
    req.dec.sample_rate     = 16000;
    req.dec.priority        = 1;
    req.dec.orig_sr         = 0;
    req.dec.vfs_ops         = &audio_vfs_ops;
    req.dec.file            = (FILE *)avi;
    req.dec.dec_type 		= "pcm";
    req.dec.sample_source   = "dac";


    //打开解码器
    if (server_request(avi->dec_server, AUDIO_REQ_DEC, &req) != 0) {
        return -1;
    }

    //开始解码
    req.dec.cmd = AUDIO_DEC_START;

    if (server_request(avi->dec_server, AUDIO_REQ_DEC, &req) != 0) {
        return -1;
    }

    return 0;
}

static int close_audio_dec(void *priv)
{
    union audio_req req = {0};
    struct avi_avi *avi = (struct avi_avi *)priv;

    req.dec.cmd = AUDIO_DEC_STOP;

    if (avi->dec_server) {

        //关闭解码器
        printf("stop dec.\n");
        server_request(avi->dec_server, AUDIO_REQ_DEC, &req);

        //关闭解码服务
        printf("close audio_server.\n");
        server_close(avi->dec_server);
        avi->dec_server = NULL;
    }


    return 0;
}
/*===========================endd=====================================*/

static void dec_jpg_to_yuv(void *priv, char *buf, int buf_len)
{
    struct avi_avi *avi = (struct avi_avi *)priv;
    struct jpeg_image_info info = {0};
    struct jpeg_decode_req req = {0};

    int pix;
    int ytype;
    int yuv_len;
    char *cy, *cb, *cr;

    info.input.data.buf = buf;
    info.input.data.len = buf_len;

    if (jpeg_decode_image_info(&info)) { //获取JPEG图片信息
        printf("jpeg_decode_image_info err\n");
        /*break;*/
    } else {
        switch (info.sample_fmt) {
        case JPG_SAMP_FMT_YUV444:
            ytype = 1;
            break;//444

        case JPG_SAMP_FMT_YUV420:
            ytype = 4;
            break;//420

        default:
            ytype = 2;
        }

        pix = info.width * info.height;
        yuv_len = pix + pix / ytype * 2;

        if (!avi->yuv_buf) {
            avi->yuv_buf = malloc(yuv_len);

            if (!avi->yuv_buf) {
                printf("avi->yuv_buf malloc err len : %d , width : %d , height : %d \n", yuv_len, info.width, info.height);
                /*break;*/
            }
        }

        /*printf("width : %d , height : %d \n", info.width, info.height);*/

        cy = avi->yuv_buf;
        cb = cy + pix;
        cr = cb + pix / ytype;

        req.input_type = JPEG_INPUT_TYPE_DATA;
        req.input.data.buf = info.input.data.buf;
        req.input.data.len = info.input.data.len;
        req.buf_y = cy;
        req.buf_u = cb;
        req.buf_v = cr;
        req.buf_width = info.width;
        req.buf_height = info.height;
        req.out_width = info.width;
        req.out_height = info.height;
        req.output_type = JPEG_DECODE_TYPE_DEFAULT;
        req.bits_mode = BITS_MODE_UNCACHE;
        req.dec_query_mode = TRUE;

        int ret = jpeg_decode_one_image(&req);//JPEG转YUV解码

        if (ret) {
            printf("jpeg decode err !!\n");
            /*break;*/
        }

#if HORIZONTAL_SCREEN
        YUV420p_Soft_Scaling(avi->yuv_buf, NULL, info.width, info.height, 320, 240);
#else
        YUV420p_Soft_Scaling(avi->yuv_buf, NULL, info.width, info.height, 240, 320);
#endif
        lcd_show_frame(avi->yuv_buf, (320 * 240 * 2));//240*320*2=153600
        Calculation_frame();
    }
}

static void audio_play(void *priv)
{
    struct avi_avi *avi = (struct avi_avi *)priv;
    os_sem_post(&avi->audio_run);
}
static void avi_audio_task_inti(void *priv)
{
    struct avi_avi *avi = (struct avi_avi *)priv;
    int w_len;
    int r_audio_len;

    //初始化cycle buffer
    avi->data_cbuf = (char *)calloc(1, CBUF_LEN);

    if (avi->data_cbuf == NULL) {
        printf("[error]>>>>>avi->data_cbuf= NULL");
    }

    cbuf_init(&avi->cbuf, avi->data_cbuf, CBUF_LEN);
    os_sem_create(&avi->rd_sem, 0);

    avi->audio_buf = (char *)calloc(1, DEC_AUDIO_LEN);//接收音频缓存

    if (avi->audio_buf == NULL) {
        printf("[error]>>>>>avi->audio_buf = NULL");
    }

    avi->audio_num = avi_get_audio_chunk_num(avi->fd, 1);

    avi->dec_server = server_open("audio_server", "dec");

    if (!avi->dec_server) {
        puts("audio server open fail.\n");
    } else {
        puts("open success");
    }

    while (1) {
        r_audio_len = avi_audio_get_frame(avi->fd, ++avi->audio_run_num, (u8 *)avi->audio_buf, DEC_AUDIO_LEN, 1); //全回放功能获取帧
        if (r_audio_len) {
            w_len = cbuf_write(&avi->cbuf, avi->audio_buf, r_audio_len);
            os_sem_set(&avi->rd_sem, 0);
            os_sem_post(&avi->rd_sem);
            avi->audio_play_size = w_len;

            if (!w_len) {
                --avi->audio_run_num;  //防止写太快填充满CbufHOY
                os_time_dly(3);
            }

            if (!avi->dec_status) {
                avi->dec_status = 1;
                open_audio_dec(avi);
            }
            os_time_dly(1);
        }
        if (avi->audio_run_num >= avi->audio_num) {
            printf(">>>>>>>close_audio_dec");
            close_audio_dec(avi);
            free(avi->data_cbuf);
            free(avi->audio_buf);
            avi->audio_buf = NULL;
            os_sem_del(&avi->rd_sem, 0);
            sys_timer_del(avi->audio_time_id);
            break;
        }
    }
}

static void jpg_play(void *priv)
{
    struct avi_avi *avi = (struct avi_avi *)priv;
    os_sem_post(&avi->jpg_run);
}
static void avi_jpg_task_inti(void *priv)
{
    struct avi_avi *avi = (struct avi_avi *)priv;

    avi->jpg_buf = (char *)calloc(1, DEC_JPG_LEN); //接收JPG缓存

    if (avi->jpg_buf == NULL) {
        printf("[error]>>>>>avi->jpg_buf = NULL");
    }

    avi->jpg_num = avi_get_video_chunk_num(avi->fd, 1);

    os_sem_create(&avi->jpg_run, 0);
    avi->jpg_time_id = sys_timer_add(avi, jpg_play, (avi->play_time * 1000) / avi->jpg_num);

    while (1) {
        os_sem_pend(&avi->jpg_run, 0);
        int r_jpg_len = avi_video_get_frame(avi->fd, ++avi->jpg_run_num, (u8 *)avi->jpg_buf, DEC_JPG_LEN, 1); //全回放功能获取帧

        if (*(u32 *)avi->jpg_buf == IDX_00DC || *(u32 *)avi->jpg_buf == IDX_01WB || *(u32 *)avi->jpg_buf == IDX_00WB) {
            r_jpg_len -= 8;
            avi->jpg_buf += 8;
        }

        if (r_jpg_len > 0) {
            dec_jpg_to_yuv(avi, avi->jpg_buf, r_jpg_len);
        } else {
            printf(">>>>>>>>>>jpg_play_close");
            sys_timer_del(avi->jpg_time_id);
            free(avi->jpg_buf);
            avi->jpg_buf = NULL;
            free(avi->yuv_buf);
            os_sem_del(&avi->jpg_run, 0);
            break;
        }
    }
}

static void avi_play_test(void *priv)
{
    int ret;
    //1.初始化屏
    user_ui_lcd_init();
    set_lcd_show_data_mode(camera);

    //8.获取播放帧数 音频包数
    struct avi_avi *avi = (struct avi_avi *)calloc(1, sizeof(struct avi_avi));

    if (avi == NULL) {
        printf("[error]>>>>>avi= NULL");
    }

    while (!storage_device_ready()) { //等待sd文件系统挂载完成
        os_time_dly(2);
        printf(">>>>>>>>>>>>wait_sd_ok");
    }

    avi->fd = fopen(AVI_PATH, "r");

    if (avi->fd == NULL) {
        printf("[error]>>>>>avi_fd= NULL");
    }

    ret = avi_playback_unpkg_init(avi->fd, 1); //解码初始化,最多10分钟视频

    if (ret) {
        printf("avi_playback_unpkg_init err!!!\n");
    }

    avi->play_time = avi_get_file_time(avi->fd, 1);

    thread_fork("avi_audio_task_inti", 10, 1024, 0, 0, avi_audio_task_inti, avi);
    thread_fork("avi_jpg_task_inti", 11, 1024, 0, 0, avi_jpg_task_inti, avi);

    //9.定时播放视频帧和音频帧

    while (1) {
        os_time_dly(100);
        if (avi->jpg_buf == NULL && avi->audio_buf == NULL) {
            avi_unpkg_exit(avi->fd, 1);
            fclose(avi->fd);
            free(avi);
            printf(">>>>>>>>>>>>free_avi");
            break;
        }
    }
}

static int avi_test_task_init(void)
{
    puts("avi_test_task_init\n\n");
    return thread_fork("avi_play_test", 11, 1024, 0, 0, avi_play_test, NULL);
}
late_initcall(avi_test_task_init);

#endif
