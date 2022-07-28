#include "system/includes.h"
#include "syscfg/config_interface.h"
#include "os/os_api.h"
#include "asm/uart_dev.h"
#include "asm/crc16.h"
#include "device/device.h"
#include "device/uart.h"
#include "app_config.h"

#define LOG_TAG     "[CI-UART]"
/* #define LOG_ERROR_ENABLE */
/* #define LOG_INFO_ENABLE */
/* #define LOG_DUMP_ENABLE */
#include "debug.h"

#if	TCFG_ONLINE_ENABLE

struct config_uart {
    u32 baudrate;
    int flowcontrol;
    const char *dev_name;
};

struct uart_hdl {
    struct config_uart config;
    void *uart_dev;
    u8 rx_type;
    volatile u8 run_flag;
    s16 data_length;
    void *pTxBuffer;
    void (*packet_handler)(const u8 *packet, int size);
};

typedef struct {
    //head
    u16 preamble;
    u8  type;
    u16 length;
    u8  crc8;
    u16 crc16;
    u8 payload[0];
} _GNU_PACKED_	uart_packet_t;

#define UART_FORMAT_HEAD  sizeof(uart_packet_t)

#define UART_PREAMBLE       0xBED6

#define UART_RX_SIZE        0x200
#define UART_TX_SIZE        0x200

static struct uart_hdl hdl;
#define __this      (&hdl)

static u8 pTxBuffer_static[UART_TX_SIZE] __attribute__((aligned(4)));       //tx memory

static void ci_data_rx_handler(u8 type)
{
    u8 pRxBuffer[UART_RX_SIZE];
    u16 crc16;
    uart_packet_t *p;

    __this->data_length = dev_read(__this->uart_dev, pRxBuffer, UART_RX_SIZE);

    if (__this->data_length <= 0) {
        /* log_e("ci_data_rx_handler err\n"); */
        return;
    }
    /* log_info_hexdump(pRxBuffer, __this->data_length); */

    if (__this->data_length <= UART_FORMAT_HEAD) {
        return;
    }

    p = (uart_packet_t *)pRxBuffer;

    if (p->preamble != UART_PREAMBLE) {
        log_info("preamble err\n");
        put_buf(pRxBuffer, __this->data_length);
        goto reset_buf;
    }

    crc16 = CRC16(pRxBuffer, UART_FORMAT_HEAD - 3);
    log_info("CRC8 0x%x / 0x%x", crc16, p->crc8);
    if (p->crc8 != (crc16 & 0xff)) {
        log_info("crc8 err\n");
        goto reset_buf;
    }
    if (__this->data_length >= p->length + UART_FORMAT_HEAD) {
        log_info("Total length : 0x%x / Rx length : 0x%x", __this->data_length, p->length + CI_FORMAT_HEAD);
        crc16 = CRC16(p->payload, p->length);
        /* log_info("CRC16 0x%x / 0x%x", crc16, p->crc16); */
        if (p->crc16 != crc16) {
            log_info("crc8 err\n");
            goto reset_buf;
        }
        __this->packet_handler(p->payload, p->length);
    }

reset_buf:
    __this->data_length = 0;
}

static void ci_uart_write(void *buf, u16 len)
{
    dev_write(__this->uart_dev, buf, len);
}

static void ci_dev_init(const void *config)
{
    __this->pTxBuffer = pTxBuffer_static;
}

static void ci_transport_task(void *arg)
{
    while (__this->run_flag) {
        ci_data_rx_handler(0);
    }
}

static int ci_dev_open(void)
{
    static char uart_circlebuf[1 * 1024] __attribute__((aligned(32))); //串口循环数据buf,根据需要设置大小

    int parm;
    __this->uart_dev = dev_open("uart1", 0);

    dev_ioctl(__this->uart_dev, UART_SET_CIRCULAR_BUFF_ADDR, (int)uart_circlebuf);

    parm = sizeof(uart_circlebuf);

    dev_ioctl(__this->uart_dev, UART_SET_CIRCULAR_BUFF_LENTH, (int)parm);

#if 0 // 是否设置为 接收完指定长度数据, spec_uart_recv才出来
    parm = 1;
    dev_ioctl(__this->uart_dev, UART_SET_RECV_ALL, (int)parm);
#endif

#if 1 // 是否设置为阻塞方式读
    parm = 1;
    dev_ioctl(__this->uart_dev, UART_SET_RECV_BLOCK, (int)parm);
#endif

    parm = 1000;
    dev_ioctl(__this->uart_dev, UART_SET_RECV_TIMEOUT, (u32)parm);

    __this->run_flag = 1;

    thread_fork("ci_transport_task", 11, 256 * 3, 0, 0, ci_transport_task, NULL);

    dev_ioctl(__this->uart_dev, UART_START, (int)0);

    return 0;
}

static int ci_dev_close(void)
{
    __this->run_flag = 0;
    dev_close(__this->uart_dev);
    return 0;
}

static void ci_dev_register_packet_handler(void (*handler)(const u8 *packet, int size))
{
    __this->packet_handler = handler;
}

static int ci_dev_send_packet(const u8 *packet, int size)
{
    uart_packet_t *p = (uart_packet_t *)__this->pTxBuffer;

    p->preamble = UART_PREAMBLE;
    p->type     = 0;
    p->length   = size;
    p->crc8     = CRC16(p, UART_FORMAT_HEAD - 3) & 0xff;
    p->crc16    = CRC16(packet, size);

    size += UART_FORMAT_HEAD;
    ASSERT(size <= UART_TX_SIZE, "Fatal Error");

    memcpy(p->payload, packet, size);

    log_info("Tx : %d", size);
    put_buf((u8 *)p, size);
    ci_uart_write(p, size);

    return 0;
}

static int ci_dev_can_send_packet_now(uint8_t packet_type)
{
    return 0;
}

// get dev api skeletons
static const ci_transport_t ci_transport_uart = {
    /* const char * name; */                                        "CI_UART",
    /* void   (*init) (const void *transport_config); */            &ci_dev_init,
    /* int    (*open)(void); */                                     &ci_dev_open,
    /* int    (*close)(void); */                                    &ci_dev_close,
    /* void   (*register_packet_handler)(void (*handler)(...); */   &ci_dev_register_packet_handler,
    /* int    (*can_send_packet_now)(uint8_t packet_type); */       &ci_dev_can_send_packet_now,
    /* int    (*send_packet)(...); */                               &ci_dev_send_packet,
};

const ci_transport_t *ci_transport_uart_instance(void)
{
    return &ci_transport_uart;
}

#if TCFG_ONLINE_ENABLE
static const ci_transport_config_uart_t config = {
    CI_TRANSPORT_CONFIG_UART,
    115200,
    0,
    0,
    NULL,
};

static int config_online_init(void)
{
    config_layer_init(ci_transport_uart_instance(), (void *)&config);
    return 0;
}
__initcall(config_online_init);
#endif

#endif
