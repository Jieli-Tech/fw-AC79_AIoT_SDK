
/*****************************************************************
>file name : uvc_dev.c
>author : lichao
>create time : Tue 13 Jun 2017 04:20:02 PM HKT
*****************************************************************/
#include "device/usb_stack.h"
#include "asm/dma.h"
#include "asm/uvc_device.h"
#include "usb_config.h"
#include "host_uvc.h"
#include "video/video_ioctl.h"
#include "video/video.h"
#include "video/videobuf.h"
#include "video/video_ppbuf.h"
#include "os/os_api.h"
#include "os/os_type.h"
#include "app_config.h"

#if TCFG_HOST_UVC_ENABLE

#define UVC_REC_JPG_HEAD_SIZE	8
#define UVC_REC_JPG_ALIGN     	512
#define UVC_JPEG_HEAD 			0xE0FFD8FF
#define USB_DMA_CP_ENABLE       (0)//注意：使用硬件dma copy需要异步接收推送数据，否则出现速度快数据错乱的问题

struct uvc_dev_control {
    OS_SEM sem;
    struct list_head dev_list;
};
struct mjpg_uvc_image {
    u8 *buf;
    u32 size;
};
struct uvc_fh {
    u8 id;
    u8 eof;
    u8 fps;
    u8 real_fps;
    u8 drop_frame;
    u8 streamoff;
    u8 uvc_out;
    u32 frame_cnt;
    u32 drop_frame_cnt;
    struct list_head entry;
    int open;
    int streamon;
    int free_size;
    u8 *buf;
    int b_offset;
    struct dma_list dma_list[16];
    u8 bfmode;
    void *ppbuf;
    struct videobuf_queue *video_q;
    struct video_device *video_dev;
    struct device device;
    void *private_data;
    OS_SEM sem;
    OS_SEM img_sem;
    struct mjpg_uvc_image img;
    volatile u8 image_req;
};
static struct uvc_dev_control uvc_dev;

#define __this  (&uvc_dev)

#define list_for_each_uvc(fh) \
        list_for_each_entry(fh, &__this->dev_list, entry)

#define list_add_uvc(fh) \
        list_add(&fh->entry, &__this->dev_list)

#define list_del_uvc(fh) \
        list_del(&fh->entry);


