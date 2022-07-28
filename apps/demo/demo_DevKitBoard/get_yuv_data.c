#include "system/includes.h"
#include "os/os_api.h"
#include "app_config.h"
#include "get_yuv_data.h"
#include "asm/isc.h"
#include "video_ioctl.h"
#include "device.h"
#include "video.h"


#ifdef CONFIG_VIDEO_ENABLE

#if (defined CONFIG_VIDEO_ENABLE && (__SDRAM_SIZE__ >= (2 * 1024 * 1024)))

#define YUV_TEST 	0
#define YUV_BLOCK_SCAN	0

#if YUV_BLOCK_SCAN
#define YUV_BLOCK_TEST	1 //只获取Y的blok测试
#define YUV_TEST_WIDTH 	160 //Y
#define YUV_TEST_HEIGHT 160 //Y
#endif

#if YUV_BLOCK_SCAN
static u8 isc_buf[YUV_DATA_WIDTH * 64 * 3 / 2] SEC_USED(.sram) ALIGNE(32);
#endif

static u8 yuv_buf[YUV_DATA_WIDTH * YUV_DATA_HEIGHT * 3 / 2] ALIGNE(32);

#if YUV_BLOCK_TEST
struct yuv_app_info {
    volatile u8 *base;
    volatile u8 *y;
    volatile u8 yuv_used;
    volatile u16 w;
    volatile u16 h;
    volatile u16 line;
};

static struct yuv_app_info yuv_app = {0};

u8 *yuv420_block_buf_get(int *size)
{
    if (yuv_app.yuv_used) {
        *size = yuv_app.w * yuv_app.h;
        return yuv_app.base;
    } else {
        return NULL;
    }
}
void yuv420_block_buf_clean(void)
{
    yuv_app.yuv_used = 0;
}
#endif

static get_yuv_cfg  __info;


int yuv420_block_scan(void *info)
{
    struct yuv_block_info *yuv_info = (struct yuv_block_info *)info;
    /*
     *算法处理，禁止加延时函数！
     */
    if (yuv_info->start) { //帧起始第一个分块

    } else {

    }
#if YUV_BLOCK_TEST
    int i, j, k;
    if (!yuv_app.base) {
        yuv_app.base = yuv_buf;
        yuv_app.y = yuv_app.base;
        yuv_app.w = YUV_TEST_WIDTH;
        yuv_app.h = YUV_TEST_HEIGHT;
        yuv_app.line = 0;
    }
    if (yuv_app.yuv_used) {
        return 0;
    }
    if (yuv_info->start && yuv_app.line < yuv_app.h) { //根据传入的Y进行裁剪
        for (j = 0, k = 0; j < yuv_info->line && k < yuv_info->line; yuv_app.line++) {
            for (i = 0; i < yuv_app.w; i++) {
                *yuv_app.y++ = *(u8 *)(yuv_info->y + k * yuv_info->width + i * yuv_info->width / yuv_app.w);
            }
            k = (++j) * yuv_info->height / yuv_app.h;
        }
    } else if (!yuv_info->start && yuv_app.line >= (yuv_app.h - 8)) { //减8是位流为了防止裁剪不是整除导致行不足
        u8 *tmp = yuv_app.y - yuv_app.w;
        for (; yuv_app.line < yuv_app.h; yuv_app.line++) {
            for (i = 0; i < yuv_app.w; i++) {
                *yuv_app.y++ = *(u8 *)(tmp + i);
            }
        }
        yuv_app.y = yuv_app.base;
        yuv_app.line = 0;
        yuv_app.yuv_used = true;
        __asm_csync();
    }
#endif
    return 0;
}

