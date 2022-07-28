#include "app_config.h"
#include "system/includes.h"
#include "device/device.h"
#include "asm/spi.h"
#include "device/gpio.h"


#define SPI_CS  IO_PORTA_00


//定义中断回调函数,发送完成和接收完成会回调该函数
void spi_irq_cb(void *priv, u8 **data, u32 *len)
{
    putchar('@');
}

void spi_cs(char val)
{
    gpio_direction_output(SPI_CS, val);
}

static void spi_test_task(void *p)
{
    char *buf = NULL;
    void *spi_hdl = NULL;
    int ret;

    //1.打开spi设备
    spi_hdl = dev_open("spi1", NULL);
    if (!spi_hdl) {
        printf("spi open err \n");
        goto exit;
    }
    //2.注册回调函数
    struct spi_cb cb;
    cb.cb_func = spi_irq_cb;//回调函数
    cb.cb_priv = NULL;//回调函数的第一个私有参数
    dev_ioctl(spi_hdl, IOCTL_SPI_SET_CB, (u32)&cb);
    dev_ioctl(spi_hdl, IOCTL_SPI_SET_USE_SEM, 0);//等待数据时，用信号量等待，不用则为抢占查询硬件中断标记，建议应信号量等待

    //3.发送一个字节，有发送字节的board.c需配置为双向莫模式（使用DO和DI数据线）
    u8 data = 0x12;
    spi_cs(0);
    dev_ioctl(spi_hdl, IOCTL_SPI_SEND_BYTE, data);//发送一个字节
    spi_cs(1);
    os_time_dly(100);

    //4.读取一个字节
    spi_cs(0);
    data = dev_ioctl(spi_hdl, IOCTL_SPI_READ_BYTE, 0);//读取一个字节
    spi_cs(1);
    os_time_dly(100);

    buf = malloc(1024);
    if (!buf) {
        printf("err spi no buf \n");
        goto exit;
    }

    //4.发送1024字节
    spi_cs(0);
    ret = dev_write(spi_hdl, buf, 1024);
    spi_cs(1);
    os_time_dly(100);

    while (1) {
        spi_cs(0);
        ret = dev_read(spi_hdl, buf, 1024);
        spi_cs(1);
        if (ret == 0) {//返回值=0则已经读取到数据
            //6. 数据存在buf，长度为1024
            //todo
            put_buf(buf, 1024);//打印数据
        }
        os_time_dly(100);
    }

exit:
    if (buf) { //释放内存
        free(buf);
    }
    if (spi_hdl) { //关闭spi设备
        dev_close(spi_hdl);
    }
    while (1) { //等待其他任务杀此任务
        os_time_dly(100);
    }
}

static int c_main(void)
{
    os_task_create(spi_test_task, NULL, 22, 1000, 0, "spi_test_task");
    return 0;
}
late_initcall(c_main);
