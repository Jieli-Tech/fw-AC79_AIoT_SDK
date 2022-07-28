#include "app_config.h"
#include "system/includes.h"
#include "device/device.h"
#include "asm/spi.h"

/********************************************************************************************
*当SPI需要接收不固定数据量时，系统打开不可屏蔽方式，详情example的不可屏蔽中断使用方法
*1.首先系统打开不可屏蔽中断方式，SPI中断优先级设为7
*2.中断函数定义在内部sram方式
*3.中断函数用户自行处理
*4.注意：使用不可屏蔽方式，接收区间，禁止对flash进行写操作，读操作只能非代码区（除非系统程序代码跑sdram模式）
*********************************************************************************************/

/******************************以下为例子****************************************************/
#define JL_SPI          	JL_SPI2 //与spi设备对应：JL_SPI0 JL_SPI1 JL_SPI2
#define RECV_SIZE_MAX   	1024 //先了解协议1帧的最大字节数在多少再配置该值，此次例子为1024
#define RECV_SPI_HEAD_EN	1 // 1:接收数据有数据头部信息，0：每次接收的是固定数据量
#define RECV_FPS        	3 //硬件缓冲帧切换使用，务必>=2帧

#define SPI_BUF_USE_SRAM    1 //使用内部sram，静态

#if SPI_BUF_USE_SRAM
static volatile char recv_buf[RECV_SIZE_MAX * RECV_FPS] SEC_USED(.sram) ALIGNED(32);//spi硬件使用接收buf
#else
static volatile char *recv_buf = NULL;//spi硬件使用接收buf
#endif
static volatile int recv_buf_size = RECV_SIZE_MAX * RECV_FPS;//先了解协议最大字节数在多少再配置该值，此次例子为1024
static volatile int recv_buf_offset = 0;
static volatile char recv_read_cnt = 0;
static volatile char recv_write_cnt = 0;

static volatile char *app_recv_buf = NULL;//应用层使用的buf
static volatile int app_recv_len = 0;
static volatile char spi_head_ok = 0;

#if RECV_SPI_HEAD_EN
//比如按照协议，对方主机SPI协议为如下,头4字节为信息，读取size可知道连续数据有多少字节。
//对方主机SPI协议发送为：先发4字节头信息，延时几个微秒就发送实际数据量，延时10几个毫秒，继续发送下一次数据，以此类推。
//注意结构体需要1字节对齐

#pragma pack(push)//保存对齐状态
#pragma pack(1)//以 1 字节对齐
struct spi_heads {
    volatile u8 head;//固定 0xA5
    volatile u8 resv;//预留字节
    volatile u16 size;//接收字节长度，此处size<=65535（注意：SPI的DMA最大数据量为SPI_MAX_SIZE，当芯片为AC790N则SPI_MAX_SIZE = 65535，当芯片为AC791N则SPI_MAX_SIZE = 2^31
};
#pragma pack(pop)//恢复默认对齐状态

static struct spi_heads *spi_head;
#define SPI_HEAD_SIZE (sizeof(struct spi_heads))
#else
#define SPI_HEAD_SIZE	RECV_SIZE_MAX  //没有头部信息，则使用最大值的固定数据量
#endif

