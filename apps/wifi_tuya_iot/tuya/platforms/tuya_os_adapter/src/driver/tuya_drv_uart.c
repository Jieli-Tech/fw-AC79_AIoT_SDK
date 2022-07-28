/*============================================================================
*                                                                            *
* Copyright (C) by Tuya Inc                                                  *
* All rights reserved                                                        *
*                                                                            *
*                                                                            *
=============================================================================*/

/*============================ INCLUDES ======================================*/
#include "system/includes.h"
#include "device.h"
#include "asm/uart.h"
#include "os/os_api.h"
#include "init.h"

#include "tuya_uart.h"
#include "tuya_os_adapter_error_code.h"
#include "tuya_os_adapt_output.h"
#include "tuya_os_adapt_memory.h"

/*============================ MACROS ========================================*/
#define UART_DEV_NUM       2
#define REC_BUFF_FIFO_LEN  1024
#define TUYA_UART_FIFO_LEN 512
/*============================ TYPES =========================================*/
typedef struct {
    tuya_uart_t         dev;
    tuya_uart_isr_cb    isr_cb;
} uart_dev_t;

typedef struct {
    unsigned char *base;  //初始化的动态非配存储空间
    int front;            //头指针，若队列不空，指向队列的头元素
    int rear;             //尾指针，若队列不空，指向队列尾元素的下一个位置
} SqQueue;

typedef struct {
    unsigned char init_flag;
    void *hdl;
    unsigned char rec_buff[REC_BUFF_FIFO_LEN];
    SqQueue fifo;
    TaskHandle_t xHandle;
} jl_uart_hdl;
/*============================ PROTOTYPES ====================================*/
static int uart_dev_init(tuya_uart_t *uart, tuya_uart_cfg_t *cfg);
static int uart_dev_write_byte(tuya_uart_t *uart, uint8_t byte);
static int uart_dev_read_byte(tuya_uart_t *uart, uint8_t *byte);
static int uart_dev_control(tuya_uart_t *uart, uint8_t cmd, void *arg);
static int uart_dev_deinit(tuya_uart_t *uart);
/*============================ LOCAL VARIABLES ===============================*/
static uart_dev_t s_uart_dev[UART_DEV_NUM];
static jl_uart_hdl s_jl_uart_hdl[UART_DEV_NUM] = {0};

static tuya_uart_ops_t  tuya_uart_dev_ops = {
    .init       = uart_dev_init,
    .read_byte  = uart_dev_read_byte,
    .write_byte = uart_dev_write_byte,
    .control    = uart_dev_control,
    .deinit     = uart_dev_deinit,
};

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ IMPLEMENTATION ================================*/

int platform_uart_init(void)
{
    memset(&s_uart_dev[TUYA_UART0], 0, sizeof(uart_dev_t));
    s_jl_uart_hdl[TUYA_UART0].init_flag = 0;
    s_uart_dev[TUYA_UART0].dev.ops = (tuya_uart_ops_t *)&tuya_uart_dev_ops;
    return tuya_driver_register(&s_uart_dev[TUYA_UART0].dev.node, TUYA_DRV_UART, TUYA_UART0);

    memset(&s_uart_dev[TUYA_UART0], 0, sizeof(uart_dev_t));
    s_jl_uart_hdl[TUYA_UART1].init_flag = 0;
    s_uart_dev[TUYA_UART1].dev.ops = (tuya_uart_ops_t *)&tuya_uart_dev_ops;
    return tuya_driver_register(&s_uart_dev[TUYA_UART1].dev.node, TUYA_DRV_UART, TUYA_UART1);
}

/**
 * @brief  入队列数据
 *
 * @param[in]  SqQueue *Q           队列
 * @param[in]  unsigned char e      数据
 */
void EnQueue(SqQueue *Q, unsigned char e)
{
    //插入元素e为Q的新的队尾元素
    if ((Q->rear + 1) % TUYA_UART_FIFO_LEN == Q->front) {
        return;    // 队列满
    }

    Q->base[Q->rear] = e;
    Q->rear = (Q->rear + 1) % TUYA_UART_FIFO_LEN;
    return;
}

