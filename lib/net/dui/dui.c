#include "server/ai_server.h"
#include "system/database.h"
#include "dui.h"
#include "iot.h"
#ifdef DUI_MEMHEAP_DEBUG
#include "mem_leak_test.h"
#endif
#include "syscfg/syscfg_id.h"

static volatile u8 msg_notify_disable;
static u8 dui_app_init_flag = 0;
static int dui_app_task_pid;
static u8 s_is_record = false;
static u8 record_break_flag;
static u32 record_sessionid; //当次录音的id,用于快速丢弃数据
static media_item_t item = {0};
static u8 play_status = STOP;
static u8 play_mode = LOOPPLAY_MODE;
static enum media_source_type source_type = 0xff;
static char dui_net_cfg_uid[64];

static struct dui_var dui_hdl;
#define __this      (&dui_hdl)


extern void sys_power_shutdown(void);
extern unsigned int random32(int type);

media_item_t *get_media_item(void)
{
    return &item;
}

u8 get_s_is_record(void)
{
    return s_is_record;
}

u8 get_record_break_flag(void)
{
    return record_break_flag;
}

void set_record_break_flag(u8 value)
{
    record_break_flag = value;
}

const char *dui_get_net_cfg_uid(void)
{
    memset(dui_net_cfg_uid, 0x00, sizeof(dui_net_cfg_uid));
    /* db_select_buffer(VM_USER_ID_INDEX, (char *)dui_net_cfg_uid, sizeof(dui_net_cfg_uid)); */
    syscfg_read(VM_USER_ID_INDEX, (char *)dui_net_cfg_uid, sizeof(dui_net_cfg_uid));
    printf("\n\nget uid = %s\n\n", dui_net_cfg_uid);
    return dui_net_cfg_uid[0] != 0 ? dui_net_cfg_uid : NULL;
}

void dui_set_net_cfg_uid(const char *uid)
{
    memset(dui_net_cfg_uid, 0x00, sizeof(dui_net_cfg_uid));
    memcpy(dui_net_cfg_uid, uid, strlen(uid) + 1);
    printf("\n\nset uid = %s\n\n", uid);
    /* db_update_buffer(VM_USER_ID_INDEX, (char *)dui_net_cfg_uid, sizeof(dui_net_cfg_uid)); */
    syscfg_write(VM_USER_ID_INDEX, (char *)dui_net_cfg_uid, sizeof(dui_net_cfg_uid));
}

const char *dui_get_userid(void)
{
    memset(dui_net_cfg_uid, 0x00, sizeof(dui_net_cfg_uid));
    /* db_select_buffer(VM_USER_ID_INDEX, (char *)dui_net_cfg_uid, sizeof(dui_net_cfg_uid)); */
    syscfg_read(VM_USER_ID_INDEX, (char *)dui_net_cfg_uid, sizeof(dui_net_cfg_uid));
    char *str = strchr(dui_net_cfg_uid, '_');
    if (!str) {
        return NULL;
    }
    dui_net_cfg_uid[str - dui_net_cfg_uid] = 0;
    /* printf("\n\nget user uid = %s\n\n", dui_net_cfg_uid); */
    return dui_net_cfg_uid[0] != 0 ? dui_net_cfg_uid : NULL;
}

const char *dui_get_device_id(void)
{
    struct dui_para *para = &__this->para;
    return para->param.deviceID;
}

const char *dui_get_version(void)
{
    return "V1.3.0";
}

const char *dui_get_product_code(void)
{
    return "01f";
}

const char *dui_get_product_id(void)
{
    return "001b";
}

const char *dui_get_product_data(void)
{
    return "20200320";
}

const int dui_media_get_playing_status(void)
{
    return play_status;
}

void dui_media_set_playing_status(int value)
{
    play_status = value;
    if (item.linkUrl) {
        iot_ctl_state(&item);
    }
}

void dui_media_music_mode_set(int mode)
{
    play_mode = mode;
}

int dui_media_music_mode_get(void)
{
    return play_mode;
}

void dui_media_set_source(enum media_source_type type)
{
    source_type = type;
}

