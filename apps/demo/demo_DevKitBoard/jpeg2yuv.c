#include "asm/jpeg_codec.h"
#include "asm/hwi.h"
#include "asm/debug.h"
#include "system/includes.h"
#include "generic/lbuf.h"
#include "fs/fs.h"
#include "os/os_api.h"
#include "video_ioctl.h"
#include "video.h"
#include "yuv_to_rgb.h"
#include "yuv_soft_scalling.h"
#include "app_config.h"

/****************************本文件功能：lbuff的jpeg转YUV420****************************************/

#define JPEG_DEC_USE_MUC	0 //1:分行解码可节省内存
#define JPEG_FPS_DBG		0 //1:打开jpeg解码帧率测试

#if !JPEG_DEC_USE_MUC
#define JPEG_DEC_FAST		0 //1:整帧快速解码
#endif

#define JPEG_DEFAULT_BUFF_SIZE	(100*1024) //默认jpeg buf内存申请大小

struct jpeg_buffer {
    u32 len;
    u8 data[0];
};
struct jpeg2yuv {
    u8 *jpeg_buf;
    void *lbuf;
    u32 jpeg_size;
    u8 *yuv420;
    u32 yuv420_size;
    u16 width;
    u16 height;
    u8 task_kill;
    u8 init;
    u8 index;
    OS_SEM sem;
    void (*cb)(u8 *data, u32 len, int width, int height);
};
static struct jpeg2yuv jpeg2yuv_info = {0};
#define __this (&jpeg2yuv_info)