/**
 * @brief  出队列数据
 *
 * @param[in]  SqQueue *Q           队列
 * @param[in]  unsigned char e      数据
 *
 */
static int DeQueue(SqQueue *Q, unsigned char *e)
{
    //若队列不空，则删除Q的对头元素，用e返回其值，并返回OK；
    //否则返回ERROR
    if (Q->front == Q->rear) {
        return OPRT_OS_ADAPTER_COM_ERROR;
    }

    *e = Q->base[Q->front];
    Q->front = (Q->front + 1) % TUYA_UART_FIFO_LEN;
    return OPRT_OS_ADAPTER_OK;
}


/**
 * @brief 串口读取队列
 *
 * @param[in]  void * pvParameters
 */
void jl_uart_read_task(void *pvParameters)
{
    int len = 0;
    unsigned  char port = 0;
    unsigned  char byte;

    port = (int)pvParameters;
    if (port > TUYA_UART1) {
        goto fail;
    }

    while (1) {
        len = dev_read(s_jl_uart_hdl[port].hdl, &byte, 1);
        if (len == 1) {
            EnQueue(&s_jl_uart_hdl[port].fifo, byte);
            if (s_uart_dev[port].isr_cb) {
                s_uart_dev[port].isr_cb(&s_uart_dev[port].dev, TUYA_UART_INT_RX_EVENT);
            }
        } else {
            if (len == -1) {
                dev_ioctl(s_jl_uart_hdl[port].hdl, UART_FLUSH, 0);
            }

            vTaskDelay(1);
        }
    }

fail:
    vTaskDelete(NULL);
}


/**
 * @brief 用于初始化串口
 *
 * @param[in]  uart     串口句柄
 * @param[in]  cfg      串口配置结构体
 */
