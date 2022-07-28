#include "server/audio_server.h"
#include "server/server_core.h"
#include "app_config.h"
#include "storage_device.h"
#include "audio_music/audio_digital_vol.h"
#include "event/key_event.h"
#include "event/device_event.h"
#include "event/net_event.h"
#include "syscfg/syscfg_id.h"
#include "system/wait.h"
#include "system/app_core.h"
#include "lwip.h"

#ifdef CONFIG_NET_MUSIC_MODE_ENABLE

#include "server/ai_server.h"
#include "network_download/net_download.h"

#define CONFIG_STORE_VOLUME
#define VOLUME_STEP 5
#define MIN_VOLUME_VALUE	5
#define MAX_VOLUME_VALUE	100
#define INIT_VOLUME_VALUE   50

struct net_music_hdl {
    char volume;
    u16 wait_download;
    int download_ready;
    int play_time;
    int total_time;
    void *net_file;
    const char *ai_name;
    struct server *dec_server;
    struct server *ai_server;
    void *digital_vol_hdl;
    char *url; //保存断点歌曲的链接
    struct audio_dec_breakpoint dec_bp;
};

static struct net_music_hdl net_music_handler;

#define __this 	(&net_music_handler)

//网络解码需要的数据访问句柄
static const struct audio_vfs_ops net_audio_dec_vfs_ops = {
    .fread = net_download_read,
    .fseek = net_download_seek,
    .flen  = net_download_get_file_len,
};

int get_app_music_volume(void)
{
    return __this->volume;
}

int get_app_music_playtime(void)
{
    return __this->play_time;
}

int get_app_music_total_time(void)
{
    return __this->total_time;
}

#ifdef CONFIG_DEC_DIGITAL_VOLUME_ENABLE
static int audio_dec_data_callback(u8 *buf, u32 len, u32 sample_rate, u8 ch_num)
{
    u32 rdlen = 0, i = 0;

    while (i < len) {
        rdlen = i + 32 * 2 * ch_num > len ? len - i : 32 * 2 * ch_num;
        user_audio_digital_volume_run(__this->digital_vol_hdl, buf + i, rdlen, sample_rate, ch_num);
        i += rdlen;
    }

    return len;
}
#endif

//检查音频需要的格式检查数据头部是否下载缓冲完成
static int __net_download_ready(void *p)
{
    __this->download_ready = net_download_check_ready(__this->net_file);
    if (__this->download_ready) {
        return 1;
    }
    return 0;
}

//暂停/继续播放
static int net_music_dec_play_pause(u8 notify)
{
    union audio_req r = {0};
    union ai_req req  = {0};

#ifdef CONFIG_DEC_ANALOG_VOLUME_ENABLE
    r.dec.attr = AUDIO_ATTR_FADE_INOUT;
#endif
    r.dec.cmd = AUDIO_DEC_PP;
    server_request(__this->dec_server, AUDIO_REQ_DEC, &r);
    if (r.dec.status == AUDIO_DEC_START) {
        /* 播放状态 */
        net_download_set_pp(__this->net_file, 0);
    } else if (r.dec.status == AUDIO_DEC_PAUSE) {
        /* 暂停状态 */
        net_download_set_pp(__this->net_file, 1);
    }

    if (notify && __this->ai_server && r.dec.status != AUDIO_DEC_STOP) {
        //notify : 是否需要通知云端暂停
        req.evt.event   = AI_EVENT_PLAY_PAUSE;
        req.evt.ai_name = __this->ai_name;
        if (r.dec.status == AUDIO_DEC_PAUSE) {
            req.evt.arg = 1;
        }
        ai_server_request(__this->ai_server, AI_REQ_EVENT, &req);
    }

    return 0;
}