static void jpeg_frame_decoder(void)
{
    int ret;
    u8 *yuv = NULL;
    u8 *cy = NULL;
    u8 *cb = NULL;
    u8 *cr = NULL;
    u8 *buf = NULL;
    u32 buf_size;
    u32 pix = 0;
    u16 width, height;
    u8 ytype = 0;
    u32 len;
    u32 cnt = 0;
    u32 time_1 = 0, time_2 = 0;
    printf("jpeg_frame_decoder start\n");
#if JPEG_DEC_FAST
    struct jpeg_dec_hdl jpeg_dec_hdl = {0};
#endif
    while (1) {
        ret = os_sem_pend(&__this->sem, 100);
        if (__this->task_kill) {
            break;
        }
        if (!ret) {
            struct jpeg_decode_req req = {0};
            struct jpeg_image_info info = {0};
            struct jpeg_buffer *plbuf = lbuf_pop(__this->lbuf, BIT(__this->index));
            if (!plbuf) {
                ret = -EINVAL;
                goto free_buf;
            }
#if JPEG_FPS_DBG
            time_1 = timer_get_ms();
            if (time_1 - time_2 > 1000) {
                time_2 = timer_get_ms();
                printf("jpg cnt = %d \n", cnt);//帧率处理测试打印
                cnt = 0;
            }
#endif

            buf_size = plbuf->len;
            buf = plbuf->data;

            info.input.data.buf = buf;
            info.input.data.len = buf_size;
            if (jpeg_decode_image_info(&info)) {
                printf("jpeg_decode_image_info err\n");
                ret = -EINVAL;
                goto free_buf;
            }
            width = info.width;
            height = info.height;
            __this->width = width;
            __this->height = height;
            pix = width * height;
            switch (info.sample_fmt) {
            case JPG_SAMP_FMT_YUV444:
                ytype = 1;
                break;//444
            case JPG_SAMP_FMT_YUV420:
                ytype = 4;
                break;//420
            default:
                ytype = 2;
                break;//422
            }

            len = pix + pix / ytype * 2;
#if JPEG_DEC_FAST
            jpeg_dec_hdl.start = true;
            ret = jpeg_decode_cyc(&jpeg_dec_hdl, buf, buf_size);
            __this->yuv420 = jpeg_dec_hdl.yuv;
            __this->yuv420_size = jpeg_dec_hdl.yuv_size;
            yuv = __this->yuv420;
            len = __this->yuv420_size;
#else
            if (!__this->yuv420) {
yuv_malloc:
                __this->yuv420_size = len;
                __this->yuv420 = malloc(len);
                if (!__this->yuv420) {
                    __this->yuv420_size = 0;
                    printf("yuv malloc err len : %d , width : %d , height : %d \n", width, height, len);
                    ret = -ENOMEM;
                    goto free_buf;
                }
            } else if (len > __this->yuv420_size) {
                free(__this->yuv420);
                goto yuv_malloc;
            }

            yuv = __this->yuv420;
            cy = yuv;
            cb = cy + pix;
            cr = cb + pix / ytype;

            req.input_type = JPEG_INPUT_TYPE_DATA;
            req.input.data.buf = buf;
            req.input.data.len = buf_size;
            req.buf_y = cy;
            req.buf_u = cb;
            req.buf_v = cr;
            req.buf_width = width;
            req.buf_height = height;
            req.out_width = width;
            req.out_height = height;
            req.output_type = JPEG_DECODE_TYPE_DEFAULT;
            req.dec_query_mode = FALSE;//TRUE;//TRUE使用查询法，FALSE使用中断法
            req.bits_mode = BITS_MODE_UNCACHE;

            ret = jpeg_decode_one_image(&req);
#endif
free_buf:
            if (plbuf) {
                lbuf_free(plbuf);
                plbuf = NULL;
            }
            if (ret) {
                printf("jpeg_dec err !!\n");
            } else {
                switch (ytype) {
                case 1:
                    YUV444pToYUV420p(yuv, yuv, width, height);
                    break;//444
                case 2:
                    YUV422pToYUV420p(yuv, yuv, width, height);
                    break;//422
                default:
                    break;//420
                }
                if (__this->cb) {
                    __this->cb(yuv, len, width, height);
                }
                cnt++;
            }
        }
    }
exit:
    if (__this->jpeg_buf) {
        free(__this->jpeg_buf);
        __this->jpeg_buf = NULL;
    }
#if JPEG_DEC_FAST
    jpeg_dec_hdl.start = 0;
    jpeg_decode_cyc(&jpeg_dec_hdl, NULL, 0);
#else
    if (__this->yuv420) {
        free(__this->yuv420);
        __this->yuv420 = NULL;
    }
#endif
    __this->jpeg_size = 0;
    __this->yuv420_size = 0;
    __this->task_kill = 0;
    __this->init = 0;
    printf("jpeg_frame_decoder exit\n");
}

#if JPEG_DEC_USE_MUC
extern void *jpg_dec_open(struct video_format *f);
extern int jpg_dec_input_data(void *_fh, void *data, u32 len);
extern int jpg_dec_set_output_handler(void *_fh, void *priv, int (*handler)(void *, struct YUV_frame_data *));
extern int jpg_dec_get_s_attr(void *_fh, struct jpg_dec_s_attr *attr);
extern int jpg_dec_set_s_attr(void *_fh, struct jpg_dec_s_attr *attr);
extern int jpg_dec_close(void *_fh);

