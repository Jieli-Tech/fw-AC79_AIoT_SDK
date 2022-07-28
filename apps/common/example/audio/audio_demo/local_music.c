#include "server/audio_server.h"
#include "server/server_core.h"
#include "app_config.h"
#include "storage_device.h"
#include "reverb_deal.h"
#include "audio_music/audio_digital_vol.h"
#include "event/key_event.h"
#include "event/device_event.h"
#include "syscfg/syscfg_id.h"
#include "system/wait.h"
#include "system/app_core.h"

#ifdef USE_AUDIO_DEMO

#ifdef CONFIG_LOCAL_MUSIC_MODE_ENABLE

#define CONFIG_STORE_VOLUME
#define VOLUME_STEP 5
#define MIN_VOLUME_VALUE	5
#define MAX_VOLUME_VALUE	100
#define INIT_VOLUME_VALUE   50

struct local_music_hdl {
    u8 local_play_all;	//1:全盘播放 0:播放目录
    char volume;
    u8 reverb_enable;
    u16 wait_sd;
    u16 wait_udisk;
    int play_time;
    int total_time;
    FILE *file;
    struct vfscan *fscan;
    struct vfscan *dir_list;
    struct server *dec_server;
    struct audio_dec_breakpoint local_bp;
    const char *local_path;
    void *digital_vol_hdl;
};

static struct local_music_hdl local_music_handler;

#define __this 	(&local_music_handler)

//获取断点数据
static int local_music_get_dec_breakpoint(struct audio_dec_breakpoint *bp)
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
static int local_music_set_dec_volume(int step)
{
    union audio_req req = {0};

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

    return 0;
}

