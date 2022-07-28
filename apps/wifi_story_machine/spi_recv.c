#include "app_config.h"
#include "system/includes.h"
#include "device/device.h"
#include "asm/spi.h"
#include "camera.h"
#include "device/gpio.h"

#ifdef CONFIG_SPI_VIDEO_ENABLE
/********************************************************************************************
*当SPI需要接收不固定数据量时，系统打开不可屏蔽方式，详情example的不可屏蔽中断使用方法
*1.首先系统打开不可屏蔽中断方式，SPI中断优先级设为7
*2.中断函数定义在内部sram方式
*3.中断函数用户自行处理
*4.注意：使用不可屏蔽方式，接收区间，禁止对flash进行写操作，读操作只能非代码区（除非系统程序代码跑sdram模式）
*********************************************************************************************/

/******************************以下为例子****************************************************/

//比如按照协议，对方主机SPI协议为如下,头4字节为信息，读取size可知道连续数据有多少字节。
//对方主机SPI协议发送为：先发4字节头信息，延时几个微秒就发送实际数据量，延时10几个毫秒，继续发送下一次数据，以此类推。
//注意结构体需要1字节对齐

struct spi_heads {
    u8 init;
    u8 reinit;
    u8 kill;
    u8 state;
    u32 err;
    volatile u32 fcnt;
    void *camera_hdl;
};

#define JL_SPI          JL_SPI2 //与spi设备对应：JL_SPI0 JL_SPI1 JL_SPI2
#define JL_SPI_NAME     "spi2" //与spi设备对应："spi1", "spi2"

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
#define SPI_FSTART	ntohl(0xff0000ab)
#define SPI_FEND	ntohl(0xff0000b6)
#define SPI_FHEAD_SIZE	4
#define SPI_FEND_SIZE	4
#define SPI_LHEAD_SIZE	4
#define SPI_LEND_SIZE	4

#define SPI_CAMERA_W						176
#define SPI_CAMERA_H						128

#define SPI_CAMERA_ONLINE_SIZE				(SPI_CAMERA_W + SPI_LHEAD_SIZE + SPI_LEND_SIZE)//y
#define SPI_CAMERA_ONFRAM_ONLY_Y_SIZE		(SPI_CAMERA_ONLINE_SIZE * SPI_CAMERA_H + SPI_FHEAD_SIZE + SPI_FEND_SIZE)
/**********SPI video ************/

#define SPI_FIRST_DMA_SIZE  (SPI_CAMERA_ONFRAM_ONLY_Y_SIZE + SPI_FHEAD_SIZE)//第一帧加第二帧的头部
#define SPI_DMA_SIZE        (SPI_CAMERA_ONFRAM_ONLY_Y_SIZE)//第一帧加第二帧的头部

#define RECV_SIZE_MAX   (SPI_CAMERA_ONFRAM_ONLY_Y_SIZE + SPI_FHEAD_SIZE + SPI_FEND_SIZE) //先了解协议1帧的最大字节数在多少再配置该值
#define RECV_FPS        5 //硬件缓冲帧切换使用，务必>=2帧

#define SPI_XCLK_PORT   IO_PORTC_00

#define SPI_BUF_USE_SRAM    1 //使用内部sram，静态
#define RECV_BUF_SIZE   (RECV_SIZE_MAX * RECV_FPS)

#if SPI_BUF_USE_SRAM
static volatile char recv_buf[RECV_BUF_SIZE] sec(.sram) ALIGNED(32);//spi硬件使用接收buf
#else
static volatile char *recv_buf = NULL;//spi硬件使用接收buf
#endif
static volatile char recv_read_cnt = 0;
static volatile char recv_write_cnt = 0;
static char *app_recv_buf = NULL;//应用层使用的buf
static volatile int app_recv_len = 0;
static struct spi_heads spi_head_info sec(.sram) = {0};

