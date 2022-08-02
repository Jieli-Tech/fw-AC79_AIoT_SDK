#include "app_config.h"
#include "device/device.h"//u8
#include "server/video_dec_server.h"//fopen
#include "system/includes.h"//GPIO
#include "lcd_drive.h"
#include "sys_common.h"
#include "yuv_soft_scalling.h"
#include "asm/jpeg_codec.h"
#include "lcd_te_driver.h"
#include "get_yuv_data.h"
#include "lcd_config.h"

#include "asm/jpeg_codec.h"
#include "qrcode/qrcode.h"
#include "json_c/json_tokener.h"
#include "mbedtls/base64.h"

#if CONFIG_LCD_QR_CODE_ENABLE
/************视频部分*将摄像头数据显示到lcd上**********************************/
/* 开关打印提示 */
#if 1
#define log_info(x, ...)    printf("\n[camera_to_lcd_test]>>>>>>>>>>>>>>>>>>>###" x " \n", ## __VA_ARGS__)
#else
#define log_info(...)
#endif


static struct qr_handle {
    void *decoder;
    u8 *buf;
    char frame_use_flog: 1;
    char ssid[33];
    char pwd[65];
} qr_hdl;

void qr_code_get_one_frame_YUV_420(u8 *buf, u16 buf_w, u16 buf_h)
{
    if (qr_hdl.frame_use_flog == 0) {
        if (qr_hdl.buf == NULL) {
            qr_hdl.buf = malloc(QR_FRAME_W * QR_FRAME_H * 3 / 2);
        }
        YUV420p_Soft_Scaling(buf, qr_hdl.buf, buf_w, buf_h, QR_FRAME_W, QR_FRAME_H);
        qr_hdl.frame_use_flog = 1;
    }
}

void qr_get_ssid_pwd(u32 ssid, u32 pwd)
{
    *((u32 *)ssid) = (u32)qr_hdl.ssid;
    *((u32 *)pwd) = (u32)qr_hdl.pwd ;
}

static void qr_code_start(void)
{
    memset(&qr_hdl, 0, sizeof(qr_hdl));
    qr_hdl.decoder = qrcode_init(QR_FRAME_W, QR_FRAME_H, QR_FRAME_W, QRCODE_MODE_NORMAL, 150, SYM_QRCODE, 1);
    if (!qr_hdl.decoder) {
        printf("[error] qrcode_init fail");
    }
}

static void qr_code_stop(void)
{
    if (qr_hdl.decoder) {
        qrcode_deinit(qr_hdl.decoder);
        qr_hdl.decoder = NULL;
        free(qr_hdl.buf);
    }
}

static void qr_code_check_run(void)
{
    if (qr_hdl.frame_use_flog) {
        int ret = -1, type = 0;
        char *buf = NULL;
        int buf_size = 0;
        int enc_type = 0;
        int md_detected = 1; //是否检测到运动物体
        json_object *new_obj = NULL;
        json_object *key = NULL;

        qrcode_detectAndDecode(qr_hdl.decoder, qr_hdl.buf, &md_detected);
        ret = qrcode_get_result(qr_hdl.decoder, &buf, &buf_size, &(enc_type));
        type = qrcode_get_symbol_type(qr_hdl.decoder);
        qr_hdl.frame_use_flog = 0;
        if (buf_size > 0 && ret == 0) {
            buf[buf_size] = 0;
            printf("qr code type = %d decode: %s\n", type, buf);
            new_obj = json_tokener_parse(buf);
            if (!new_obj) {
                goto __exit;
            }

            const char *str_ssid = json_object_get_string(json_object_object_get(new_obj, "ssid"));
            if (str_ssid) {
                sprintf(qr_hdl.ssid, "%s", str_ssid);
                printf("qr code ssid : %s\n", qr_hdl.ssid);
            } else {
                goto __exit;
            }

            const char *str_pwd = json_object_get_string(json_object_object_get(new_obj, "pass"));
            if (str_pwd) {
                sprintf(qr_hdl.pwd, "%s", str_pwd);
                printf("qr code pwd : %s\n", qr_hdl.pwd);
            } else {
                str_pwd = "";
                sprintf(qr_hdl.pwd, "%s", str_pwd);
            }
        }
__exit:
        if (new_obj) {
            json_object_put(new_obj);
        }
    }
}
static void qr_code_task(void *priv)
{
    qr_code_start();
    while (1) {
        qr_code_check_run();
        os_time_dly(15);
    }
    qr_code_stop();
}
int qr_code_test(void)
{
    if (qr_hdl.buf == NULL) {
        printf("<<<<<<<<<<<<<<<<<qr_hdl.buf[malloc fail]");
    }
    return thread_fork("qr_code_task", 5, 1024 * 6, 0, 0, qr_code_task, NULL);
}
late_initcall(qr_code_test);

#endif