//获取解码器状态
static int local_music_get_dec_status(void)
{
    union audio_req req = {0};

    req.dec.cmd     = AUDIO_DEC_GET_STATUS;
    server_request(__this->dec_server, AUDIO_REQ_DEC, &req);

    return req.dec.status;
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

//暂停/继续播放
static int local_music_dec_play_pause(void)
{
    union audio_req r = {0};

#ifdef CONFIG_DEC_ANALOG_VOLUME_ENABLE
    r.dec.attr = AUDIO_ATTR_FADE_INOUT;
#endif
    r.dec.cmd = AUDIO_DEC_PP;
    return server_request(__this->dec_server, AUDIO_REQ_DEC, &r);
}

//停止播放
static int local_music_dec_stop(void)
{
    int err = 0;
    union audio_req req = {0};

    if (!__this->file) {
        return 0;
    }

    log_i("local_music_dec_stop\n");

    req.dec.cmd = AUDIO_DEC_STOP;
    server_request(__this->dec_server, AUDIO_REQ_DEC, &req);

    int argv[2];
    argv[0] = AUDIO_SERVER_EVENT_END;
    argv[1] = (int)__this->file;
    server_event_handler_del(__this->dec_server, 2, argv);

    fclose(__this->file);
    __this->file = NULL;

    return 0;
}

//解码文件
static int local_music_dec_file(FILE *file)
{
    int err;
    union audio_req req = {0};

    log_i("local_music_dec_local_file\n");

    if (!file) {
        return -1;
    }

    local_music_dec_stop();

    req.dec.cmd             = AUDIO_DEC_OPEN;
    req.dec.volume          = __this->volume;
    req.dec.output_buf_len  = 6 * 1024;
    req.dec.file            = file;
    req.dec.channel         = 0;
    req.dec.sample_rate     = 0;
    req.dec.priority        = 1;
    req.dec.sample_source   = CONFIG_AUDIO_DEC_PLAY_SOURCE;
#if 0	//变声变调功能
    req.dec.speedV = 80; // >80是变快，<80是变慢，建议范围：30到130
    req.dec.pitchV = 32768; // >32768是音调变高，<32768音调变低，建议范围20000到50000
    req.dec.attr = AUDIO_ATTR_PS_EN;
#endif

#if TCFG_EQ_ENABLE && defined EQ_CORE_V1
    req.dec.attr |= AUDIO_ATTR_EQ_EN;
#if TCFG_LIMITER_ENABLE
    req.dec.attr |= AUDIO_ATTR_EQ32BIT_EN;
#endif
#if TCFG_DRC_ENABLE
    req.dec.attr |= AUDIO_ATTR_DRC_EN;
#endif
#endif

#if CONFIG_DEC_DECRYPT_ENABLE
    //播放加密文件
    extern const struct audio_vfs_ops *get_decrypt_vfs_ops(void);
    req.dec.vfs_ops = get_decrypt_vfs_ops();
    req.dec.attr |= AUDIO_ATTR_DECRYPT_DEC;
#endif

#ifdef CONFIG_DEC_DIGITAL_VOLUME_ENABLE
    if (!__this->digital_vol_hdl) {
        __this->digital_vol_hdl = user_audio_digital_volume_open(0, 31, 1);
    }
    if (__this->digital_vol_hdl) {
        user_audio_digital_volume_reset_fade(__this->digital_vol_hdl);
        user_audio_digital_volume_set(__this->digital_vol_hdl, 31);
        req.dec.dec_callback = audio_dec_data_callback;
    }
#endif

    err = server_request(__this->dec_server, AUDIO_REQ_DEC, &req);
    if (err) {
        log_e("audio_dec_open: err = %d\n", err);
        fclose(file);
        return err;
    }

    __this->play_time = req.dec.play_time;
    __this->total_time = req.dec.total_time; //获取播放总时长

    req.dec.cmd = AUDIO_DEC_START;

    err = server_request(__this->dec_server, AUDIO_REQ_DEC, &req);
    if (err) {
        log_e("audio_dec_start: err = %d\n", err);
        fclose(file);
        return err;
    }

    __this->file = file;

    log_i("play_music_file: suss\n");

    return 0;
}

//切换上一首或下一首
static int local_music_dec_switch_file(int fsel_mode)
{
    int i = 0;
    FILE *file = NULL;
    int len = 0;
    char name[16] = {0};

    log_i("local_music_dec_switch_file\n");

    if (!__this->fscan || !__this->fscan->file_number) {
        return -1;
    }

__retry:
    do {
        file = fselect(__this->fscan, fsel_mode, fsel_mode == FSEL_BY_NUMBER ? (CPU_RAND() % __this->fscan->file_number) + 1 : 0);
        if (file) {
            memset(name, 0, sizeof(name));
            len = fget_name(file, (u8 *)name, sizeof(name) - 1);
            if (len > 0) {
                log_i("play file name : %s\n", name);
            }
            ++i;
            break;
        }
        if (fsel_mode == FSEL_NEXT_FILE) {
            fsel_mode = FSEL_FIRST_FILE;
        } else if (fsel_mode == FSEL_PREV_FILE) {
            fsel_mode = FSEL_LAST_FILE;
        } else {
            break;
        }
    } while (i++ < __this->fscan->file_number);

    if (!file) {
        return -1;
    }

    if (0 != local_music_dec_file(file)) {
        if (fsel_mode == FSEL_FIRST_FILE) {
            fsel_mode = FSEL_NEXT_FILE;
        } else if (fsel_mode == FSEL_LAST_FILE) {
            fsel_mode = FSEL_PREV_FILE;
        }
        goto __retry;
    }

    return 0;
}

//切换文件夹
static int local_music_dec_switch_dir(int fsel_mode)
{
    int len = 0;
    int i = 0;
    char name[16] = {0};
    char path[128] = {0};
    FILE *dir = NULL;
    FILE *file = NULL;

    log_i("local_music_dec_switch_dir\n");

    if (!__this->local_path) {
        return -1;
    }

    if (__this->local_play_all && __this->local_path != CONFIG_MUSIC_PATH_FLASH) {
        //全盘搜索
        if (__this->fscan) {
            fscan_release(__this->fscan);
        }
#if CONFIG_DEC_DECRYPT_ENABLE
        __this->fscan = fscan(__this->local_path, "-r -tMP3WMAWAVM4AAMRAPEFLAAACSPXOPUDTSADPSMP -sn", 2);
#else
        __this->fscan = fscan(__this->local_path, "-r -tMP3WMAWAVM4AAMRAPEFLAAACSPXOPUDTSADP -sn", 2);
#endif
        if (!__this->fscan) {
            return -1;
        }
        return local_music_dec_switch_file(FSEL_FIRST_FILE);
    }

    //搜索文件夹
    if (!__this->dir_list) {
        __this->dir_list = fscan(__this->local_path, "-d -sn", 2);
        if (!__this->dir_list || __this->dir_list->file_number == 0) {
            log_w("no_music_dir_find\n");
            return -1;
        }
    }

    //选择文件夹
__again:
    do {
        dir = fselect(__this->dir_list, fsel_mode, 0);
        if (dir) {
            i++;
            break;
        }
        if (fsel_mode == FSEL_NEXT_FILE) {
            fsel_mode = FSEL_FIRST_FILE;
        } else if (fsel_mode == FSEL_PREV_FILE) {
            fsel_mode = FSEL_LAST_FILE;
        } else {
            log_w("fselect_dir_faild, create dir\n");
            return -1;
        }
    } while (i++ < __this->dir_list->file_number);

    if (!dir) {
        return -1;
    }

    len = fget_name(dir, (u8 *)name, sizeof(name) - 1);
    if (0 == len) {
        fclose(dir);
        if (fsel_mode == FSEL_FIRST_FILE) {
            fsel_mode = FSEL_NEXT_FILE;
        } else if (fsel_mode == FSEL_LAST_FILE) {
            fsel_mode = FSEL_PREV_FILE;
        }
        goto __again;
    }

    fclose(dir);

    if (__this->fscan) {
        fscan_release(__this->fscan);
    }

    fname_to_path(path, __this->local_path, name, len, 1, 0);

#if 0	//此处播放指定目录，用户填写的目录路径要注意中文编码问题，看不懂就直接用16进制把路径打印出来
    const char *user_dir = "";
    if (!file && strcmp(path, user_dir)) {
        log_i("dir name : %s\n", path);
        if (fsel_mode == FSEL_FIRST_FILE) {
            fsel_mode = FSEL_NEXT_FILE;
        } else if (fsel_mode == FSEL_LAST_FILE) {
            fsel_mode = FSEL_PREV_FILE;
        }
        goto __again;
    }
#endif

    log_i("fscan path : %s\n", path);

    /*搜索文件夹下的音频文件，按序号排序*/
#if CONFIG_DEC_DECRYPT_ENABLE
    __this->fscan = fscan(path, "-tMP3WMAWAVM4AAMRAPEFLAAACSPXOPUDTSADPSMP -sn", 2);
#else
    __this->fscan = fscan(path, "-tMP3WMAWAVM4AAMRAPEFLAAACSPXOPUDTSADP -sn", 2);
#endif

    if (!file) {
        if (!__this->fscan || !__this->fscan->file_number) {
            if (fsel_mode == FSEL_FIRST_FILE) {
                fsel_mode = FSEL_NEXT_FILE;
            } else if (fsel_mode == FSEL_LAST_FILE) {
                fsel_mode = FSEL_PREV_FILE;
            }
            goto __again;
        }
        local_music_dec_switch_file(FSEL_FIRST_FILE);
    }

    return 0;
}

//快进快退,单位是秒,暂时只支持MP3格式
static int local_music_dec_seek(int seek_step)
{
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

    log_i("local music seek step : %d\n", seek_step);

    return server_request(__this->dec_server, AUDIO_REQ_DEC, &r);
}

//释放资源，切换播放源设备
static int local_music_switch_local_device(const char *path)
{
    log_i("local_music_switch_local_device\n");

    if (__this->dir_list) {
        fscan_release(__this->dir_list);
        __this->dir_list = NULL;
    }
    if (__this->fscan) {
        fscan_release(__this->fscan);
        __this->fscan = NULL;
    }
    if (__this->wait_sd) {
        wait_completion_del(__this->wait_sd);
        __this->wait_sd = 0;
    }
    if (__this->wait_udisk) {
        wait_completion_del(__this->wait_udisk);
        __this->wait_udisk = 0;
    }

    local_music_dec_stop();

    if (path == NULL) {
        return -1;
    }

    __this->local_path = path;

    local_music_dec_switch_dir(FSEL_FIRST_FILE);

    return 0;
}

static void dec_server_event_handler(void *priv, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_SERVER_EVENT_ERR:
        log_i("local_music: AUDIO_SERVER_EVENT_ERR\n");
    case AUDIO_SERVER_EVENT_END:
        log_i("local_music: AUDIO_SERVER_EVENT_END\n");
        local_music_dec_stop();
        local_music_dec_switch_file(FSEL_NEXT_FILE);
        break;
    case AUDIO_SERVER_EVENT_CURR_TIME:
        log_d("play_time: %d\n", argv[1]);
        __this->play_time = argv[1];
        break;
    }
}

