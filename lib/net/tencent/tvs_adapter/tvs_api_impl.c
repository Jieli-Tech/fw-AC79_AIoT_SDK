#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "tvs_api/tvs_api.h"
#include "tvs_alert_impl.h"
#include "tvs_platform_impl.h"
#include "tvs_mediaplayer_impl.h"
#include "tvs_auth_manager.h"
#include "tvs_api_config.h"
#include "tvs_api_impl.h"
#include "os_wrapper.h"
#include "version.h"
#include "server/ai_server.h"
#include "os/os_api.h"
#include "tvs_executor_service.h"

#ifdef TVS_SDK_VERSION
MODULE_VERSION_EXPORT(tvs_sdk, TVS_SDK_VERSION);
#endif

const struct ai_sdk_api tc_tvs_api;

static void net_check();

static u8 ai_connected = 0;
static int tc_tvs_pid = 0;
static int net_check_pid = 0;
u8 net_connected = 0;


#if ARREARS_ENABLE
int arrears_type;			//欠费类型
u8  audio_play_disable;			//tts音频播放禁用
int 	arrears_flag;			//欠费标志位

#endif

bool BTCombo_complete;			//蓝牙配网完成标志位
void *BTCombo_mutex;

extern int qcloud_tvs_task_start(void *priv);
extern void qcloud_tvs_task_stop(void);
extern void tvs_dns_refresh_stop();
extern void tvs_ping_refresh_stop();
extern void tvs_platform_adapter_on_network_state_changed(bool connect);
extern bool tvs_ping_one();
extern bool sg_qcloud_tvs_task_running;

__attribute__((weak)) int get_app_music_volume(void)
{
    return 100;
}

const int tc_events[][2] = {
    { AI_EVENT_SPEAK_END, TC_SPEAK_END },
    { AI_EVENT_MEDIA_END, TC_MEDIA_END },
    { AI_EVENT_MEDIA_START, TC_MEDIA_START},
    { AI_EVENT_MEDIA_STOP, TC_MEDIA_STOP},
    { AI_EVENT_PREVIOUS_SONG, TC_PREVIOUS_SONG },
    { AI_EVENT_NEXT_SONG, TC_NEXT_SONG },
    { AI_EVENT_RECORD_START, TC_RECORD_START },
    { AI_EVENT_RECORD_BREAK, TC_RECORD_BREAK },
    { AI_EVENT_RECORD_STOP, TC_RECORD_STOP },
    { AI_EVENT_COLLECT_RES, TC_COLLECT_RESOURCE },
    { AI_EVENT_PLAY_PAUSE, TC_PLAY_PAUSE},
    { AI_EVENT_VOLUME_CHANGE, TC_VOLUME_CHANGE},
    { AI_EVENT_VOLUME_INCR, TC_VOLUME_INCR},
    { AI_EVENT_VOLUME_DECR, TC_VOLUME_DECR},
    { AI_EVENT_CUSTOM_FUN, TC_RECV_CHAT},
    { AI_EVENT_QUIT, TC_QUIT },
};


static int event_to_tc(int event)
{
    for (int i = 0; i < ARRAY_SIZE(tc_events); i++) {
        if (tc_events[i][0] == event) {
            return tc_events[i][1];
        }
    }
    return -1;
}

static int tc_tvs_do_event(int event, int arg)
{
    int err;
    int argc = 2;
    int argv[2];
    int tc_event;

    printf("tc_tvs_do_event %d\n", event);

    tc_event = event_to_tc(event);

    if (tc_event == -1) {
        return 0;
    }

    argv[0] = tc_event;
    argv[1] = arg;

    err = os_taskq_post_type("tc_tvs_task", Q_USER, argc, argv);
    if (err != OS_NO_ERR) {
        printf("send msg to tc_tvs_task fail, event : %d\n", tc_event);
    }

    return 0;
}
/**********************************/


static bool g_tvs_status_idle = true;

extern const char *tvs_get_states_name(tvs_recognize_state state);

extern void init_authorize_on_boot();

static void on_tvs_sdk_enter_working_state()
{
    // SDK进入工作状态，比如开始语音识别、开始播控等
}

