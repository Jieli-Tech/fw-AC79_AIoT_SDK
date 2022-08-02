
#include "system/includes.h"
#include "server/video_server.h"
#include "app_config.h"
#include "asm/debug.h"
#include "asm/osd.h"
#include "asm/isc.h"
#include "asm/spi.h"
#include "app_database.h"
#include "storage_device.h"
#include "server/ctp_server.h"
#include "os/os_api.h"
#include "camera.h"
#include "stream_protocol.h"
#include "yuv_soft_scalling.h"

#ifdef CONFIG_NET_ENABLE
#include "net_server.h"
#include "server/net_server.h"
#endif

#ifdef CONFIG_SPI_VIDEO_ENABLE

#if (__SDRAM_SIZE__ < (2*1024*1024))
#error "sdram size no enough , sdram size must >= (2*1024*1024), noto : CONFIG_VIDEO1_ENABLE !!!"
#endif

#define SPI_VIDEO_SPIDEV_NAME		"spi1"
#define SPI_VIDEO_USE_SPI_RECV		0   //1使用SPI接收(需要软件分离YUV),0使用硬件接收YUV
#define SPI_VIDEO_YUV_USE_DMA   	1   //使用DMA拷贝
#define SPI_VIDEO_REVERSAL   		90  //spi镜头翻转 0 90 180 270
#define SPI_VIDEO_XCLK_12M_EN		0   //spi镜头的xclk时钟12M/24M, 1:12M, 0:24M
#define SPI_VIDEO_ONLY_Y			0   //1:只有Y数据
#define SPI_APP_USE_ORIGINAL_SIZE	1   //1:手机APP使用摄像头尺寸不缩放
#define SPI_YUV_ALWAYS_ON           0   //1:YUV一直打开

/////////////////摄像头时钟输出/////////////////////////
#ifdef SPI_VIDEO_XCLK_12M_EN
#ifdef CONFIG_CPU_WL80  //AC79N
#define SPI_VIDEO_XCLK_PORT		IO_PORTC_00
#else
#define SPI_VIDEO_XCLK_PORT		IO_PORTC_00
#endif
#endif

#define SPI_VIDEO_POWERDOWN_IO  IO_PORTH_09

/////////////////BT656硬件接收/////////////////////////
#define SPI_VIDEO_PCLK_PORT		IO_PORTC_08
#define SPI_VIDEO_MISO_PORT		IO_PORTC_07
///////////////////////////////////////////

/**********SPI state ************/
#define SPI_VIDEO_IDEL		0x0
#define SPI_VIDEO_OPEN		0x1
#define SPI_VIDEO_OPENING	0x2
#define SPI_VIDEO_CLOSE		0x3
#define SPI_VIDEO_CLOSEING	0x4
#define SPI_VIDEO_RESEST	0x5

/**********SPI video ************/
#define SPI_LSTART	ntohl(0xff000080)
#define SPI_LEND	ntohl(0xff00009d)
#define SPI_FSTART	ntohl(0xff0000ab)//ntohl(0xff0000a9)
#define SPI_FEND	ntohl(0xff0000b6)
#define SPI_FHEAD_SIZE	4
#define SPI_FEND_SIZE	4
#define SPI_LHEAD_SIZE	4
#define SPI_LEND_SIZE	4

#define SPI_CAMERA_W						240
#define SPI_CAMERA_H						320

#if SPI_VIDEO_ONLY_Y
#define SPI_CAMERA_ONLINE_SIZE				(SPI_CAMERA_W + SPI_LHEAD_SIZE + SPI_LEND_SIZE)//y
#else
#define SPI_CAMERA_ONLINE_SIZE				(SPI_CAMERA_W * 4 / 2 + SPI_LHEAD_SIZE + SPI_LEND_SIZE)//yuv422
#endif

#define SPI_CAMERA_ONEFRAM_SIZE				(SPI_CAMERA_ONLINE_SIZE * SPI_CAMERA_H + SPI_FHEAD_SIZE + SPI_FEND_SIZE)
#define SPI_CAMERA_ONLINE_YUV422_SIZE		(SPI_CAMERA_W * 4 / 2)
#define SPI_CAMERA_ONFRAM_YUV422_SIZE		(SPI_CAMERA_ONLINE_YUV422_SIZE * SPI_CAMERA_H)
#define SPI_CAMERA_ONFRAM_YUV420_SIZE		(SPI_CAMERA_W * SPI_CAMERA_H * 3 / 2)
#define SPI_CAMERA_ONFRAM_ONLY_Y_SIZE		(SPI_CAMERA_W * SPI_CAMERA_H)