static int uvc_img_cap(void *_fh, u32 arg)
{
    struct uvc_fh *fh = (struct uvc_fh *)_fh;
    struct video_image_capture *icap = (struct video_image_capture *)arg;
    u32 size;
    u32 *head;
    int err = 0;
    if (!icap) {
        return -EINVAL;
    }
    if (fh) {
        fh->img.buf = icap->baddr;
        fh->img.size = icap->size;
        os_sem_create(&fh->img_sem, 0);
        err = os_sem_pend(&fh->img_sem, 500);
        if (err) {
            log_e("image err");
        }
        fh->image_req = false;
        head = (u32 *)(fh->img.buf + 8);
        if (*head == UVC_JPEG_HEAD) {
            fh->img.size -= 8;
            memcpy(icap->baddr, icap->baddr + 8, fh->img.size);
        }
        os_sem_del(&fh->img_sem, 0);
        if (icap->size < fh->img.size) {
            log_e("image capture size is too small !!!\n");
        } else if (!err) {
            log_d("image capture : %d.%dKB\n", fh->img.size / 1024, (fh->img.size % 1024) * 10 / 1024);
        }
        icap->size = fh->img.size;
        return err;
    } else {
        log_e("image err in on open uvc\n");
    }
    return err;
}
void *uvc_buf_malloc(struct uvc_fh *fh, u32 size)
{
    void *b;
    struct video_device *video_dev = (struct video_device *)fh->video_dev;
    if (video_dev->bfmode == VIDEO_PPBUF_MODE) {
        b = video_ppbuf_malloc(video_dev->ppbuf, size);
    } else {
        b = video_buf_malloc(video_dev, size);
    }
    return b;
}
void *uvc_buf_realloc(struct uvc_fh *fh, void *buf, int size)
{
    struct video_device *video_dev = (struct video_device *)fh->video_dev;
    if (video_dev->bfmode == VIDEO_PPBUF_MODE) {
        return video_ppbuf_realloc(video_dev->ppbuf, buf, size);
    } else {
        return video_buf_realloc(video_dev, buf, size);
    }
}
void uvc_buf_free(struct uvc_fh *fh, void *buf)
{
    struct video_device *video_dev = (struct video_device *)fh->video_dev;
    if (video_dev->bfmode == VIDEO_PPBUF_MODE) {
        return video_ppbuf_free(video_dev->ppbuf, buf);
    } else {
        return video_buf_free(video_dev, buf);
    }
}
u32 uvc_buf_free_space(struct uvc_fh *fh)
{
    struct video_device *video_dev = (struct video_device *)fh->video_dev;
    if (video_dev->bfmode == VIDEO_PPBUF_MODE) {
        return video_ppbuf_free_size(video_dev->ppbuf);
    } else {
        return video_buf_free_space(video_dev);
    }
}
void uvc_buf_stream_finish(struct uvc_fh *fh, void *buf)
{
    struct video_device *video_dev = (struct video_device *)fh->video_dev;
    u32 size;

    if (!buf) {
        return;
    }
    if (video_dev->bfmode == VIDEO_PPBUF_MODE) {
        if (os_sem_valid(&fh->img_sem) && fh->image_req) {
            size = video_ppbuf_size(video_dev->ppbuf, buf);
            size = MIN(size, fh->img.size);
            memcpy(fh->img.buf, buf, size);
            fh->img.size = size;
            os_sem_post(&fh->img_sem);
            fh->image_req = false;
        }
        video_ppbuf_output(video_dev->ppbuf, buf);
    } else {
        if (os_sem_valid(&fh->img_sem) && fh->image_req) {
            size = video_buf_size(buf);
            size = MIN(size, fh->img.size);
            memcpy(fh->img.buf, buf, size);
            fh->img.size = size;
            os_sem_post(&fh->img_sem);
            fh->image_req = false;
        }
        video_buf_stream_finish(video_dev, buf);
    }
}
static int uvc_dev_reqbufs(void *_fh, struct uvc_reqbufs *b)
{
    /*
    struct uvc_fh *fh = (struct uvc_fh *)_fh;
    struct video_reqbufs vb_req = {0};

    vb_req.buf = b->buf;
    vb_req.size = b->size;

    return videobuf_reqbufs(fh->video_q, (struct video_reqbufs *)&vb_req);
    */
    return 0;
}

static int uvc_dev_qbuf(void *_fh, struct video_buffer *b)
{
    /*
    struct uvc_fh *fh = (struct uvc_fh *)_fh;

    return videobuf_qbuf(fh->video_q, b);
    */
    return 0;
}


static int uvc_dev_dqbuf(void *_fh, struct video_buffer *b)
{
    /*
    struct uvc_fh *fh = (struct uvc_fh *)_fh;

    if (fh->uvc_out) {
        return -ENODEV;
    }
    return videobuf_dqbuf(fh->video_q, b);
    */
    return 0;
}