static void on_tvs_sdk_exit_working_state(void *state_param)
{
    // SDK回到空闲状态
    tvs_api_state_param *api_state_param = (tvs_api_state_param *)state_param;

    if (api_state_param == NULL) {
        return;
    }

    if (api_state_param->control_type == TVS_CONTROL_PLAY_FINISH) {
        switch (api_state_param->error) {
        case TVS_API_ERROR_NETWORK_ERROR:
            // 播放歌曲列表时，网络异常
            break;
        case TVS_API_ERROR_NO_MORE_MEDIA:
            // 播放歌曲列表时，未下发媒体，一般是歌单播放结束，没有下一首了
            break;
        }
    }
}

bool tvs_sdk_is_idle()
{
    // SDK是否处于空闲状态
    return g_tvs_status_idle;
}

static void callback_on_state_changed(tvs_recognize_state last_state, tvs_recognize_state new_state, void *param)
{
    // TO-DO 监听SDK状态变换
    g_tvs_status_idle = (new_state == TVS_STATE_IDLE);

    if (last_state == TVS_STATE_IDLE && new_state != TVS_STATE_IDLE) {
        on_tvs_sdk_enter_working_state();
    } else if (last_state != TVS_STATE_IDLE && new_state == TVS_STATE_IDLE) {
        on_tvs_sdk_exit_working_state(param);
    }
}

static void callback_on_terminal_sync(const char *text, const char *token)
{
    TVS_ADAPTER_PRINTF("api impl get terminal sync %s -- %s\n", text, token);
    // TO-DO 监听APP端推送
}

static void callback_on_mode_changed(tvs_mode src_mode, tvs_mode dst_mode)
{
    TVS_ADAPTER_PRINTF("api impl get mode changed %d -- %d\n", src_mode, dst_mode);
    // TO-DO 监听模式变换
}

static void callback_on_expect_speech()
{
    TVS_ADAPTER_PRINTF("api impl on expect speech\n");
    //tvs_api_start_recognize();
    // TO-DO 多轮会话，在此函数中拉起下一轮语音识别流程
    tvs_taskq_post(TC_RECORD_START);
}

static void callback_on_tvs_control(const char *json)
{
    TVS_ADAPTER_PRINTF("api impl get control %s\n", json);

    // TO-DO 处理自定义技能
}

static void callback_on_speech_reader_stop(int session_id, int error)
{
    if (tvs_api_get_current_session_id() == session_id) {

        TVS_ADAPTER_PRINTF("on speech reader stop, session id %d, error %d\n", session_id, error);
        // SDK发送数据结束,如果本地录音没有结束，需要通知其结束
        switch (error) {
        case TVS_API_AUDIO_PROVIDER_ERROR_NONE:
            // 正常结束
            break;
        case TVS_API_AUDIO_PROVIDER_ERROR_STOP_CAPTURE:
            // 收到云端VAD End标识
            printf("-------%s---------%d", __func__, __LINE__);
            break;
        case TVS_API_AUDIO_PROVIDER_ERROR_TIME_OUT:
            // 收到SDK Timeout标识，一般是网络原因导致HTTP超时
            printf("-------%s---------%d", __func__, __LINE__);
            break;
        case TVS_API_AUDIO_PROVIDER_ERROR_NETWORK:
            // 网络出错
            printf("-------%s---------%d", __func__, __LINE__);
            break;
        default:
            // 其他原因
            printf("-------%s---------%d", __func__, __LINE__);
            break;
        }
    }
}

void tvs_api_impl_speech_write_audio(const char *audio_data, int data_size)
{
    tvs_api_audio_provider_write(audio_data, data_size);
}

void tvs_api_impl_speech_wakeup()
{
    int session_id = tvs_api_new_session_id();
    tvs_api_audio_provider_writer_begin();

    //需要将“叮当叮当”替换为接入方自己的热词
    int ret = tvs_api_on_voice_wakeup(session_id, "小智管家", 0, 0);

    if (ret != 0) {
        // SDK启动识别流程出错，此处需要通知录音模块结束
        tvs_api_audio_provider_writer_end();
    }
}

void tvs_api_impl_speech_write_end()
{
    tvs_api_audio_provider_writer_end();
}