static int uart_dev_init(tuya_uart_t *uart, tuya_uart_cfg_t *cfg)
{
    unsigned char *port_name = NULL;
    BaseType_t ret = 0;

    if (TUYA_UART0 == uart->node.port) {
        port_name = "uart1";
    } else if (TUYA_UART1 == uart->node.port) {
        port_name = "uart2";
    } else {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    if (s_jl_uart_hdl[uart->node.port].init_flag) {
        return OPRT_OS_ADAPTER_OK;
    }

    s_jl_uart_hdl[uart->node.port].hdl = dev_open(port_name, NULL);
    if (!s_jl_uart_hdl[uart->node.port].hdl) {
        tuya_os_adapt_output_log("dev_open uart error \r\n");
        return OPRT_OS_ADAPTER_COM_ERROR;
    }

    s_jl_uart_hdl[uart->node.port].fifo.base = tuya_os_adapt_system_malloc(TUYA_UART_FIFO_LEN);
    if (s_jl_uart_hdl[uart->node.port].fifo.base == NULL) {
        dev_close(s_jl_uart_hdl[uart->node.port].hdl);
        return OPRT_OS_ADAPTER_MALLOC_FAILED;
    }
    s_jl_uart_hdl[uart->node.port].fifo.front = 0;
    s_jl_uart_hdl[uart->node.port].fifo.rear = 0;
    ret = xTaskCreate(jl_uart_read_task, "adapt_uart", 1024 * 4, (void *)uart->node.port, 5, &s_jl_uart_hdl[uart->node.port].xHandle);
    if (ret != pdPASS) {
        dev_close(s_jl_uart_hdl[uart->node.port].hdl);
        tuya_os_adapt_system_free(s_jl_uart_hdl[uart->node.port].fifo.base);
        return OPRT_OS_ADAPTER_THRD_CREAT_FAILED;
    }

    dev_ioctl(s_jl_uart_hdl[uart->node.port].hdl, UART_SET_CIRCULAR_BUFF_ADDR, (int)s_jl_uart_hdl[uart->node.port].rec_buff);

    /* 1 . 设置串口接收缓存数据的循环buf长度 */
    dev_ioctl(s_jl_uart_hdl[uart->node.port].hdl, UART_SET_CIRCULAR_BUFF_LENTH, REC_BUFF_FIFO_LEN);

    /* 3 . 设置接收数据为阻塞方式,需要非阻塞可以去掉,建议加上超时设置 */
    dev_ioctl(s_jl_uart_hdl[uart->node.port].hdl, UART_SET_RECV_BLOCK, 1);

    /* 4 . 使能特殊串口,启动收发数据 */
    dev_ioctl(s_jl_uart_hdl[uart->node.port].hdl, UART_START, 0);

    dev_ioctl(s_jl_uart_hdl[uart->node.port].hdl, UART_SET_BAUDRATE, cfg->baudrate);

    s_jl_uart_hdl[uart->node.port].init_flag = 1;

    tuya_os_adapt_output_log("jl uart success\r\n");

    return OPRT_OS_ADAPTER_OK;
}

static int uart_dev_control(tuya_uart_t *uart, uint8_t cmd, void *arg)
{
    uart_dev_t *uart_dev = (uart_dev_t *)uart;

    switch (cmd) {
    case TUYA_DRV_SET_INT_CMD:
        if ((uint32_t)arg == TUYA_DRV_INT_RX_FLAG) {
            //! TODO:
        } else if ((uint32_t)arg == TUYA_DRV_INT_TX_FLAG) {
            //! TODO:
        } else {
            return OPRT_INVALID_PARM;
        }
        break;

    case TUYA_DRV_CLR_INT_CMD:
        if ((uint32_t)arg == TUYA_DRV_INT_RX_FLAG) {
            //! TODO:
        } else if ((uint32_t)arg == TUYA_DRV_INT_TX_FLAG) {
            //! TODO:
        } else {
            return OPRT_INVALID_PARM;
        }
        break;

    case TUYA_DRV_SET_ISR_CMD:
        uart_dev->isr_cb = (tuya_uart_isr_cb)arg;
        break;
    }

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief 用于发送一个字节
 *
 * @param[in]  uart     串口句柄
 * @param[in]  byte     要发送的字节
 */
static int uart_dev_write_byte(tuya_uart_t *uart, uint8_t byte)
{
    if (uart->node.port > TUYA_UART1) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    if (!s_jl_uart_hdl[uart->node.port].init_flag) {
        return OPRT_OS_ADAPTER_COM_ERROR;
    }

    dev_write(s_jl_uart_hdl[uart->node.port].hdl, &byte, 1);

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief 用于读取一个字节
 *
 * @param[in]   uart     串口句柄
 * @param[out]  ch       要读取的字节指针
 */
static int uart_dev_read_byte(tuya_uart_t *uart, uint8_t *byte)
{
    int ret = OPRT_OS_ADAPTER_OK;

    if (uart->node.port > TUYA_UART1) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    if (!s_jl_uart_hdl[uart->node.port].init_flag) {
        return OPRT_OS_ADAPTER_COM_ERROR;
    }

    ret = DeQueue(&s_jl_uart_hdl[uart->node.port].fifo, byte);
    if (ret) {
        return OPRT_OS_ADAPTER_COM_ERROR;
    }

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief 用于释放串口
 *
 * @param[in]  port     串口句柄
 */
static int uart_dev_deinit(tuya_uart_t *uart)
{
    if (uart->node.port > TUYA_UART1) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    dev_close(s_jl_uart_hdl[uart->node.port].hdl);
    vTaskDelete(s_jl_uart_hdl[uart->node.port].xHandle);
    tuya_os_adapt_system_free(s_jl_uart_hdl[uart->node.port].fifo.base);

    s_uart_dev[uart->node.port].isr_cb = NULL;
    s_jl_uart_hdl[uart->node.port].init_flag = 0;
    s_jl_uart_hdl[uart->node.port].hdl = NULL;

    return OPRT_OS_ADAPTER_OK;
}
