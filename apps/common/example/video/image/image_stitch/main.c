#include "img_stitch.h"
#include "system/includes.h"
#include "server/video_server.h"
#include "app_config.h"
#include "asm/debug.h"
#include "asm/osd.h"
#include "asm/isc.h"
#include "app_database.h"
#include "storage_device.h"
#include "server/ctp_server.h"
#include "os/os_compat.h"
#include "camera.h"
#include "net_config.h"
#include "net_server.h"
#include "server/net_server.h"
#include "stream_protocol.h"
#include "yuv_soft_scalling.h"
#include "sys_common.h"

#ifdef CONFIG_SPI_VIDEO_ENABLE
struct img_stitch_struct {
    u8 init_flog;
    OS_SEM r_sem;
    int all_w;
    u8 *fbuf;
    u8 kill;
} img_stitch_info = { 0 };

static void sdfile_save_test(char *buf, int len, char one_file, char close)
{
    static FILE *one_fd = NULL;

    if (one_file && !close) {
        if (!one_fd) {
            one_fd = fopen(CONFIG_ROOT_PATH"YUV/test/YUV_***.YUV", "w+");
        }

        if (one_fd) {
            fwrite(one_fd, buf, len);
        }
    } else if (!one_file) {
        FILE *fd = fopen(CONFIG_ROOT_PATH"YUV/test/YUV_***.YUV", "w+");

        if (fd) {
            fwrite(fd, buf, len);
            fclose(fd);
            printf("---> writing ok size = %d\n", len);
        }
    }

    if (close && one_fd) {
        fclose(one_fd);
        one_fd = NULL;
        printf("---> close file \n\n");
    }
}

static void save_outdata_to_SD(char *buf, int len)
{
    sdfile_save_test(buf, len, 0, 0);
}