//定义中断回调函数，注意：中断函数不能调用系统API（调试期间可加打印printf函数），调用函数需要用户自写，调用其他API也需要指定到指定在内部sram，即sec(.volatile_ram_code)
static ___interrupt void spi_irq_cb(void) sec(.volatile_ram_code) //中断函数，指定在内部sram
{
    JL_SPI->CON |= BIT(14);//清除中断标记
    asm volatile("csync;");
    if (recv_buf) {
#if RECV_SPI_HEAD_EN
        if (!spi_head_ok) { //没有接收头，就先解析头
            spi_head = (struct spi_heads *)(recv_buf + recv_write_cnt * RECV_SIZE_MAX + recv_buf_offset);
            if (spi_head->head == 0xA5) { //接收到头部，则接收剩下数据
                recv_buf_offset += SPI_HEAD_SIZE;
                JL_SPI->ADR = (u32)(recv_buf + recv_write_cnt * RECV_SIZE_MAX + recv_buf_offset);
                JL_SPI->CNT = (u32)spi_head->size;//接收剩下字节
                spi_head_ok = true;
            } else { //头不对，继续接收头
                JL_SPI->ADR = (u32)(recv_buf + recv_write_cnt * RECV_SIZE_MAX + recv_buf_offset);
                JL_SPI->CNT = (u32)SPI_HEAD_SIZE;//继续接收数据头
            }
        } else
#endif
        {
            //接收的是数据
            spi_head_ok = false;
            if (((recv_write_cnt + 1) == RECV_FPS && !recv_read_cnt) || (recv_read_cnt && (recv_write_cnt + 1) == recv_read_cnt)) {
#if RECV_SPI_HEAD_EN
                recv_buf_offset -= SPI_HEAD_SIZE;
#else
                recv_buf_offset = 0;
#endif
            } else {
                recv_write_cnt = (++recv_write_cnt) >= RECV_FPS ? 0 : recv_write_cnt;
                recv_buf_offset = 0;
            }
            JL_SPI->ADR = (u32)(recv_buf + recv_write_cnt * RECV_SIZE_MAX + recv_buf_offset);//偏移地址
            JL_SPI->CNT = (u32)SPI_HEAD_SIZE;//继续接收数据头
        }
    }
    asm volatile("csync;");
}
static void spi_test_task(void *p)
{
    void *spi_hdl = NULL;
    int ret;

    //1.打开spi设备
    spi_hdl = dev_open("spi1", NULL);
    if (!spi_hdl) {
        printf("spi open err \n");
        goto exit;
    }
    //2.注册中断函数
    dev_ioctl(spi_hdl, IOCTL_SPI_SET_IRQ_FUNC, (u32)spi_irq_cb);
#if !SPI_BUF_USE_SRAM
    recv_buf = malloc(recv_buf_size);
    if (!recv_buf) {
        printf("err spi no buf \n");
        goto exit;
    }
#endif // SPI_BUF_USE_SRAM
    //3.配置接收模式，开启中断，开启SPI
    JL_SPI->CON &= ~(BIT(10) | BIT(11) | BIT(2)); //1bit模式
    JL_SPI->CON |= BIT(12);//接收模式
    JL_SPI->CON |= BIT(13);//开启中断
    JL_SPI->CON |= BIT(0);//开启SPI

    recv_write_cnt = 0;
    recv_read_cnt = 0;
    recv_buf_offset = 0;
    app_recv_buf = NULL;
    //4.配置接收地址，如对方主机SPI协议为如下，注意：写JL_SPI->CNT之前需保证此时对方SPI还没有发送数据，否则中途接收数据会出错
    JL_SPI->ADR = (u32)recv_buf;//配置硬件接收地址
    //此处根据实际应用选择接收字节数，比如收到4字节数据头，在数据头解析后可知下次应该收多少字节
    JL_SPI->CNT = (u32)SPI_HEAD_SIZE;//写该寄存器则正式启动硬件接收，例如此处先接收4字节，接收4字节完就进spi_irq_cb中断解析

    while (1) {
        //5.接收数据查询
        //不可屏蔽中断方法，中断函数不能使用系统API，只能查询法
        if (recv_read_cnt < recv_write_cnt || recv_read_cnt != recv_write_cnt) {
#if RECV_SPI_HEAD_EN //接收包含有头部信息处理分支
            struct spi_heads *spi_head = (struct spi_heads *)(recv_buf + recv_read_cnt * RECV_SIZE_MAX);
            if (spi_head->head == 0xA5) { //接收到头部，则接收剩下数据
                app_recv_buf = (char *)spi_head + SPI_HEAD_SIZE;
                app_recv_len = spi_head->size;
                //todo，数据处理
                put_buf(app_recv_buf, app_recv_len);


            }
#else //接收没有头部信息处理分支
            u8 *data = recv_buf + recv_write_cnt * RECV_SIZE_MAX;
            //todo，数据处理
            put_buf(data, RECV_SIZE_MAX);
#endif
            recv_read_cnt = (++recv_read_cnt) >= RECV_FPS ? 0 : recv_read_cnt;
        }
        os_time_dly(1);//延时1个tick，防止看门狗。不允许丢帧则注释这行：当注释掉这个行代码时，该任务优先级需要8以下（否则影响系统其他任务调度）。
        //taskYIELD();//可以使用空闲时候调度任务一次
    }

exit:
#if !SPI_BUF_USE_SRAM
    if (recv_buf) { //释放内存
        free(recv_buf);
        recv_buf = NULL;
    }
#endif
    if (spi_hdl) { //关闭spi设备
        dev_close(spi_hdl);
    }
    while (1) { //等待其他任务杀此任务
        os_time_dly(100);//延时100个tick，防止看门狗
    }
}

static int c_main1(void)
{
    os_task_create(spi_test_task, NULL, 8, 1000, 0, "spi_test_task");
    return 0;
}
late_initcall(c_main1);
