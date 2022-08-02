#include "wf_uart_protocal.h"
#include "system/includes.h"
#include "app_config.h"
#include "uart.h"

static u8 cbuf[1 * 128] __attribute__((aligned(32))); //用于串口接收缓存数据的循环cbuf
static void *g_hdl = NULL;
static uart_protocal_frame g_pf;
static u8 uart_dev_init_ok = 0;
static OS_MUTEX g_mutex;
extern const u8 CONFIG_SDIO_SLAVE_MODE;

static void uart_dev_init(void *priv)
{
    u8 recv_buf[64];
    int len;
    int err;

    os_mutex_create(&g_mutex);
    g_hdl = dev_open("uart2", NULL);
    if (!g_hdl) {
        printf("open uart err !!!\n");
        return;
    }

    dev_ioctl(g_hdl, UART_SET_CIRCULAR_BUFF_ADDR, (int)cbuf);
    dev_ioctl(g_hdl, UART_SET_CIRCULAR_BUFF_LENTH, sizeof(cbuf));
    dev_ioctl(g_hdl, UART_SET_RECV_BLOCK, 1);
    dev_ioctl(g_hdl, UART_START, 0);

    uart_dev_init_ok = 1;
    printf("uart_dev_init OK!\n");

    while (1) {
        len = dev_read(g_hdl, recv_buf, 64);
        if (len <= 0) {
            if (len == UART_CIRCULAR_BUFFER_WRITE_OVERLAY) {
                puts("\n UART_CIRCULAR_BUFFER_WRITE_OVERLAY err\n");
                dev_ioctl(g_hdl, UART_FLUSH, 0);
            } else if (len == UART_RECV_TIMEOUT) {
                puts("UART_RECV_TIMEOUT...\r\n");
            }
            continue;
        }

        err = uart_recv_parse(recv_buf, len);
        if (err < 0) {
            err = PARM_SET_ERR;
            uart_dev_send_cmd(PARM_SET_RSP, &err, 1);
        } else {
            err = PARM_SET_OK;
            uart_dev_send_cmd(PARM_SET_RSP, &err, 1);
        }
    }

    dev_close(g_hdl);
}

static uart_err_t uart_action(uart_protocal_frame *pf)
{
    uart_err_t err = UART_ERR_OK;

    switch (pf->data.optcode) {
    case WIFI_SET_CHANNEL:
        printf("wifi channel set : %d\n", pf->data.data[0]);
        wifi_set_channel(pf->data.data[0]);
        break;
    default:
        puts("cmd not default!\n");
        err = UART_OPTCODE_NOT_DEFAULT;
        break;
    }

    return err;
}

static uart_err_t uart_recv_parse(u8 *recv_data, u32 len)
{
    uart_protocal_frame *pt;
    uart_err_t err = UART_ERR_OK;

    if (len < PROTOCAL_FRAME_SIZE) {
        return UART_FRAME_ERR;
    }

    pt = (uart_protocal_frame *)(recv_data);
    if (pt->data.mark0 == SYNC_MARK0 && pt->data.mark1 == SYNC_MARK1) {
        u16 c_crc = CRC16(pt, sizeof(uart_protocal_frame) - 2);
        if (c_crc == pt->data.crc) {
            err = uart_action(pt);
        } else {
            puts("UART_FRAME_CRC_ERR\n");
            err = UART_FRAME_CRC_ERR;
        }
    } else {
        puts("UART_FRAME_ERR\n");
        err = UART_FRAME_ERR;
    }

    return err;
}

uart_err_t uart_dev_send_cmd(u8 cmd, u8 *data, u32 data_len)
{
    u8 recv_buf[64];
    int len;

    if (!uart_dev_init_ok) {
        puts("UART_DEV_NOT_INIT\n");
        return UART_DEV_NOT_INIT;
    }

    if (data_len > 4 || data == NULL) {
        puts("uart_dev_send_cmd err!");
        return UART_PARAM_INVAILD;
    }

    os_mutex_pend(&g_mutex, 0);
    memset(&g_pf, 0, sizeof(uart_protocal_frame));
    g_pf.data.mark0 = SYNC_MARK0;
    g_pf.data.mark1 = SYNC_MARK1;
    g_pf.data.length = data_len;
    g_pf.data.optcode = cmd;
    memcpy(g_pf.data.data, data, data_len);
    g_pf.data.crc = CRC16(&g_pf, sizeof(uart_protocal_frame) - 2);
    dev_write(g_hdl, &g_pf, sizeof(uart_protocal_frame));
    os_mutex_post(&g_mutex);

    return UART_ERR_OK;
}

void uart_main(void)
{
    if (CONFIG_SDIO_SLAVE_MODE) {
        if (thread_fork("uart_dev_init", 10, 512, 0, NULL, uart_dev_init, NULL) != OS_NO_ERR) {
            printf("thread fork fail\n");
        }
    }
}

late_initcall(uart_main);

