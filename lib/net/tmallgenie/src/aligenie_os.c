#include "aligenie_os.h"
#include "device/device.h"
#include "fs/fs.h"
#include "wifi/wifi_connect.h"
#include "database.h"
#include "os/os_api.h"
#include "system/timer.h"
#include "syscfg/syscfg_id.h"

extern int gettimeofday(struct timeval *tv, void *tz);
extern u32 timer_get_ms(void);
extern int print(char **out, char *end, const char *format, va_list args);


void *AG_OS_MALLOC(unsigned int len)
{
    return malloc(len);
}
void *AG_OS_CALLOC(size_t len, unsigned int size)
{
    return calloc(len, size);
}
void *AG_OS_REALLOC(void *ptr, unsigned int newsize)
{
    return realloc(ptr, newsize);
}
void AG_OS_FREE(void *ptr)
{
    free(ptr);
}

int ag_os_get_free_heap_size(void)
{
    return 0;
}

static const char sdk_config[] = "XdQnuGyO xxxx 5479295d2ad548b69f448b810c575f83 jieli AC790N AG_KIDS_STORY FreeRTOS b3Iieu0XoXiEDF8FpXbf0F6fv6F3XoH0p33F38";

int ag_os_read_sdk_config(char *buf)
{
    strcpy(buf, sdk_config);
    return 0;
}

unsigned int ag_os_get_feature_config(void)
{
    return AG_FEATURE_ASK_CMD_QUEUE | AG_FEATURE_VOICE_MESSAGE | AG_FEATURE_VOLUME_CONTROL ;	//TODO
}

int ag_os_read_sdcard_file(const char *filename, char *buf, int bufLen)
{
    char name[64];
    int len = 0;

    const char *p = strrchr(filename, '/');
    if (!p) {
        return 0;
    }

    snprintf(name, sizeof(name), "storage/sd0/C%s", p);

    FILE *fp = fopen(name, "r");
    if (fp) {
        len = fread(buf, 1, bufLen, fp);
        fclose(fp);
    }

    return len;
}

int ag_os_get_firmware_version(char *firmware_version)
{
    strcpy(firmware_version, FIRMWARE_VERSION);
    return 0;
}

int ag_os_generate_md5(unsigned char *out, const unsigned char *data, unsigned int datalen)
{
    mbedtls_md5(data, datalen, out);
    return 0;
}

int ag_os_generate_hmac_sha256(unsigned char *out,
                               const unsigned char *data, unsigned int datalen,
                               const unsigned char *key, unsigned int keylen)
{
    if (0 != mbedtls_md_hmac(&mbedtls_sha256_info, (unsigned char *)key, keylen, (unsigned char *)data, datalen, out)) {
        return -1;
    }
    return 0;
}

static AG_OS_LOGLEVEL_E ag_log_level = AG_OS_LOGLEVEL_ALL;

void ag_os_log_set_level(AG_OS_LOGLEVEL_E level)
{
    ag_log_level = level;
}

void ag_os_log_print(const char *tag,
                     AG_OS_LOGLEVEL_E loglevel,
                     const char *format,
                     ...)
{

    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
}

time_t ag_os_get_time(time_t *t)
{
    return time(t);
}

struct tm *ag_os_get_gmtime(const time_t *timep)
{
    static struct tm t;
    return gmtime_r(timep, &t);
}

int ag_os_gettimeofday(struct timeval *restrict tp, void *restrict tzp)
{
    return gettimeofday(tp, tzp); //这函数只能差值
}

struct tm *ag_os_get_localtime_r(const time_t *timep, struct tm *result)
{
    time_t t = time(NULL) + 28800;
    return localtime_r(&t, result);
}

int ag_os_timediff_ms_get(uint32_t *ms_current)
{
#if 0
    static long long time1 = 0;

    long long time2 = timer_get_ms();

    *ms_current = time2 - time1;

    time1 = time2;
#else
    *ms_current = timer_get_ms();

#endif
    return 0;
}

struct vm2data {
    char record;
    char key[AG_OS_FLASH_MAX_LENGTH_KEY];
    char outval[AG_OS_FLASH_MAX_LENGTH_VALUE];
};