#define SPI_CAMERA_FIRST_DMA_SIZE			(SPI_CAMERA_ONEFRAM_SIZE > SPI_MAX_SIZE ? (SPI_MAX_SIZE / SPI_CAMERA_ONLINE_SIZE * SPI_CAMERA_ONLINE_SIZE + SPI_FHEAD_SIZE) : SPI_CAMERA_ONEFRAM_SIZE)
#define SPI_CAMERA_MAX_DMA_CNT				(SPI_CAMERA_ONEFRAM_SIZE > SPI_MAX_SIZE ? (SPI_MAX_SIZE / SPI_CAMERA_ONLINE_SIZE * SPI_CAMERA_ONLINE_SIZE) : SPI_MAX_SIZE)

#define SPI_CAMERA_BUFF_FPS	2
#if SPI_VIDEO_USE_SPI_RECV
static u8 spi_video_buf[SPI_CAMERA_ONEFRAM_SIZE * SPI_CAMERA_BUFF_FPS] SEC_USED(.sram) ALIGNE(32);
#else
static u8 spi_video_buf[SPI_CAMERA_ONFRAM_YUV420_SIZE * SPI_CAMERA_BUFF_FPS] SEC_USED(.sram) ALIGNE(32);
#endif

#define SPI_VIDEO_BUFF_SIZE		(100*1024)

struct spi_video {
    void *spi_hdl;
    void *spi_wakeup;
    void *camera_hdl;
    u8 *fbuf;
    u32 fsize;
    u8 frame;
    u32 out_frame;
    u16 width;
    u16 height;
    struct spi_user su;
    u8 reinit;
    u8 init;
    u8 state;
    u8 cpu1_wakeup;
    u8 app_state;
    u8 yuv_flow;
    u32 pid;
    struct server *net_video_rec;
    u8 *video_buf;
    void *net_priv;
    void (*yuv_cb)(u8 *buf, u32 size, int width, int height);

#if !SPI_VIDEO_USE_SPI_RECV
    u8 *frame_buf;
    u32 frame_buf_size;
    u32 frame_buf_cnt;
    u8 frame_done;
    u8 dma_copy_id;
    OS_SEM sem;
#endif

} spi_video_info = {0};

extern void bt656_one_line_init(u32 y, u32 yuv_size, int width, int height, sen_in_format_t mode);
extern void bt656_one_line_io_init(u32 pclk_gpio, u32 miso_gpio);
extern void bt656_one_line_framedone_reg(int *fdone_cb, void *parm);
extern void bt656_one_line_en(char en);
extern void bt656_one_line_exit(void);
void sdfile_save_test(char *buf, int len, char one_file, char close);

int spi_video_task_create(void *priv);
static int spi_camera_close(void);

