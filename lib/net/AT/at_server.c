/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-03-30     chenyong     first version
 * 2018-04-14     chenyong     modify parse arguments
 */

#include "common.h"
#include "at.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//#include "rthw.h"

#define LOG_TAG              "at.svr"
#include "at_log.h"

#ifdef AT_USING_SERVER

#define AT_CMD_CHAR_0                  '0'
#define AT_CMD_CHAR_9                  '9'
#define AT_CMD_QUESTION_MARK           '?'
#define AT_CMD_EQUAL_MARK              '='
#define AT_CMD_L_SQ_BRACKET            '['
#define AT_CMD_R_SQ_BRACKET            ']'
#define AT_CMD_L_ANGLE_BRACKET         '<'
#define AT_CMD_R_ANGLE_BRACKET         '>'
#define AT_CMD_COMMA_MARK              ','
#define AT_CMD_SEMICOLON               ';'
#define AT_CMD_CR                      '\r'
#define AT_CMD_LF                      '\n'
static at_server_t at_server_local = RT_NULL;
static at_cmd_t cmd_table = RT_NULL;
static rt_size_t cmd_num;

extern rt_size_t at_utils_send(rt_device_t dev,
                               rt_off_t    pos,
                               const void *buffer,
                               rt_size_t   size);
/**
 * AT server request arguments parse arguments
 *
 * @param req_args request arguments
 * @param req_expr request expression
 *
 * @return  -1 : parse arguments failed
 *           0 : parse without match
 *          >0 : The number of arguments successfully parsed
 */
int at_req_parse_args(const char *req_args, const char *req_expr, ...)
{
    va_list args;
    int req_args_num = 0;

    RT_ASSERT(req_args);
    RT_ASSERT(req_expr);

    va_start(args, req_expr);

    req_args_num = vsscanf(req_args, req_expr, args);

    va_end(args);

    return req_args_num;
}

/**
 * AT server send command execute result to AT device
 *
 * @param result AT command execute result
 */
void at_server_print_result(at_result_t result)
{
    switch (result) {
    case AT_RESULT_OK:
        printf("");
        printf("OK");
        break;

    case AT_RESULT_FAILE:
        printf("");
        printf("ERROR");
        break;

    case AT_RESULT_NULL:
        break;

    case AT_RESULT_CMD_ERR:
        printf("ERR CMD MATCH FAILED!");
        at_server_print_result(AT_RESULT_FAILE);
        break;

    case AT_RESULT_CHECK_FAILE:
        printf("ERR CHECK ARGS FORMAT FAILED!");
        at_server_print_result(AT_RESULT_FAILE);
        break;

    case AT_RESULT_PARSE_FAILE:
        printf("ERR PARSE ARGS FAILED!");
        at_server_print_result(AT_RESULT_FAILE);
        break;

    default:
        break;
    }
}

/**
 *  AT server print all commands to AT device
 */
void rt_at_server_print_all_cmd(void)
{
    rt_size_t i = 0;

    printf("Commands list : ");

    for (i = 0; i < cmd_num; i++) {
        printf("%s", cmd_table[i].name);

        if (cmd_table[i].args_expr) {
            printf("%s", cmd_table[i].args_expr);
        } else {
            printf("%c%c", AT_CMD_CR, AT_CMD_LF);
        }
    }
}

/**
 * Send data to AT Client by uart device.
 *
 * @param server current AT server object
 * @param buf   send data buffer
 * @param size  send fixed data size
 *
 * @return >0: send data size
 *         =0: send failed
 */
rt_size_t at_server_send(at_server_t server, const char *buf, rt_size_t size)
{
    RT_ASSERT(buf);

    if (server == RT_NULL) {
        printf("input AT Server object is NULL, please create or get AT Server object!");
        return 0;
    }

    return at_utils_send(server->device, 0, buf, size);
}

/**
 * AT Server receive fixed-length data.
 *
 * @param client current AT Server object
 * @param buf   receive data buffer
 * @param size  receive fixed data size
 * @param timeout  receive data timeout (ms)
 *
 * @note this function can only be used in execution function of AT commands
 *
 * @return >0: receive data size
 *         =0: receive failed
 */