//停止播放
static int net_music_dec_stop(void)
{
    union audio_req r = {0};
    union ai_req req = {0};

    if (!__this->net_file) {
        return 0;
    }

    log_i("net_music_dec_stop\n");

    net_download_buf_inactive(__this->net_file);

    if (__this->wait_download) {
        /*
         * 歌曲还未开始播放，删除wait
         */
        /* sys_timer_del(__this->wait_download); */
        wait_completion_del(__this->wait_download);
        __this->wait_download = 0;
    } else {
        r.dec.cmd = AUDIO_DEC_STOP;
        server_request(__this->dec_server, AUDIO_REQ_DEC, &r);

        int argv[2];
        argv[0] = AUDIO_SERVER_EVENT_END;
        argv[1] = (int)__this->net_file;
        server_event_handler_del(__this->dec_server, 2, argv);
    }

    extern int lwip_canceladdrinfo(void);
    lwip_canceladdrinfo();
    //释放网络下载资源
    net_download_close(__this->net_file);
    __this->net_file = NULL;

    return 0;
}

//播放结束
static int net_music_dec_end(void)
{
    union ai_req req = {0};

    log_i("net_music_dec_end\n");

    net_music_dec_stop();

    if (!__this->ai_server) {
        return 0;
    }

    /* 歌曲播放完成，发送此命令后ai平台会发送新的URL */
    req.evt.event   = AI_EVENT_MEDIA_END;
    req.evt.ai_name     = __this->ai_name;
    return ai_server_request(__this->ai_server, AI_REQ_EVENT, &req);
}

//数据缓冲已完成，开始解码
static int __net_music_dec_file(void *file)
{
    int err;
    union ai_req r = {0};
    union audio_req req = {0};

    __this->wait_download = 0;

    if (__this->download_ready < 0) {
        /* 网络下载失败 */
        goto __err;
    }

    //获取网络资源的格式
    req.dec.dec_type = net_download_get_media_type(file);
    if (req.dec.dec_type == NULL) {
        goto __err;
    }
    log_i("url_file_type: %s\n", req.dec.dec_type);

    net_download_set_read_timeout(file, 5000);

    req.dec.cmd             = AUDIO_DEC_OPEN;
    req.dec.volume          = __this->volume;
    req.dec.output_buf_len  = 6 * 1024;
    req.dec.file            = (FILE *)file;
    req.dec.channel         = 0;
    req.dec.sample_rate     = 0;
    req.dec.priority        = 1;
    req.dec.vfs_ops         = &net_audio_dec_vfs_ops;
    req.dec.sample_source   = CONFIG_AUDIO_DEC_PLAY_SOURCE;
    /* req.dec.bp              = &__this->dec_bp; //恢复断点 */
#if 0	//变声变调功能
    req.dec.speedV = 80; // >80是变快，<80是变慢，建议范围：30到130
    req.dec.pitchV = 32768; // >32768是音调变高，<32768音调变低，建议范围20000到50000
    req.dec.attr = AUDIO_ATTR_PS_EN;
#endif

#ifdef CONFIG_DEC_DIGITAL_VOLUME_ENABLE
    if (!__this->digital_vol_hdl) {
        __this->digital_vol_hdl = user_audio_digital_volume_open(0, 31, 1);
    }
    if (__this->digital_vol_hdl && !__this->play_tts) {
        user_audio_digital_volume_reset_fade(__this->digital_vol_hdl);
        user_audio_digital_volume_set(__this->digital_vol_hdl, 31);
        req.dec.dec_callback = audio_dec_data_callback;
    }
#endif

    err = server_request(__this->dec_server, AUDIO_REQ_DEC, &req);
    if (err) {
        goto __err;
    }

    __this->play_time = req.dec.play_time;
    __this->total_time = req.dec.total_time;

    net_download_set_read_timeout(file, 0);

    req.dec.cmd = AUDIO_DEC_START;

    server_request(__this->dec_server, AUDIO_REQ_DEC, &req);

    net_download_set_pp(file, 0);

    return 0;

__err:
    log_e("play_net_music_faild\n");

    net_download_buf_inactive(file);

    req.dec.cmd = AUDIO_DEC_STOP;
    server_request(__this->dec_server, AUDIO_REQ_DEC, &req);

    net_download_close(file);
    __this->net_file = NULL;

    if (__this->ai_server) {
        r.evt.event   = AI_EVENT_MEDIA_END;
        r.evt.ai_name   = __this->ai_name;
        ai_server_request(__this->ai_server, AI_REQ_EVENT, &r);
    }

    return -1;
}