void tvs_api_impl_speech_init()
{
    tvs_api_audio_provider_listen(callback_on_speech_reader_stop);
}

static void callback_on_asr_result(const char *asr_text, bool is_end)
{
    printf("callback_on_asr_result %s -- isEnd %d\n", asr_text, is_end);
}

static void callback_on_unbind(long long unbind_timeMs)
{
    time_t seconds = unbind_timeMs / 1000;
//	printf("api impl get unbind:%s\n", asctime(localtime( &seconds)));
}

extern void tvs_config_enable_ip_provider(bool enable);
extern char *mediaplayer_adapter_get_url_token(void);

static void tvs_api_impl_init(void *priv)
{
    g_tvs_status_idle = true;

    //开启打印功能
    tvs_api_log_enable(true);

    //设置speech通知回调
    tvs_api_impl_speech_init();

    // 初始化API，设置回调监听
    tvs_api_callback api_callback = {0};
    api_callback.on_state_changed = callback_on_state_changed;
    api_callback.on_terminal_sync = callback_on_terminal_sync;
    api_callback.on_mode_changed = callback_on_mode_changed;
    api_callback.on_expect_speech = callback_on_expect_speech;
    api_callback.on_recv_tvs_control = callback_on_tvs_control;
    api_callback.on_unbind = callback_on_unbind;

    tvs_default_config config = {0};
    // 默认环境，量产版本必须默认为正式环境
    config.def_env = TVS_API_ENV_NORMAL;
    // 默认沙箱，量产版本必须默认为false
    config.def_sandbox_open = false;

    config.recorder_bitrate = 16000;
    config.recorder_channels = 1;

    // 设置云小微产品信息
    tvs_product_qua qua = {0};
    qua.version = "1.0.0.1000";
    qua.package_name = "com.huwen.aiot.guomei.voicebox";

    //true代表执行容灾机制
    tvs_config_enable_ip_provider(true);

    //互斥锁初始化,回调注册，语音，设备信息配置写入
    tvs_api_init(&api_callback, &config, &qua);

    //关闭https功能
    extern void tvs_config_enable_https(bool enable);
    //tvs_config_enable_https(true);
    tvs_config_enable_https(false);


    extern void tvs_config_print_asr_result(bool enable);
    // 允许输出ASR结果
    tvs_config_print_asr_result(true);
    //设置asr回调函数
    tvs_api_set_asr_callback(callback_on_asr_result);

    // 初始化platform adapter，赋予SDK操作终端的能力
    tvs_platform_adaptor platform_adapter_impl = {0};
    tvs_init_platform_adapter_impl(&platform_adapter_impl);
    tvs_platform_adapter_init(&platform_adapter_impl);

    // 初始化media player adapter，赋予SDK播放网络媒体的能力
    tvs_mediaplayer_adapter media_adapter_impl = {0};
    tvs_init_mediaplayer_adapter_impl(&media_adapter_impl);
    tvs_mediaplayer_adapter_init(&media_adapter_impl);

    // 初始化alert adapter，赋予SDK操作闹钟的能力
    tvs_alert_adapter alert_adapter_impl = {0};
    tvs_init_alert_adater_impl(&alert_adapter_impl);
    tvs_alert_adapter_init(&alert_adapter_impl);

#if USE_MAC_DEVICE_NAME

    extern void get_mac_device_name(void);
    get_mac_device_name();	//获取mac地址作为腾讯云的设备名

    extern u8 is_psk_available(void);
    //这里提示用户需要进行一次配网去获取psk
    if (!is_psk_available()) {
        printf("psk is not available, please auth again-------%s-------%d\r\n", __func__, __LINE__);
    }
#endif

#if ARREARS_ENABLE
    //从flash读取欠费标志位
    extern void load_arrears_info(void);
    load_arrears_info();
#endif
    // 授权
    init_authorize_on_boot();

    // 启动SDK
    tvs_api_start();

    tvs_executor_upload_volume(get_app_music_volume());

    int msg[32];
    int err;

    while (1) {
        err = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (err != OS_TASKQ || msg[0] != Q_USER) {
            continue;
        }

        switch (msg[1]) {
        case TC_MEDIA_END:
            tvs_mediaplayer_adapter_on_play_finished(mediaplayer_adapter_get_url_token());
            break;
        case TC_MEDIA_START:
            tvs_mediaplayer_adapter_on_play_started(mediaplayer_adapter_get_url_token());
            break;
        case TC_MEDIA_STOP:
            tvs_mediaplayer_adapter_on_play_stopped(0, mediaplayer_adapter_get_url_token());
            break;
        case TC_PLAY_PAUSE:
            if (msg[2]) {
                tvs_mediaplayer_adapter_on_play_stopped(0, mediaplayer_adapter_get_url_token());
            } else {
                tvs_mediaplayer_adapter_on_play_started(mediaplayer_adapter_get_url_token());
            }
            break;
        case TC_PREVIOUS_SONG:
            tvs_api_playcontrol_previous();
            break;
        case TC_NEXT_SONG:
            tvs_api_playcontrol_next();
            break;
        case TC_VOLUME_CHANGE:
            tvs_platform_adapter_on_volume_changed(msg[2]);
            break;
        case TC_RECORD_START:
            extern void record_sem_post(void);
            extern void alarm_rings_stop(int tvs_alert_stop_reason);
            alarm_rings_stop(TVS_ALART_STOP_REASON_NEW_RECO);
            record_sem_post();
            tvs_api_start_recognize();
            break;
        case TC_RECORD_STOP:
            tvs_api_stop_recognize();
            break;
        case TC_RECORD_BREAK:
            tvs_api_stop_all_activity();
            break;
        case TC_QUIT:
            return;
        default:
            break;
        }
    }
}

