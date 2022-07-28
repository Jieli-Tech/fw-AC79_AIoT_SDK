#ifndef _JL_UART_ADAPT_H_
#define _JL_UART_ADAPT_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "os/os_api.h"

typedef struct {
    unsigned int baud_rate;
    unsigned char *buf;
    unsigned int buf_size;
    unsigned int port;
    unsigned int exit_flag;
    unsigned int r_flag;
    void (*rx_callback)(void *param);
    void *param;
    void *hdl;
    int pid;
    OS_SEM psem;
    char r_char;
} jl_uart_t;



/** 设置回调*/
void jl_uart_set_callback(jl_uart_t *uart_dev, void (*cb)(void *param));

/** 发送一个字节*/
void jl_tx_chr(jl_uart_t *uart_dev, const char c);

/** 接收一个字节*/
char jl_rx_chr(jl_uart_t *uart_dev);

/**退出串口*/
void jl_uart_exit(jl_uart_t *uart_dev);

/** 串口初始化*/
int  jl_uart_dev_init(jl_uart_t *uart_dev);

#ifdef __cplusplus
}
#endif

#endif