void scan_pen_task(void *priv)
{
    int msg[32];
    int out_size;
    int FAST_TH = 85;
    int MATCH_TH = 2560;
    int  w = 176;
    int  h = 128;
    int w_out = w * 5;
    int h_out = h + 64;
    int Max_corner_count = 64;
    int Fast_corner_mode = 1;
    int h_off = 32;
    int offset_th = 7;
    int nms_windon_size = 7;
    int Max_filter_th_x = 5;
    int Max_filter_th_y = 3;
    int Frame_stride = 15;
    int Adjustment_enable = 1;
    int Max_off_y = 15;

    uint8_t *cat_buf0;
    uint8_t *cat_buf1;
    uint8_t *output_img;
    uint8_t *output_img1;

    cat_buf0 = malloc(w * h);
    cat_buf1 = malloc(w * h);
    output_img = malloc(w_out * h_out);
    output_img1 = malloc(w_out * h_out);

    for (int j = 0; j < h_out; j++) {
        for (int i = 0; i < w_out; i++) {
            output_img[j * w_out + i] = 0;
        }
    }

    os_sem_pend(&img_stitch_info.r_sem, 0);
    memcpy(cat_buf0, img_stitch_info.fbuf, w * h);

    os_sem_pend(&img_stitch_info.r_sem, 0);
    memcpy(cat_buf1, img_stitch_info.fbuf, w * h);

    img_stitch_init(cat_buf0, w, h, output_img, w_out, h_out, h_off, Max_corner_count, Fast_corner_mode, FAST_TH, MATCH_TH, offset_th, nms_windon_size, Max_filter_th_x, Max_filter_th_y, Frame_stride, Adjustment_enable, Max_off_y);



    while (1) {
        int start = hi_jiffies;

        printf("timeeeeeeee!!!!!=%d", hi_jiffies);
        int stitch_result = img_stitch_process(cat_buf0, cat_buf1, output_img, 0, 0);
        int start1 = hi_jiffies;
        printf("timeeeeeeee=%d", (start1 - start) * 2);
        printf("stitch_result=%d", stitch_result);

        if (stitch_result != 1 && stitch_result != 6) { //没有拼接成功 并且 BUFF没有满
            for (int i = 0; i < 1; i++) { //拼接识别尝试下一帧的拼接
                printf("i=========%d", i);
                os_sem_pend(&img_stitch_info.r_sem, 0);

                if (img_stitch_info.kill) {
                    goto exit;
                }

                memcpy(cat_buf1, img_stitch_info.fbuf, w * h);
                stitch_result = img_stitch_process(cat_buf0, cat_buf1, output_img, 0, 0); //0,0不强制拼接  1,0，强制拼接

                if (stitch_result == 1) { //拼接成功
                    printf("stitch_result_______________=%d\n", stitch_result);
                    break;
                }
            }
        }

        if (stitch_result != 1) { //遇到拼接失败
            printf("fail stitch");
            /*stitch_result= img_stitch_process(cat_buf0, cat_buf1, output_img,1,0);//0,0不强制拼接  1,0，强制拼接*/
            printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> out ");
            int result = post_process(output_img, output_img1, w_out, h_out, w / 2, h / 2); //该函数用于输出 将不水平的文字放置水平

            if (result) { //输出校准的数据
                save_outdata_to_SD(output_img1, w_out * h_out); //这里写卡会比较费时间
            } else { //数据不需要校准
                save_outdata_to_SD(output_img, w_out * h_out);
            }

            memset(output_img, 0x00, w_out * h_out);
            os_sem_pend(&img_stitch_info.r_sem, 0);
            if (img_stitch_info.kill) {
                goto exit;
            }

            memcpy(cat_buf0, img_stitch_info.fbuf, w * h);
            os_sem_pend(&img_stitch_info.r_sem, 0);

            if (img_stitch_info.kill) {
                goto exit;
            }

            memcpy(cat_buf1, img_stitch_info.fbuf, w * h);
            img_stitch_reinit(cat_buf0, w, h, output_img, w_out, h_out, h_off, Max_corner_count, Fast_corner_mode, FAST_TH, MATCH_TH, Adjustment_enable);
            continue;
        }

        if (stitch_result == 6) { //输出拼接的结果
            printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> out ");
            int result = post_process(output_img, output_img1, w_out, h_out, w / 2, h / 2);

            if (result) { //输出校准的数据
                save_outdata_to_SD(output_img1, w_out * h_out); //这里写卡会比较费时间
            } else { //数据不需要校准
                save_outdata_to_SD(output_img, w_out * h_out);
            }

            os_sem_pend(&img_stitch_info.r_sem, 0);
            memcpy(cat_buf0, img_stitch_info.fbuf, w * h);

            if (img_stitch_info.kill) {
                goto exit;
            }

            os_sem_pend(&img_stitch_info.r_sem, 0);
            memcpy(cat_buf1, img_stitch_info.fbuf, w * h);

            if (img_stitch_info.kill) {
                goto exit;
            }

            img_stitch_reinit(cat_buf0, w, h, output_img, w_out, h_out, h_off, Max_corner_count, Fast_corner_mode, FAST_TH, MATCH_TH, Adjustment_enable);
            continue;
        }

        memcpy(cat_buf0, cat_buf1, w * h);//将下一帧数据传给上一帧
        os_sem_pend(&img_stitch_info.r_sem, 0);

        if (img_stitch_info.kill) {
            goto exit;
        }

        memcpy(cat_buf1, img_stitch_info.fbuf, w * h);

        int end = hi_jiffies;
        printf("time=%d", (end - start) * 2);
    }

exit:

    img_stitch_process(cat_buf0, cat_buf1, output_img, 0, 1); //0,0不强制拼接  1,0，强制拼接
    save_outdata_to_SD(output_img, w_out * h_out);

    free(cat_buf0);
    free(cat_buf1);
    free(output_img);
    free(output_img1);
    os_sem_del(&img_stitch_info.r_sem, 0);
    img_stitch_deinit();

    img_stitch_info.kill = 0;
    img_stitch_info.init_flog = 1;

    while (1) { //等待其他任务杀此任务
        os_time_dly(100);//延时100个tick，防止看门狗
    }
}

void send_yuv_data(char *buf, u16 len)
{
    img_stitch_info.fbuf = buf;
    os_sem_post(&img_stitch_info.r_sem);
    os_taskq_post("scan_pen_task", 1, len);//完成一帧数据的接收
}

void spi_video_init(void)
{
    if (!img_stitch_info.init_flog) {
        img_stitch_info.init_flog = 1;
        int spi_recv_task_init(void);
        spi_recv_task_init();
    }
}

void scan_pen_task_init(void)
{
    os_sem_create(&img_stitch_info.r_sem, 0);
    printf(">>>>>>>*************************************>>>>>>spi_recv_task_init");

    if (img_stitch_info.init_flog == 1) {
        img_stitch_info.init_flog = 2;
        img_stitch_info.kill = 0;

        thread_fork("scan_pen_task", 28, 1024, 32, NULL, scan_pen_task, NULL);
    }
}

void scan_pen_task_uninit(void)
{
    printf("---> scan_pen_task_kill\n");
    img_stitch_info.kill = 1;
    os_taskq_post("scan_pen_task", 1, 100);//完成一帧数据的接收

    /*os_sem_post(&img_stitch_info.r_sem);*/
    while (img_stitch_info.kill) {
        os_time_dly(2);
    }

    task_kill("scan_pen_task");
    printf(">>>>>>>>>>>>>scan_pen_task_uninit");
}
#endif

