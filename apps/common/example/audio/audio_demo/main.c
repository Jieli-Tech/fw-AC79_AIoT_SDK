#include "server/audio_server.h"
#include "server/server_core.h"
#include "app_config.h"
#include "syscfg/syscfg_id.h"
#include "system/app_core.h"
#include "event/key_event.h"
#include "fs/fs.h"
#include "system/includes.h"
#include "event/device_event.h"
#include "event/net_event.h"

#ifdef USE_AUDIO_DEMO

static int main_key_event_handler(struct key_event *key)
{
    switch (key->action) {
    case KEY_EVENT_CLICK:
        switch (key->value) {
        case KEY_MODE:
            audio_demo_mode_switch();
            break;
        default:
            return false;
        }
        break;
    case KEY_EVENT_LONG:
        break;
    default:
        return false;
    }

    return true;
}

static int main_dev_event_handler(struct device_event *event)
{
    switch (event->event) {
    case DEVICE_EVENT_IN:
        break;
    case DEVICE_EVENT_OUT:
        break;
    case DEVICE_EVENT_CHANGE:
        break;
    }
    return 0;
}

#ifdef CONFIG_NET_ENABLE
static int main_net_event_hander(struct net_event *event)
{
    switch (event->event) {
    case NET_EVENT_CMD:
        break;
    case NET_EVENT_DATA:
        break;
    }

    return false;
}
#endif

/*
 *  * 默认的系统事件处理函数
 *   * 当所有活动的app的事件处理函数都返回false时此函数会被调用
 *    */
void app_default_event_handler(struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        main_key_event_handler((struct key_event *)event->payload);
        break;
    case SYS_TOUCH_EVENT:
        break;
    case SYS_DEVICE_EVENT:
        main_dev_event_handler((struct device_event *)event->payload);
        break;
#ifdef CONFIG_NET_ENABLE
    case SYS_NET_EVENT:
        main_net_event_hander((struct net_event *)event->payload);
        break;
#endif

    default:
        break;
    }
}


struct audio_app_t {
    const char *tone_file_name;
    const char *app_name;
};

static const struct audio_app_t audio_app_table[] = {
#ifdef CONFIG_LOCAL_MUSIC_MODE_ENABLE
    {"FlashMusic.mp3", "local_music"},
#endif
#ifdef CONFIG_NET_MUSIC_MODE_ENABLE
    {"NetMusic.mp3", "net_music"  },
#endif
#ifdef CONFIG_RECORDER_MODE_ENABLE
    {"Recorder.mp3", "recorder"},
#endif
#ifdef CONFIG_ASR_ALGORITHM_ENABLE
    {"AiSpeaker.mp3", "ai_speaker"},
#endif
};

static u8 mode_index = 0;

static void dec_server_event_handler(void *priv, int argc, int *argv)
{
    union audio_req r = {0};

    switch (argv[0]) {
    case AUDIO_SERVER_EVENT_ERR:
        log_i("tone: AUDIO_SERVER_EVENT_ERR\n");
    case AUDIO_SERVER_EVENT_END:
        log_i("tone: AUDIO_SERVER_EVENT_END\n");
        r.dec.cmd = AUDIO_DEC_STOP;
        server_request(priv, AUDIO_REQ_DEC, &r);
        server_close(priv); //priv是server_register_event_handler_to_task的priv参数
        fclose((FILE *)argv[1]); //argv[1]是解码开始时传递进去的文件句柄
        key_event_enable();
        //等提示音播完了再切换模式
        struct intent it;
        init_intent(&it);
        it.name = audio_app_table[mode_index++].app_name;
        start_app(&it);
        break;
    case AUDIO_SERVER_EVENT_CURR_TIME:
        log_i("play_time: %d\n", argv[1]);
        break;
    default:
        break;
    }
}

//播放提示音
static int app_play_tone_file(const char *path)
{
    int err = 0;
    union audio_req req = {0};

    log_d("play tone file : %s\n", path);

    FILE *file = fopen(path, "r");
    if (!file) {
        return -1;
    }

    void *dec_server = server_open("audio_server", "dec");
    if (!dec_server) {
        goto __err;
    }
    server_register_event_handler_to_task(dec_server, dec_server, dec_server_event_handler, "app_core");

    req.dec.cmd             = AUDIO_DEC_OPEN;
    req.dec.volume          = 50;
    req.dec.output_buf_len  = 4 * 1024;
    req.dec.file            = file;
    req.dec.sample_source   = "dac";

    syscfg_read(CFG_MUSIC_VOL, &req.dec.volume, sizeof(req.dec.volume));

    err = server_request(dec_server, AUDIO_REQ_DEC, &req);
    if (err) {
        goto __err;
    }

    req.dec.cmd = AUDIO_DEC_START;

    err = server_request(dec_server, AUDIO_REQ_DEC, &req);
    if (err) {
        goto __err;
    }

    key_event_disable();

    return 0;

__err:

    if (dec_server) {
        server_close(dec_server);
    }
    if (file) {
        fclose(file);
    }

    return err;
}

int audio_demo_mode_switch(void)
{
    if (mode_index >= ARRAY_SIZE(audio_app_table)) {
        mode_index = 0;
    }
    struct intent it;

    if (get_current_app()) {
        init_intent(&it);
        it.name = audio_app_table[mode_index].app_name;
        it.action = ACTION_STOP;    //退出当前模式
        start_app(&it);
    }

    char path[64];

    sprintf(path, "%s%s", CONFIG_VOICE_PROMPT_FILE_PATH, audio_app_table[mode_index].tone_file_name);

    return app_play_tone_file(path);
}

static void audio_demo_init(void)
{
    os_time_dly(20);
    audio_demo_mode_switch();
    while (1) {
        os_time_dly(2);
    }
}

static int audio_demo_init_task(void)
{
    return thread_fork("audio_demo_init", 11, 1024, 0, 0, audio_demo_init, NULL);
}
late_initcall(audio_demo_init_task);

#endif//USE_AUDIO_DEMO
