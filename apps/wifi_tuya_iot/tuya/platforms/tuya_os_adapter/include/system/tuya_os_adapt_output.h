/**
 * @file tuya_os_adapt_output.h
 * @brief 日志操作接口
 *
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 *
 */

#ifndef _TUYA_OS_ADAPT_OUTPUT_H
#define _TUYA_OS_ADAPT_OUTPUT_H

#include <stdbool.h>
#include <stdint.h>
#include "tuya_os_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief tuya_os_adapt_output_log用于输出log信息
 *
 * @param[in] *str log buffer指针
 * 函数实现在tuya_os_adapt_output.c
 */
void tuya_os_adapt_output_log(const signed char *str);

/**
 * @brief 用于关闭原厂sdk默认的log口
 *
 */
int tuya_os_adapt_log_close(void);

/**
 * @brief 用于恢复原厂sdk默认的log口
 *
 */
int tuya_os_adapt_log_open(void);

int tuya_os_adapt_reg_output_intf(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

