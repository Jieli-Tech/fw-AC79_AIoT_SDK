/*Aligenie SDK OS porting interfaces*/
/*Ver.20150509*/

#ifndef _ALIGENIE_PORTING_OS_HEADER_
#define _ALIGENIE_PORTING_OS_HEADER_

#ifdef __cplusplus
extern "C"
{
#endif

#include "aligenie_libs.h"

/************************* MEMORY *************************/
#include <stdlib.h>


#define KEY_FIRMWARE_VERSION "fwver"
#ifndef VERSION_FROM_MK
#define FIRMWARE_VERSION   "1.0.0-I-20200921.0123"
#endif

extern void *AG_OS_MALLOC(unsigned int len);
extern void *AG_OS_CALLOC(size_t len, unsigned int size);
extern void *AG_OS_REALLOC(void *ptr, unsigned int newsize);
extern void AG_OS_FREE(void *ptr);

#define AG_OS_MEMSET(ptr, value, len)  memset(ptr, value, len)
#define AG_OS_MEMCPY(dest, src, len)   memcpy(dest, src, len)

/*return: free heap size in bytes*/
extern int ag_os_get_free_heap_size();

/************************* CONFIG *************************/

#define AG_OS_SDK_CONFIG_MAX_LENGTH     (1024)

/**
 * Read sdk config.
 * This config is necessary for Aligenie to run.
 * Config data need to be burned-in by factory.
 * And these data should be able to be read through this function from aligenie sdk.
 *
 * params
 *     buf: data read. Max length is AG_OS_SDK_CONFIG_MAX_LENGTH
 *
 * return
 *     zero or a possitive value for read bytes,
 *     or a negative value for fail.
 *
 * ATTENTION: If it returns fail, sdk will retry forever, because if not, this device cannot be used.
 */
extern int ag_os_read_sdk_config(char *buf);

/**
 * Feature list configuration.
 */
#define AG_FEATURE_COUNT_DOWN       (1U << 0)   //counting down support
#define AG_FEATURE_VOICE_MESSAGE    (1U << 1)   //voice message (intercom) support
#define AG_FEATURE_NIGHT_LIGHT      (1U << 2)   //ai control night light support
#define AG_FEATURE_VOLUME_CONTROL   (1U << 3)   //ai volume control support
#define AG_FEATURE_LOCAL_CLOCK      (1U << 4)   //local clock support
#define AG_FEATURE_OTA_UPDATEK      (1U << 5)   //gateway will not check periodic and no updating logic if configure to 1
#define AG_FEATURE_UTP_CONTROL      (1U << 6)   //support for UTP control logic, U for audioplay, T for TTS and P for prompt
#define AG_FEATURE_ASK_CMD_QUEUE    (1U << 7)   //combined command, detailed info can be got from Tmail genie APP
#define AG_FEATURE_DEV_SETTING      (1U << 8)   //personalize setting
#define AG_FEATURE_IOT_SUBDEV       (1U << 9)   //point-to-point IOT
#define AG_FEATURE_OPERATION        (1U << 10)  //feedback for wake up

/**
 * return value:
 *   feature defined above, combined by bit-or
 * eg.
 *   return AG_FEATURE_VOICE_MESSAGE|AG_FEATURE_VOLUME_CONTROL;
 */
extern unsigned int ag_os_get_feature_config();


/************************* SDCARD *************************/

#define AG_OS_SERVER_CONFIG_MAX_LENGTH  (64)

/**
 * get file stat from sdcard.
 *
 * Params filename NOT INCLUDE the path prefix of "sdcard" node.
 *
 * For example:
 *     There is a directory named "system" in the SD card root,
 *     and a file "a.txt" in that directory,
 *     the parameter of filename will be: "system/a.txt"
 *
 * retrun
 *     zero or a possitive value for read bytes,
 *     or a negative value for fail.
 */
/*deleted*/ //extern int ag_os_read_server_config(const char * filename, char * buf);

extern int ag_os_read_sdcard_file(const char *filename, char *buf, int bufLen);

/************************ FW ver **************************/

/**
 * get firmware version, this version will be sent to ali server.
 *
 * Recommend format:
 *     1.0.0-R-20180605.1118
 *
 * return:
 *     int value: 0 for success or other value for fail.
 *     Can be a string constant, but DO NOT RETURN NULL.
 */
extern int ag_os_get_firmware_version(char *firmware_version);


/********************* Encryption *************************/

/**
 * Calculate MD5
 * return 0 for success, other for fail.
 */
extern int ag_os_generate_md5(unsigned char *out, const unsigned char *data, unsigned int datalen);

/**
 * Calculate sha256
 *
 * Params
 *     out: encrypt result
 *     data: source data
 *     key: sha256 encrypt key
 *
 * return
 *     0 for success,
 *     other for fail.
 */
extern int ag_os_generate_hmac_sha256(unsigned char *out,
                                      const unsigned char *data, unsigned int datalen,
                                      const unsigned char *key, unsigned int keylen);

/************************* LOG ****************************/
typedef enum {
    AG_OS_LOGLEVEL_NONE,
    AG_OS_LOGLEVEL_ERROR,
    AG_OS_LOGLEVEL_WARN,
    AG_OS_LOGLEVEL_DEBUG,
    AG_OS_LOGLEVEL_VERBOSE,
    AG_OS_LOGLEVEL_ALL,
} AG_OS_LOGLEVEL_E;

void ag_os_log_set_level(AG_OS_LOGLEVEL_E level);

void ag_os_log_print(const char *tag,
                     AG_OS_LOGLEVEL_E loglevel,
                     const char *format,
                     ...);
/************************** TIME **************************/
#include <time.h>
/**
 * same as standard time_t time(time_t *t)
 */
time_t ag_os_get_time(time_t *t);

/**
 * get UTC time, same as struct tm *gmtime(const time_t *timep)
 */
struct tm *ag_os_get_gmtime(const time_t *timep);

/**
 * same as int gettimeofday(struct timeval *restrict tp, void *restrict tzp)
 * param tzp will always be NULL for now.
 */
int ag_os_gettimeofday(struct timeval *restrict tp, void *restrict tzp);

struct tm *ag_os_get_localtime_r(const time_t *timep, struct tm *result);

/**
* Get ms information the moment this API is called, marked as start of time
* and a second call of this function will get another ms informtion, whcih
* will be marked as stop of time.
* The difference between stop and start will be used by SDK as time elapsed between
* sequently call of this function.
* time diff = ms_current_2nd - ms_current_1st;
* return 0 if succcess or other value if failed
*/
int ag_os_timediff_ms_get(uint32_t *ms_current);

/************************* FLASH **************************/
/* Read/write flash data with key-value pair
 * Max key length: 20 bytes
 * Max value length: 64 bytes
 * Max number of pairs: 20
 */

#define AG_OS_FLASH_MAX_LENGTH_KEY      (20)
#define AG_OS_FLASH_MAX_LENGTH_VALUE    (64)

/**
 * params outval: DO NOT add \0 at outval[len]
 * return
 *   success: 0
 *   fail: a negative value
 */
extern int ag_os_flash_read(const char *const key, char *outval, size_t len);

/**
 * return
 *   success: 0
 *   fail: a negative value
 */
extern int ag_os_flash_write(const char *const key, const char *const inval, size_t len);

/************************* TASK ***************************/
#define AG_OS_TASK_RET_OK          0
#define AG_OS_TASK_RET_FAIL        1

typedef void *AG_TASK_T;

typedef enum {
    AG_TASK_PRIORITY_LOWEST = 5,
    AG_TASK_PRIORITY_LOWER,
    AG_TASK_PRIORITY_NORMAL,
    AG_TASK_PRIORITY_HIGHER,
    AG_TASK_PRIORITY_HIGHEST,
} AG_TASK_PRIORITY_E;

typedef void *(*ag_os_task_runner)(void *);

extern int ag_os_task_create(AG_TASK_T *task_handler,
                             const char *name,
                             ag_os_task_runner func,
                             AG_TASK_PRIORITY_E priority,
                             int max_stack_size,
                             void *arg);
extern int ag_os_task_destroy(AG_TASK_T *task_handler);

/*mutex lock*/
typedef void *AG_MUTEX_T;
extern void ag_os_task_mutex_init(AG_MUTEX_T *mutex);
extern void ag_os_task_mutex_lock(AG_MUTEX_T *mutex);
extern void ag_os_task_mutex_unlock(AG_MUTEX_T *mutex);
extern void ag_os_task_mutex_delete(AG_MUTEX_T *mutex);

/*cond lock*/
typedef void *AG_COND_T;
extern void ag_os_task_cond_init(AG_COND_T *cond);
extern void ag_os_task_cond_wait(AG_COND_T *cond);
extern void ag_os_task_cond_signal(AG_COND_T *cond);

/* task delay */
#define AG_OS_TASK_DELAY_FOREVER (0xffffffffUL)
extern void ag_os_task_mdelay(uint32_t time_ms);

/*queue*/
typedef void *AG_QUEUE_T;
//wait time
#define AG_OS_QUEUE_TIMEOUT_MAX (0xffffffffUL)
//return value
#define AG_OS_QUEUE_RET_OK          0
#define AG_OS_QUEUE_RET_FAIL        1
#define AG_OS_QUEUE_RET_PUSH_FULL   2
#define AG_OS_QUEUE_RET_POP_EMPTY   3

extern int ag_os_queue_create(AG_QUEUE_T *handler, const char *name, uint32_t message_size, uint32_t number_of_messages);
extern int ag_os_queue_push_to_back(AG_QUEUE_T *handler, void *msg,  uint32_t timeout_ms);
extern int ag_os_queue_push_to_front(AG_QUEUE_T *handler, void *msg,  uint32_t timeout_ms);
extern int ag_os_queue_pop(AG_QUEUE_T *handler, void *msg,  uint32_t timeout_ms);
extern int ag_os_queue_destroy(AG_QUEUE_T *handler);
extern bool ag_os_queue_is_empty(AG_QUEUE_T *handler);
extern bool ag_os_queue_is_full(AG_QUEUE_T *handler);

/*timer*/
#define AG_OS_TIMER_OK      0
#define AG_OS_TIMER_FAIL    1

typedef void *AG_TIMER_T;
typedef void (*ag_os_timer_callback)(void *arg);

extern int ag_os_timer_create(AG_TIMER_T *handler, uint32_t time_interval_ms, bool is_auto_reset, ag_os_timer_callback func, void *arg);
extern int ag_os_timer_start(AG_TIMER_T *handler);
extern int ag_os_timer_stop(AG_TIMER_T *handler);
extern int ag_os_timer_destroy(AG_TIMER_T *handler);
extern void *ag_os_timer_get_arg(AG_TIMER_T *handler);


/*event group bit lock*/
typedef void *AG_EVENTGROUP_T;
extern int ag_os_eventgroup_create(AG_EVENTGROUP_T *handler);
extern int ag_os_eventgroup_delete(AG_EVENTGROUP_T *handler);
extern int ag_os_eventgroup_set_bits(AG_EVENTGROUP_T *handler, int bits);
extern int ag_os_eventgroup_clear_bits(AG_EVENTGROUP_T *handler, int bits);
extern int ag_os_eventgroup_wait_bits(AG_EVENTGROUP_T *handler, int bits, int isClearOnExit, int isWaitForAllBits, int waitMs);

/*device control*/
extern int ag_os_device_standby();
extern int ag_os_device_shutdown();

/*vad type*/
typedef enum {
    AG_OS_AUTO_VAD_OFF,
    AG_OS_AUTO_VAD_ON,
} AG_OS_AUTO_VAD_TYPE_E;

extern AG_OS_AUTO_VAD_TYPE_E ag_os_get_auto_vad_type();


/**
 * 返回true：开启游客模式
 * 返回false：关闭游客模式
 * 游客模式：当在调用aligenie_sdk.h中的ag_sdk_init()之前没有调用过ag_sdk_set_register_info()，且设备从未激活过，那么会尝试以访客模式注册设备。
 * 在访客模式中，无法使用需要身份鉴别才能访问的领域和技能。
 */
extern bool ag_os_is_guest_enabled();

/**
 * mac: 调用方已开辟的栈空间，长度为18字节
 * 实现建议：将mac地址snprintf到mac参数中，格式：12:34:56:78:9A:BC
 * 返回值：成功返回0，失败返回其他值。
 */
extern int ag_os_get_mac_address(char *mac);


/**
 * Whether to disable AI initiative action,
 * Reture true if you want to disable initiative AI QA
 */
extern bool ag_os_ai_initiative_disable(void);

#ifdef __cplusplus
}
#endif
#endif /*_ALIGENIE_PORTING_OS_HEADER_*/