enum media_source_type dui_media_get_source(void)
{
    return source_type;
}

void dui_ai_media_audio_play(const char *url)
{
    struct dui_para *para = &__this->para;
    strcpy(para->reply.url, url);
    dui_media_set_source(MEDIA_SOURCE_AI);
    dui_media_audio_play(para->reply.url);
}

void dui_iot_media_audio_play(const char *url)
{
    struct dui_para *para = &__this->para;
    strcpy(para->reply.url, url);
    dui_media_set_source(MEDIA_SOURCE_IOT);
    dui_media_audio_play(para->reply.url);
}

int dui_net_music_play_prev(void)
{
    if (dui_media_get_source() == MEDIA_SOURCE_IOT) {
        return iot_ctl_play_prev();
    } else if (dui_media_get_source() == MEDIA_SOURCE_AI) {
        int play_list_match = get_play_list_match();
        int play_list_max = get_play_list_max();
        play_list_match = play_list_match == 0 ? (play_list_max - 1) : --play_list_match;
        set_play_list_match(play_list_match);
        dui_play_playlist_for_match(&item, play_list_match);
        if (item.linkUrl) {
            dui_ai_media_audio_play(item.linkUrl);
        }
    }
    return 0;
}

int dui_net_music_play_next(void)
{
    if (dui_media_get_source() == MEDIA_SOURCE_IOT) {
        return iot_ctl_play_next();
    } else if (dui_media_get_source() == MEDIA_SOURCE_AI) {
        int play_list_match = get_play_list_match();
        int play_list_max = get_play_list_max();
        play_list_match = play_list_match == (play_list_max - 1) ? 0 : ++play_list_match;
        set_play_list_match(play_list_match);
        dui_play_playlist_for_match(&item, play_list_match);
        if (item.linkUrl) {
            dui_ai_media_audio_play(item.linkUrl);
        }
    }
    return 0;
}

media_item_t *dui_get_ai_music_item(void)
{
    return &item;
}

void dui_shutdown(void *priv)
{
    sys_power_shutdown();
}

u32 get_record_sessionid(void)
{
    return record_sessionid;//每次启动录音都会增加
}

struct dui_var *get_dui_hdl(void)
{
    return __this;
}

