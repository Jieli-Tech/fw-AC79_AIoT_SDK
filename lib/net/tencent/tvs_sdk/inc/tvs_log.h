#ifndef __TVS_LOG_H__
#define __TVS_LOG_H__

#ifndef TVS_LOG_DEBUG
#define TVS_LOG_DEBUG  1
#endif

#ifndef TVS_LOG_DEBUG_MODULE
#define TVS_LOG_DEBUG_MODULE  "D"
#endif

#ifndef CONFIG_USE_DEFAULT_PRINT
#define CONFIG_USE_DEFAULT_PRINT 1
#endif

#ifndef CONFIG_USE_MTK_LOG
#define CONFIG_USE_MTK_LOG    0
#endif

// 配置printf，有的平台不能用这个函数，可以在编译脚本中加-DTVS_SYS_PRINTF=xxxx
#ifndef TVS_SYS_PRINTF
#define TVS_SYS_PRINTF    printf
#endif

int tvs_log_print_enable();

#if CONFIG_USE_MTK_LOG

#include "syslog.h"
extern log_control_block_t log_control_block_TVS_module;

#define TVS_LOG_PRINTF(fmt, arg...) \
		if (TVS_LOG_DEBUG && tvs_log_print_enable()) { LOG_I(TVS, "[%s]" fmt, TVS_LOG_DEBUG_MODULE, ##arg); }

#else

#include "os_wrapper.h"

#define TVS_LOG_PRINTF(fmt, ...) \
	do {							\
		if (TVS_LOG_DEBUG && tvs_log_print_enable()) TVS_SYS_PRINTF("[TVS][%s:%lu]" fmt"  ---------\r\n", TVS_LOG_DEBUG_MODULE, os_wrapper_get_time_ms(), ##__VA_ARGS__); \
	} while (0)


#define TVS_LOG_ERROR(fmt, ...) \
	do {							\
		if (TVS_LOG_DEBUG) TVS_SYS_PRINTF("[TVS][%s:%lu]" fmt"  ---------\r\n", TVS_LOG_DEBUG_MODULE, os_wrapper_get_time_ms(), ##__VA_ARGS__); \
	} while (0)

#endif

#endif