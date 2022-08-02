
#include "ui/ui.h"
#include "ui_api.h"
#include "ui_action_video.h"

#include "server/server_core.h"
#include "os/os_api.h"
#include "system/includes.h"
#include "app_config.h"
#include "storage_device.h"
#include "server/video_dec_server.h"
#include "video/video.h"
#include "yuv_to_rgb.h"


#ifdef CONFIG_UI_ENABLE

#if 1
#define log_info(x, ...)    printf("\r\n[UI]" x " ", ## __VA_ARGS__)
#else
#define log_info(...)
#endif

struct video_data_type {
    cbuffer_t cbuf;
    u8 *cycle_buf;
    u32 frame_size;
    u8 *rgb_buf;
    u8 *yoffset;
    u8 *uoffset;
    u8 *voffset;
    u8 *wbuf;
    u32 width;
    u32 height;
    OS_SEM sem;
};

struct video_dec_hdl {
#if 0
    u8 status;
    char curr_dir;
    u8 file_type;
    u8 ff_fr_times;
    u8 need_move;
    char *cur_path;
    int wait;
    int timer;
    int timeout;
#endif
    struct server *video_dec;
    FILE *file;
    u8 *audio_buf;
    u8 *video_buf;
    u32 lcd_size;
    struct lcd_interface *lcd_hdl;
    struct video_data_type video_data;
    u8 dec_end_flag;
};

#define AUDIO_DEC_BUF_SIZE		(50 * 1024)
#define VIDEO_DEC_BUF_SIZE		(200 * 1024)
/* #define UI_VIDEO_TEST */

static struct video_dec_hdl dec_hdl;


static void vdec_server_event_handler(void *priv, int argc, int *argv)
{
    struct video_dec_hdl *dec_handler = (struct video_dec_hdl *)priv;

    switch (argv[0]) {
    case VIDEO_DEC_EVENT_CURR_TIME:
        /* 发送当前播放时间给UI */
        log_info("play time = %ds.\n", argv[1]);
        break;

    case VIDEO_DEC_EVENT_END:
        log_info("video dec end\n");
        dec_handler->dec_end_flag = 1;
        os_sem_post(&dec_handler->video_data.sem);
        break;

    case VIDEO_DEC_EVENT_ERR:
        /* 解码出错，如果存储设备没有被拔出则播放前一个文件 */
        log_info("video dec err\n");
        break;
    }
}

static int get_yuv_data(void *hdl, void *frame)
{
    struct video_data_type *video_data = &dec_hdl.video_data;
    struct YUV_frame_data *_frame = (struct YUV_frame_data *)frame;
    u32 wlen;
    u8 *wbuf;
    u32 ysize = 0;

    video_data->width  = _frame->width;
    video_data->height = _frame->height;

    if (_frame->line_num == 0) {
        video_data->yoffset = NULL;
        video_data->uoffset = NULL;
        video_data->voffset = NULL;
        video_data->wbuf = NULL;
        video_data->frame_size = _frame->width * _frame->height * 3 / 2;
        wbuf = cbuf_write_alloc(&video_data->cbuf, &wlen);

        if ((wlen >= video_data->frame_size) && wbuf) {
            video_data->wbuf = wbuf;
            video_data->yoffset = wbuf;
            video_data->uoffset = wbuf + _frame->width * _frame->height;
            video_data->voffset = wbuf + _frame->width * _frame->height + _frame->width * _frame->height / 4;
        }
    }

    if (video_data->wbuf) {
        ysize = _frame->width * _frame->data_height;
        memcpy(video_data->yoffset, _frame->y, ysize);
        memcpy(video_data->uoffset, _frame->u, ysize / 4);
        memcpy(video_data->voffset, _frame->v, ysize / 4);

        video_data->yoffset += ysize;
        video_data->uoffset += ysize / 4;
        video_data->voffset += ysize / 4;

        if ((_frame->line_num + _frame->data_height) >= _frame->height) {
            /* static int i = 0; */
            /* log_info("frame = %d.\n", i++); */
            cbuf_write_updata(&video_data->cbuf, video_data->frame_size);
            os_sem_post(&video_data->sem);
            video_data->wbuf = NULL;
        }
    }

    return 0;
}

static void display_task(void *priv)
{
    u8 *rbuf;
    u32 rlen;
    struct video_dec_hdl *dec_handler = (struct video_dec_hdl *)priv;
    struct video_data_type *video_data = &dec_handler->video_data;

#ifdef UI_VIDEO_TEST
    void *y_file;
    void *r_file;
    const char *y_file_path = "storage/sd0/C/test1.yuv";
    const char *r_file_path = "storage/sd0/C/test2.rgb";

    y_file = fopen(y_file_path, "w+");
    if (!y_file) {
        log_info("open y_file err\n");
        return;
    }

    r_file = fopen(r_file_path, "w+");
    if (!r_file) {
        log_info("open r_file err\n");
        return;
    }
#endif

    for (;;) {
        os_sem_pend(&video_data->sem, 50);

        if (dec_handler->dec_end_flag == 1) {
            dec_handler->dec_end_flag = 0;
#ifdef UI_VIDEO_TEST
            fclose(y_file);
            fclose(r_file);
#endif
            stop_play_video();
            fclose(dec_handler->file);
            dec_handler->file = NULL;
            return;
        }

        rbuf = cbuf_read_alloc(&video_data->cbuf, &rlen);

        if (rbuf && rlen) {
            yuv420p_quto_rgb565(rbuf, video_data->rgb_buf, video_data->width, video_data->height, 1);

#ifdef UI_VIDEO_TEST
            fwrite(y_file, rbuf, rlen);
            fwrite(r_file, video_data->rgb_buf, dec_handler->lcd_size * 2);
#endif

            /* log_info("start show_frame = %d.\n", i); */
            dec_handler->lcd_hdl->draw(video_data->rgb_buf, dec_handler->lcd_size * 2, 1);
            /* static int i = 0; */
            /* log_info("end show_frame = %d.\n", i); */
            /* i++; */
            cbuf_read_updata(&video_data->cbuf, rlen);
        }
    }
}