static int local_music_mode_init(void)
{
    log_i("local_music_play_main\n");

    memset(__this, 0, sizeof(struct local_music_hdl));

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
    __this->local_play_all = 1;

    __this->dec_server = server_open("audio_server", "dec");
    if (!__this->dec_server) {
        return -1;
    }
    server_register_event_handler_to_task(__this->dec_server, NULL, dec_server_event_handler, "app_core");

    if (storage_device_ready()) {
        return local_music_switch_local_device(CONFIG_MUSIC_PATH_SD);
    } else {
        return local_music_switch_local_device(CONFIG_MUSIC_PATH_FLASH);
    }
}

static void local_music_mode_exit(void)
{
#if defined CONFIG_REVERB_MODE_ENABLE && defined CONFIG_AUDIO_MIX_ENABLE
    if (__this->reverb_enable) {
        echo_reverb_uninit();
    }
#endif
    local_music_switch_local_device(NULL);
    server_close(__this->dec_server);
    __this->dec_server = NULL;
}

static int local_music_key_click(struct key_event *key)
{
    int ret = true;

    switch (key->value) {
    case KEY_OK:
        local_music_dec_play_pause();
        break;
    case KEY_VOLUME_DEC:
        local_music_set_dec_volume(-VOLUME_STEP);
        break;
    case KEY_VOLUME_INC:
        local_music_set_dec_volume(VOLUME_STEP);
        break;
    default:
        ret = false;
        break;
    }

    return ret;
}