struct yuv_recv {
    volatile unsigned char *y;
    volatile unsigned char *u;
    volatile unsigned char *v;
    volatile int y_size;
    volatile int u_size;
    volatile int v_size;
    volatile int recv_size;
    volatile int size;
    volatile char complit;
    volatile int yuv_cb_cnt;
};
static int yuv_out_cb(void *priv, struct YUV_frame_data *p)
{
    struct yuv_recv *rec = (struct yuv_recv *)priv;
    u8 type = (p->pixformat == VIDEO_PIX_FMT_YUV444) ? 1 : ((p->pixformat == VIDEO_PIX_FMT_YUV422) ? 2 : 4);
    ++rec->yuv_cb_cnt;
    if (!rec->complit) {
        memcpy(rec->y + rec->y_size, p->y, p->width * p->data_height);
        memcpy(rec->u + rec->u_size, p->u, p->width * p->data_height / type);
        memcpy(rec->v + rec->v_size, p->v, p->width * p->data_height / type);
        rec->y_size += p->width * p->data_height;
        rec->u_size += p->width * p->data_height / type;
        rec->v_size += p->width * p->data_height / type;
        rec->recv_size +=  p->width * p->data_height + (p->width * p->data_height / type) * 2;
        if (rec->recv_size >= rec->size) {
            rec->complit = 1;
        }
    } else {
        printf("err in complit  = %d , type = %s \n", rec->complit, (type == 1) ? "YUV444" : (type == 2) ? "yuv422" : "YUV420");
    }
    return 0;
}
static void jpeg_frame_mcu_decoder(void)
{
    int ret;
    int pix, width, height, ytype, len;
    unsigned char *yuv = NULL;
    u8 *buf = NULL;
    u32 buf_size;
    u32 cnt = 0;
    u32 time_1 = 0, time_2 = 0;
    struct yuv_recv yuv_rec_data = {0};

    void *fh = jpg_dec_open(NULL);
    if (!fh) {
        printf("err in jpg_dec_open \n\n");
        return ;
    }
    struct jpg_dec_s_attr jpg_attr;
    jpg_dec_get_s_attr(fh, &jpg_attr);
    jpg_attr.max_o_width  = 1920;//max w
    jpg_attr.max_o_height = 1200;//max h
    jpg_dec_set_s_attr(fh, &jpg_attr);
    jpg_dec_set_output_handler(fh, (void *)&yuv_rec_data, yuv_out_cb);

    printf("jpeg_frame_decoder start\n");
    while (1) {
        ret = os_sem_pend(&__this->sem, 100);
        if (__this->task_kill) {
            break;
        }
#if JPEG_FPS_DBG
        time_1 = timer_get_ms();
        if (time_1 - time_2 > 1000) {
            time_2 = timer_get_ms();
            printf("jpg cnt = %d \n", cnt);//帧率处理测试打印
            cnt = 0;
        }
#endif
        if (!ret) {
            struct jpeg_decode_req req = {0};
            struct jpeg_image_info info = {0};
            struct jpeg_buffer *plbuf = lbuf_pop(__this->lbuf, BIT(__this->index));
            if (!plbuf) {
                ret = -EINVAL;
                goto free_buf;
            }
            buf_size = plbuf->len;
            buf = plbuf->data;

            info.input.data.buf = buf;
            info.input.data.len = buf_size;
            if (jpeg_decode_image_info(&info)) {
                printf("jpeg_decode_image_info err\n");
                ret = -EINVAL;
                goto free_buf;
            }
            width = info.width;
            height = info.height;
            __this->width = width;
            __this->height = height;
            pix = width * height;
            switch (info.sample_fmt) {
            case JPG_SAMP_FMT_YUV444:
                ytype = 1;
                break;//444
            case JPG_SAMP_FMT_YUV420:
                ytype = 4;
                break;//420
            default:
                ytype = 2;
                break;//422
            }
            len = pix + pix / ytype * 2;

            if (!__this->yuv420) {
yuv_malloc:
                __this->yuv420_size = len;
                __this->yuv420 = malloc(len);
                if (!__this->yuv420) {
                    __this->yuv420_size = 0;
                    printf("yuv malloc err len : %d , width : %d , height : %d \n", width, height, len);
                    ret = -ENOMEM;
                    goto free_buf;
                }
            } else if (len > __this->yuv420_size) {
                free(__this->yuv420);
                goto yuv_malloc;
            }
            yuv = __this->yuv420;

            yuv_rec_data.y = yuv;
            yuv_rec_data.u = yuv_rec_data.y + pix;
            yuv_rec_data.v = yuv_rec_data.u + pix / ytype;
            yuv_rec_data.size = len;
            yuv_rec_data.recv_size = 0;
            yuv_rec_data.y_size = 0;
            yuv_rec_data.u_size = 0;
            yuv_rec_data.v_size = 0;
            yuv_rec_data.complit = 0;
            yuv_rec_data.yuv_cb_cnt = 0;

            ret = jpg_dec_input_data(fh, buf, buf_size);
free_buf:
            if (plbuf) {
                lbuf_free(plbuf);
                plbuf = NULL;
            }
            if (ret || !yuv_rec_data.complit || yuv_rec_data.recv_size != yuv_rec_data.size) {
                printf("jpg dec err !!\n");
            } else {
                switch (ytype) {
                case 1:
                    yuv_rec_data.recv_size = YUV444pToYUV420p(yuv, yuv, width, height);
                    break;//444
                case 2:
                    yuv_rec_data.recv_size = YUV422pToYUV420p(yuv, yuv, width, height);
                    break;//422
                default:
                    break;//420
                }
                if (__this->cb) {
                    __this->cb(yuv, len, width, height);
                }
                cnt++;
            }
        }
    }
exit:
    if (__this->jpeg_buf) {
        free(__this->jpeg_buf);
        __this->jpeg_buf = NULL;
    }
    if (__this->yuv420) {
        free(__this->yuv420);
        __this->yuv420 = NULL;
    }
    if (fh) {
        jpg_dec_close(fh);
    }
    os_sem_del(&__this->sem, 0);
    __this->jpeg_size = 0;
    __this->yuv420_size = 0;
    __this->task_kill = 0;
    __this->cb = NULL;
    __this->init = 0;
    printf("jpeg_frame_decoder exit\n");
}
#endif
int jpeg2yuv_open(void)
{
    int ret;
    if (__this->init) {
        return 0;
    }
    __this->task_kill = 0;
    __this->yuv420 = NULL;
    __this->yuv420_size = 0;
    __this->cb = NULL;
    __this->jpeg_size = JPEG_DEFAULT_BUFF_SIZE;
    __this->jpeg_buf =  malloc(__this->jpeg_size);
    if (!__this->jpeg_buf) {
        return -ENOMEM;
    }
    __this->index = 0;
    __this->lbuf = lbuf_init(__this->jpeg_buf, __this->jpeg_size, 4, sizeof(struct jpeg_buffer));
    __this->init = true;
    os_sem_create(&__this->sem, 0);
#if JPEG_DEC_USE_MUC
    ret = thread_fork("jpeg_frame_mcu_decoder", 15, 1024 * 1, 0, 0, jpeg_frame_mcu_decoder, NULL);
#else
    ret = thread_fork("jpeg_frame_decoder", 15, 1024 * 1, 0, 0, jpeg_frame_decoder, NULL);
#endif
    if (ret) {
        __this->init = 0;
        if (__this->jpeg_buf) {
            free(__this->jpeg_buf);
            __this->jpeg_buf = NULL;
        }
        os_sem_del(&__this->sem, 0);
        return ret;
    }
    return ret;
}
void jpeg2yuv_yuv_callback_register(void (*cb)(u8 *data, u32 len, int width, int height))
{
    __this->cb = cb;
}
int jpeg2yuv_jpeg_frame_write(u8 *buf, u32 len)
{
    int ret = -EINVAL;
    if (!__this->init) {
        return 0;
    }
    u32 size = lbuf_free_space(__this->lbuf);
    if (len < size) {
        struct jpeg_buffer *p = lbuf_alloc(__this->lbuf, len);
        if (p) {
            p->len = len;
            memcpy(p->data, buf, len);
            lbuf_push((void *)p, BIT(__this->index));
            os_sem_post(&__this->sem);
            ret = 0;
        }
    }
    if (ret) {
        /*printf("lost jpeg frame \n");*/
        putchar('A');
    }
    return ret;
}
void jpeg2yuv_close(void)
{
    if (__this->init) {
        __this->task_kill = true;
        os_sem_post(&__this->sem);
        while (__this->task_kill) {
            os_time_dly(2);
        }
        __this->init = 0;
        printf("jpeg2yuv_close ok\n");
    }
}