static int net_music_dec_file(const char *url)
{
    int err;
    struct net_download_parm parm = {0};

    net_music_dec_stop();

    if (!url) {
        return -1;
    }

    log_i("net_download_open\n");

    parm.url                = url;
    //网络缓冲buf大小
#ifdef CONFIG_NO_SDRAM_ENABLE
    parm.cbuf_size          = 150 * 1024;
#else
    parm.cbuf_size          = 500 * 1024;
#endif
    //设置网络下载超时
    parm.timeout_millsec    = 10000;
#ifdef CONFIG_DOWNLOAD_SAVE_FILE
    if (storage_device_ready()) {
        parm.save_file			= 1;
        parm.file_dir			= NULL;
    }
#endif
    parm.seek_threshold     = 1024 * 200;	//用户可适当调整
    /* parm.seek_low_range     = __this->dec_bp.fptr;    //恢复断点时设置网络的开始下载地址 */

    err = net_download_open(&__this->net_file, &parm);
    if (err) {
        log_e("net_download_open: err = %d\n", err);
        return err;
    }

    /*异步等待网络下载ready，防止网络阻塞导致app_core卡住 */
    __this->wait_download = wait_completion(__net_download_ready,
                                            (int (*)(void *))__net_music_dec_file, __this->net_file, NULL);

    return 0;
}

//切换上一首或下一首
static int net_music_dec_switch_file(int fsel_mode)
{
    union ai_req req = {0};

    if (!__this->ai_server) {
        return 0;
    }

    log_i("net_music_dec_switch_file\n");

    if (!strcmp(__this->ai_name, "dlna")) {
        return 0;
    }

    net_music_dec_stop();

    if (fsel_mode == FSEL_NEXT_FILE) {
        req.evt.event = AI_EVENT_NEXT_SONG;
    } else if (fsel_mode == FSEL_PREV_FILE) {
        req.evt.event = AI_EVENT_PREVIOUS_SONG;
    } else {
        return 0;
    }

    req.evt.ai_name = __this->ai_name;
    return ai_server_request(__this->ai_server, AI_REQ_EVENT, &req);
}

//获取断点数据
static int net_music_get_dec_breakpoint(struct audio_dec_breakpoint *bp)
{
    int err;
    union audio_req r = {0};

    bp->len = 0;
    r.dec.bp = bp;
    r.dec.cmd = AUDIO_DEC_GET_BREAKPOINT;

    if (bp->data) {
        free(bp->data);
        bp->data = NULL;
    }

    err = server_request(__this->dec_server, AUDIO_REQ_DEC, &r);
    if (err) {
        return err;
    }

    if (r.dec.status == AUDIO_DEC_STOP) {
        bp->len = 0;
        free(bp->data);
        bp->data = NULL;
        return -1;
    }
    /* put_buf(bp->data, bp->len); */

    return 0;
}

//设置音量大小
static int net_music_set_dec_volume(int step)
{
    union audio_req req = {0};
    union ai_req ai = {0};

    int volume = __this->volume + step;
    if (volume < MIN_VOLUME_VALUE) {
        volume = MIN_VOLUME_VALUE;
    } else if (volume > MAX_VOLUME_VALUE) {
        volume = MAX_VOLUME_VALUE;
    }
    if (volume == __this->volume) {
        return -1;
    }
    __this->volume = volume;

    log_d("set_dec_volume: %d\n", volume);

    req.dec.cmd     = AUDIO_DEC_SET_VOLUME;
    req.dec.volume  = volume;
    server_request(__this->dec_server, AUDIO_REQ_DEC, &req);

#ifdef CONFIG_STORE_VOLUME
    syscfg_write(CFG_MUSIC_VOL, &__this->volume, sizeof(__this->volume));
#endif

    if (__this->ai_server) {
        ai.evt.event       = AI_EVENT_VOLUME_CHANGE;
        ai.evt.arg         = __this->volume;
        ai.evt.ai_name     = __this->ai_name;
        return ai_server_request(__this->ai_server, AI_REQ_EVENT, &ai);
    }

    return 0;
}