int uvc_mjpg_stream_out(void *fd, int cnt, void *stream_list, int state)
{
    struct uvc_fh *fh = (struct uvc_fh *)fd;
    struct uvc_stream_list *list = (struct uvc_stream_list *)stream_list;
    int offset;
    int copy_size;
    int i;
    int err = 0;
    u32 tmp_jiffies = 0;

    if ((cnt < 0) || !list) {
        /*putchar('E');*/
        if (fh->buf) {
            uvc_buf_free(fh, fh->buf);
            fh->buf = NULL;
            if (state != STREAM_SOF) { // eof == 2 next frame start
                fh->drop_frame = 1;
            }
            err = -EINVAL;
        } else if (state == STREAM_SOF) {
            fh->drop_frame = 0;
        }
        goto _exit;
    }

    if (fh->drop_frame) {
        if (state == STREAM_EOF) {
            fh->drop_frame = 0;
        }
        goto _exit;
    }

    if (!fh->buf) {
        fh->free_size = uvc_buf_free_space(fh);
        if (fh->free_size > 1024) {
            fh->buf = uvc_buf_malloc(fh, fh->free_size);
            fh->free_size -= UVC_REC_JPG_ALIGN;//减512防止realloc时512对齐断言
        }
        if (!fh->buf) {
            err = -ENOMEM;
            goto _exit;
        }
        fh->b_offset = 0;
    }

    for (i = 0; i < cnt; i++) {
        if ((fh->b_offset + list[i].length + UVC_REC_JPG_HEAD_SIZE) > fh->free_size) {
            uvc_buf_free(fh, fh->buf);
            fh->buf = NULL;
            fh->drop_frame = 1;
            /*putchar('d');*/
            err = -EFAULT;
            goto _exit;
        }
#if USB_DMA_CP_ENABLE
        fh->dma_list[i].src_addr = list[i].addr;
        fh->dma_list[i].dst_addr = fh->buf + UVC_REC_JPG_HEAD_SIZE + fh->b_offset;
        fh->dma_list[i].len = list[i].length;
#else
        memcpy(fh->buf + UVC_REC_JPG_HEAD_SIZE + fh->b_offset, list[i].addr, list[i].length);
#endif
        fh->b_offset += list[i].length;
    }

#if USB_DMA_CP_ENABLE
    dma_task_copy(fh->dma_list, cnt);//注意：使用硬件dma copy需要异步接收推送数据，否则出现速度快数据错乱的问题
#endif

    if (state == STREAM_EOF) {
        fh->frame_cnt++;
        if (fh->real_fps && fh->fps && fh->real_fps < fh->fps) {
            u32 drop = fh->frame_cnt * (fh->fps - fh->real_fps) / fh->fps;
            if (fh->drop_frame_cnt != drop) {
                fh->drop_frame_cnt = drop;
                uvc_buf_free(fh, fh->buf);
                fh->buf = NULL;
                goto _exit;
            }
        }
        u32 req_size = ADDR_ALIGNE(fh->b_offset + UVC_REC_JPG_HEAD_SIZE, UVC_REC_JPG_ALIGN);
        fh->buf = uvc_buf_realloc(fh, fh->buf, req_size);
        if (fh->buf) {
            /*memset(fh->buf + fh->b_offset + UVC_REC_JPG_HEAD_SIZE, 0, req_size - fh->b_offset - UVC_REC_JPG_HEAD_SIZE);*/
            u32 *head = (u32 *)(fh->buf + UVC_REC_JPG_HEAD_SIZE);
            if (*head == UVC_JPEG_HEAD) {
                uvc_buf_stream_finish(fh, fh->buf);//注意：使用硬件dma copy需要异步接收推送数据，否则出现速度快数据错乱的问题
            } else {
                uvc_buf_free(fh, fh->buf);
            }
        }
        fh->buf = NULL;
    }

_exit:
    fh->streamoff = 0;
    return err;
}
static int uvc_stream_on(void *_fh, int index)
{
    int err = 0;
    u8 channel = 0;
    struct uvc_fh *fh = (struct uvc_fh *)_fh;

    os_sem_pend(&fh->sem, 0);

    if (fh->uvc_out) {
        os_sem_post(&fh->sem);
        return -EINVAL;
    }

    fh->eof = 1;
    fh->drop_frame = 0;

    if (fh->streamon) {
        os_sem_post(&fh->sem);
        return 0;
    }
    err = uvc_host_open_camera(fh->private_data);
    if (err) {
        printf("uvc_stream_on err\n");
        os_sem_post(&fh->sem);
        return err;
    }
    fh->streamon++;
    os_sem_post(&fh->sem);
    return err;
}
static int uvc_set_real_fps(void *_fh, u8 fp)
{
    int err;
    struct uvc_fh *fh = (struct uvc_fh *)_fh;
    fh->real_fps = fp;
    return 0;
}
static int uvc_stream_off(void *_fh, int index)
{
    int err = 0;
    struct uvc_fh *fh = (struct uvc_fh *)_fh;
    u32 time = jiffies + msecs_to_jiffies(10);

    os_sem_pend(&fh->sem, 0);
    if (fh->streamon && --fh->streamon) {
        os_sem_post(&fh->sem);
        return 0;
    }
    if (!fh->uvc_out) {
        uvc_host_close_camera(fh->private_data);
    }
    fh->streamoff = 1;
    while (fh->streamoff) {
        if (time_after(jiffies, time)) {
            break;
        }
    }

    if (fh->buf) {
        uvc_buf_free(fh, fh->buf);
        fh->buf = NULL;
        fh->b_offset = 0;
    }
    os_sem_post(&fh->sem);
    return err;

}

