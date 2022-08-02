#include "system/includes.h"
#include "app_config.h"
#include "uart.h"


#ifdef USE_UART_TX_RX_TEST_DEMO
#define UART_MAX_RECV_SIZE   1024 //最大接收个数
static u8 uart_buf[UART_MAX_RECV_SIZE] __attribute__((aligned(32)));

static u32 start_sigle = 0x12345678;
static u32 start_sigle_ok = 0x87654321;

#if 0 //接收板子
static void uart_recv_test_task(void *priv)
{
    int len = sizeof(uart_buf);
    int cnt = 0;
    u8 *recv_buf;
    u32 seq = 0;
    void *hdl = dev_open("uart2", NULL);

    if (!hdl) {
        printf("open uart err !!!\n");
        return ;
    }
    recv_buf = zalloc(UART_MAX_RECV_SIZE);
    if (!recv_buf) {
        printf("malloc uart err !!!\n");
        return ;
    }

    /* 1 . 设置接收数据地址 */
    dev_ioctl(hdl, UART_SET_CIRCULAR_BUFF_ADDR, (int)uart_buf);

    /* 2 . 设置接收数据地址长度 */
    dev_ioctl(hdl, UART_SET_CIRCULAR_BUFF_LENTH, len);

    /* 3 . 设置接收数据为阻塞方式,需要非阻塞可以去掉,建议加上超时设置 */
    dev_ioctl(hdl, UART_SET_RECV_BLOCK, 1);

    u32 parm = 500;//接收函数的超时时间要比发送函数超时时间小
    dev_ioctl(hdl, UART_SET_RECV_TIMEOUT, (u32)parm);

    /* 4 . 使能特殊串口 */
    dev_ioctl(hdl, UART_START, 0);

    printf("---> uart_recv_test_task running \n");

start:
    //协商有起始信息才能往下继续，否则出现接收端还没有准备接收就发送，发送到一半接收端才配置接收的现象
    while (1) {
        memset(recv_buf, 0, 4);
        len = dev_read(hdl, recv_buf, UART_MAX_RECV_SIZE);
        if (len <= 0) {
            printf("\n start_sigle waite , len = %d\n", len);
            if (len == -1) {
                dev_ioctl(hdl, UART_FLUSH, 0);
            }
            os_time_dly(1);
            continue;
        }
        u32 *start_sigle_check = (u32 *)recv_buf;
        if (*start_sigle_check == start_sigle) { //起始信号标志ok
            printf("\n ---> start_sigle_check ok \n");
            break;
        }
    }
    memcpy(recv_buf, &start_sigle_ok, 4);
    dev_write(hdl, recv_buf, 4);
    while (1) {
        /* 5 . 接收数据 */
        u8 cnt = 5;
read_data:
        len = dev_read(hdl, recv_buf, UART_MAX_RECV_SIZE);
        if (len <= 0) {
            printf("\n recv err len = %d\n", len);
            if (len == -1) {
                dev_ioctl(hdl, UART_FLUSH, 0);
            }
            os_time_dly(1);
            if (--cnt == 0) { //5次接收超时回到起始信号
                goto start;
            }
            goto read_data;
        } else if (len <= 8) { //<=8个数据不是正常数据，可能是start 信号，不做数据校验处理(否则数据处理异常)
            if (--cnt == 0) { //5次接收数据异常回到起始信号
                goto start;
            }
            goto read_data;
        }

        /////////////////数据校验:最小9个字节///////////////////////////
        u32 sum = 0;
        u32 check_sum = 0;
        u32 i = 0;
        while (i < (len - 8)) { //校验接收数据和
            sum += recv_buf[i];
            i++;
        }
        //8个数据：序列号seq + 和校验
        check_sum |= recv_buf[len - 8] & 0xff;
        check_sum |= (recv_buf[len - 7] & 0xff) << 8;
        check_sum |= (recv_buf[len - 6] & 0xff) << 16;
        check_sum |= (recv_buf[len - 5] & 0xff) << 24;
        seq = 0;
        seq |= recv_buf[len - 4] & 0xff;
        seq |= (recv_buf[len - 3] & 0xff) << 8;
        seq |= (recv_buf[len - 2] & 0xff) << 16;
        seq |= (recv_buf[len - 1] & 0xff) << 24;
        /////////////////数据校验///////////////////////////

        /////////////////数据校验是否成功返回信息给对方///////////////////////////
        if (check_sum == sum) {
            printf("recv ok seq = %d , len = %d ,check_sum = 0x%x \n", seq, len, check_sum);
            dev_write(hdl, "recv ok", strlen("recv ok"));//发送接收成功命令
        } else {
            printf("recv err seq = %d ,len = %d , check_sum = 0x%x , sum = 0x%x \n", seq, len, check_sum, sum);
            dev_write(hdl, "recv err", strlen("recv err"));//发送接收成功命令
            if (--cnt == 0) { //5次接收出错回到起始信号
                goto start;
            }
            goto read_data;
        }
        memset(recv_buf, 0, sizeof(recv_buf));
    }
}

