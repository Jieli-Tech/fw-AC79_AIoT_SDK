// Copyright (2017) Baidu Inc. All rights reserved.
//
// File: baidu_ca_adapter.c
// Auth: Zhang Leliang (zhangleliang@baidu.com)
// Desc: Adapt the IoT CA to different platform.

#include "duerapp_config.h"
#include "lightduer_adapter.h"

#include "os/os_api.h"

#include "baidu_ca_adapter_internal.h"

#include "lightduer_debug.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"
#include "lightduer_net_transport.h"
#include "lightduer_sleep.h"
#include "lightduer_timestamp.h"
#include "lightduer_random.h"
#include "lightduer_random_impl.h"
#ifdef ENABLE_REPORT_SYSTEM_INFO
#include "lightduer_statistics.h"
#endif

extern u32 timer_get_ms(void);

static duer_u32_t duer_timestamp_obtain()
{
    return (duer_u32_t)timer_get_ms();
}

static void duer_sleep_impl(duer_u32_t ms)
{
    msleep(ms);
}

void baidu_ca_adapter_initialize()
{
    baidu_ca_memory_init(
        NULL,
        bcamem_alloc,
        bcamem_realloc,
        bcamem_free);

    baidu_ca_mutex_init(
        create_mutex,
        lock_mutex,
        unlock_mutex,
        delete_mutex_lock);

    baidu_ca_debug_init(NULL, bcadbg);

    bcasoc_initialize();

    baidu_ca_transport_init(
        bcasoc_create,
        bcasoc_connect,
        bcasoc_send,
        bcasoc_recv,
        bcasoc_recv_timeout,
        bcasoc_close,
        bcasoc_destroy);

    baidu_ca_timestamp_init(duer_timestamp_obtain);

    baidu_ca_sleep_init(duer_sleep_impl);

    duer_random_init(duer_random_impl);

#ifdef ENABLE_REPORT_SYSTEM_INFO
    duer_statistics_initialize();
#endif

#ifdef DUER_HEAP_MONITOR
    baidu_ca_memory_hm_init();
#endif // DUER_HEAP_MONITOR
}

void baidu_ca_adapter_finalize(void)
{
#ifdef ENABLE_REPORT_SYSTEM_INFO
    duer_statistics_finalize();
#endif

    /* bcasoc_finalize(); */

    baidu_ca_memory_uninit();

#ifdef DUER_HEAP_MONITOR
    baidu_ca_memory_hm_destroy();
#endif // DUER_HEAP_MONITOR
}