static int uvc_dev_init(const struct dev_node *node, void *_data)
{
    //struct uvc_platform_data *data = (struct uvc_platform_data *)_data;
    os_sem_create(&__this->sem, 1);
    INIT_LIST_HEAD(&__this->dev_list);
    return 0;
}

static bool uvc_dev_online(const struct dev_node *node)
{
    int err = 0;
    int id;

    id = uvc_host_online();
    if (id < 0) {
        return false;
    }

    return true;
}

static int uvc_dev_out(void *fd)
{
    struct uvc_fh *fh = (struct uvc_fh *)fd;

    os_sem_pend(&fh->sem, 0);

    fh->uvc_out = 1;

    os_sem_post(&fh->sem);
    return 0;

}

static int uvc_dev_open(const char *_name, struct device **device, void *arg)
{
    struct uvc_fh *fh = NULL;
    struct uvc_host_param param = {0};
    char name[8];
    int id;

    struct video_var_param_info *info = (struct video_var_param_info *)arg;
    id = info->f->uvc_id;
    os_sem_pend(&__this->sem, 0);
    list_for_each_uvc(fh) {
        if (fh->id == id) {
            fh->open++;
            fh->uvc_out = 0;
            *device = &fh->device;
            (*device)->private_data = fh;
            os_sem_post(&__this->sem);
            return 0;
        }
    }

    fh = (struct uvc_fh *)zalloc(sizeof(*fh));
    if (!fh) {
        os_sem_post(&__this->sem);
        return -ENOMEM;
    }
    sprintf(name, "uvc%d", id);
    param.name = name;//node->name;
    param.uvc_stream_out = uvc_mjpg_stream_out;
    param.uvc_out = uvc_dev_out;
    param.priv = fh;
    printf("open uvc name = %s \n", name);
    fh->private_data = uvc_host_open(&param);
    if (!fh->private_data) {
        free(fh);
        os_sem_post(&__this->sem);
        return -EINVAL;
    }
    fh->video_dev = (struct video_device *)info->priv;
    if (fh->video_dev->bfmode == VIDEO_PPBUF_MODE && !fh->video_dev->ppbuf) {
        fh->video_dev->ppbuf = video_ppbuf_open();
        if (!fh->video_dev->ppbuf) {
            free(fh);
            log_e("video_ppbuf_open no men");
            return -ENOMEM;
        }
    }

    fh->id = id;
    fh->open = 1;
    os_sem_create(&fh->sem, 0);

    list_add_uvc(fh);

    os_sem_post(&fh->sem);
    os_sem_post(&__this->sem);
    *device = &fh->device;
    (*device)->private_data = fh;
    return 0;
}

static int uvc_querycap(struct uvc_fh *fh, struct uvc_capability *cap)
{
    int num;
    struct uvc_frame_info *reso_table;

    os_sem_pend(&fh->sem, 0);
    num = uvc_host_get_pix_table(fh->private_data, &reso_table);

    if (num > (sizeof(cap->reso) / sizeof(cap->reso[0]))) {
        num = sizeof(cap->reso) / sizeof(cap->reso[0]);
    }

    memcpy(cap->reso, reso_table, sizeof(cap->reso[0]) * num);
    cap->fmt = UVC_CAMERA_FMT_MJPG;
    cap->reso_num = num;
    cap->fps = uvc_host_get_fps(fh->private_data);
    fh->fps = cap->fps;
    //printf("defualt : uvc_querycap fps = %d, %d*%d\n", cap->fps, cap->reso[cap->reso_num - 1].width, cap->reso[cap->reso_num - 1].height);
    os_sem_post(&fh->sem);
    return 0;
}