static int uart_test(void)
{
    return thread_fork("uart_recv_test_task", 20, 2048, 0, 0, uart_recv_test_task, NULL);
}
late_initcall(uart_test);

#else //发送板子
static void uart_send_test_task(void *priv)
{
    int len = sizeof(uart_buf);
    int cnt = 0;
    u32 seq = 0;
    u8 *recv_buf;
    void *hdl = dev_open("uart2", NULL);

    if (!hdl) {
        printf("open uart err !!!\n");
        return ;
    }
    recv_buf = zalloc(UART_MAX_RECV_SIZE);
    if (!recv_buf) {
        printf("malloc uart err !!!\n");
        return ;
    }

    /* 1 . 设置接收数据地址 */
    dev_ioctl(hdl, UART_SET_CIRCULAR_BUFF_ADDR, (int)uart_buf);

    /* 2 . 设置接收数据地址长度 */
    dev_ioctl(hdl, UART_SET_CIRCULAR_BUFF_LENTH, len);

    /* 3 . 设置接收数据为阻塞方式,需要非阻塞可以去掉,建议加上超时设置 */
    dev_ioctl(hdl, UART_SET_RECV_BLOCK, 1);

    u32 parm = 1000;//发送函数的超时时间要比接收函数超时时间大
    dev_ioctl(hdl, UART_SET_RECV_TIMEOUT, (u32)parm);

    /* 4 . 使能特殊串口 */
    dev_ioctl(hdl, UART_START, 0);

    printf("---> uart_send_test_task running \n");

start:
    seq = 0;
    //协商有起始信息才能往下继续，否则出现接收端还没有准备接收就发送，发送到一半接收端才配置接收的现象
    while (1) {
        memcpy(recv_buf, &start_sigle, 4);
        dev_write(hdl, recv_buf, 4);
        memset(recv_buf, 0, 4);
        len = dev_read(hdl, recv_buf, UART_MAX_RECV_SIZE);
        if (len <= 0) {
            printf("\n start_sigle waite , len = %d\n", len);
            if (len == -1) {
                dev_ioctl(hdl, UART_FLUSH, 0);
            }
            os_time_dly(1);
            continue;
        }
        u32 *start_sigle_check = (u32 *)recv_buf;
        if (*start_sigle_check == start_sigle_ok) { //起始信号标志ok
            printf("\n ---> start_sigle_check ok \n");
            break;
        }
    }
    os_time_dly(5);//延时50ms等对面设备准备接收配置
    while (1) {
        /* 5 . 发送数据 */
        u8 cnt = 5;
send_data:
        /////////////////随机数据合成///////////////////////////
        while (1) { //随机长度
            len = JL_RAND->R64L % UART_MAX_RECV_SIZE;
            if (len > 8 && len <= UART_MAX_RECV_SIZE) { //9到UART_MAX_RECV_SIZE的随机长度
                break;
            }
        }
        u32 sum = 0;
        u32 i = 0;
        while (i < (len - 8)) { //随机数据，加上和校验
            recv_buf[i] = (u8)JL_RAND->R64L;
            sum += recv_buf[i];
            i++;
        }
        seq++;

        /////////////////发送数据最小9个字节///////////////////////////
        //8个数据：序列号seq + 和校验
        recv_buf[len - 8] = sum & 0xff;
        recv_buf[len - 7] = (sum >> 8) & 0xff;
        recv_buf[len - 6] = (sum >> 16) & 0xff;
        recv_buf[len - 5] = (sum >> 24) & 0xff;

        recv_buf[len - 4] = seq & 0xff;
        recv_buf[len - 3] = (seq >> 8) & 0xff;
        recv_buf[len - 2] = (seq >> 16) & 0xff;
        recv_buf[len - 1] = (seq >> 24) & 0xff;
        /////////////////随机数加上和校验///////////////////////////

        printf("send seq = %d ,len = %d, sum = 0x%x \n", seq, len, sum);
        dev_write(hdl, recv_buf, len);
read_data:
        len = dev_read(hdl, recv_buf, UART_MAX_RECV_SIZE);
        if (len <= 0) {
            printf("\n recv ack err len = %d\n", len);
            if (len == -1) {
                dev_ioctl(hdl, UART_FLUSH, 0);
            }
            os_time_dly(1);
            if (--cnt == 0) { //5次接收超时回到起始信号
                goto start;
            }
            goto read_data;
        }
        recv_buf[len] = 0;

        /////////////////检查对方是否成功接收信息///////////////////////////
        if (strstr(recv_buf, "recv ok")) {
            printf("send ok \n");
        } else {
            printf("send err, send next packet len = %d %s \n", len, recv_buf);
            if (--cnt == 0) { //5次对方接收数据出错回到起始信号
                goto start;
            }
            goto send_data;
        }
        memset(recv_buf, 0, sizeof(recv_buf));
    }
}

static int uart_test(void)
{
    return thread_fork("uart_send_test_task", 20, 2048, 0, 0, uart_send_test_task, NULL);
}
late_initcall(uart_test);

#endif
#endif


