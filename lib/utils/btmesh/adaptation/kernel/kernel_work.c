#include "adaptation.h"
#include "system/timer.h"

#define LOG_TAG             "[MESH-kwork]"
/* #define LOG_INFO_ENABLE */
#define LOG_DEBUG_ENABLE
#define LOG_WARN_ENABLE
#define LOG_ERROR_ENABLE
#define LOG_DUMP_ENABLE
#include "mesh_log.h"


#if ADAPTATION_COMPILE_DEBUG

u32 k_uptime_get(void)
{
    return 0;
}

u32 k_uptime_get_32(void)
{
    return 0;
}

u32 k_delayed_work_remaining_get(struct k_delayed_work *timer)
{
    return 0;
}

void k_delayed_work_cancel(struct k_delayed_work *timer) {}

void k_delayed_work_submit(struct k_delayed_work *timer, u32 timeout) {}

void k_work_submit(struct k_work *work) {}

void k_delayed_work_init(struct k_delayed_work *timer, void *callback) {}

#else

#define WORK_CALLBACK(_func, _param)   ((void (*)(struct k_work *))_func)(_param)

u32 k_uptime_get(void)
{
    return sys_timer_get_ms();
}

u32 k_uptime_get_32(void)
{
    return sys_timer_get_ms();
}

u32 k_delayed_work_remaining_get(struct k_delayed_work *timer)
{
    return timer->end_time - k_uptime_get_32();
}

void k_delayed_work_cancel(struct k_delayed_work *timer)
{
    BT_INFO("--func=%s", __FUNCTION__);

    if (timer->work.systimer) {
        sys_timeout_del(timer->work.systimer);
        BT_INFO("timer remove id = %u, 0x%x", timer->work.systimer, timer);
    }

    timer->work.systimer = 0;
}

static void k_delayed_work_cb_entry(void *priv)
{
    struct k_delayed_work *timer = (struct k_delayed_work *)priv;

    BT_INFO("--func=%s, 0x%x", __FUNCTION__, timer);

    k_delayed_work_cancel(timer);

    WORK_CALLBACK(timer->work.callback, &timer->work);
}

void k_delayed_work_submit(struct k_delayed_work *timer, u32 timeout)
{
    BT_INFO("--func=%s", __FUNCTION__);
    BT_INFO("timeout= %d ms", timeout);

    /* k_delayed_work_cancel(timer); */

    if (0 == timer->work.systimer) {
        /* timer->work.systimer = sys_timer_register(timeout, k_delayed_work_cb_entry); */
        /* sys_timer_set_context(timer->work.systimer, timer); */
        timer->work.systimer = sys_timer_add(timer, k_delayed_work_cb_entry, timeout);
        BT_INFO("reg new id = %u, 0x%x", timer->work.systimer, timer);
    } else {
        sys_timer_modify(timer->work.systimer, timeout);
        BT_INFO("only change");
    }

    timer->end_time = k_uptime_get_32() + timeout;

    ASSERT(timer->work.systimer);
}

void k_work_submit(struct k_work *work)
{
    WORK_CALLBACK(work->callback, work);
}

void k_delayed_work_init(struct k_delayed_work *timer, void *callback)
{
    timer->work.callback = callback;
}

#endif /* ADAPTATION_COMPILE_DEBUG */