static int local_music_key_long(struct key_event *key)
{
    switch (key->value) {
    case KEY_OK:
        local_music_dec_switch_dir(FSEL_NEXT_FILE);
        break;
    case KEY_VOLUME_DEC:
        local_music_dec_switch_file(FSEL_PREV_FILE);
        break;
    case KEY_VOLUME_INC:
        local_music_dec_switch_file(FSEL_NEXT_FILE);
        break;
    case KEY_MODE:
#if defined CONFIG_REVERB_MODE_ENABLE && defined CONFIG_AUDIO_MIX_ENABLE
        if (__this->reverb_enable) {
            //关闭混响
            echo_reverb_uninit();
            __this->reverb_enable = 0;
        } else {
            //配置混响参数
            const struct __HOWLING_PARM_ howling_parm = {13, 20, 20, 300, 5, -50000/*-25000*/, 0, 16000, 1};
#if CONFIG_AUDIO_ENC_SAMPLE_SOURCE != AUDIO_ENC_SAMPLE_SOURCE_MIC
            echo_reverb_init(48000, 16000, BIT(CONFIG_AUDIO_ENC_SAMPLE_SOURCE + 3), 100, __this->volume, NULL, NULL, (void *)&howling_parm, NULL);
#else
            echo_reverb_init(48000, 16000, BIT(CONFIG_REVERB_ADC_CHANNEL), 100, __this->volume, NULL, NULL, (void *)&howling_parm, NULL);
#endif
            __this->reverb_enable = 1;
        }
#else
        if (storage_device_ready()) {
            local_music_switch_local_device(CONFIG_MUSIC_PATH_SD);
        }
#endif
        break;
    default:
        break;
    }

    return true;
}

static int local_music_key_event_handler(struct key_event *key)
{
    switch (key->action) {
    case KEY_EVENT_CLICK:
        return local_music_key_click(key);
    case KEY_EVENT_LONG:
        return local_music_key_long(key);
    default:
        break;
    }

    return true;
}

/*
 *设备响应函数
 */
static int local_music_device_event_handler(struct sys_event *sys_eve)
{
    struct device_event *device_eve = (struct device_event *)sys_eve->payload;
    /* SD卡插拔处理 */
    if (sys_eve->from == DEVICE_EVENT_FROM_SD) {
        switch (device_eve->event) {
        case DEVICE_EVENT_IN:
            //等待SD卡挂载完成才开始搜索文件
            __this->wait_sd = wait_completion(sdcard_storage_device_ready,
                                              (int (*)(void *))local_music_switch_local_device,
                                              CONFIG_MUSIC_PATH_SD, device_eve->arg);
            break;
        case DEVICE_EVENT_OUT:
            //SD卡拔出，释放资源
            local_music_switch_local_device(NULL);
            break;
        }
#if TCFG_UDISK_ENABLE
        /* U盘插拔处理 */
    } else if (sys_eve->from == DEVICE_EVENT_FROM_USB_HOST && !strncmp((const char *)device_eve->value, "udisk", 5)) {
        switch (device_eve->event) {
        case DEVICE_EVENT_IN:
            //等待U盘挂载完成才开始搜索文件
            __this->wait_udisk = wait_completion(udisk_storage_device_ready,
                                                 (int (*)(void *))local_music_switch_local_device,
                                                 CONFIG_MUSIC_PATH_UDISK, ((const char *)device_eve->value)[5] - '0');
            break;
        case DEVICE_EVENT_OUT:
            //U盘拔出，释放资源
            local_music_switch_local_device(NULL);
            break;
        }
#endif
    }

    return true;
}

static int local_music_event_handler(struct application *app, struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        return local_music_key_event_handler((struct key_event *)event->payload);
    case SYS_DEVICE_EVENT:
        return local_music_device_event_handler(event);
    default:
        return false;
    }
}

static int local_music_state_machine(struct application *app, enum app_state state, struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        break;
    case APP_STA_START:
        local_music_mode_init();
        break;
    case APP_STA_PAUSE:
        break;
    case APP_STA_RESUME:
        break;
    case APP_STA_STOP:
        local_music_mode_exit();
        break;
    case APP_STA_DESTROY:
        break;
    }

    return 0;
}

static const struct application_operation local_music_ops = {
    .state_machine  = local_music_state_machine,
    .event_handler 	= local_music_event_handler,
};

REGISTER_APPLICATION(local_music) = {
    .name 	= "local_music",
    .ops 	= &local_music_ops,
    .state  = APP_STA_DESTROY,
};

#endif

#endif//USE_AUDIO_DEMO