static void init_dui_para(struct dui_para *para)
{
//使用白名单
#if 1
    strcpy(para->param.productKey, "321af2bf1b52c93d75db7d300ee5f5b0");
    strcpy(para->param.productId, "279593780");
    strcpy(para->param.ProductSecret, "cd474e21e4f7b8725f78024a945df569");
    strcpy(para->param.deviceID, "ceed10b41feb");
//不使用白名单
#else
    strcpy(para->param.productKey, "58f1aeeb54fadab27a6ce70fd222ec46");
    strcpy(para->param.productId, "279594353");
    strcpy(para->param.ProductSecret, "89249b0fb48d7c12454a079fc97aee72");

    u8 mac_addr[6];
    extern int wifi_get_mac(u8 * mac);
    wifi_get_mac(mac_addr);
    /*DUI deviceID格式*/
    sprintf(para->param.deviceID, "00000%s%s%s%02X%02X%02X%02X%02X%02X",
            dui_get_product_id(),
            dui_get_product_code(),
            dui_get_product_data(),
            mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
#endif
}

static void dui_app_task(void *priv)
{
    int err;
    int msg[32];
    struct dui_var *p = (struct dui_var *)priv;
    struct dui_para *para = &p->para;

    u32 delay_ms = 1000;
    u8 dly_cnt = 0;
    u8 retry = 5;

    init_dui_para(para);

    while (retry > 0 && !__this->exit_flag) {
        if (dly_cnt--) {
            os_time_dly(10);
            continue;
        }
        err = get_dui_token(para);
        if (!err) {
            break;
        }
        --retry;
        delay_ms <<= 1;
        dly_cnt = delay_ms / 100;
    };

    if (err || __this->exit_flag) {
        goto exit;
    }

    iot_mgr_init(dui_get_device_id());
    dui_net_thread_run(para);

    while (1) {
        err = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (err != OS_TASKQ || msg[0] != Q_USER) {
            continue;
        }

        //统一在这里给网络线程发消息
        switch (msg[1]) {
        case DUI_SPEAK_END:
            puts("===========DUI_SPEAK_END\n");
            //播放完提示音，判断是不是需要重新启动录音

            u8 get_alarm_ring_flag(void);
            void set_alarm_ring_flag(u8 value);
            if (get_alarm_ring_flag()) {
                set_alarm_ring_flag(0);
                u8 get_alarm_type(void);
                if (get_alarm_type() == ALARM_CLOCK_TYPE) {
                    dui_event_notify(AI_SERVER_EVENT_PLAY_BEEP, "reminder.mp3");
                } else {
                    dui_event_notify(AI_SERVER_EVENT_PLAY_BEEP, "schedule.mp3");
                }
                break;
            }
            if (get_reopen_record()) {
                DUI_OS_TASKQ_POST("dui_app_task", 1, DUI_RECORD_START);
                break;
            }
            if (para->reply.url[0]) {
                dui_media_audio_play(para->reply.url);
                para->reply.url[0] = 0;
            } else {
                dui_media_audio_resume_play();
            }
            break;
        case DUI_SPEAK_START:
            break;
        case DUI_RECORD_START:
            puts("===========DUI_SPEAK_START\n");
            if (!s_is_record) {
                DUI_OS_TASKQ_POST("dui_net_task", 1, DUI_RECORD_START_MSG);
                __this->voice_mode = msg[2] & 0x3;
                p->use_vad = (msg[2] & VAD_ENABLE) ? 1 : 0;
                record_sessionid++;
                set_record_break_flag(0);
                set_reopen_record(0);
                dui_recorder_start(16000, msg[2], p->use_vad);
                s_is_record = true;
                msg_notify_disable = 1;
            }
            break;
        case DUI_RECORD_SEND:
            puts("===========DUI_SPEAK_SEND\n");
            DUI_OS_TASKQ_POST("dui_net_task", 1, DUI_RECORD_SEND_MSG); //消息转发
            break;
        case DUI_RECORD_STOP:
            puts("===========DUI_SPEAK_STOP\n");
            if (s_is_record) {
                int dui_recorder_stop(u8 voice_mode);
                dui_recorder_stop(0);
                s_is_record = false;
                msg_notify_disable = 0;
                DUI_OS_TASKQ_POST("dui_net_task", 1, DUI_RECORD_STOP_MSG);
            }
            break;
        case DUI_RECORD_BREAK:
            printf("\n DUI_RECORD_BREAK\n");
            set_record_break_flag(1);
            if (s_is_record) {
                int dui_recorder_stop(u8 voice_mode);
                dui_recorder_stop(0);
                s_is_record = false;
                msg_notify_disable = 0;
                DUI_OS_TASKQ_POST("dui_net_task", 1, DUI_RECORD_STOP_MSG);
            }
            if (msg[2] && msg[3]) {
                DUI_OS_TASKQ_POST("dui_net_task", 3, msg[2], msg[3], msg[4]);
            }
            break;
        case DUI_RECORD_ERR:
            if (s_is_record) {
                dui_recorder_stop(__this->voice_mode);
                s_is_record = false;
                msg_notify_disable = 0;
                DUI_OS_TASKQ_POST("dui_net_task", 1, DUI_RECORD_STOP_MSG);
            }
            break;
        case DUI_PREVIOUS_SONG:
            dui_net_music_play_prev();
            break;
        case DUI_NEXT_SONG:
            dui_net_music_play_next();
            break;
        case DUI_MEDIA_END:
            puts("\nDUI_MEDIA_END\n");
            dui_media_set_playing_status(STOP);
            if (dui_media_get_source() == MEDIA_SOURCE_AI) {
                para->reply.url[0] = 0;
                int mode = dui_media_music_mode_get();
                int play_list_match = get_play_list_match();
                int play_list_max = get_play_list_max();
                printf("\nplay_list_max = %d\n", play_list_max);
                if (mode == ORDERPLAY_MODE) {
                    play_list_match = play_list_match == (play_list_max - 1) ? (play_list_max) : ++play_list_match; //顺序播放，播放完停止播放
                } else if (mode == LOOPPLAY_MODE) {
                    play_list_match = play_list_match == (play_list_max - 1) ? 0 : ++play_list_match; //循环播放，播放完重头开始
                } else if (mode == RANDOMPLAY_MODE) {
                    play_list_match = random32(0) % (play_list_max);
                }
                set_play_list_match(play_list_match);
                dui_play_playlist_for_match(&item, play_list_match);
                if (item.linkUrl[0] != 0) {
                    dui_ai_media_audio_play(item.linkUrl);
                }
            } else if (dui_media_get_source() == MEDIA_SOURCE_IOT) {
                iot_ctl_play_done();
            }
            break;
        case DUI_MEDIA_STOP:
            puts("\nDUI_MEDIA_STOP\n");
            dui_media_set_playing_status(STOP);
            para->reply.url[0] = 0;
            break;
        case DUI_MEDIA_START:
            dui_media_set_playing_status(PLAY);
            puts("\nDUI_MEDIA_START\n");
            if (dui_media_get_source() == MEDIA_SOURCE_AI) {
                int play_list_match = get_play_list_match();
                dui_play_playlist_for_match(&item, play_list_match);
                if (item.linkUrl) {
                    iot_ctl_play_dui(&item);
                }
            } else {
                dui_media_music_item(&item);
            }
            item.duration = get_app_music_total_time();
            break;
        case DUI_PLAY_PAUSE:
            if (play_status == PLAY) {

                if (dui_media_get_source() == MEDIA_SOURCE_IOT) {
                    iot_ctl_play_pause();
                }

                dui_media_set_playing_status(PAUSE);

            } else if (play_status == PAUSE) {
                if (dui_media_get_source() == MEDIA_SOURCE_IOT) {
                    iot_ctl_play_resume();
                }
                dui_media_set_playing_status(PLAY);

            }
            break;
        case DUI_QUIT:
            if (s_is_record) {
                dui_recorder_stop(0);
                s_is_record = false;
                DUI_OS_TASKQ_POST("dui_net_task", 1, DUI_RECORD_STOP_MSG);
            }
            DUI_OS_TASKQ_POST("dui_net_task", 1, DUI_QUIT_MSG);
            iot_mgr_deinit();
            dui_net_thread_kill();
            goto exit;
            break;
        case DUI_VOLUME_CHANGE:
            iot_ctl_play_volume(msg[2]);
        default:
            break;
        }
    }

exit:
    printf("\n %s  exit \n", __func__);
}

u8 dui_app_get_connect_status(void)
{
    struct dui_para *para = &__this->para;
    return para->hdl.connect_status;
}

u8 get_dui_msg_notify(void)
{
    return msg_notify_disable;
}

void set_dui_msg_notify(u8 value)
{
    msg_notify_disable = value;
}

int dui_app_init(void)
{
    struct dui_para *para = &__this->para;
    if (!dui_app_init_flag) {
        dui_app_init_flag = 1;
        __this->exit_flag = 0;
        msg_notify_disable = 0;
        para->hdl.connect_status = 0;
        return thread_fork("dui_app_task", 15, 1536, 256, &dui_app_task_pid, dui_app_task, __this);
    }
    return -1;
}

void dui_app_uninit(void)
{
    struct dui_para *para = &__this->para;
    if (dui_app_init_flag) {
        dui_app_init_flag = 0;
        para->hdl.connect_status = 0;
        para->reply.sessionId[0] = 0;
        __this->exit_flag = 1;
        do {
            if (OS_Q_FULL != os_taskq_post("dui_app_task", 1, DUI_QUIT)) {
                break;
            }
            log_e("dui_app_task send msg QUIT timeout \n");
            os_time_dly(5);
        } while (1);
        thread_kill(&dui_app_task_pid, KILL_WAIT);
    }
}

