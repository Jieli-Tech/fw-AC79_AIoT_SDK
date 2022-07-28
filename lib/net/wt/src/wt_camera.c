#include "vt_bk.h"
#include "os/os_api.h"
#include "asm/jpeg_codec.h"
#include "os/os_api.h"


struct camera_obj {
    void *camera;
    pfun_camera_preview_cb preview_cb;
    pfun_camera_picture_cb picture_cb;
    void *userp;
    pfun_vt_task_cb task_cb;
    void *task_ctx;
    u8 take_photo_flag;
};

static struct camera_obj camera_object = {0};

extern void set_get_yuv_data_cb(void (*cb)(unsigned char *data, unsigned int len, int width, int height));
extern void get_yuv_init(void (*cb)(unsigned char *data, unsigned int len, int width, int height));
extern void get_yuv_uninit(void);
extern void vt_reconghttpRequest_http_request_break(void);

static void wt_preview_cb(unsigned char *data, unsigned int size, int width, int height)
{
    if (camera_object.take_photo_flag) {
        camera_object.take_photo_flag = 0;
        struct jpeg_encode_req req = {0};
        int ocr_flag = (width >= 640) ? 1 : 0;
        u32 buf_len = ocr_flag ? 60 * 1024 : 30 * 1024;
        u8 *jpg_img = malloc(buf_len);
        if (!jpg_img) {
            puts("jpeg buf not enough !!!\n");
            return;
        }

        req.q = 6;
        req.format = JPG_SAMP_FMT_YUV420;
        req.data.buf = jpg_img;
        req.data.len = buf_len;
        req.width = ocr_flag ? width : (width / 2);
        req.height = ocr_flag ? height : (height / 2);
        req.y = data;
        req.u = req.y + req.width * req.height;
        req.v = req.u + req.width * req.height / 4;

        if (0 == jpeg_encode_one_image(&req)) {
            printf("jpeg_encode_one_image ok\n");
            vt_reconghttpRequest_http_request_break();
            if (camera_object.picture_cb) {
                camera_object.picture_cb(req.data.buf, req.data.len, camera_object.userp);
            }
        }
        free(jpg_img);
    } else {
        if (camera_object.preview_cb) {
            camera_object.preview_cb(data, size, camera_object.userp);
        }
    }
}

static void *camera_open(void)//ok
{
    memset(&camera_object, 0, sizeof(camera_object));
    get_yuv_init(NULL);
    return &camera_object;
}

static int camera_release(void *dev)//ok
{
    struct camera_obj *obj = (struct camera_obj *)dev;
    get_yuv_uninit();
    printf("camera_release\n");
    return 0;
}

static int camera_set_params(void *dev, vt_camera_param_t *params)//ok
{
    struct camera_obj *obj = (struct camera_obj *)dev;
    //...
    return 0;
}

static int camera_get_params(void *dev, vt_camera_param_t *params)//ok
{
    struct camera_obj *obj = (struct camera_obj *)dev;
    //...
    return 0;
}

static int camera_set_preview_cb(void *dev, pfun_camera_preview_cb preview_cb, void *userp)
{
    struct camera_obj *obj = (struct camera_obj *)dev;
    obj->preview_cb = preview_cb;
    obj->userp = userp;
    return 0;
}

static int camera_set_picture_cb(void *dev, pfun_camera_picture_cb picture_cb, void *userp)
{
    struct camera_obj *obj = (struct camera_obj *)dev;
    obj->picture_cb = picture_cb;
    obj->userp = userp;
    return 0;
}

static int camera_start_preview(void *dev)
{
    struct camera_obj *obj = (struct camera_obj *)dev;
    set_get_yuv_data_cb(wt_preview_cb);
    return 0;
}

static int camera_stop_preview(void *dev)
{
    struct camera_obj *obj = (struct camera_obj *)dev;
    set_get_yuv_data_cb(NULL);
    return 0;
}

static int camera_take_picture(void *dev)
{
    struct camera_obj *obj = (struct camera_obj *)dev;
    obj->take_photo_flag = 1;
    return 0;
}

static void camera_task_cb(void *args)
{
    int ret = 0;
    struct camera_obj *obj = (struct camera_obj *)args;

    while (1) {
        /*通知SDK，线程已经running起来了，同时调用SDK的TASK任务接口*/
        ret = obj->task_cb(obj->task_ctx, E_VT_TASKS_RUNNING);
        /*函数返回了 <0 表明任务请求退出TASK*/
        if (ret < 0) {
            printf("task request exit\n");
            break;
        }
    }
    /*通知SDK，TASK确实退出了，用于状态确认*/
    ret = obj->task_cb(obj->task_ctx, E_VT_TASKS_EXITED);
    printf("task_cb exit\n");
}

/*注册给SDK，SDK调用此接口启动TASK*/
static int camera_start_task_cb(pfun_vt_task_cb taskcb, void *ctx)
{
    struct camera_obj *obj = (struct camera_obj *)&camera_object;
    obj->task_cb = taskcb;
    obj->task_ctx = ctx;
    return thread_fork("wt_camer_app", 20, 1024, 0, NULL, camera_task_cb, (void *)obj);
}

static const camera_ops_t mcamera_ops = {
    camera_open,
    camera_release,
    camera_set_params,
    camera_get_params,
    camera_set_preview_cb,
    camera_set_picture_cb,
    camera_start_preview,
    camera_stop_preview,
    camera_take_picture,
    camera_start_task_cb
};

const camera_ops_t *get_linux_camera(void)
{
    return &mcamera_ops;
}

