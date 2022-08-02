#ifndef __UART_PROTOCAL_H
#define __UART_PROTOCAL_H

#include "system/includes.h"

#define PROTOCAL_SIZE       4
#define SYNC_MARK0          0xAA
#define SYNC_MARK1          0x55

//操作码
#define WIFI_SET_CHANNEL    0x1
#define PARM_SET_RSP 	    0x2

//错误码
#define PARM_SET_OK 0x80
#define PARM_SET_ERR 0x81

typedef union {
    u8 raw_data[PROTOCAL_SIZE + 6];
    struct {
        u8 mark0;
        u8 mark1;
        u8 length;
        u8 optcode; //操作码
        u8 data[PROTOCAL_SIZE];
        u16 crc;
    } data;
} uart_protocal_frame;

#define PROTOCAL_FRAME_SIZE (sizeof(uart_protocal_frame))

typedef enum {
    UART_ERR_OK = 0,
    UART_FRAME_ERR = -1,
    UART_FRAME_CRC_ERR = -2,
    UART_OPTCODE_NOT_DEFAULT = -3,
    UART_DEV_NOT_INIT = -4,
    UART_PARAM_INVAILD = -5,
    UART_PARAM_SET_ERR = -6,
} uart_err_t;

uart_err_t uart_dev_send_cmd(u8 cmd, u8 *data, u32 data_len);

#endif //__UART_PROTOCAL_H