static void free_video_memory(struct video_dec_hdl *dec_handler)
{
    log_info("free_video_memory.\n");
    free(dec_handler->video_data.rgb_buf);
    free(dec_handler->video_data.cycle_buf);
    free(dec_handler->audio_buf);
    free(dec_handler->video_buf);
    dec_handler->video_data.rgb_buf = NULL;
    dec_handler->video_data.cycle_buf = NULL;
    dec_handler->audio_buf = NULL;
    dec_handler->video_buf = NULL;
}

static int malloc_video_memory(struct video_dec_hdl *dec_handler)
{
    dec_handler->video_data.rgb_buf = malloc(dec_handler->lcd_size * 2);
    dec_handler->video_data.cycle_buf = malloc(dec_handler->lcd_size * 3 / 2);
    if ((!dec_handler->video_data.rgb_buf) || (!dec_handler->video_data.cycle_buf)) {
        free_video_memory(dec_handler);
        return -ENOMEM;
    }

    dec_handler->audio_buf = malloc(AUDIO_DEC_BUF_SIZE);
    dec_handler->video_buf = malloc(VIDEO_DEC_BUF_SIZE);
    if ((!dec_handler->audio_buf) || (!dec_handler->video_buf)) {
        free_video_memory(dec_handler);
        return -ENOMEM;
    }

    return 0;
}

int start_play_video(const char *path)
{
    int err;
    struct video_dec_arg arg = {0};
    union video_dec_req req = {0};
    struct lcd_info lcd_info = {0};
    struct video_dec_hdl *dec_handler = &dec_hdl;

#ifdef UI_VIDEO_TEST
    log_info("waiting sd to mount......\n");
    while (!storage_device_ready());
#endif

    log_info("avi decode start.\n");

    memset(dec_handler, 0, sizeof(struct video_dec_hdl));

    dec_handler->file = fopen(path, "r");
    if (dec_handler->file == NULL) {
        log_info("open avi file err\n");
        return -EFAULT;
    }

    dec_handler->lcd_hdl = lcd_get_hdl();
    ASSERT(dec_handler->lcd_hdl != NULL);

    log_info("lcd_hdl get success.\n");
    dec_handler->lcd_hdl->get_screen_info(&lcd_info);
    dec_handler->lcd_hdl->set_draw_area(0, lcd_info.width - 1, 0, lcd_info.height - 1);
    log_info("lcd size w = %d.  h = %d.\n", lcd_info.width, lcd_info.height);
    dec_handler->lcd_size = lcd_info.width * lcd_info.height;

    if (malloc_video_memory(dec_handler) != 0) {
        return -ENOMEM;
    }

    os_sem_create(&dec_handler->video_data.sem, 0);
    cbuf_init(&dec_handler->video_data.cbuf, dec_handler->video_data.cycle_buf, dec_handler->lcd_size * 3 / 2);

    arg.dev_name = "video_dec";
    arg.audio_buf_size = AUDIO_DEC_BUF_SIZE;
    arg.video_buf_size = VIDEO_DEC_BUF_SIZE;
    arg.audio_buf = dec_handler->audio_buf;
    arg.video_buf = dec_handler->video_buf;

    dec_handler->video_dec = server_open("video_dec_server", &arg);
    if (!dec_handler->video_dec) {
        log_info("video_dec_server open fail\n");
        free_video_memory(dec_handler);
        return -EFAULT;
    }
    server_register_event_handler(dec_handler->video_dec, dec_handler, vdec_server_event_handler);

    req.dec.preview = 0;
    req.dec.volume = 100;
    req.dec.get_yuv = get_yuv_data;
    req.dec.file = dec_handler->file;

    err = server_request(dec_handler->video_dec, VIDEO_REQ_DEC_START, &req);
    if (err) {
        log_info("start dec err = %d.\n", err);
        server_close(dec_handler->video_dec);
        dec_handler->video_dec = NULL;
        free_video_memory(dec_handler);
        return -EFAULT;
    }
    log_info("start dec succ\n");

    return thread_fork("display_task", 20, 1024, 0, NULL, display_task, dec_handler);
}

int stop_play_video(void)
{
    struct video_dec_hdl *dec_handler = &dec_hdl;
    int err = -1;

    if (dec_handler->video_dec) {
        err = server_request(dec_handler->video_dec, VIDEO_REQ_DEC_STOP, NULL);
        log_info("server close.\n");
        server_close(dec_handler->video_dec);
        dec_handler->video_dec = NULL;
        free_video_memory(dec_handler);
    }

    return err;
}

int pause_play_video(void)
{
    struct video_dec_hdl *dec_handler = &dec_hdl;
    int err = -1;

    if (dec_handler->video_dec) {
        err = server_request(dec_handler->video_dec, VIDEO_REQ_DEC_PLAY_PAUSE, NULL);
    }
    return err;
}

void ui_server_init(void)
{
    /* ui_init(&ui_cfg_data); */
    extern const struct ui_devices_cfg ui_cfg_data;
    lcd_ui_init(&ui_cfg_data);
}

#endif