//获取解码器状态
static int net_music_get_dec_status(void)
{
    union audio_req req = {0};

    req.dec.cmd     = AUDIO_DEC_GET_STATUS;
    server_request(__this->dec_server, AUDIO_REQ_DEC, &req);

    return req.dec.status;
}

//快进快退,单位是秒,暂时只支持MP3格式
static int net_music_dec_seek(int seek_step)
{
    int err;
    union audio_req r = {0};

    if (0 == seek_step) {
        return 0;
    }

    if (__this->total_time != 0 && __this->total_time != -1) {
        if (__this->play_time + seek_step <= 0 || __this->play_time + seek_step >= __this->total_time) {
            log_e("local music seek out of range\n");
            return -1;
        }
    }

    if (seek_step > 0) {
        r.dec.cmd = AUDIO_DEC_FF;
        r.dec.ff_fr_step = seek_step;
    } else {
        r.dec.cmd = AUDIO_DEC_FR;
        r.dec.ff_fr_step = -seek_step;
    }

    log_i("net music seek step : %d\n", seek_step);

    return server_request(__this->dec_server, AUDIO_REQ_DEC, &r);
}

static void dec_server_event_handler(void *priv, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_SERVER_EVENT_ERR:
        log_i("net_music: AUDIO_SERVER_EVENT_ERR\n");
    case AUDIO_SERVER_EVENT_END:
        log_i("net_music: AUDIO_SERVER_EVENT_END\n");
        net_music_dec_end();
        break;
    case AUDIO_SERVER_EVENT_CURR_TIME:
        log_d("play_time: %d\n", argv[1]);
        __this->play_time = argv[1];
        break;
    }
}

//第三方平台的事件通知回调
static void ai_server_event_handler(void *priv, int argc, int *argv)
{
    if (!__this->ai_server) {
        switch (argv[0]) {
        case AI_SERVER_EVENT_URL:
            free((void *)argv[1]);
            break;
        }
        return;
    }

    switch (argv[0]) {
    case AI_SERVER_EVENT_CONNECTED:
        break;
    case AI_SERVER_EVENT_DISCONNECTED:
        break;
    case AI_SERVER_EVENT_URL:
    case AI_SERVER_EVENT_URL_TTS:
    case AI_SERVER_EVENT_URL_MEDIA:
        __this->total_time = 0;	//清空上一首歌的信息
        __this->ai_name = (const char *)argv[2];
        const char *url = (const char *)argv[1];
        free(__this->url);
        __this->url = malloc(strlen(url) + 1);
        if (__this->url) {
            strcpy(__this->url, url);
        }
        net_music_dec_file(url);
        break;
    case AI_SERVER_EVENT_CONTINUE:
        if (AUDIO_DEC_PAUSE == net_music_get_dec_status()) {
            net_music_dec_play_pause(0);
        }
        break;
    case AI_SERVER_EVENT_PAUSE:
        if (AUDIO_DEC_START == net_music_get_dec_status()) {
            net_music_dec_play_pause(0);
        }
        break;
    case AI_SERVER_EVENT_STOP:
        net_music_dec_stop();
        break;
    case AI_SERVER_EVENT_RESUME_PLAY:
        break;
    case AI_SERVER_EVENT_SEEK:
        net_music_dec_seek(argv[1] - __this->play_time);
        break;
    case AI_SERVER_EVENT_VOLUME_CHANGE:
        net_music_set_dec_volume(argv[1] - __this->volume);
        break;
    case AI_SERVER_EVENT_SET_PLAY_TIME:
        __this->play_time = argv[1];
        break;
    default:
        break;
    }
}