#define IRQ_USE_SEM_POST    0 //中断使用信号量通知
#if IRQ_USE_SEM_POST
static OS_SEM spi_sem;
#endif

/*#define SPI_IO_DBG*/

#ifdef SPI_IO_DBG
#define DBG_PORT_BIT    BIT(3)
#define DBG_PORT        JL_PORTB
#endif
//定义中断回调函数，注意：中断函数不能调用系统API（调试期间可加打印printf函数），调用函数需要用户自写，调用其他API也需要指定到指定在内部sram，即sec(.volatile_ram_code)

static ___interrupt void spi_irq_cb(void) //sec(.volatile_ram_code) //中断函数，指定在内部sram
{
    JL_SPI->CON |= BIT(14);//清除中断标记
    asm volatile("csync;");
#ifdef SPI_IO_DBG
    DBG_PORT->OUT ^= DBG_PORT_BIT;
#endif
    if (recv_buf) {
        char *next_head_buf;
        char *next_buf;
        next_head_buf = (char *)(recv_buf + recv_write_cnt * RECV_SIZE_MAX + SPI_CAMERA_ONFRAM_ONLY_Y_SIZE);
        if (((recv_write_cnt + 1) == RECV_FPS && !recv_read_cnt) || (recv_read_cnt && (recv_write_cnt + 1) == recv_read_cnt)) {
            goto next_recv;
        } else {
            recv_write_cnt = (++recv_write_cnt) >= RECV_FPS ? 0 : recv_write_cnt;
        }
next_recv:
        next_buf = (char *)(recv_buf + recv_write_cnt * RECV_SIZE_MAX);
        for (int i = 0; i < SPI_FHEAD_SIZE; i++) {
            *next_buf++ = *next_head_buf++;
        }
        JL_SPI->ADR = (u32)next_buf;//偏移地址
        JL_SPI->CNT = (u32)SPI_DMA_SIZE;//继续接收数据头
        spi_head_info.fcnt++;
#if IRQ_USE_SEM_POST
        os_sem_post(&spi_sem);//不可屏蔽不能使用2
#endif
    }
#ifdef SPI_IO_DBG
    DBG_PORT->OUT ^= DBG_PORT_BIT;
#endif

    asm volatile("csync;");
}