void tvs_taskq_post(int msg)
{
    os_taskq_post("tc_tvs_task", 1, msg);
}

static void net_check()
{
    int cnt = 300;
    while (ai_connected) {
        if (tvs_ping_one()) {
            net_connected = 1;
            break;
        } else {
            net_connected = 0;
        }
        os_time_dly(cnt);
        cnt += 200;
        if (cnt >= 1000) {
            cnt = 300;
        }
    }
}

static void qcloud_tvs_task_disconnect()
{
    tvs_platform_adapter_on_network_state_changed(false);
    qcloud_tvs_task_stop();
    if (!net_check_pid) {
        thread_fork("net_check", 20, 1024, 0, &net_check_pid, net_check, NULL);
    }
}

static void qcloud_tvs_task_connect()
{
    if (!sg_qcloud_tvs_task_running   && read_BTCombo()) {
        //通知SDK联网
        tvs_platform_adapter_on_network_state_changed(true);
        thread_fork("qcloud_tvs_task_start", 20, 2048, 20, NULL, qcloud_tvs_task_start, NULL);
    }
}
static int tc_tvs_check(void)
{
    if (net_connected) {
        qcloud_tvs_task_connect();
        return AI_STAT_CONNECTED;
    } else {
        qcloud_tvs_task_disconnect();
        return AI_STAT_DISCONNECTED;
    }

}

static int tc_tvs_connect(void)
{
    ai_connected = 1;
    //已经创建过不再创建
    if (!tc_tvs_pid) {
        thread_fork("tc_tvs_task", 20, 1100, 64, &tc_tvs_pid, tvs_api_impl_init, NULL);
    }
    if (!net_check_pid) {
        thread_fork("net_check", 20, 1024, 0, &net_check_pid, net_check, NULL);
    }


    return 0;
}

static int tc_tvs_disconnect(void)
{

    ai_connected = 0;
    while (get_current_node()) {
        tvs_api_stop_all_activity();	//关闭所有活动
        os_time_dly(50);
    }
    qcloud_tvs_task_stop();
    tvs_ping_refresh_stop();	//关闭ping定时器
    tvs_dns_refresh_stop();		//关闭dns定时器
    tvs_platform_adapter_on_network_state_changed(false);

    return 0;
}


REGISTER_AI_SDK(tc_tvs_api) = {
    .name           = "tencent",
    .connect        = tc_tvs_connect,
    .state_check    = tc_tvs_check,
    .do_event       = tc_tvs_do_event,
    .disconnect     = tc_tvs_disconnect,
};