static struct yuv_block_info spi_blk_info = {0};
static int (*yuv_done_callback)(void *priv, int id, void *blk_info);
static int *yuv_done_callback_priv;
/*int mjpg_yuv_frame_done_callback_set(void *func, void *priv)*/
int yuv_frame_done_callback_set(void *func, void *priv)
{
    yuv_done_callback = (int(*)(void *, int, void *))func;
    yuv_done_callback_priv = (int *)priv;
    __asm_csync();
    return 1;
}
static void yuv_frame_done_callback_clear(void)
{
    yuv_done_callback = NULL;
    yuv_done_callback_priv = NULL;
    __asm_csync();
}
#if !SPI_VIDEO_USE_SPI_RECV
int bt656_one_line_framedone_callback(void *priv, int id, void *blk_info)
{
    struct yuv_block_info *block_info = (struct yuv_block_info *)blk_info;
    char *buf = (char *)block_info->y;
    int len = block_info->ylen + block_info->ulen + block_info->vlen;

    if (!spi_video_info.frame_done && buf) {
#if SPI_VIDEO_YUV_USE_DMA
        spi_video_info.dma_copy_id = dma_copy_async(spi_video_info.frame_buf + spi_video_info.fsize, buf, len);
#else
        memcpy(spi_video_info.frame_buf + spi_video_info.fsize, buf, len);
#endif
        spi_video_info.frame_done = TRUE;
        os_sem_post(&spi_video_info.sem);
        return 0;
    }
    return -EINVAL;
}
#endif
int get_wl80_spec_jpeg_q_val(void)
{
    return 7;//spi video Q值返回
}
static int spi_camera_event(void *priv, int argc, int *argv)
{
    switch (argv[0]) {
    case VIDEO_SERVER_PKG_ERR:
        spi_camera_close();
        log_e("spi video_server_pkg_err\n");
        break;
    default:
        break;
    }
    return 0;
}
void spi_video_yuv_cb_reg(void (*cb)(u8 *buf, u32 size, int width, int height))
{
    spi_video_info.yuv_cb = cb;
}
int spi_camera_width_get(void)
{
    int w;
#if ((SPI_VIDEO_REVERSAL == 0) || ((SPI_VIDEO_REVERSAL == 180)))
    w = SPI_CAMERA_W;
#else
    w = SPI_CAMERA_H;
#endif
#if SPI_APP_USE_ORIGINAL_SIZE
    return w;
#endif
    return (w / 32 * 32 * 2) / 32 * 32;//32对齐后，2倍放大
}
int spi_camera_height_get(void)
{
    int h;
#if ((SPI_VIDEO_REVERSAL == 0) || ((SPI_VIDEO_REVERSAL == 180)))
    h = SPI_CAMERA_H;
#else
    h = SPI_CAMERA_W;
#endif
#if SPI_APP_USE_ORIGINAL_SIZE
    return h;
#endif
    return (h / 16 * 16 * 3 / 2) / 16 * 16;//16对齐后，1.5倍放大
}
int spi_video_wait_done(void)
{
    u32 to = 300;
    while ((spi_video_info.state == SPI_VIDEO_OPENING || spi_video_info.state == SPI_VIDEO_RESEST) && --to) {
        os_time_dly(1);
    }
    return 0;
}
static int spi_camera_reset(void)
{
    int err;
    union video_req req = {0};

    if (spi_video_info.camera_hdl) {
        dev_ioctl(spi_video_info.camera_hdl, CAMERA_SET_SENSOR_RESET, 0);
        spi_video_info.state = SPI_VIDEO_OPEN;
    } else if (spi_video_info.net_video_rec) {
        req.rec.channel = 1;
        req.rec.state = 0;//传参
        req.rec.priv = (void *)CAMERA_SET_SENSOR_RESET; //镜头复位命令
        err = server_request(spi_video_info.net_video_rec, VIDEO_REQ_DEVICE_IOCTL, &req);//请求镜头复位IOCTL
        if (err != 0) {
            printf("spi_camera_reset err %d  \n", err);
            return -EINVAL;
        }
        spi_video_info.state = SPI_VIDEO_OPEN;
    }
    return 0;
}
static int net_stream_open(void *net_priv)
{
    int err = 0;
#ifdef CONFIG_NET_ENABLE
    union video_req req = {0};
    struct video_text_osd text_osd = {0};
    struct video_graph_osd graph_osd;
    u16 max_one_line_strnum;

    if (!spi_video_info.video_buf) {
        spi_video_info.video_buf = malloc(SPI_VIDEO_BUFF_SIZE);
        if (!spi_video_info.video_buf) {
            err = -EINVAL;
            goto error;
        }
    }
    if (spi_video_info.net_video_rec) {
        printf("net_stream_open is open \n");
        return 0;
    } else {
        spi_video_info.net_video_rec = server_open("video_server", "video1.0");
        if (!spi_video_info.net_video_rec) {
            err = -EINVAL;
            goto error;
        }
        server_register_event_handler(spi_video_info.net_video_rec, (void *)0, spi_camera_event);
    }
    req.rec.bfmode = VIDEO_PPBUF_MODE;
    req.rec.wl80_spec_mode = VIDEO_WL80_SPEC_DOUBLE_REC_MODE;
    req.rec.picture_mode = 0;//非绘本模式
    req.rec.isc_sbuf = NULL;
    req.rec.sbuf_size = 0;
    req.rec.block_done_cb = NULL;
    req.rec.camera_type = VIDEO_CAMERA_NORMAL;
    req.rec.channel = 1;
    req.rec.width 	= spi_camera_width_get();
    req.rec.height 	= spi_camera_height_get();
    req.rec.state 	= VIDEO_STATE_START;
    req.rec.fpath 	= CONFIG_REC_PATH_0;
    req.rec.format 	= NET_VIDEO_FMT_AVI;
    req.rec.fname   = "vid_***.avi";
    req.rec.quality = VIDEO_MID_Q;
    req.rec.fps = 0;
    req.rec.real_fps = 15;
    req.rec.net_par.net_audrt_onoff  = 0;
    req.rec.net_par.net_vidrt_onoff = 1;
    req.rec.audio.sample_rate = 0;
    req.rec.audio.channel 	= 1;
    req.rec.audio.channel_bit_map = 0;
    req.rec.audio.volume    = 0;
    req.rec.audio.buf = NULL;
    req.rec.audio.buf_len = 0;
    req.rec.pkg_mute.aud_mute = !db_select("mic");

    /*
    *码率，I帧和P帧比例，必须是偶数（当录MOV的时候才有效）,
    *roio_xy :值表示宏块坐标， [6:0]左边x坐标 ，[14:8]右边x坐标，[22:16]上边y坐标，[30:24]下边y坐标,写0表示1个宏块有效
    * roio_ratio : 区域比例系数
    */
    req.rec.abr_kbps = 0;
    req.rec.IP_interval = 0;


    /*
    *慢动作倍数(与延时摄影互斥,无音频); 延时录像的间隔ms(与慢动作互斥,无音频)
    */
    req.rec.slow_motion = 0;
    req.rec.tlp_time = 0;//db_select("gap");
    req.rec.buf = spi_video_info.video_buf;
    req.rec.buf_len = SPI_VIDEO_BUFF_SIZE;
    if (net_priv) {
        struct sockaddr_in *addr = ctp_srv_get_cli_addr(net_priv);
        if (!addr) {
            addr = cdp_srv_get_cli_addr(net_priv);
        }
#if (defined CONFIG_NET_UDP_ENABLE)
        sprintf(req.rec.net_par.netpath, "udp://%s:%d"
                , inet_ntoa(addr->sin_addr.s_addr)
                , _BEHIND_PORT);
#elif (defined CONFIG_NET_TCP_ENABLE)
        sprintf(req.rec.net_par.netpath, "tcp://%s:%d"
                , inet_ntoa(addr->sin_addr.s_addr)
                , _BEHIND_PORT);
#elif (defined CONFIG_USR_VIDEO_ENABLE)
        sprintf(req.rec.net_par.netpath, "usr://%s", CONFIG_USR_PATH);
#endif
    } else {
#if (defined CONFIG_USR_VIDEO_ENABLE)
        sprintf(req.rec.net_par.netpath, "usr://%s", CONFIG_USR_PATH);
#endif
    }
    err = server_request(spi_video_info.net_video_rec, VIDEO_REQ_REC, &req);
    if (err != 0) {
        printf("start rec err = %d\n", err);
        err = -EINVAL;
        goto error;
    }
    printf(">>>>>>spi video width=%d    height=%d\n", req.rec.width, req.rec.height);
    req.rec.priv = (void *)yuv_frame_done_callback_set;
    server_request(spi_video_info.net_video_rec, VIDEO_REQ_SET_ANOTHER_YUV_CALLBACK, &req);
#else
    printf("\nerr spi video open !!!\n\n");
#endif
    return err;
error:
    spi_video_info.state = SPI_VIDEO_CLOSE;
    if (spi_video_info.net_video_rec) {
        server_close(spi_video_info.net_video_rec);
        spi_video_info.net_video_rec = NULL;
    }
    return err;
}
static int net_stream_close(void)
{
    int err = 0;
    u8 state = spi_video_info.state;
    yuv_frame_done_callback_clear();
    if (spi_video_info.net_video_rec) {
        union video_req req = {0};
        req.rec.channel = 1;
        req.rec.state = VIDEO_STATE_STOP;
        err = server_request(spi_video_info.net_video_rec, VIDEO_REQ_REC, &req);
        if (err != 0) {
            printf("spi video stop rec err %d\n", err);
            spi_video_info.state = state;
            return -EINVAL;
        }
        server_close(spi_video_info.net_video_rec);
        spi_video_info.net_video_rec = NULL;
    }
    return err;
}
static int spi_camera_open(void *net_priv)
{
    int err = 0;
    if (spi_video_info.state == SPI_VIDEO_OPEN || spi_video_info.state == SPI_VIDEO_OPENING) {
        puts("spi video is open \n");
        return 0;
    }
    /*------SPI摄像头的CLK，OUP_CH2 24M-----*/
#ifdef SPI_VIDEO_POWERDOWN_IO
    gpio_direction_output(SPI_VIDEO_POWERDOWN_IO, 0);
#endif
#ifdef SPI_VIDEO_XCLK_12M_EN
#if SPI_VIDEO_XCLK_12M_EN
    gpio_set_output_clk(SPI_VIDEO_XCLK_PORT, 12);
#else
    gpio_output_channle(SPI_VIDEO_XCLK_PORT, CH3_PLL_24M);
#endif
#endif

    if (spi_video_info.state == SPI_VIDEO_RESEST) {
        printf("--->SPI_VIDEO_RESEST\n");
        spi_video_info.state = SPI_VIDEO_OPENING;
        return spi_camera_reset();
    }

    spi_video_info.state = SPI_VIDEO_OPENING;

    if (!net_priv) {
        spi_video_info.camera_hdl = dev_open("video1.0", NULL);
        if (!spi_video_info.camera_hdl) {
            printf("camera open err\n");
            err = -EINVAL;
            goto error;
        }
    } else {
        err = net_stream_open(net_priv);
    }
    spi_video_info.state = SPI_VIDEO_OPEN;
    printf("camera open ok\n");
    return 0;

error:
    spi_video_info.state = SPI_VIDEO_CLOSE;
    if (spi_video_info.net_video_rec) {
        server_close(spi_video_info.net_video_rec);
        spi_video_info.net_video_rec = NULL;
    }
    return err;
}
static int spi_camera_close(void)
{
    int err = 0;
    u8 state;

    if (!spi_video_info.init) {
        printf("spi_video no init \n");
        return -EINVAL;
    }
    if (spi_video_info.state == SPI_VIDEO_CLOSE || spi_video_info.state == SPI_VIDEO_CLOSEING) {
        puts("spi video is close \n");
        return 0;
    }
    state = spi_video_info.state;
    spi_video_info.state = SPI_VIDEO_CLOSEING;
#ifdef SPI_VIDEO_POWERDOWN_IO
    gpio_direction_output(SPI_VIDEO_POWERDOWN_IO, 1);
#endif
#ifdef SPI_VIDEO_XCLK_12M_EN
#if SPI_VIDEO_XCLK_12M_EN
    gpio_clear_output_clk(SPI_VIDEO_XCLK_PORT, 12);
#else
    gpio_clear_output_channle(SPI_VIDEO_XCLK_PORT, CH3_PLL_24M);
#endif
#endif

    if (spi_video_info.camera_hdl) {
        dev_close(spi_video_info.camera_hdl);
        spi_video_info.camera_hdl = NULL;
    }
    err = net_stream_close();
    spi_video_info.state = SPI_VIDEO_CLOSE;
    return err;
}
static void spi_video_task(void *priv)
{
    int ret;
    u8 *read_buf;
    u8 *read_addr;
    int max = 0;
    int wlen = 0;
    int yaddr, uaddr, vaddr;
    int offset, yoffset, uoffset, voffset;
    int sline_cnt;
    int online_data_size;
    int sample_wcnt;
    int sample_hcnt;
    int *head;
    int *end;
    int msg[4];


reinit:
    spi_video_info.net_priv				= priv;
    spi_video_info.reinit 				= FALSE;
    spi_video_info.frame 				= SPI_CAMERA_BUFF_FPS;
    spi_video_info.width 				= SPI_CAMERA_W;
    spi_video_info.height 				= SPI_CAMERA_H;
    spi_video_info.frame_done 			= FALSE;

#if SPI_VIDEO_USE_SPI_RECV
#if SPI_VIDEO_ONLY_Y
    spi_video_info.fsize 				= SPI_CAMERA_ONFRAM_ONLY_Y_SIZE;
#else
    spi_video_info.fsize 				= SPI_CAMERA_ONFRAM_YUV422_SIZE;
#endif
    spi_video_info.su.first_dma_size 	= SPI_CAMERA_FIRST_DMA_SIZE;
    spi_video_info.su.dma_max_cnt 		= SPI_CAMERA_MAX_DMA_CNT;
    spi_video_info.su.block_size 		= SPI_CAMERA_ONEFRAM_SIZE;
    spi_video_info.su.buf_size 			= spi_video_info.su.block_size * spi_video_info.frame;
    if (!spi_video_info.su.buf) {
        spi_video_info.su.buf = spi_video_buf;//malloc(spi_video_info.su.buf_size);
        if (!spi_video_info.su.buf) {
            printf("no mem spi video \n");
            goto exit;
        }
    }
    if (!spi_video_info.fbuf) {
        spi_video_info.fbuf = malloc(spi_video_info.fsize);
        if (!spi_video_info.fbuf) {
            printf("no mem spi video frame\n");
            goto exit;
        }
    }

    spi_video_info.spi_hdl = dev_open(SPI_VIDEO_SPIDEV_NAME, NULL);
    if (!spi_video_info.spi_hdl) {
        printf("spi open err \n");
        goto exit;
    }
#else
    spi_video_info.frame_buf_cnt 	= 0;
    spi_video_info.fsize 				= SPI_CAMERA_ONFRAM_YUV420_SIZE;
    spi_video_info.frame_buf_size		= SPI_CAMERA_ONFRAM_YUV420_SIZE * SPI_CAMERA_BUFF_FPS;
    if (!spi_video_info.frame_buf) {
        spi_video_info.frame_buf = spi_video_buf;//malloc(spi_video_info.frame_buf_size);
        if (!spi_video_info.frame_buf) {
            printf("no mem spi video \n");
            goto exit;
        }
    }
    if (!spi_video_info.fbuf) {
        spi_video_info.fbuf = malloc(spi_video_info.fsize);
        if (!spi_video_info.fbuf) {
            printf("no mem spi video frame\n");
            goto exit;
        }
    }
    os_sem_set(&spi_video_info.sem, 0);
    bt656_one_line_init((u32)spi_video_info.frame_buf,
                        spi_video_info.width * spi_video_info.height * 3 / 2,
                        spi_video_info.width,
                        spi_video_info.height,
                        SEN_IN_FORMAT_YUYV);
    bt656_one_line_io_init(SPI_VIDEO_PCLK_PORT, SPI_VIDEO_MISO_PORT);
    bt656_one_line_framedone_reg((int *)bt656_one_line_framedone_callback, NULL);
    bt656_one_line_en(1);
#endif
    u8 cnt = 5;
redo:
    if (spi_camera_open(spi_video_info.net_priv)) {
        if (--cnt > 0) {
            goto redo;
        }
        goto exit;
    }
#if SPI_VIDEO_USE_SPI_RECV
    dev_ioctl(spi_video_info.spi_hdl, IOCTL_SPI_SET_IRQ_CPU_ID, (u32)1);
    dev_ioctl(spi_video_info.spi_hdl, IOCTL_SPI_SET_USER_INFO, (u32)&spi_video_info.su);
#endif

    spi_video_info.init = TRUE;
    spi_video_info.out_frame = 0;
    printf("---> read_buf \n");
    while (1) {
        os_taskq_accept(ARRAY_SIZE(msg), msg);
#if SPI_VIDEO_USE_SPI_RECV
        ret = dev_ioctl(spi_video_info.spi_hdl, IOCTL_SPI_READ_DATA, (u32)&read_addr);
        if (thread_kill_req()) {
            break;
        }
        if (ret > 0) {
            head = (int *)read_addr; //校验
            end = (int *)&read_addr[ret - 4]; //校验
            if (*head == SPI_FSTART && *end == SPI_FEND) {
                spi_video_info.out_frame++;
                if (spi_video_info.out_frame <= 4) {//打开镜头后的前几帧出现光强变化：由黑变亮，在接收数据出错重新打开会使得APP出现闪光
                    dev_ioctl(spi_video_info.spi_hdl, IOCTL_SPI_FREE_DATA, 0);
                    continue;
                }
                wlen = 0;
                offset = 0;
                sline_cnt = 0;
                read_buf = read_addr;
                read_buf += SPI_FHEAD_SIZE;//偏移头
                online_data_size = SPI_CAMERA_ONLINE_SIZE - SPI_LHEAD_SIZE - SPI_LEND_SIZE;
                while (wlen < spi_video_info.fsize && offset < ret) {
                    head = (int *)read_buf;
                    if (*head == SPI_LSTART) {
                        sline_cnt++;
                        memcpy((int)read_addr + wlen, (int)head + SPI_LHEAD_SIZE, online_data_size);
                        wlen += online_data_size;
                        read_buf += SPI_CAMERA_ONLINE_SIZE;
                        offset += SPI_CAMERA_ONLINE_SIZE;
                        continue;
                    } else if (*head == SPI_FEND) {
                        break;
                    }
                    read_buf += SPI_LHEAD_SIZE;
                    offset += SPI_LHEAD_SIZE;
                    if (offset >= ret) {
                        break;
                    }
                }
                //printf("--->wlen=%d, %d, %d ",wlen,spi_video_info.fsize,sline_cnt,spi_video_info.height);
                if (wlen == spi_video_info.fsize && sline_cnt == spi_video_info.height) {
                    int out_w = spi_video_info.width;
                    int out_h = spi_video_info.height;
                    int src_w = spi_video_info.width;
                    int src_h = spi_video_info.height;
                    memcpy(spi_video_info.fbuf, read_addr, wlen);
                    putchar('R');

#if !SPI_VIDEO_ONLY_Y
                    read_buf = read_addr;
                    wlen = YUYV422ToYUV422p(spi_video_info.fbuf, read_buf, src_w, src_h);
                    wlen = YUV422pToYUV420p(read_buf, spi_video_info.fbuf, src_w, src_h);
                    wlen = YUV420p_REVERSAL(spi_video_info.fbuf, read_buf, src_w, src_h, &out_w, &out_h, SPI_VIDEO_REVERSAL);
                    if (!wlen) {
                        goto free_data;
                    }
                    //使用地址:read_buf , 使用长度:wlen
                    if (spi_video_info.yuv_cb) {
                        spi_video_info.yuv_cb(read_buf, wlen, out_w, out_h);
                    }
                    src_w = out_w;
                    src_h = out_h;
                    if (yuv_done_callback && spi_video_info.app_state) {
                        /*************宽32对齐，高16对齐，对齐后进行YUV提取****************/
                        YUV420p_ALIGN(read_buf, src_w, src_h, &out_w, &out_h, 32, 16);
                        spi_blk_info.y = (u8 *)read_buf;
                        spi_blk_info.u = spi_blk_info.y + out_w * out_h;
                        spi_blk_info.v = spi_blk_info.u + out_w * out_h / 4;
                        spi_blk_info.width = out_w;
                        spi_blk_info.height = out_h;
                        yuv_done_callback((void *)yuv_done_callback_priv, 1, (void *)&spi_blk_info);
                    } else {
                        /*
                        static u32 cnt = 0;
                        cnt++;
                        sdfile_save_test(read_buf, src_w * src_h * 3 / 2, 1, 0);
                        if(cnt == 100){
                        	sdfile_save_test(read_buf, src_w * src_h * 3 / 2, 1, 1);
                        }
                        */
                    }
#endif
                }
free_data:
                dev_ioctl(spi_video_info.spi_hdl, IOCTL_SPI_FREE_DATA, 0);
            } else {
                printf("---->SPI_FSTART = 0x%x , 0x%x \n", SPI_FSTART, SPI_FEND);
                printf("---->head       = 0x%x , 0x%x \n", *head, *end);
                printf("---->recv data err , reinit spi camera !!!\n");
                dev_ioctl(spi_video_info.spi_hdl, IOCTL_SPI_FREE_DATA, 0);
                spi_video_info.reinit = TRUE;//需要重新打开摄像头
                spi_video_info.state = SPI_VIDEO_RESEST;
                break;
            }
        }
#else
        ret = os_sem_pend(&spi_video_info.sem, 100);
        if (thread_kill_req()) {
            break;
        }
        if (!ret) {
            if (spi_video_info.frame_done) {
                u8 *yuv_buf = spi_video_info.frame_buf + spi_video_info.fsize;
                int out_w = spi_video_info.width;
                int out_h = spi_video_info.height;
                int src_w = spi_video_info.width;
                int src_h = spi_video_info.height;
                spi_video_info.frame_buf_cnt++;

#if SPI_VIDEO_YUV_USE_DMA
                dma_copy_async_wait(spi_video_info.dma_copy_id);
#endif
                read_buf = spi_video_info.fbuf;
                wlen = YUV420p_REVERSAL(yuv_buf, read_buf, src_w, src_h, &out_w, &out_h, SPI_VIDEO_REVERSAL);
                if (!wlen) {
                    goto free_data;
                }
                //使用地址:read_buf , 使用长度:wlen
                if (spi_video_info.yuv_cb) {
                    spi_video_info.yuv_cb(read_buf, wlen, out_w, out_h);
                }
                src_w = out_w;
                src_h = out_h;
                /*************宽32对齐，高16对齐，对齐后进行YUV提取****************/
                if (yuv_done_callback) {
                    YUV420p_ALIGN(read_buf, src_w, src_h, &out_w, &out_h, 32, 16);
                    spi_blk_info.y = (u8 *)read_buf;
                    spi_blk_info.u = spi_blk_info.y + out_w * out_h;
                    spi_blk_info.v = spi_blk_info.u + out_w * out_h / 4;
                    spi_blk_info.width = out_w;
                    spi_blk_info.height = out_h;
                    yuv_done_callback((void *)yuv_done_callback_priv, 1, (void *)&spi_blk_info);
                } else {
                    putchar('R');
                    /*static u32 cnt = 0;
                    cnt++;
                    sdfile_save_test(read_buf, src_w * src_h * 3 / 2, 1, cnt >= 100);*/
                }
free_data:
                spi_video_info.frame_done = FALSE;
            }
        }
#endif
    }

exit:
    if (spi_video_info.spi_hdl) {
        dev_close(spi_video_info.spi_hdl);
        spi_video_info.spi_hdl = NULL;
    }
    if (spi_video_info.reinit) {
        goto reinit;
    }
#if !SPI_VIDEO_USE_SPI_RECV
    bt656_one_line_exit();
#endif

    if (spi_video_info.net_video_rec) {
        spi_camera_close();
        spi_video_info.net_video_rec = NULL;
    }

    if (spi_video_info.su.buf) {
        /*free(spi_video_info.su.buf);*/
        spi_video_info.su.buf = NULL;
    }
    if (spi_video_info.fbuf) {
        free(spi_video_info.fbuf);
        spi_video_info.fbuf = NULL;
    }
    spi_video_info.init = FALSE;
    printf("\n\n spi_video_task exit \n\n");
}

