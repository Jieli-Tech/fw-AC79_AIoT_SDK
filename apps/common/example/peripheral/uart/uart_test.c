#include "system/includes.h"
#include "app_config.h"
#include "uart.h"



#ifdef USE_UART_TEST_DEMO

static u8 buf[1 * 1024] __attribute__((aligned(32))); //用于串口接收缓存数据的循环buf
static void uart_test_main(void *priv)
{
    u8 recv_buf[64];
    int send_buf[64];	//DMA发送需要4字节对齐
    int len;
    int cnt = 0;
    void *hdl = dev_open("uart2", NULL);
    if (!hdl) {
        printf("open uart err !!!\n");
        return ;
    }
    /* 1 . 设置串口接收缓存数据的循环buf地址 */
    dev_ioctl(hdl, UART_SET_CIRCULAR_BUFF_ADDR, (int)buf);

    /* 1 . 设置串口接收缓存数据的循环buf长度 */
    dev_ioctl(hdl, UART_SET_CIRCULAR_BUFF_LENTH, sizeof(buf));

    /* 3 . 设置接收数据为阻塞方式,需要非阻塞可以去掉,建议加上超时设置 */
    dev_ioctl(hdl, UART_SET_RECV_BLOCK, 1);

    /* u32 parm = 1000; */
    /* dev_ioctl(hdl, UART_SET_RECV_TIMEOUT, (u32)parm); //超时设置 */

    /* 4 . 使能特殊串口,启动收发数据 */
    dev_ioctl(hdl, UART_START, 0);

    printf("uart_test_task running \n");

    while (1) {
        /* 5 . 接收数据 */
        len = dev_read(hdl, recv_buf, 64);
        if (len <= 0) {
            printf("\n  uart recv err len = %d\n", len);
            if (len == UART_CIRCULAR_BUFFER_WRITE_OVERLAY) {
                printf("\n UART_CIRCULAR_BUFFER_WRITE_OVERLAY err\n");
                dev_ioctl(hdl, UART_FLUSH, 0); //如果由于用户长期不取走接收的数据导致循环buf接收回卷覆盖,因此直接冲掉循环buf所有数据重新接收
            } else if (len == UART_RECV_TIMEOUT) {
                puts("UART_RECV_TIMEOUT...\r\n");
            }
            continue;
        }
        printf("\n uart recv len = %d\n", len);
        //把串口接收到的数据发送回去
        dev_write(hdl, recv_buf, len);
        cnt += len;
        len = sprintf(send_buf, "Rx_Cnt = %d\n", cnt);
        //统计串口接收到的数据发送回去
        dev_write(hdl, send_buf, len);
    }
    dev_close(hdl);
}
static int c_main1(void)
{
    os_task_create(uart_test_main, NULL, 10, 1000, 0, "uart_test_main");
    return 0;
}

late_initcall(c_main1);




#endif