rt_size_t at_server_recv(at_server_t server, char *buf, rt_size_t size, rt_int32_t timeout)
{
    rt_size_t read_idx = 0;
    rt_err_t result = RT_EOK;
    char ch = 0;

    RT_ASSERT(buf);

    if (server == RT_NULL) {
        printf("input AT Server object is NULL, please create or get AT Server object!");
        return 0;
    }

    while (1) {
        if (read_idx < size) {
            /* check get data value */
            result = server->get_char();
            if (result != RT_EOK) {
                printf("AT Server receive failed, uart device get data error.");
                return 0;
            }

            buf[read_idx++] = ch;
        } else {
            break;
        }
    }

    return read_idx;
}

at_server_t at_get_server(void)
{
    RT_ASSERT(at_server_local);
    RT_ASSERT(at_server_local->status != AT_STATUS_UNINITIALIZED);

    return at_server_local;
}

static rt_err_t at_check_args(const char *args, const char *args_format)
{
    rt_size_t left_sq_bracket_num = 0, right_sq_bracket_num = 0;
    rt_size_t left_angle_bracket_num = 0, right_angle_bracket_num = 0;
    rt_size_t comma_mark_num = 0;
    rt_size_t i = 0;

    RT_ASSERT(args);
    RT_ASSERT(args_format);

    for (i = 0; i < strlen(args_format); i++) {
        switch (args_format[i]) {
        case AT_CMD_L_SQ_BRACKET:
            left_sq_bracket_num++;
            break;

        case AT_CMD_R_SQ_BRACKET:
            right_sq_bracket_num++;
            break;

        case AT_CMD_L_ANGLE_BRACKET:
            left_angle_bracket_num++;
            break;

        case AT_CMD_R_ANGLE_BRACKET:
            right_angle_bracket_num++;
            break;

        default:
            break;
        }
    }

    if (left_sq_bracket_num != right_sq_bracket_num || left_angle_bracket_num != right_angle_bracket_num
        || left_sq_bracket_num > left_angle_bracket_num) {
        return -RT_ERROR;
    }

    for (i = 0; i < strlen(args); i++) {
        if (args[i] == AT_CMD_COMMA_MARK) {
            comma_mark_num++;
        }
    }

    if ((comma_mark_num + 1 < left_angle_bracket_num - left_sq_bracket_num)
        || comma_mark_num + 1 > left_angle_bracket_num) {
        return -RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t at_cmd_process(at_cmd_t cmd, const char *cmd_args)
{
    at_result_t result = AT_RESULT_OK;

    RT_ASSERT(cmd);
    RT_ASSERT(cmd_args);

    if (cmd_args[0] == AT_CMD_EQUAL_MARK && cmd_args[1] == AT_CMD_QUESTION_MARK && cmd_args[2] == AT_CMD_CR) {
        if (cmd->test == RT_NULL) {
            at_server_print_result(AT_RESULT_CMD_ERR);
            return -RT_ERROR;
        }

        result = cmd->test();
        at_server_print_result(result);
    } else if (cmd_args[0] == AT_CMD_QUESTION_MARK && cmd_args[1] == AT_CMD_CR) {
        if (cmd->query == RT_NULL) {
            at_server_print_result(AT_RESULT_CMD_ERR);
            return -RT_ERROR;
        }

        result = cmd->query();
        at_server_print_result(result);
    } else if (cmd_args[0] == AT_CMD_EQUAL_MARK
               || (cmd_args[0] >= AT_CMD_CHAR_0 && cmd_args[0] <= AT_CMD_CHAR_9 && cmd_args[1] == AT_CMD_CR)) {
        if (cmd->setup == RT_NULL) {
            at_server_print_result(AT_RESULT_CMD_ERR);
            return -RT_ERROR;
        }

        if (at_check_args(cmd_args, cmd->args_expr) < 0) {
            at_server_print_result(AT_RESULT_CHECK_FAILE);
            return -RT_ERROR;
        }

        result = cmd->setup(cmd_args);
        at_server_print_result(result);
    } else if (cmd_args[0] == AT_CMD_CR) {
        if (cmd->exec == RT_NULL) {
            at_server_print_result(AT_RESULT_CMD_ERR);
            return -RT_ERROR;
        }

        result = cmd->exec();
        at_server_print_result(result);
    } else {
        return -RT_ERROR;
    }

    return RT_EOK;
}

static at_cmd_t at_find_cmd(const char *cmd)
{
    rt_size_t i = 0;

    RT_ASSERT(cmd_table);
    for (i = 0; i < cmd_num; i++) {
        if (!strcasecmp(cmd, cmd_table[i].name)) {
            return &cmd_table[i];
        }
    }
    return RT_NULL;
}

static rt_err_t at_cmd_get_name(const char *cmd_buffer, char *cmd_name)
{
    rt_size_t cmd_name_len = 0, i = 0;

    RT_ASSERT(cmd_name);
    RT_ASSERT(cmd_buffer);

    for (i = 0; i < strlen(cmd_buffer) + 1; i++) {
        if (*(cmd_buffer + i) == AT_CMD_QUESTION_MARK || *(cmd_buffer + i) == AT_CMD_EQUAL_MARK
            || *(cmd_buffer + i) == AT_CMD_CR
            || (*(cmd_buffer + i) >= AT_CMD_CHAR_0 && *(cmd_buffer + i) <= AT_CMD_CHAR_9)) {
            cmd_name_len = i;
            memcpy(cmd_name, cmd_buffer, cmd_name_len);
            *(cmd_name + cmd_name_len) = '\0';

            return RT_EOK;
        }
    }

    return -RT_ERROR;
}

/****************************************************************************************************************************************/
#include "device.h"
#include "asm/uart.h"
#include "os/os_api.h"
#include "init.h"

__attribute__((weak)) const char *at_get_uart(void)
{
    return "uart1";
}
static void *uart_hdl;
static u8 uart_buf[128] __attribute__((aligned(32))); //用于串口接收缓存数据的循环buf
static int at_uart_init(void)
{
    uart_hdl = dev_open(at_get_uart(), NULL);
    if (!uart_hdl) {
        printf("at %s init fail !!!\n", at_get_uart());
        return -1;
    }
    /* 1 . 设置串口接收缓存数据的循环buf地址 */
    dev_ioctl(uart_hdl, UART_SET_CIRCULAR_BUFF_ADDR, (int)uart_buf);

    /* 1 . 设置串口接收缓存数据的循环buf长度 */
    dev_ioctl(uart_hdl, UART_SET_CIRCULAR_BUFF_LENTH, sizeof(uart_buf));

    /* 3 . 设置接收数据为阻塞方式,需要非阻塞可以去掉,建议加上超时设置 */
    dev_ioctl(uart_hdl, UART_SET_RECV_BLOCK, 1);

    /* 4 . 使能特殊串口,启动收发数据 */
    dev_ioctl(uart_hdl, UART_START, 0);

    return 0;
}

static int at_server_getchar(void)
{
    char c;
    if (dev_read(uart_hdl, &c, 1) <= 0) {
        puts("\n  at_server_getchar err\n");
    }
    //printf("c:%c\r\n", c);
    return c;
}

/****************************************************************************************************************************************/

static void server_parser(at_server_t server)
{
#define ESC_KEY                 0x1B
#define BACKSPACE_KEY           0x08
#define DELECT_KEY              0x7F

    char cur_cmd_name[AT_CMD_NAME_LEN] = { 0 };
    at_cmd_t cur_cmd = RT_NULL;
    char *cur_cmd_args = RT_NULL, ch, last_ch;
    int i;
    RT_ASSERT(server);
    RT_ASSERT(server->status != AT_STATUS_UNINITIALIZED);

    while (1) {
        ch = at_server_getchar();
        printf("ch:%c\r\n", ch);
        if (ESC_KEY == ch) {
            break;
        }

        if (server->echo_mode) {
            if (ch == AT_CMD_CR || (ch == AT_CMD_LF && last_ch != AT_CMD_CR)) {
                printf("%c%c", AT_CMD_CR, AT_CMD_LF);
            } else if (ch == AT_CMD_LF) {
                // skip the end sign check
            } else if (ch == BACKSPACE_KEY || ch == DELECT_KEY) {
                if (server->cur_recv_len) {
                    server->recv_buffer[--server->cur_recv_len] = 0;
                    printf("\b \b");
                }

                continue;
            } else {
                //printf("%c", ch);
            }
        }

        server->recv_buffer[server->cur_recv_len++] = ch;
        last_ch = ch;

        if (!strstr(server->recv_buffer, server->end_mark)) {
            continue;
        }

        if (at_cmd_get_name(server->recv_buffer, cur_cmd_name) < 0) {
            at_server_print_result(AT_RESULT_CMD_ERR);
            goto __retry;
        }

        for (i = 0; i < server->cur_recv_len; i++) {
            printf("cur_name:%c\r\n", cur_cmd_name[i]);
        }

        cur_cmd = at_find_cmd(cur_cmd_name);
        if (!cur_cmd) {
            at_server_print_result(AT_RESULT_CMD_ERR);
            goto __retry;
        }

        cur_cmd_args = server->recv_buffer + strlen(cur_cmd_name);
        if (at_cmd_process(cur_cmd, cur_cmd_args) < 0) {
            goto __retry;
        }

__retry:
        memset(server->recv_buffer, 0x00, AT_SERVER_RECV_BUFF_LEN);
        server->cur_recv_len = 0;
    }
}

static rt_err_t at_rx_ind(rt_device_t dev, rt_size_t size)
{
    if (size > 0) {
        rt_sem_release(at_server_local->rx_notice);
    }

    return RT_EOK;
}

#if defined(__ICCARM__) || defined(__ICCRX__)               /* for IAR compiler */
#pragma section="RtAtCmdTab"
#endif

int at_server_init(void)
{
    at_uart_init();
    rt_err_t result = RT_EOK;
    rt_err_t open_result = RT_EOK;

    if (at_server_local) {
        return result;
    }

    /* initialize the AT commands table.*/
#if defined(__CC_ARM)                                 /* ARM C Compiler */
    extern const int RtAtCmdTab$$Base;
    extern const int RtAtCmdTab$$Limit;
    cmd_table = (at_cmd_t)&RtAtCmdTab$$Base;
    cmd_num = (at_cmd_t)&RtAtCmdTab$$Limit - cmd_table;
#elif defined (__ICCARM__) || defined(__ICCRX__)      /* for IAR Compiler */
    cmd_table = (at_cmd_t)__section_begin("RtAtCmdTab");
    cmd_num = (at_cmd_t)__section_end("RtAtCmdTab") - cmd_table;
#elif defined (__GNUC__)                             /* for GCC Compiler */
    extern const int __rtatcmdtab_start;
    extern const int __rtatcmdtab_end;
    cmd_table = (at_cmd_t)&__rtatcmdtab_start;
    cmd_num = (at_cmd_t) &__rtatcmdtab_end - cmd_table;

    at_result_t at_exec(void);
    printf("cmd_nub22 =0x%x,0x%x,0x%x %d \r\n", &__rtatcmdtab_start, &__rtatcmdtab_end, at_exec, cmd_num);
#endif /* defined(__CC_ARM) */

    at_server_local = (at_server_t) calloc(1, sizeof(struct at_server));
    if (!at_server_local) {
        result = -RT_ENOMEM;
        printf("AT server session initialize failed! No memory for at_server structure !");
        goto __exit;
    }

    at_server_local->echo_mode = 1;
    at_server_local->status = AT_STATUS_UNINITIALIZED;

    memset(at_server_local->recv_buffer, 0x00, AT_SERVER_RECV_BUFF_LEN);
    at_server_local->cur_recv_len = 0;
    at_server_local->get_char = at_server_getchar;
    memcpy(at_server_local->end_mark, AT_CMD_END_MARK, sizeof(AT_CMD_END_MARK));

    at_server_local->parser_entry = server_parser;
    os_task_create(server_parser, at_server_local, 12, 2 * 1024, 0, "at_svr");

__exit:
    if (!result) {
        at_server_local->status = 1;

        printf("RT-Thread AT server (V%s) initialize success.", AT_SW_VERSION);
    } else {
        if (at_server_local) {
            free(at_server_local);
        }

        printf("RT-Thread AT server (V%s) initialize failed(%d).", AT_SW_VERSION, result);
    }

    return result;
}
INIT_COMPONENT_EXPORT(at_server_init);

RT_WEAK void at_port_reset(void)
{
    cpu_reset();
    printf("The reset for AT server is implement.");
}

RT_WEAK void at_port_factory_reset(void)
{
    printf("The factory reset for AT server need implement by customer.");
}

#endif /* AT_USING_SERVER */

