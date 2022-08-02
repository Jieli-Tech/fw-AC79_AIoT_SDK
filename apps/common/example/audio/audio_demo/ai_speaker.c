#include "system/app_core.h"
#include "os/os_api.h"
#include "app_config.h"
#include "event/key_event.h"

#ifdef USE_AUDIO_DEMO

#if defined CONFIG_ASR_ALGORITHM_ENABLE && CONFIG_ASR_ALGORITHM == AISP_ALGORITHM

extern int aisp_open(u16 sample_rate);
extern void aisp_suspend(void);
extern void aisp_resume(void);
extern int aisp_close(void);

static int ai_speaker_mode_init(void)
{
    log_i("ai_speaker_play_main\n");

    return aisp_open(16000);
}

static void ai_speaker_mode_exit(void)
{
    aisp_close();
}

static int ai_speaker_key_click(struct key_event *key)
{
    int ret = true;

    switch (key->value) {
    case KEY_OK:
        break;
    default:
        ret = false;
        break;
    }

    return ret;
}

static int ai_speaker_key_long(struct key_event *key)
{
    return true;
}

static int ai_speaker_key_event_handler(struct key_event *key)
{
    switch (key->action) {
    case KEY_EVENT_CLICK:
        return ai_speaker_key_click(key);
    case KEY_EVENT_LONG:
        return ai_speaker_key_long(key);
    default:
        break;
    }

    return true;
}

static int ai_speaker_event_handler(struct application *app, struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        return ai_speaker_key_event_handler((struct key_event *)event->payload);
    default:
        return false;
    }
}

static int ai_speaker_state_machine(struct application *app, enum app_state state, struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        break;
    case APP_STA_START:
        ai_speaker_mode_init();
        break;
    case APP_STA_PAUSE:
        break;
    case APP_STA_RESUME:
        break;
    case APP_STA_STOP:
        ai_speaker_mode_exit();
        break;
    case APP_STA_DESTROY:
        break;
    }

    return 0;
}

static const struct application_operation ai_speaker_ops = {
    .state_machine  = ai_speaker_state_machine,
    .event_handler 	= ai_speaker_event_handler,
};

REGISTER_APPLICATION(ai_speaker) = {
    .name 	= "ai_speaker",
    .ops 	= &ai_speaker_ops,
    .state  = APP_STA_DESTROY,
};

#endif

#endif//USE_AUDIO_DEMO
