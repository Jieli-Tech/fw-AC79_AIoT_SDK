#ifndef _ALIGENIE_PORTING_LIBS_HEADER_
#define _ALIGENIE_PORTING_LIBS_HEADER_

/***FUNCTIONALITY MACROS***/
#define AG_PLAY_INTERCOM_IGNORE_NEXT_PREV 1
#define AG_PLAY_PUSH_CMD_EXTERNAL 0

/*********************** C or GCC libs ********************/
#include <sys/time.h>
// #include <stdio.h>
#include <unistd.h>
#include "stdbool.h"
#include "string.h"

/************************ PLATFORM ************************/
#if defined(TOOLCHAIN_ARM)
typedef unsigned int uint32_t;
#endif

#ifndef bool
//#define bool   unsigned char
//#define false  (0)
//#define true   (1)
#endif

/************************* SOCKET *************************/
/*Need a BSD-Standard implementation of socket */
#include "lwip/sockets.h"
#include "lwip/netif.h"
#include "lwip/netdb.h"
#include "lwip/inet.h"

#define ALIGENIE_WEBSOCKET

#ifdef ALIGENIE_WEBSOCKET
/************************* websocket *************************/
#define SUPPORT_MBEDTLS
//#define SUPPORT_WOLFSSL
#define SUPPORT_REDUCE_MEM
//#define X871
#define AligenieSDK
#define LITEWS_OS_RTOS

#ifdef SUPPORT_MBEDTLS
//#include "mbedtls/net_sockets.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/md5.h"
#include "mbedtls/md.h"
#include "mbedtls/md_internal.h"
#endif /*SUPPORT_MBEDTLS*/

#ifdef SUPPORT_WOLFSSL
#include "../../wolfssl/include/wolfssl/ssl.h"
#endif /*SUPPORT_WOLFSSL*/
#endif /*ALIGENIE_WEBSOCKET*/

#endif /*_ALIGENIE_PORTING_LIBS_HEADER_*/