int ag_os_flash_read(const char *const key, char *outval, size_t len)
{
    int ret = 0;
    struct vm2data *v = AG_OS_MALLOC(sizeof(struct vm2data));

    if (!v) {
        return -1;
    }

    for (int i = VM_AG_KEY_INFO_IDX_START; i <= VM_AG_KEY_INFO_IDX_END; i++) {
        ret = syscfg_read(i, (void *)v, sizeof(struct vm2data));
        if (ret < 0) {
            continue;
        }
        if (!memcmp(v->key, key, strlen(key))) {
            memcpy(outval, v->outval, len);
            free(v);
            return 0;
        }
        memset(v, 0, sizeof(struct vm2data));
    }

    free(v);
    return -1;
}

int ag_os_flash_write(const char *const key, const char *outval, size_t len)
{
    int ret = 0;
    struct vm2data *v = AG_OS_MALLOC(sizeof(struct vm2data));

    if (!v) {
        return -1;
    }

    for (int i = VM_AG_KEY_INFO_IDX_START; i <= VM_AG_KEY_INFO_IDX_END; i++) {
        ret = syscfg_read(i, (void *)v, sizeof(struct vm2data));
        if (ret > 0 && v->record == 1) {
            if (!memcmp(v->key, key, strlen(key))) {
                memcpy(v->outval, outval, len);
                syscfg_write(i, (void *)v, sizeof(struct vm2data));
                free(v);
                return 0;
            }
        } else {
            memcpy(v->key, key, strlen(key));
            memcpy(v->outval, outval, len);
            v->record = 1;
            syscfg_write(i, (void *)v, sizeof(struct vm2data));
            free(v);
            return 0;
        }
        memset(v, 0, sizeof(struct vm2data));
    }

    free(v);

    return -1;
}

int ag_os_task_create(AG_TASK_T *task_handler,
                      const char *name,
                      ag_os_task_runner func,
                      AG_TASK_PRIORITY_E priority,
                      int max_stack_size,
                      void *arg)
{
    return thread_fork(name, priority, max_stack_size / 4, 0, (int *)task_handler, (void (*)(void *p_arg))func, arg);
}

int ag_os_task_destroy(AG_TASK_T *task_handler)
{
    thread_kill((int *)task_handler, KILL_WAIT);
    return 0;
}

void ag_os_task_mutex_init(AG_MUTEX_T *mutex)
{
    OS_MUTEX *m = calloc(sizeof(OS_MUTEX), 1);
    if (m) {
        os_mutex_create(m);
    }
    *mutex = m;
}
void ag_os_task_mutex_lock(AG_MUTEX_T *mutex)
{
    os_mutex_pend((OS_MUTEX *)*mutex, 0);
}
void ag_os_task_mutex_unlock(AG_MUTEX_T *mutex)
{
    os_mutex_post((OS_MUTEX *)*mutex);
}
void ag_os_task_mutex_delete(AG_MUTEX_T *mutex)
{
    if (*mutex) {
        os_mutex_del((OS_MUTEX *)*mutex, 0);
        free(*mutex);
    }
}

void ag_os_task_cond_init(AG_COND_T *cond)
{
    OS_SEM *sem = calloc(sizeof(OS_SEM), 1);
    if (sem) {
        os_sem_create(sem, 0);
    }
    *cond = sem;
}
void ag_os_task_cond_wait(AG_COND_T *cond)
{
    os_sem_pend((OS_SEM *)*cond, 0);
}
void ag_os_task_cond_signal(AG_COND_T *cond)
{
    os_sem_post((OS_SEM *)*cond);
}

void ag_os_task_mdelay(uint32_t time_ms)
{
    msleep(time_ms);
}

int ag_os_queue_create(AG_QUEUE_T *handler, const char *name, uint32_t message_size, uint32_t number_of_messages)
{
    OS_QUEUE *queue = NULL;

    if (NULL == (queue = (OS_QUEUE *)zalloc(sizeof(OS_QUEUE) + message_size * number_of_messages))) {
        return -1;
    }

    ASSERT(message_size == sizeof(void *));

    if (0 != os_q_create_static(queue, (u8 *)queue + sizeof(OS_QUEUE), number_of_messages)) {
        free(queue);
        return -1;
    }

    *handler = (AG_QUEUE_T)queue;
    return 0;
}
int ag_os_queue_push_to_back(AG_QUEUE_T *handler, void *msg,  uint32_t timeout_ms)
{
    while (1) {
        if (OS_NO_ERR == os_q_post_to_back((OS_QUEUE *)*handler, msg, 0)) {
            break;
        }
        ag_os_task_mdelay(timeout_ms);
    }

    return 0;
}
int ag_os_queue_push_to_front(AG_QUEUE_T *handler, void *msg,  uint32_t timeout_ms)
{
    while (1) {
        if (OS_NO_ERR == os_q_post_to_front((OS_QUEUE *)*handler, msg, 0)) {
            break;
        }
        ag_os_task_mdelay(timeout_ms);
    }

    return 0;
}
int ag_os_queue_pop(AG_QUEUE_T *handler, void *msg,  uint32_t timeout_ms)
{
    return os_q_recv((OS_QUEUE *)*handler, msg, timeout_ms);
}
int ag_os_queue_destroy(AG_QUEUE_T *handler)
{
    os_q_del_static((OS_QUEUE *)*handler, 0);
    free(*handler);
    *handler = NULL;
    return 0;
}
bool ag_os_queue_is_empty(AG_QUEUE_T *handler)
{
    return os_q_query((OS_QUEUE *)*handler) ? 0 : 1;
}
bool ag_os_queue_is_full(AG_QUEUE_T *handler)
{
    return os_q_is_full((OS_QUEUE *)*handler);
}

