#include "system/includes.h"
#include "app_config.h"
#include "device/device.h"
#include "device/uart.h"
#include "jl_uart_adapt.h"

const char *uart_num[3] = {
    "uart0",
    "uart1",
    "uart2"
};

static char rx_one_char(jl_uart_t *uart_dev)
{
    if (NULL == uart_dev) {
        printf("rx_one_char : param error\n");
        return -1;
    }

    int len = 0;
    unsigned char c = -1;
    while (1) {
        if (uart_dev->exit_flag) {
            uart_dev->exit_flag = 0;
            return -1;
        }

        len = dev_read(uart_dev->hdl, &c, 1);
        if (len <= 0) {
            if (len == -1) {
                dev_ioctl(uart_dev->hdl, UART_FLUSH, 0);
            }
            os_time_dly(1);
            continue;
        } else {
            return c;
        }
    }
}

static int jl_uart_initialize(void *priv)
{
    if (NULL == priv) {
        printf("jl_uart_initialize : param err\n");
        return -1;
    }

    jl_uart_t *uart_dev = (jl_uart_t *)priv;

    if (uart_dev->port > 2 || uart_dev->port < 0) {
        return -1;
    }


    printf("jl_uart_initialize port : %s\n", uart_num[uart_dev->port]);
    uart_dev->hdl = dev_open(uart_num[uart_dev->port], NULL);
    if (NULL == uart_dev->hdl) {
        printf("jl_uart_initialize : dev_open error\n");
        return -1;
    }

    uart_dev->buf = (char *)malloc(uart_dev->buf_size);
    if (NULL == uart_dev->buf) {
        printf("jl_uart_initialize : malloc err\n");
        return -1;
    }

    /* 设置串口接收缓存数据的循环buf地址 */
    dev_ioctl(uart_dev->hdl, UART_SET_CIRCULAR_BUFF_ADDR, (int)uart_dev->buf);

    /* 设置串口接收缓存数据的循环buf长度 */
    dev_ioctl(uart_dev->hdl, UART_SET_CIRCULAR_BUFF_LENTH, uart_dev->buf_size);

    /* 设置串口波特率 */
    dev_ioctl(uart_dev->hdl, UART_SET_BAUDRATE, uart_dev->baud_rate);

    /* 设置接收数据为阻塞方式,需要非阻塞可以去掉,建议加上超时设置 */
    /* dev_ioctl(jl_hdl, UART_SET_RECV_BLOCK, 1); */

    dev_ioctl(uart_dev->hdl, UART_START, 0);

    os_sem_create(&(uart_dev->psem), 0);

    for (;;) {
        uart_dev->r_char = rx_one_char(uart_dev);
        if (-1 == uart_dev->r_char) {
            break;
        }

        uart_dev->r_flag = 1;

        if (uart_dev->rx_callback) {
            uart_dev->rx_callback(uart_dev->param);
        }

        os_sem_pend(&(uart_dev->psem), 0);
    }

    dev_close(uart_dev->hdl);
    uart_dev->hdl = NULL;
    if (uart_dev->buf) {
        free(uart_dev->buf);
        uart_dev->buf = NULL;
    }
}

/** 设置回调*/
void jl_uart_set_callback(jl_uart_t *uart_dev, void (*cb)(void *param))
{
    if (NULL == uart_dev || NULL == cb) {
        printf("jl_uart_set_callback :param error\n");
        return;
    }

    uart_dev->rx_callback = cb;
}

/** 发送一个字节*/
void jl_tx_chr(jl_uart_t *uart_dev, const char c)
{
    if (NULL == uart_dev) {
        printf("jl_tx_chr ： param error\n");
        return;
    }

    dev_write(uart_dev->hdl, &c, 1);
}

/** 接收一个字节*/
char jl_rx_chr(jl_uart_t *uart_dev)
{
    if (NULL == uart_dev) {
        printf("jl_rx_chr : param error\n");
        return -1;
    }

    char c;

    if (!uart_dev->r_flag) {
        return -1;
    }

    c = uart_dev->r_char;

    //回显
    //jl_tx_chr(uart_dev, c);

    uart_dev->r_flag = 0;

    os_sem_post(&(uart_dev->psem));

    return c;
}

/**退出串口*/
void jl_uart_exit(jl_uart_t *uart_dev)
{
    if (NULL == uart_dev) {
        printf("jl_uart_exit : param error\n");
        return;
    }

    uart_dev->exit_flag = 1;
    os_time_dly(10);
    thread_kill(&(uart_dev->pid), KILL_WAIT);
}

/** 串口初始化*/
int  jl_uart_dev_init(jl_uart_t *uart_dev)
{
    if (NULL == uart_dev) {
        printf("jl_uart_dev_init : init error\n");
        return -1;
    }

    if (thread_fork("jl_uart_initialize", 10, 1024, 0, &(uart_dev->pid), jl_uart_initialize, uart_dev) != OS_NO_ERR) {
        printf("thread fork fail\n");
        return -1;
    }

    return 0;
}



/*********************************uart text*************************************/

#if 0
static void rx_call(void *param)
{
    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>%s, %d\n", __FUNCTION__, __LINE__);
}

static jl_uart_t uart_dev = {
    .baud_rate = 1000000,
    .buf = NULL,
    .buf_size = 1 * 512,
    .port = 0,
    .exit_flag = 0,
    .r_flag = 0,
    .hdl = NULL,
    .rx_callback = rx_call,
    .param = NULL
};

int cnt = 0;
static void read_rx_task(void *priv)
{
    for (;;) {
        jl_rx_chr(&uart_dev);

        cnt++;
        if (cnt == 100) {
            //jl_uart_exit(&uart_dev);
        }
        //os_time_dly(1);
    }
}

void read_rx_thread(void)
{
    if (thread_fork("read_rx_task", 10, 512, 0, NULL, read_rx_task, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}

void jl_uart_test(void)
{
    read_rx_thread();

    os_time_dly(10);

    jl_uart_dev_init(&uart_dev);

}

#endif


