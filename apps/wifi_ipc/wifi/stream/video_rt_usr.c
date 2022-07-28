#include "app_config.h"
#include "system/sys_common.h"
#include "server/rt_stream_pkg.h"
#include "stream_core.h"
#include "sock_api/sock_api.h"
#include "get_yuv_data.h"

#ifdef CONFIG_VIDEO_ENABLE

#ifdef CONFIG_QR_CODE_NET_CFG
#include "asm/jpeg_codec.h"
#include "qrcode/qrcode.h"
#include "json_c/json_tokener.h"
#include "mbedtls/base64.h"
#endif

extern int qr_code_net_set_ssid_pwd(const char *ssid, const char *pwd);
extern int page_turning_recognition(void *data, u32 length, u8 buf_copy);
extern u8 page_turning_get_mode(void);
extern u8 get_turing_ocr_flag(void);
extern u8 get_turing_finger_flag(void);
extern u8 is_in_config_network_state(void);
extern u32 timer_get_ms(void);

#ifdef CONFIG_PAGE_TURNING_DET_ENABLE
static jl_ptd_t  jl_ptd;
static u32 page_tri_time;
static u8 page_tri;
#endif

#ifdef CONFIG_TURING_PAGE_TURNING_DET_ENABLE
static MotionDetection md;
#endif

#ifdef CONFIG_QR_CODE_NET_CFG

static void *qr_decoder;
static u8 qr_flag;

#ifdef CONFIG_TURING_SDK_ENABLE
static char turing_openid[12];

const char *get_qrcode_result_openid(void)
{
    return turing_openid[0] ? turing_openid : NULL;
}
#endif

void qr_code_net_cfg_start(void)
{
#ifdef CONFIG_TURING_SDK_ENABLE
    memset(turing_openid, 0, sizeof(turing_openid));
#endif
    qr_flag = 1;
    qr_decoder = qrcode_init(QR_FRAME_W, QR_FRAME_H, QR_FRAME_W, QRCODE_MODE_NORMAL, 150, SYM_QRCODE, 1);
}

void qr_code_net_cfg_stop(void)
{
    if (qr_decoder) {
        qrcode_deinit(qr_decoder);
        qr_decoder = NULL;
    }
}

static int qr_code_net_cfg_process(void *qr_decoder, u8 *inputFrame)
{
    int ret = -1, type = 0;
    char *buf = NULL;
    int buf_size = 0;
    int md_detected = 0; //是否检测到运动物体
    json_object *new_obj = NULL;
    json_object *key = NULL;

    if (qr_decoder)  {
        qrcode_detectAndDecode(qr_decoder, inputFrame, &md_detected);

        ret = qrcode_get_result(qr_decoder, &buf, &buf_size);
        type = qrcode_get_symbol_type(qr_decoder);

        if (buf_size > 0 && ret == 0) {
            buf[buf_size] = 0;
#ifdef CONFIG_TURING_SDK_ENABLE
            unsigned long olen = 0;
            ret = mbedtls_base64_decode((u8 *)buf, strlen(buf), &olen, (u8 *)buf, strlen(buf));
            if (ret) {
                return -1;
            }
            buf[olen] = 0;
#endif
            printf("qr code type = %d decode: %s\n", type, buf);
            new_obj = json_tokener_parse(buf);
            if (!new_obj) {
                goto __exit;
            }
            const char *str_ssid = json_object_get_string(json_object_object_get(new_obj, "ssid"));
            if (str_ssid) {
                printf("qr code net config ssid : %s\n", str_ssid);
            } else {
                goto __exit;
            }
#ifdef CONFIG_TURING_SDK_ENABLE
            const char *openid = json_object_get_string(json_object_object_get(new_obj, "TuringBindID"));
            if (openid) {
                snprintf(turing_openid, sizeof(turing_openid), "%s", openid);
            }

            const char *str_pwd = json_object_get_string(json_object_object_get(new_obj, "pwd"));
#else
            const char *str_pwd = json_object_get_string(json_object_object_get(new_obj, "pass"));
#endif
            if (str_pwd) {
                printf("qr code net config pwd : %s\n", str_pwd);
            } else {
                str_pwd = "";
            }

            qr_code_net_set_ssid_pwd(str_ssid, str_pwd);
            ret = 0;
            qr_flag = 0;
        }
    }

__exit:
    if (new_obj) {
        json_object_put(new_obj);
    }

    return ret;
}

static void cvHalfYUV420(unsigned char *dataSrc, unsigned char *dataDst, int srcHeight, int srcWidth)
{
    //support YUV420
    int i = 0;
    for (int y = 0; y < srcHeight; y += 2) {
        for (int x = 0; x < srcWidth; x += 2) {
            dataDst[i] = dataSrc[y * srcWidth + x];
            i++;
        }
    }
    for (int y = 0; y < srcHeight / 2; y += 2) {
        for (int x = 0; x < srcWidth; x += 4) {
            dataDst[i] = dataSrc[(srcWidth * srcHeight) + (y * srcWidth) + x];
            i++;
            dataDst[i] = dataSrc[(srcWidth * srcHeight) + (y * srcWidth) + (x + 1)];
            i++;
        }
    }
}
#endif

