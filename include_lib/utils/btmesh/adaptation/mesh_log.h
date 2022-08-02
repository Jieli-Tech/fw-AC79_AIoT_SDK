#ifndef __MESH_LOG_H__
#define __MESH_LOG_H__

#include "system/debug.h"

// -- output terminal color
#define RedBold             "\033[31;1;7m" // 红色加粗
#define RedBoldBlink        "\033[31;1;5m" // 红色加粗、闪烁
#define BlueBold            "\033[34;1;7m" // 蓝色加粗
#define BlueBoldBlink       "\033[34;1;5m" // 蓝色加粗、闪烁
#define PurpleBold          "\033[35;1m"   // 紫色加粗
#define PurpleBoldBlink     "\033[35;1;5m" // 紫色加粗、闪烁
#define Reset               "\033[0;25m"   // 颜色复位

#ifndef LOG_TAG_CONST

#undef log_info
#undef log_debug
#undef log_warn
#undef log_error
#undef log_info_hexdump
#undef log_char

#ifdef LOG_INFO_ENABLE
#define log_info(format, ...)       log_print(__LOG_INFO, NULL, "[Info] :" _LOG_TAG format "\r\n", ## __VA_ARGS__)
#else
#define log_info(...)
#endif /* LOG_INFO_ENABLE */

#ifdef LOG_DEBUG_ENABLE
#define log_debug(format, ...)      log_print(__LOG_DEBUG, NULL, "[Debug] :" _LOG_TAG format "\r\n", ## __VA_ARGS__)
#else
#define log_debug(...)
#endif /* LOG_DEBUG_ENABLE */

#ifdef LOG_WARN_ENABLE
#define log_warn(format, ...)       log_print(__LOG_WARN, NULL, "[Warn] :" _LOG_TAG format "\r\n", ## __VA_ARGS__)
#else
#define log_warn(...)
#endif /* LOG_WARN_ENABLE */

#ifdef LOG_ERROR_ENABLE
#define log_error(format, ...)      log_print(__LOG_ERROR, NULL, "[Error] :" _LOG_TAG format "\r\n", ## __VA_ARGS__)
#else
#define log_error(...)
#endif /* LOG_ERROR_ENABLE */

#ifdef LOG_DUMP_ENABLE
#define log_info_hexdump(x,y)       printf_buf(x,y)
#else
#define log_info_hexdump(...)
#endif /* LOG_DUMP_ENABLE */

#ifdef LOG_CHAR_ENABLE
#define log_char(x)                 putchar(x)
#else
#define log_char(x)
#endif /* LOG_CHAR_ENABLE */

#endif /* LOG_TAG_CONST */

//< debug remap
#if MESH_CODE_LOG_DEBUG_EN
#define BT_INFO             log_info
#define BT_INFO_HEXDUMP     log_info_hexdump
#define BT_DBG              log_debug
#define BT_WARN             log_warn
#define BT_ERR              log_error
#else
#define BT_INFO
#define BT_INFO_HEXDUMP
#define BT_DBG
#define BT_WARN
#define BT_ERR              log_error
#endif /* MESH_CODE_LOG_DEBUG_EN */

#endif /* __MESH_LOG_H__ */