static int uvc_req_processing_unit(struct uvc_fh *fh, struct uvc_processing_unit *pu)
{
    int err = 0;

    os_sem_pend(&fh->sem, 0);

    if (!fh->uvc_out) {
        err = uvc_host_req_processing_unit(fh->private_data, pu);
    }

    os_sem_post(&fh->sem);
    return err;
}

static int uvc_dev_reset(struct uvc_fh *fh)
{
    int err = 0;
    os_sem_pend(&fh->sem, 0);

    if (!fh->uvc_out) {
        err = uvc_force_reset(fh->private_data);
    }

    os_sem_post(&fh->sem);

    return err;
}

static int usb_ioctl(struct uvc_fh *fh, u32 cmd, void *arg)
{
    int err = 0;
    if (!fh) {
        return -EINVAL;
    }
    os_sem_pend(&fh->sem, 0);

    if (!fh->uvc_out) {
        err = uvc2usb_ioctl(fh->private_data, cmd, arg);
    }

    os_sem_post(&fh->sem);

    return err;
}

static int uvc_dev_ioctl(struct device *device, u32 cmd, u32 arg)
{
    int ret = 0;
    struct uvc_fh *fh = (struct uvc_fh *)device->private_data;

    switch (cmd) {
    case UVCIOC_QUERYCAP:
        ret = uvc_querycap(fh, (struct uvc_capability *)arg);
        break;
    case UVCIOC_SET_CAP_SIZE:
        ret = uvc_host_set_pix(fh->private_data, (int)arg);
        break;
    case UVCIOC_REQBUFS:
        ret = uvc_dev_reqbufs(fh, (struct uvc_reqbufs *)arg);
        break;
    case UVCIOC_DQBUF:
        ret = uvc_dev_dqbuf(fh, (struct video_buffer *)arg);
        break;
    case UVCIOC_QBUF:
        ret = uvc_dev_qbuf(fh, (struct video_buffer *)arg);
        break;
    case UVCIOC_STREAM_ON:
        ret = uvc_stream_on(fh, arg);
        break;
    case UVCIOC_STREAM_OFF:
        ret = uvc_stream_off(fh, arg);
        break;
    case UVCIOC_GET_IMAMGE:
        ret = uvc_img_cap(fh, arg);
        break;
    case UVCIOC_RESET:
        ret = uvc_dev_reset(fh);
        break;
    case UVCIOC_REQ_PROCESSING_UNIT:
        ret = uvc_req_processing_unit(fh, (struct uvc_processing_unit *)arg);
        break;
    case UVCIOC_SET_CUR_GRAY:
        /*set_uvc_gray(arg);*/
        break;
    case UVCIOC_SET_CUR_FPS:
        uvc_set_real_fps(fh, (u8)arg);
        break;
    default:
        ret = usb_ioctl(fh, cmd, (void *)arg);
        break;
    }

    return ret;
}

static int uvc_dev_close(struct device *device)
{
    struct uvc_fh *fh = (struct uvc_fh *)device->private_data;
    struct video_device *video_dev = (struct video_device *)fh->video_dev;

    if (fh && fh->open) {
        os_sem_pend(&fh->sem, 0);
        uvc_host_close(fh->private_data);
        if (video_dev->bfmode == VIDEO_PPBUF_MODE) {
            video_ppbuf_close(video_dev->ppbuf);
        }
        list_del_uvc(fh);
        free(fh);
    }
    return 0;
}



const struct device_operations uvc_dev_ops = {
    .init  =  uvc_dev_init,
    .open  =  uvc_dev_open,
    .ioctl =  uvc_dev_ioctl,
    .close =  uvc_dev_close,
    .online = uvc_dev_online,
};
#endif