static void get_one_frame(u8 *inputFrame, u32 len, int width, int height)
{
#ifdef CONFIG_QR_CODE_NET_CFG
    if (is_in_config_network_state()) {
        if (qr_decoder && qr_flag) {
#if 1
            cvHalfYUV420(inputFrame, inputFrame, height, width);
#else
            extern int YUV420p_Soft_Scaling(unsigned char *src, unsigned char *out, int src_w, int src_h, int out_w, int out_h);
            YUV420p_Soft_Scaling(inputFrame, inputFrame, width, height, width / 2, height / 2);
#endif
            qr_code_net_cfg_process(qr_decoder, inputFrame);
        }
    }
#endif

#ifdef CONFIG_PAGE_TURNING_DET_ENABLE
    if (page_turning_get_mode() == PICTURE_DET_ENTER_MODE && ptd_process(&jl_ptd, inputFrame)) {
        page_tri = 1;
        page_tri_time = timer_get_ms();
    }
#endif

#ifdef CONFIG_TURING_PAGE_TURNING_DET_ENABLE
    if (page_turning_get_mode() == PICTURE_DET_ENTER_MODE) {
        u8 ocr_flag = get_turing_ocr_flag();
        u8 finger_flag = get_turing_finger_flag();
        unsigned char *outImg = getMotionResult(&md, inputFrame, width, height, 1, TL_CV_YUV2GRAY_420, timer_get_ms(), ocr_flag, finger_flag);
        if (outImg) {
            struct jpeg_encode_req req = {0};
            u32 buf_len = ocr_flag ? 60 * 1024 : 30 * 1024;
            u8 *jpg_img = malloc(buf_len);
            if (!jpg_img) {
                puts("jpeg buf not enough !!!\n");
                return;
            }

            //q >=2 , size  = w*h*q*0.32
            req.q = 5;	//jepg编码质量(范围0-13),质量越好,下面的编码buf需要适当增大
            req.format = JPG_SAMP_FMT_YUV420;
            req.data.buf = jpg_img;
            req.data.len = buf_len;
            req.width = ocr_flag ? width : (width / 2);
            req.height = ocr_flag ? height : (height / 2);
            req.y = outImg;
            req.u = req.y + req.width * req.height;
            req.v = req.u + req.width * req.height / 4;

            if (0 == jpeg_encode_one_image(&req)) {
                printf("jpeg_encode_one_image ok \n\n");
                if (0 != page_turning_recognition(req.data.buf, req.data.len, 0)) {
                    free(jpg_img);
                }
            }
        }
    }
#endif
}

int page_turning_det_uninit(void)
{
    get_yuv_uninit();
#ifdef CONFIG_PAGE_TURNING_DET_ENABLE
    ptd_deinit(&jl_ptd);
#endif
#ifdef CONFIG_TURING_PAGE_TURNING_DET_ENABLE
    cvFreeImageTL(md.frame1);
#endif
    return 0;
}

#ifdef CONFIG_QR_CODE_NET_CFG
void config_network_get_yuv_init(void)
{
    get_yuv_init(get_one_frame);
}

void config_network_get_yuv_uninit(void)
{
    get_yuv_uninit();
}
#endif

static u32(*video_rt_cb)(void *, u8 *, u32);
static void *cb_priv;

void set_video_rt_cb(u32(*cb)(void *, u8 *, u32), void *priv)
{
    video_rt_cb = cb;
    cb_priv = priv;
}

static void *video_rt_usr_init(const char *path, const char *mode)
{
    //连接服务器操作
    puts("video_rt_usr_init\n");

    struct rt_stream_info *r_info = calloc(1, sizeof(struct rt_stream_info));
    if (r_info == NULL) {
        printf("%s malloc fail\n", __FILE__);
        return NULL;
    }

    return (void *)r_info;
}

static int video_rt_usr_send(void *hdr, void *data, u32 len, u8 type)
{
    struct rt_stream_info *r_info = (struct rt_stream_info *)hdr;

    if (video_rt_cb) {
        video_rt_cb(cb_priv, data, len);
    }

#ifdef CONFIG_PAGE_TURNING_DET_ENABLE
    static u32 recognition_time = 0;
    static u32 timehdl;
    u32 dtime;

    if (type == JPEG_TYPE_VIDEO && page_turning_get_mode() == PICTURE_DET_ENTER_MODE) {
        if (page_tri) {
            dtime = timer_get_ms() - page_tri_time;
            //翻页时间已经过去500s并且上次距离上次识别已经过去500ms
            if (dtime > 500 && (timer_get_ms() - recognition_time) > 500) {
                if (0 == page_turning_recognition(data, len, 1)) {
                    recognition_time = timer_get_ms();
                    page_tri = 0;
                }
            }
        }
    }
#endif

    return len;
}

static int video_rt_usr_uninit(void *hdr)
{
    puts("video_rt_usr_uninit\n\n\n\n\n\n");
    struct rt_stream_info *r_info = (struct rt_stream_info *)hdr;
    free(r_info);
    return 0;
}

REGISTER_NET_VIDEO_STREAM_SUDDEV(usr_video_stream_sub) = {
    .name = "usr",
    .open =  video_rt_usr_init,
    .write = video_rt_usr_send,
    .close = video_rt_usr_uninit,
};

#endif