int spi_video_task_create(void *priv)
{
    int to = 50;
    int err;

#if SPI_YUV_ALWAYS_ON
    if (spi_video_info.init) {
        printf("spi_video already open \n");
        if (priv) {
            return net_stream_open(priv);
        }
        return 0;
    }
#endif

    spi_video_wait_done();
    err = spi_camera_close();
    if (!err) {
        thread_kill(&spi_video_info.pid, 0);//关闭再重新打开
    }
    if (priv) {
        spi_video_info.app_state = TRUE;
        printf("---> app open \n");
    } else {
        spi_video_info.yuv_flow = TRUE;
        printf("---> yuv_flow open \n");
    }

#if CPU_CORE_NUM == 1
    if (!spi_video_info.cpu1_wakeup) {
        EnableOtherCpu();
        spi_video_info.cpu1_wakeup = true;
    }
#endif

#if !SPI_VIDEO_USE_SPI_RECV
    if (!os_sem_valid(&spi_video_info.sem)) {
        os_sem_create(&spi_video_info.sem, 0);
    }
#endif
    thread_fork("spi_video_task", 10, 512, 64, &spi_video_info.pid, spi_video_task, priv);
    while (!spi_video_info.init && to--) {
        os_time_dly(1);
    }
    if (spi_video_info.init) {
        return 0;
    }
    return -EINVAL;
}
int spi_video_task_init(void)
{
    printf("---> spi_video_task_init\n");
    spi_video_task_create(NULL);
    return 0;
}

