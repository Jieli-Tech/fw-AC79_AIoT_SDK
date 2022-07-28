/**
 * Copyright (2017) Baidu Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * File: duerapp_config.h
 * Auth: Renhe Zhang (v_zhangrenhe@baidu.com)
 * Desc: Duer Configuration.
 */

#ifndef BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_DCS3_LINUX_DUERAPP_CONFIG_H
#define BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_DCS3_LINUX_DUERAPP_CONFIG_H

#ifdef bool
#undef bool
#endif

#include "typedef.h"
#include <stdlib.h>

#include "lightduer_log.h"

// #include "app_config.h"

#define DUER_RESTART_TIMEOUT		1000

#define OTA_REG_UPDATE

#define LOAD_PROFILE_BY_FLASH
#define DUER_WECHAT_SUPPORT
// #define DUER_HTTPS_SUPPORT

// #define DUER_SAVE_RECORD_FILE
#define USE_OWN_SPEEX_LIB			1
#define DUER_PROFILE_PATH	"mnt/spiflash/res/profile.txt"

/* #define DUER_USE_VAD */

// #define DUER_SAMPLE_RATE   8000
#define DUER_SAMPLE_RATE   16000

/*********dueros official config*********/
#define LIMIT_LISTEN_TIME
#define SET_MAX_LISTEN_TIME			31000

#if USE_OWN_SPEEX_LIB == 0
#define DUER_COMPRESS_QUALITY       8
#endif

#define CONNECT_NONBLOCK
#define BREAK_DNS_QUERY

#define ENABLE_REPORT_SYSTEM_INFO
// #define DUER_DEBUG_LEVEL 3
// #define DISABLE_TCP_NODELAY
// #define DUER_SENDTIMEOUT    8000 // 8s
// #define DUER_UDP_REPORTER
// #define DISABLE_SPLIT_SEND
// #define DUER_MAX_MSG_NUM_IN_CA_QUEUE 20
// #define MAX_INTERVAL_KEEP_CA_QUEUE 4000 // ms
// #define MAX_INTERVAL_KEEP_VOICE_QUEUE 1900 //ms when interval > 2s,the server will stop immediately.
// #define ENABLE_RECV_TIMEOUT_THRESHOLD
// #define DUER_KEEPALIVE_INTERVAL (55 * 1000)
// #define DUER_DEBUG_AFTER_SEND
#define DUER_EMTTER_STACK_SIZE      (1536)
#define DUER_EMTTER_QUEUE_LENGTH    (0)
#define DUER_SOCKET_TASK_STACK_SIZE	(512)
#define DUER_SOCKET_TASK_QUEUE_LENGTH    (0)
#define DUER_OTA_TASK_STACK_SIZE	(1536)
#define DUER_OTA_TASK_QUEUE_LENGTH    (0)
// #define DUER_STATISTICS_E2E
// #define DISABLE_OPEN_MIC_AUTOMATICALLY
// #define LIGHTDUER_DS_LOG_DEBUG
// #define LIGHTDUER_DS_LOG_CACHE_DEBUG
// #define LIGHTDUER_DS_REPORT_DEBUG
// #define DUER_HTTP_DNS_SUPPORT
// #define NTP_DEBUG
// #define ENABLE_DUER_STORE_VOICE
// #define DUER_VOICE_SEND_ASYNC
// #define DISABLE_SPX_ON_WCHAT
// #define ENABLE_DUER_PRINT_SPEEX_COST
// #define ENABLE_LIGHTDUER_SNPRINTF
// #define DUER_DEBUG_FULL_PATH
// #define DUER_MEMORY_USAGE
// #define NET_TRANS_ENCRYPTED_BY_AES_CBC
// #define DEV_DEBUG_AES
// #define DEV_INFO_AES
// #define LIGHTDUER_AES_DEBUG
// #define EXECUTE_DISPLAY_DIRECTIVE_ASYNC
// #define DUER_AIRKISS_DEVICE_TYPE
// #define DUER_BJSON_PREALLOC_ITEM
// #define DUER_HEAP_MONITOR
// #define DUER_COAP_HEADER_SEND_FIRST
// #define DUER_SEND_WITHOUT_FIRST_CACHE

/*
 * Load profile
 */
const char *duer_load_profile(const char *path);

#endif // BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_DCS3_LINUX_DUERAPP_CONFIG_H