static void get_yuv_task(void *priv)
{
    int err;
    struct video_format f = {0};
    struct yuv_image yuv = {0};
    void *dev;
    void (*cb)(u8 * data, u32 len, int width, int height);

    /*
     *step 1
     */
    dev = dev_open(YUV_DATA_SOURCE, NULL);
    if (!dev) {
        printf("open video err !!!!\n\n");
        return;
    }

#if 1
    /********************YUV内部加外部双BUF,上层处理时间可长一些************************/
    /*
     *step 2
     */
    f.type = VIDEO_BUF_TYPE_VIDEO_OVERLAY;
    f.pixelformat = VIDEO_PIX_FMT_YUV420;
#if YUV_BLOCK_SCAN
    f.static_buf = (u8 *)isc_buf;
    f.sbuf_size = sizeof(isc_buf);
    f.block_done_cb = yuv420_block_scan;
#endif
    err = dev_ioctl(dev, VIDIOC_SET_FMT, (u32)&f);
    if (err) {
        goto error;
    }

    /*
     *step 3
     */
    //BUFF配置1：使用外部BUFF，上层处理时间最大为摄像头的帧率时间 - 15ms，GC0308默认驱动处理最大时间35m
#if 1 //YUV分块(行)回调，用不到yuv一帧，则关闭
    yuv.addr = (u8 *)yuv_buf;
    yuv.size = sizeof(yuv_buf);
#endif
    err = dev_ioctl(dev, VIDIOC_SET_OVERLAY, (u32)&yuv);
    if (err) {
        goto error;
    }

#else
    /********************YUV支持外部静态BUF,上层处理时间较短************************/
    /*
     *step 2
     */
    f.type = VIDEO_BUF_TYPE_VIDEO_OVERLAY;
    f.pixelformat = VIDEO_PIX_FMT_YUV420;
    f.static_buf = (u8 *)yuv_buf;
    f.sbuf_size = sizeof(yuv_buf);
#if YUV_BLOCK_SCAN
    f.block_done_cb = yuv420_block_scan;
#endif
    err = dev_ioctl(dev, VIDIOC_SET_FMT, (u32)&f);
    if (err) {
        goto error;
    }

    //BUFF配置2：使用内部BUFF，内存节省450K,上层处理时间最大为摄像头的帧结束到帧起始时间，需要看摄像头帧同步信号波形测量，GC0308默认驱动处理最大时间16ms
    err = dev_ioctl(dev, VIDIOC_SET_OVERLAY, (u32)0);
    if (err) {
        goto error;
    }
#endif


#if YUV_TEST
    FILE *fd = fopen(CONFIG_ROOT_PATH"yuv_1.yuv", "wb+");
    int cnt = 0;
#endif
    while (1) {
        if (thread_kill_req()) {
            break;
        }

        cb = __info.cb;

        /*
         *step 4
         */
        err = dev_ioctl(dev, VIDIOC_GET_OVERLAY, (u32)&yuv);
        if (!err) {
            //do something in this step
#if YUV_TEST
            if (fd) {
                cnt++;
                fwrite(yuv.addr, 1, yuv.size, fd);
                if (cnt > 100) {
                    fclose(fd);
                    fd = NULL;
                }
                printf("write file ok : %d \n", cnt);
            }
#else
#if YUV_BLOCK_TEST
            int len;
            u8 *buf = yuv420_block_buf_get(&len);
            if (buf) {
                putchar('R');
                if (cb) {
                    cb(buf, len, yuv_app.w, yuv_app.h);
                }
                yuv420_block_buf_clean();
            }
#else
            if (cb) {
                cb(yuv.addr, yuv.size, YUV_DATA_WIDTH, YUV_DATA_HEIGHT);
            }
#endif
#endif
        }
#if YUV_TEST
        else if (fd) {
            fclose(fd);
            fd = NULL;
        }
#endif

        /*
         *step 5
         */
        dev_ioctl(dev, VIDIOC_CLEAR_OVERLAY, 0);
    }
error:
    /*
     *step 6
     */
    dev_close(dev);
    __info.init = 0;
}

void get_yuv_init(void (*cb)(u8 *data, u32 len, int width, int height))
{
    if (__info.init) {
        return;
    }
    __info.init = 1;
    __info.cb = cb;
    thread_fork("GET_YUV_TASK", 8, 2048, 0, &__info.pid, get_yuv_task, NULL);
}

void get_yuv_uninit(void)
{
    if (!__info.init) {
        return;
    }
    thread_kill(&__info.pid, KILL_WAIT);
    __info.cb = NULL;
    __info.init = 0;
}

void set_get_yuv_data_cb(void (*cb)(u8 *data, u32 len, int width, int height))
{
    __info.cb = cb;
}

#endif
#endif