static int spi_camera_open(void)
{
    int err = 0;
    if (spi_head_info.state == SPI_VIDEO_OPEN || spi_head_info.state == SPI_VIDEO_OPENING) {
        puts("spi video is open \n");
        return 0;
    }
    spi_head_info.state = SPI_VIDEO_OPENING;
    /*------SPI摄像头的CLK，OUP_CH2 24M-----*/
    gpio_output_channle(SPI_XCLK_PORT, CH1_PLL_24M);

    if (spi_head_info.state == SPI_VIDEO_RESEST) {
        printf("--->SPI_VIDEO_RESEST\n");
        spi_head_info.state = SPI_VIDEO_OPENING;
        if (spi_head_info.camera_hdl) {
            dev_ioctl(spi_head_info.camera_hdl, CAMERA_SET_SENSOR_RESET, 0);
            spi_head_info.state = SPI_VIDEO_OPEN;
        }
    } else {
        spi_head_info.camera_hdl = dev_open("video1.0", NULL);
        if (!spi_head_info.camera_hdl) {
            printf("camera open err\n");
            err = -EINVAL;
            goto error;
        }
    }
    spi_head_info.state = SPI_VIDEO_OPEN;
    spi_head_info.init = 1;
    printf("camera open ok\n");
    return 0;

error:
    spi_head_info.state = SPI_VIDEO_CLOSE;
    return err;
}
static int spi_camera_close(void)
{
    if (!spi_head_info.init) {
        printf("spi_video no init \n");
        return -EINVAL;
    }
    if (spi_head_info.state == SPI_VIDEO_CLOSE || spi_head_info.state == SPI_VIDEO_CLOSEING) {
        puts("spi video is close \n");
        return 0;
    }
    spi_head_info.state = SPI_VIDEO_CLOSEING;

    gpio_clear_output_channle(SPI_XCLK_PORT, CH1_PLL_24M);

    if (spi_head_info.camera_hdl) {
        dev_close(spi_head_info.camera_hdl);
        spi_head_info.camera_hdl = NULL;
    }
    spi_head_info.state = SPI_VIDEO_CLOSE;
    return 0;
}
static void spi_recv_task(void *p)
{
    void *spi_hdl = NULL;
    int ret;
    int wlen, offset, online_data_size, sline_cnt;
    int *head, *end;
    char *read_buf;
    int now = 0, last = 0, last_fp = 0;
    int msg[4];
    u32 timer_get_ms(void);

    //1.打开spi设备
    spi_hdl = dev_open(JL_SPI_NAME, NULL);
    if (!spi_hdl) {
        printf("spi open err \n");
        goto exit;
    }
    //2.注册中断函数
    dev_ioctl(spi_hdl, IOCTL_SPI_SET_IRQ_FUNC, (u32)spi_irq_cb);

#if IRQ_USE_SEM_POST
    os_sem_create(&spi_sem, 0);
#endif

#if !SPI_BUF_USE_SRAM
    recv_buf = malloc(RECV_BUF_SIZE);
    if (!recv_buf) {
        printf("err spi no buf \n");
        goto exit;
    }
#endif // SPI_BUF_USE_SRAM

reinit:
    ret = spi_camera_open();
    if (ret) {
        printf("spi_camera_open err \n\n");
        goto exit;
    }

    /*for(int i = 0; i<32;i++){*/
    /*recv_buf[RECV_BUF_SIZE + i] = (u8)i;*/
    /*}*/

    //3.配置接收模式，开启中断，开启SPI
    JL_SPI->CON &= ~(BIT(10) | BIT(11) | BIT(2)); //1bit模式
    JL_SPI->CON |= BIT(12);//接收模式
    JL_SPI->CON |= BIT(13);//开启中断
    JL_SPI->CON |= BIT(0);//开启SPI

#ifdef SPI_IO_DBG
    DBG_PORT->DIR &= ~DBG_PORT_BIT;
    DBG_PORT->PU &= ~DBG_PORT_BIT;
    DBG_PORT->PD &= ~DBG_PORT_BIT;
    DBG_PORT->OUT &= ~DBG_PORT_BIT;
#endif
    spi_head_info.fcnt = 0;
    spi_head_info.err = 0;
    recv_write_cnt = 0;
    recv_read_cnt = 0;
    app_recv_buf = NULL;
    //4.配置接收地址，如对方主机SPI协议为如下，注意：写JL_SPI->CNT之前需保证此时对方SPI还没有发送数据，否则中途接收数据会出错
    JL_SPI->ADR = (u32)recv_buf;//配置硬件接收地址
    //此处根据实际应用选择接收字节数，比如收到4字节数据头，在数据头解析后可知下次应该收多少字节
    JL_SPI->CNT = (u32)SPI_FIRST_DMA_SIZE;//写该寄存器则正式启动硬件接收，例如此处先接收4字节，接收 SPI_DMA_FISRT_SIZE 字节完就进spi_irq_cb中断解析

    printf("---> SPI_FIRST_DMA_SIZE = %d ,SPI_DMA_SIZE = %d\n", SPI_FIRST_DMA_SIZE, SPI_DMA_SIZE);
    printf("---> spi_head_info.kill = %d \n", spi_head_info.kill);
    while (1) {
        //5.接收数据查询

#if IRQ_USE_SEM_POST
        ret = os_sem_pend(&spi_sem, 10);
#else
//        os_time_dly(1);//延时1个tick，防止看门狗。不允许丢帧则注释这行：当注释掉这个行代码时，该任务优先级需要8以下（否则影响系统其他任务调度）。
        taskYIELD();//可以使用空闲时候调度任务一次
#endif

        if (spi_head_info.kill) {
            spi_head_info.reinit = 0;
            printf("---> kill req \n");
            break;
        }

#if 1
        now = timer_get_ms();
        if (last && (now - last) >= 1000) {
            printf("---> camera fp = %d \n", spi_head_info.fcnt - last_fp);
            last_fp = spi_head_info.fcnt;
            last = now;
        } else if (!last) {
            last = now;
        }
#endif
        if (recv_read_cnt < recv_write_cnt || recv_read_cnt != recv_write_cnt) {
            app_recv_buf = (char *)(recv_buf + recv_read_cnt * RECV_SIZE_MAX);
            app_recv_len = SPI_CAMERA_ONFRAM_ONLY_Y_SIZE;
            head = (int *)app_recv_buf; //校验
            end = (int *)&app_recv_buf[app_recv_len - SPI_FEND_SIZE]; //校验
            if (*head == SPI_FSTART && *end == SPI_FEND) {
                read_buf = (char *)app_recv_buf;
                read_buf += SPI_FHEAD_SIZE;//偏移头
                wlen = 0;
                sline_cnt = 0;
                online_data_size = SPI_CAMERA_ONLINE_SIZE - SPI_LHEAD_SIZE - SPI_LEND_SIZE;
                while (wlen < SPI_CAMERA_ONFRAM_ONLY_Y_SIZE && read_buf < (read_buf + app_recv_len)) {
                    head = (int *)read_buf;
                    if (*head == SPI_LSTART) {
                        sline_cnt++;
                        memcpy((char *)app_recv_buf + wlen, (char *)((int)head + SPI_LHEAD_SIZE), online_data_size);
                        wlen += online_data_size;
                        read_buf += SPI_CAMERA_ONLINE_SIZE;
                        continue;
                    } else if (*head == SPI_FEND) {
                        break;
                    }
                    read_buf += SPI_LHEAD_SIZE;
                }
                if (sline_cnt == SPI_CAMERA_H) {
                    void send_yuv_data(char *buf, u16 len);
                    send_yuv_data(app_recv_buf, wlen);
                    //todo，数据处理
                }
                spi_head_info.err = 0;
            } else {
                spi_head_info.err++;
                printf("---> %d, %d \n", recv_read_cnt, recv_write_cnt);
                printf("--->recv err 0x%x, 0x%x \n", *head, *end);
                if (spi_head_info.err >= 5) {
                    spi_head_info.reinit = TRUE;//需要重新打开摄像头
                    spi_head_info.state = SPI_VIDEO_RESEST;
                    break;
                }
            }
            //读取++
            recv_read_cnt = (++recv_read_cnt) >= RECV_FPS ? 0 : recv_read_cnt;
        }

    }

exit:
    spi_camera_close();
    if (spi_hdl) { //关闭spi设备
        JL_SPI->CON &= ~BIT(0);
        dev_close(spi_hdl);
    }
    if (spi_head_info.reinit) {
        goto reinit;
    }
#if !SPI_BUF_USE_SRAM
    if (recv_buf) { //释放内存
        free(recv_buf);
        recv_buf = NULL;
    }
#endif
    spi_head_info.kill = 0;
    while (1) { //等待其他任务杀此任务
        os_time_dly(100);//延时100个tick，防止看门狗
    }
}

int spi_recv_task_init(void)
{
    spi_head_info.kill = 0;
    os_task_create(spi_recv_task, NULL, 20, 2048, 0, "spi_recv_task");
    return 0;
}
int spi_recv_task_kill(void)
{
    printf("---> spi_recv_task_kill\n");
    spi_head_info.kill = 1;
#if IRQ_USE_SEM_POST
    os_sem_post(&spi_sem);
#endif
    while (spi_head_info.kill) {
        os_time_dly(2);
    }
    task_kill("spi_recv_task");
    spi_head_info.kill = 0;
    return 0;
}
#endif

