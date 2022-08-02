/**
 * @file tuya_os_adapt_memory.h
 * @brief 内存操作接口封装
 *
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 *
 */

#ifndef __TUYA_OS_ADAPT_MEMORY_H__
#define __TUYA_OS_ADAPT_MEMORY_H__


#include <stddef.h>
#include "tuya_os_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief tuya_os_adapt_system_malloc用于分配内存
 *
 * @param[in]       size        需要分配的内存大小
 * @return  分配得到的内存指针
 */
void *tuya_os_adapt_system_malloc(const size_t size);

/**
 * @brief tuya_os_adapt_system_free用于释放内存
 *
 * @param[in]       ptr         需要释放的内存指针
 */
void tuya_os_adapt_system_free(void *ptr);

int tuya_os_adapt_reg_memory_intf(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // __TUYA_OS_ADAPT_MEMORY_H__

