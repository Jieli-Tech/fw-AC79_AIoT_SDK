/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-04-01     armink       first version
 * 2018-04-04     chenyong     add base commands
 */

#include "at.h"
#include <stdlib.h>
#include <string.h>


#ifdef AT_USING_SERVER

#define AT_ECHO_MODE_CLOSE             0
#define AT_ECHO_MODE_OPEN              1

extern at_server_t at_get_server(void);

at_result_t at_exec(void)
{
    return AT_RESULT_OK;
}
AT_CMD_EXPORT("AT", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_exec);

static at_result_t atz_exec(void)
{
    printf("OK");

    at_port_factory_reset();

    return AT_RESULT_NULL;
}
AT_CMD_EXPORT("ATZ", RT_NULL, RT_NULL, RT_NULL, RT_NULL, atz_exec);

static at_result_t at_rst_exec(void)
{
    printf("OK");

    at_port_reset();

    return AT_RESULT_NULL;
}
AT_CMD_EXPORT("AT+RST", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_rst_exec);

static at_result_t ate_setup(const char *args)
{
    int echo_mode = atoi(args);

    if (echo_mode == AT_ECHO_MODE_CLOSE || echo_mode == AT_ECHO_MODE_OPEN) {
        at_get_server()->echo_mode = echo_mode;
    } else {
        return AT_RESULT_FAILE;
    }

    return AT_RESULT_OK;
}
AT_CMD_EXPORT("ATE", "<value>", RT_NULL, RT_NULL, ate_setup, RT_NULL);

static at_result_t at_show_cmd_exec(void)
{
    extern void rt_at_server_print_all_cmd(void);

    rt_at_server_print_all_cmd();

    return AT_RESULT_OK;
}
AT_CMD_EXPORT("AT&L", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_show_cmd_exec);

static at_result_t at_uart_query(void)
{
    printf("AT+UART=%d,%d,%d", 460800, 8, 1);

    return AT_RESULT_OK;
}

static at_result_t at_uart_setup(const char *args)
{

    printf("UART baudrate : %d", 460800);
    printf("UART databits : %d", 8);
    printf("UART stopbits : %d", 1);

    return AT_RESULT_OK;
}

AT_CMD_EXPORT("AT+UART", "=<baudrate>,<databits>,<stopbits>", RT_NULL, at_uart_query, at_uart_setup, RT_NULL);

static at_result_t at_test_exec(void)
{
    printf("AT test commands execute!");
    return AT_RESULT_OK;

}
static at_result_t at_test_query(void)
{
    printf("AT test commands query!");
    return AT_RESULT_OK;
}

AT_CMD_EXPORT("AT+TEST", "=<value1>", RT_NULL, at_test_query, RT_NULL, at_test_exec);

static at_result_t at_tcp_server_run(void)
{
    tcp_server_run();
    return AT_RESULT_OK;
}
AT_CMD_EXPORT("AT+TS", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_tcp_server_run);

static at_result_t at_tcp_client_run(void)
{
    tcp_client_run();
    return AT_RESULT_OK;
}
AT_CMD_EXPORT("AT+TC", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_tcp_client_run);

static at_result_t at_udp_server_run(void)
{
    udp_server_run();
    return AT_RESULT_OK;
}
AT_CMD_EXPORT("AT+US", RT_NULL, RT_NULL, RT_NULL, RT_NULL, at_udp_server_run);

#endif /* AT_USING_SERVER */