int spi_video_task_kill(void *priv)
{
#if SPI_YUV_ALWAYS_ON
    if (priv) {
        return net_stream_close();
    }
#endif

    spi_video_wait_done();

#if !SPI_VIDEO_USE_SPI_RECV
    if (os_sem_valid(&spi_video_info.sem)) {
        os_sem_post(&spi_video_info.sem);
    }
#endif
    if (priv) {
        spi_video_info.app_state = FALSE;
        if (spi_video_info.yuv_flow) {
            thread_kill(&spi_video_info.pid, 0);//关闭再重新打开
            spi_video_task_create(NULL);
        }
        printf("---> app close \n");
    } else {
        spi_video_info.yuv_flow = FALSE;
        if (spi_video_info.app_state) {
            thread_kill(&spi_video_info.pid, 0);//关闭再重新打开
            spi_video_task_create(spi_video_info.net_priv);
        }
        printf("---> yuv_flow close \n");
    }
    if (spi_video_info.init && !spi_video_info.app_state && !spi_video_info.yuv_flow) {
        printf("---> spi video task kill \n");
        thread_kill(&spi_video_info.pid, 0);
#if !SPI_VIDEO_USE_SPI_RECV
        if (os_sem_valid(&spi_video_info.sem)) {
            os_sem_del(&spi_video_info.sem, 0);
        }
#endif
    }
    return 0;
}
#endif

void sdfile_save_test(char *buf, int len, char one_file, char close)
{
    static FILE *one_fd = NULL;
    if (one_file && !close) {
        if (!one_fd) {
            /*one_fd = fopen(CONFIG_ROOT_PATH"YUV/test/jpg_***.jpg", "w+");*/
            one_fd = fopen(CONFIG_ROOT_PATH"YUV/test/yuv_***.yuv", "w+");
        }
        if (one_fd && len) {
            fwrite(buf, 1, len, one_fd);
        }
    } else if (!one_file) {
        /*FILE *fd = fopen(CONFIG_ROOT_PATH"YUV/test/jpg_***.jpg", "w+");*/
        FILE *fd = fopen(CONFIG_ROOT_PATH"YUV/test/yuv_***.yuv", "w+");
        if (fd) {
            if (len) {
                fwrite(buf, 1, len, fd);
            }
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