typedef struct {
    unsigned short id;
    unsigned char oneshort;
    ag_os_timer_callback cb;
    u32 msec;
    void *priv;
} OS_Timer_t;

int ag_os_timer_create(AG_TIMER_T *handler, uint32_t time_interval_ms, bool is_auto_reset, ag_os_timer_callback func, void *arg)
{
    OS_Timer_t *timer = (OS_Timer_t *)zalloc(sizeof(OS_Timer_t));
    if (!timer) {
        *handler = NULL;
        return -1;
    }

    timer->cb = func;
    timer->priv = arg;

    if (time_interval_ms > 0 && time_interval_ms < 10) {
        time_interval_ms = 10;
    }

    timer->msec = time_interval_ms;
    timer->oneshort = !is_auto_reset;
    *handler = timer;

    return 0;
}
int ag_os_timer_start(AG_TIMER_T *handler)
{
    OS_Timer_t *timer = (OS_Timer_t *)*handler;
    if (timer->oneshort) {
        timer->id = sys_timeout_add_to_task("sys_timer", timer->priv, timer->cb, timer->msec);
    } else {
        timer->id = sys_timer_add_to_task("sys_timer", timer->priv, timer->cb, timer->msec);
    }

    return 0;
}
int ag_os_timer_stop(AG_TIMER_T *handler)
{
    OS_Timer_t *timer = (OS_Timer_t *)*handler;

    if (timer->id) {
        sys_timeout_del(timer->id);
        timer->id = 0;
    }

    return 0;

}
int ag_os_timer_destroy(AG_TIMER_T *handler)
{
    ag_os_timer_stop(handler);
    free(*handler);
    return 0;
}
void *ag_os_timer_get_arg(AG_TIMER_T *handler)
{
    OS_Timer_t *timer = (OS_Timer_t *)*handler;

    return timer->priv;
}

int ag_os_eventgroup_create(AG_EVENTGROUP_T *handler)
{
    return os_eventgroup_create((OS_EVENT_GRP **)handler, 0);
}
int ag_os_eventgroup_delete(AG_EVENTGROUP_T *handler)
{
    return os_eventgroup_delete((OS_EVENT_GRP *)*handler, 1);
}
int ag_os_eventgroup_set_bits(AG_EVENTGROUP_T *handler, int bits)
{
    return os_eventgroup_set_bits((OS_EVENT_GRP *)*handler, bits);
}
int ag_os_eventgroup_clear_bits(AG_EVENTGROUP_T *handler, int bits)
{
    return os_eventgroup_clear_bits((OS_EVENT_GRP *)*handler, bits);
}
int ag_os_eventgroup_wait_bits(AG_EVENTGROUP_T *handler, int bits, int isClearOnExit, int isWaitForAllBits, int waitMs)
{
    return os_eventgroup_wait_bits((OS_EVENT_GRP *)*handler, bits, isClearOnExit, isWaitForAllBits, waitMs);
}

AG_OS_AUTO_VAD_TYPE_E ag_os_get_auto_vad_type(void)
{
    return AG_OS_AUTO_VAD_ON;
}

bool ag_os_is_guest_enabled(void)
{
    return 0;
}

int ag_os_get_mac_address(char *mac)
{
    u8 hmac[6];
    wifi_get_mac(hmac);
    sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x", hmac[0], hmac[1], hmac[2], hmac[3], hmac[4], hmac[5]);
    return 0;
}

bool ag_os_ai_initiative_disable(void)
{
    return 1;
}

int	rand_r(unsigned *__seed)
{
    return rand();
}

