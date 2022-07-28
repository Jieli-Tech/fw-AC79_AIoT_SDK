#include "img_stitch.h"
#include "system/includes.h"
#include "server/video_server.h"
#include "app_config.h"
#include "asm/debug.h"
#include "asm/osd.h"
#include "asm/isc.h"
#include "asm/jpeg_codec.h"
#include "storage_device.h"
#include "server/ctp_server.h"
#include "camera.h"
#include "net_server.h"
#include "server/net_server.h"
#include "stream_protocol.h"
#include "yuv_soft_scalling.h"
#include "sys_common.h"

#include "json_c/json_object.h"
#include "json_c/json.h"
#include "json_c/json_tokener.h"
#include "http/http_cli.h"

#include "lcd_drive.h"
#include "ui/ui.h"
#include "ename.h"
#include "ui_api.h"

//#define SAVE_DATA_TO_SD

#ifdef CONFIG_SPI_VIDEO_ENABLE
struct img_stitch_struct {
    u8 init_flog;
    OS_SEM r_sem;
    OS_SEM http_sem;
    OS_SEM http_get_str;
    int all_w;
    u8 *fbuf;
    u8 *yuv_420;
    char *img_buf1;
    char *jpg_buf;
    u32 jpg_len;
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
            fwrite(buf, len, 1, one_fd);
        }
    } else if (!one_file) {
        FILE *fd = fopen(CONFIG_ROOT_PATH"YUV/test/YUV_***.YUV", "w+");

        if (fd) {
            fwrite(buf, len, 1, fd);
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

void yuv_y_to_jpg(char *yuv_y, w, h) //事先已经将YUV_Y填充好UV
{
    u32 buf_len =  60 * 1024;
    char *jpg_img = calloc(1, buf_len);//存储编码出来的JPG图片

    img_stitch_info.jpg_buf = jpg_img;

    struct jpeg_encode_req req = {0};
    req.q = 5;	//jepg编码质量(范围0-13),0最小 质量越好,下面的编码buf需要适当增大
    req.format = JPG_SAMP_FMT_YUV420;
    req.data.buf = img_stitch_info.jpg_buf;
    req.data.len = w * h * 3 / 2;
    req.width = w ;
    req.height =  h ;
    req.y = yuv_y;
    req.u = req.y + (w * h);
    req.v = req.u + (w * h) / 4;
    int err = jpeg_encode_one_image(&req);//获取一张JPG图片
    if (!err) {
        printf(">>>>>>>>>>>get_jpg");
    } else {
        printf("[error]>>>>>>>>>>>get_jpg fail");
    }
    img_stitch_info.jpg_buf = req.data.buf;
    img_stitch_info.jpg_len = req.data.len;
#ifdef SAVE_DATA_TO_SD
    save_outdata_to_SD(req.data.buf, req.data.len);
#endif

    os_sem_post(&img_stitch_info.http_sem);
    os_sem_pend(&img_stitch_info.http_get_str, 0);
}

void scan_pen_task(void *priv)
{
    int msg[32];
    int out_size;
    int FAST_TH = 55;
    int MATCH_TH = 2560;
    int  w = 176;
    int  h = 128;
    int w_out = w * 5;
    int h_out = h + 64 * 2;
    int Max_corner_count = 64;
    int Fast_corner_mode = 1;
    int NMS_mode = 1;
    int h_off = 32 * 2;
    int offset_th = 8;
    int nms_windon_size = 7;
    int Max_filter_th_x = 6;
    int Max_filter_th_y = 6;
    int Frame_stride = 15;
    int Adjustment_enable = 1;
    int Max_off_y = 15;
    int Fast_corner_target = 23;
    int Fast_corner_target_min = 21;
    int Fast_corner_target_max = 25;
    int Fast_th_min = 45;
    int Fast_th_max = 110;
    uint32_t diff_ths[3] = {11, 7, 4};
    int strengths[4] = {8, 8, 6, 3};
    uint8_t Fast_ths[5] = { 50, 60, 70, 80, 90 };

    uint8_t *cat_buf0;
    uint8_t *cat_buf1;
    uint8_t *output_img;
    /*uint8_t *output_img1;*/


    cat_buf0 = calloc(1, w * h);
    if (cat_buf0 == NULL) {
        printf(">>>cat_buf0 = null", cat_buf0);
    }
    cat_buf1 = calloc(1, w * h);
    if (cat_buf1 == NULL) {
        printf(">>>cat_buf0 = null", cat_buf1);
    }
    output_img = calloc(1, w_out * h_out * 3 / 2); //直接申请YUV_buf
    if (output_img == NULL) {
        printf(">>>cat_buf0 = null", output_img);
    }
    memset(output_img + w_out * h_out, 0x80, w_out * h_out * 1 / 2);

    /*output_img1 = calloc(1, w_out * h_out * 3/2);*/
    /*if(output_img1 == NULL){*/
    /*printf(">>>cat_buf0 = null", output_img1);*/
    /*}	*/
    /*memset(output_img1+w_out * h_out, 0x80, w_out * h_out * 1/2);*/

    os_sem_pend(&img_stitch_info.r_sem, 0);
    memcpy(cat_buf0, img_stitch_info.fbuf, w * h);

    os_sem_pend(&img_stitch_info.r_sem, 0);
    memcpy(cat_buf1, img_stitch_info.fbuf, w * h);

    img_stitch_init(cat_buf0, w, h, output_img, w_out, h_out, h_off, Max_corner_count, Fast_corner_mode, FAST_TH, MATCH_TH, offset_th, nms_windon_size, Max_filter_th_x, Max_filter_th_y, Frame_stride, Adjustment_enable, Max_off_y, Fast_corner_target, Fast_corner_target_min, Fast_corner_target_max, Fast_th_min, Fast_th_max, diff_ths, strengths, Fast_ths, NMS_mode);

    while (1) {

        int stitch_result = img_stitch_process(cat_buf0, cat_buf1, output_img, 0, 0);
        if (stitch_result != 1) {
            printf(">>>>>>>>>>>>stitch_result=%d", stitch_result);
        }

        if (stitch_result == 5 || stitch_result == 8 || stitch_result == 6) {
            printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> out ");
            /*int result = post_process(output_img, output_img1, w_out, h_out, w / 2, h / 2); //该函数用于输出 将不水平的文字放置水平*/
            /*if (result) { //输出校准的数据*/
            /*yuv_y_to_jpg(output_img1, w_out, h_out);*/
            /*} else { //数据不需要校准*/
            yuv_y_to_jpg(output_img, w_out, h_out);
#ifdef SAVE_DATA_TO_SD
            save_outdata_to_SD(output_img, w_out * h_out);
#endif
            /*}*/

            memset(output_img, 0x00, w_out * h_out);
            /*memset(output_img1, 0x00, w_out * h_out);*/
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
            img_stitch_reinit(cat_buf0, w, h, output_img, w_out, h_out, h_off, Max_corner_count, Fast_corner_mode, FAST_TH, MATCH_TH, Adjustment_enable, Fast_corner_target
                              , Fast_corner_target_min, Fast_corner_target_max, Fast_th_min, Fast_th_max, diff_ths, strengths, Fast_ths, NMS_mode);
        }

        if (stitch_result != 1) { //遇到拼接失败
            printf("fail stitch");
            for (u8 i = 0; i < 3; i++) { //尝试后面三帧的拼接结果
                os_sem_pend(&img_stitch_info.r_sem, 0);
                if (img_stitch_info.kill) {
                    goto exit;
                }
                memcpy(cat_buf1, img_stitch_info.fbuf, w * h);
                /*stitch_result = img_stitch_process(cat_buf0, cat_buf1, output_img, 0, 0); //0,0不强制拼接  1,0，强制拼接*/
                if (stitch_result != 1) {
                    printf(">>>>>>>>>>>>stitch_result=%d", stitch_result);
                    if (i == 2) { //连续两次拼接识别 重新拼接
                        memset(output_img, 0x00, w_out * h_out);
                        /*memset(output_img1, 0x00, w_out * h_out);*/
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
                        img_stitch_reinit(cat_buf0, w, h, output_img, w_out, h_out, h_off, Max_corner_count, Fast_corner_mode, FAST_TH, MATCH_TH, Adjustment_enable, Fast_corner_target
                                          , Fast_corner_target_min, Fast_corner_target_max, Fast_th_min, Fast_th_max, diff_ths, strengths, Fast_ths, NMS_mode);
                    }
                }
                if (stitch_result) {
                    break;
                }
            }
            printf(">>>>>>>>>>>>stitch break ^^^^^^^^^^^i");
        }

        memcpy(cat_buf0, cat_buf1, w * h);//将下一帧数据传给上一帧
        os_sem_pend(&img_stitch_info.r_sem, 0);

        if (img_stitch_info.kill) {
            goto exit;
        }
        memcpy(cat_buf1, img_stitch_info.fbuf, w * h);
    }
exit:
    /*img_stitch_process(cat_buf0, cat_buf1, output_img, 0, 1); //0,0不强制拼接  1,0，强制拼接*/
    free(cat_buf0);
    free(cat_buf1);
    free(output_img);
    /*free(output_img1);*/
    img_stitch_deinit();

    img_stitch_info.kill = 0;
    img_stitch_info.init_flog = 1;

}

void send_yuv_data(char *buf, u16 len)
{
    img_stitch_info.fbuf = buf;
    os_sem_post(&img_stitch_info.r_sem);
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
    printf(">>>>>>>*************************************>>>>>>spi_recv_task_init");
    os_sem_create(&img_stitch_info.r_sem, 0);

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
    os_sem_post(&img_stitch_info.r_sem);

    while (img_stitch_info.kill) {
        os_time_dly(2);
    }
    os_sem_del(&img_stitch_info.r_sem, 0);
    printf(">>>>>>>>>>>>>scan_pen_task_uninit");
}

/*******************HTTP数据处理线程******************************/

#define HTTP_DEMO_URL  "https://log.jieliapp.com/predict/ocr_system"

static void http_post_test_task(void *priv)
{
    os_sem_create(&img_stitch_info.http_sem, 0);
    os_sem_create(&img_stitch_info.http_get_str, 0);

    char *send_buf;
    char *buffer;

    while (1) {
        os_sem_pend(&img_stitch_info.http_sem, 0);

        buffer = calloc(1, img_stitch_info.jpg_len * 2 * 4 / 5); //不清楚具体输出多大 但是不会超过两倍
        printf(">>>>>>>>>>>img_stitch_info.jpg_len*2 * 4/5 = %d", img_stitch_info.jpg_len * 2 * 4 / 5);
        if (NULL == buffer) {
            printf(">>>>>>>>>>>src = NULL");
        }
        size_t len;

        mbedtls_base64_encode(buffer, img_stitch_info.jpg_len * 2, &len, img_stitch_info.jpg_buf, img_stitch_info.jpg_len);

        free(img_stitch_info.jpg_buf);

        struct json_object *img_obj;
        struct json_object *img_obj2;
        img_obj = json_object_new_object();
        img_obj2 = json_object_new_array();

        json_object_array_add(img_obj2, json_object_new_string(buffer));
        json_object_object_add(img_obj, "images", img_obj2);

        send_buf = calloc(1, strlen(json_object_get_string(img_obj)));
        strcat(send_buf, json_object_to_json_string_ext(img_obj, JSON_C_TO_STRING_NOSLASHESCAPE));
        if (send_buf == NULL) {
            printf(">>>>>>>>>>>send_buf  = NULL");
        }

        httpcli_ctx *ctx;
        ctx = (httpcli_ctx *)calloc(1, sizeof(httpcli_ctx));
        if (!ctx) {
            printf("calloc fail\n");
            return;
        }

        http_body_obj http_body_buf;
        memset(&http_body_buf, 0x0, sizeof(http_body_obj));

        http_body_buf.recv_len = 0;
        http_body_buf.buf_len = strlen(send_buf);
        http_body_buf.buf_count = 1;
        http_body_buf.p = (char *) malloc(http_body_buf.buf_len * http_body_buf.buf_count);
        if (!http_body_buf.p) {
            free(ctx);
            printf(">>>>>>>>>>>>http_body_buf fail");
            return;
        }

        ctx->url = HTTP_DEMO_URL;
        ctx->timeout_millsec = 5000;
        ctx->priv = &http_body_buf;
        ctx->connection = "close";

        printf("http post test start\n\n");
        ctx->data_format = "application/json";
        ctx->post_data = send_buf;
        ctx->data_len = strlen(send_buf);
        int ret = httpcli_post(ctx);


        if (ret != HERROR_OK) {
            printf("http client test faile\n");
        } else {

            if (http_body_buf.recv_len > 0) {
                printf("\nReceive %d Bytes from (%s)\n", http_body_buf.recv_len, HTTP_DEMO_URL);
                printf("%s\n", http_body_buf.p);
            }
            json_object_put(img_obj);
            struct json_object *img_obj3;
            img_obj3 = json_tokener_parse(http_body_buf.p);
            if (!img_obj3) {
                printf("%d img_obj NULL", __LINE__);
            }

            char *str_get;
            str_get = json_object_get_string(json_object_object_get(img_obj3, "results"));
            char *out_json;
            out_json = strstr(str_get, "{");
            struct json_object *img_obj4;
            img_obj4 = json_tokener_parse(out_json);

            char *out;
            out = json_object_get_string(json_object_object_get(img_obj4, "text"));
            if (out != NULL) {
                printf(">>>>>>>>>>out=%s", out);
#ifdef CONFIG_UI_ENABLE
                ui_text_set_textu_by_id(BASEFORM_5, out, strlen(out), FONT_DEFAULT | FONT_SHOW_MULTI_LINE);//显示获取到的字符串
#endif
            } else {
                printf("[error]>>>>>>>>>>out=NULL");
            }
            free(http_body_buf.p);
            free(send_buf);
            free(buffer);
            free(ctx);
            json_object_put(img_obj3);
            json_object_put(img_obj4);

            os_sem_set(&img_stitch_info.http_get_str, 0);
            os_sem_post(&img_stitch_info.http_get_str);
        }
    }
}

void http_post_test_init(void *priv)
{
    thread_fork("http_post_test_task", 28, 2 * 1024, 0, NULL, http_post_test_task, NULL);
}
late_initcall(http_post_test_init);



#endif