static int net_music_mode_init(void)
{
    log_i("net_music_play_main\n");

    memset(__this, 0, sizeof(struct net_music_hdl));

#ifdef CONFIG_STORE_VOLUME
    if (syscfg_read(CFG_MUSIC_VOL, &__this->volume, sizeof(__this->volume)) < 0 ||
        __this->volume < MIN_VOLUME_VALUE || __this->volume > MAX_VOLUME_VALUE) {
        __this->volume = INIT_VOLUME_VALUE;
    }
#else
    __this->volume = INIT_VOLUME_VALUE;
#endif
    if (__this->volume < 0 || __this->volume > MAX_VOLUME_VALUE) {
        __this->volume = INIT_VOLUME_VALUE;
    }

    __this->ai_name = "unknown";

    __this->dec_server = server_open("audio_server", "dec");
    if (!__this->dec_server) {
        return -1;
    }
    server_register_event_handler_to_task(__this->dec_server, NULL, dec_server_event_handler, "app_core");

    if (lwip_dhcp_bound()) {
        __this->ai_server = server_open("ai_server", NULL);
        if (__this->ai_server) {
            union ai_req req = {0};
            server_register_event_handler_to_task(__this->ai_server, NULL, ai_server_event_handler, "app_core");
            ai_server_request(__this->ai_server, AI_REQ_CONNECT, &req);
        }
    }

    return 0;
}

static void net_music_mode_exit(void)
{
    net_music_dec_stop();
    server_close(__this->dec_server);
    __this->dec_server = NULL;
    if (__this->ai_server) {
        server_close(__this->ai_server);
        __this->ai_server = NULL;
    }
    free(__this->url);
    __this->url = NULL;
}

static int net_music_key_click(struct key_event *key)
{
    int ret = true;

    switch (key->value) {
    case KEY_OK:
        net_music_dec_play_pause(1);
        break;
    case KEY_VOLUME_DEC:
        net_music_set_dec_volume(-VOLUME_STEP);
        break;
    case KEY_VOLUME_INC:
        net_music_set_dec_volume(VOLUME_STEP);
        break;
    default:
        ret = false;
        break;
    }

    return ret;
}

static int net_music_key_long(struct key_event *key)
{
    switch (key->value) {
    case KEY_OK:
        /* net_music_dec_file(""); */
        break;
    case KEY_VOLUME_DEC:
        net_music_dec_switch_file(FSEL_PREV_FILE);
        break;
    case KEY_VOLUME_INC:
        net_music_dec_switch_file(FSEL_NEXT_FILE);
        break;
    case KEY_MODE:
        net_music_get_dec_breakpoint(&__this->dec_bp);
        break;
    default:
        break;
    }

    return true;
}

static int net_music_key_event_handler(struct key_event *key)
{
    switch (key->action) {
    case KEY_EVENT_CLICK:
        return net_music_key_click(key);
    case KEY_EVENT_LONG:
        return net_music_key_long(key);
    default:
        break;
    }

    return true;
}

static int net_music_net_event_handler(struct net_event *event)
{
    switch (event->event) {
    case NET_EVENT_CONNECTED:
        if (!__this->ai_server) {
            __this->ai_server = server_open("ai_server", NULL);
            if (__this->ai_server) {
                union ai_req req = {0};
                server_register_event_handler_to_task(__this->ai_server, NULL, ai_server_event_handler, "app_core");
                ai_server_request(__this->ai_server, AI_REQ_CONNECT, &req);
            }
        }
        break;
    default:
        break;
    }

    return false;
}

static int net_music_event_handler(struct application *app, struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        return net_music_key_event_handler((struct key_event *)event->payload);
    case SYS_NET_EVENT:
        return net_music_net_event_handler((struct net_event *)event->payload);
    default:
        return false;
    }
}

static int net_music_state_machine(struct application *app, enum app_state state, struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        break;
    case APP_STA_START:
        net_music_mode_init();
        break;
    case APP_STA_PAUSE:
        break;
    case APP_STA_RESUME:
        break;
    case APP_STA_STOP:
        net_music_mode_exit();
        break;
    case APP_STA_DESTROY:
        break;
    }

    return 0;
}

static const struct application_operation net_music_ops = {
    .state_machine  = net_music_state_machine,
    .event_handler 	= net_music_event_handler,
};

REGISTER_APPLICATION(net_music) = {
    .name 	= "net_music",
    .ops 	= &net_music_ops,
    .state  = APP_STA_DESTROY,
};

#endif
