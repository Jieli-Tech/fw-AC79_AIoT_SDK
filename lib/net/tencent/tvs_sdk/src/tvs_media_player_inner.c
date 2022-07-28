#define TVS_LOG_DEBUG   1
#define TVS_LOG_DEBUG_MODULE  "MEDIA"
#include "tvs_log.h"

#include "tvs_media_player_interface.h"
#include "tvs_media_player_inner.h"
#include "tvs_executor_service.h"
#include "tvs_core.h"
#include "tvs_threads.h"
#include "tvs_api_config.h"


extern u8 alarm_rings;
typedef enum {
    TVS_PLAYBACK_IDLE,
    TVS_PLAYBACK_NEW_SOURCE,
    TVS_PLAYBACK_STARTED,
    TVS_PLAYBACK_STOPPED,
    TVS_PLAYBACK_PAUSED,
    TVS_PLAYBACK_BUFFER_UNDERRUN,
    TVS_PLAYBACK_FINISHED,
    TVS_PLAYBACK_RESUMED,
} tvs_playback_state;

// 用于日志打印的状态字符串，与tvs_playback_state枚举一一对应
static const char *const play_back_print_str[] = {"IDLE", "NEWSOURCE", "PLAYING", "STOPPED", "PAUSED", "BUFFER_UNDERRUN", "FINISHED", "RESUME" };

// 用于媒体上下文的状态字符串，与tvs_playback_state枚举一一对应
// 在set source状态的时候打断当前会话，上下文中需要传PAUSED
static const char *const play_back_state_str[] = {"IDLE", "PAUSED", "PLAYING", "STOPPED", "PAUSED", "BUFFER_UNDERRUN", "FINISHED", "PLAYING" };

// 用于上报的状态字符串，与tvs_playback_state枚举一一对应
static const char *const player_upload_str[] = {"", "", "PlaybackStarted", "PlaybackStopped", "PlaybackStopped", "", "", "PlaybackStarted"};

static tvs_playback_state g_tvs_playback_state = TVS_PLAYBACK_IDLE;
static char *g_player_token = NULL;
static char *g_player_url = NULL;
static unsigned int g_player_offset = 0;

TVS_LOCKER_DEFINE

__attribute__((weak)) int get_listen_flag()
{
    return 0;
}

const char *get_player_upload_state(int state)
{
    int size = sizeof(player_upload_str) / sizeof(char *);

    if (state < 0 || state >= size) {
        return NULL;
    }

    return player_upload_str[state];
}

static void release_last_source()
{
    if (g_player_token != NULL) {
        TVS_FREE(g_player_token);
        g_player_token = NULL;
    }

    if (g_player_url != NULL) {
        TVS_FREE(g_player_url);
        g_player_url = NULL;
    }
}

static void set_player_state(tvs_playback_state state)
{
    if (g_player_token == NULL) {
        return;
    }
    if ((g_tvs_playback_state == TVS_PLAYBACK_FINISHED || g_tvs_playback_state == TVS_PLAYBACK_STOPPED) && (state != TVS_PLAYBACK_STARTED && state != TVS_PLAYBACK_NEW_SOURCE)) {
        TVS_LOG_PRINTF("invalid player state %s, last is %s\n", play_back_print_str[state], play_back_print_str[g_tvs_playback_state]);
        return;
    }

    g_tvs_playback_state = state;

    TVS_LOG_PRINTF("current player state %s\n", play_back_print_str[g_tvs_playback_state]);
}

static void tvs_media_player_inner_upload_song_status(int status)
{
    const char *name = get_player_upload_state(status);
    if (name == NULL || strlen(name) == 0) {
        return;
    }

    tvs_executor_upload_media_player_state(status);
}

static void tvs_media_player_inner_upload_song_finish()
{
    tvs_executor_param_events ev_param = {0};
    tvs_executor_start_control(TVS_EXECUTOR_CMD_PLAY_FINISH, &ev_param);
}

static void tvs_media_callback_finished(const char *token)
{
    if (g_tvs_playback_state == TVS_PLAYBACK_FINISHED) {
        return;
    }

    set_player_state(TVS_PLAYBACK_FINISHED);

    tvs_media_player_inner_upload_song_finish();
}

static bool is_current_media_end()
{
    if (g_tvs_playback_state == TVS_PLAYBACK_FINISHED || g_tvs_playback_state == TVS_PLAYBACK_STOPPED || g_tvs_playback_state == TVS_PLAYBACK_IDLE) {
        return true;
    }

    return false;
}

static void tvs_media_callback_stopped(int error, const char *token)
{
    if (is_current_media_end()) {
        return;
    }
    set_player_state(TVS_PLAYBACK_STOPPED);
    tvs_media_player_inner_upload_song_status(TVS_PLAYBACK_STOPPED);
}

const char *tvs_media_player_inner_get_playback_state()
{
    return play_back_state_str[g_tvs_playback_state];
}

int tvs_media_player_inner_get_playback_offset()
{
    if (g_player_token == NULL) {
        return 0;
    }

    return tvs_media_player_get_offset(g_player_token);
}

char *tvs_media_player_inner_get_token()
{
    return g_player_token;
}

char *tvs_media_player_inner_get_url()
{
    return g_player_url;
}

int tvs_media_player_inner_stop_play()
{
    do_lock();
    if (is_current_media_end()) {
        do_unlock();
        return 0;
    }

    set_player_state(TVS_PLAYBACK_STOPPED);
    tvs_media_player_inner_upload_song_status(TVS_PLAYBACK_STOPPED);

    int ret = tvs_media_player_stop_play(g_player_token);
    do_unlock();
    return ret;
}

int tvs_media_player_inner_pause_play()
{
    do_lock();

    if (is_current_media_end()) {
        if (alarm_rings) {
            tvs_media_player_pause_play(g_player_token);
        }
        do_unlock();
        return 0;
    }


    //添加一个pause判断, 已经pause上报了不在重复上报
    if (g_tvs_playback_state == TVS_PLAYBACK_PAUSED) {
        if (alarm_rings) {
            tvs_media_player_pause_play(g_player_token);
        }
        do_unlock();
        TVS_LOG_PRINTF("already paused\n");
        return 0;
    }

    set_player_state(TVS_PLAYBACK_PAUSED);
    tvs_media_player_inner_upload_song_status(TVS_PLAYBACK_PAUSED);
    int ret = tvs_media_player_pause_play(g_player_token);
    do_unlock();
    return ret;
}

int tvs_media_player_inner_get_offset()
{
    do_lock();

    int offset = tvs_media_player_get_offset(g_player_token);
    do_unlock();
    TVS_LOG_PRINTF("call %s, offset %d\n", __func__, offset);
    return offset;
}

static void upload_player_started()
{
    tvs_media_player_inner_upload_song_status(TVS_PLAYBACK_STARTED);
}

int tvs_media_player_inner_start_play()
{
    int ret = 0;
    do_lock();
    if (g_tvs_playback_state == TVS_PLAYBACK_STARTED || g_tvs_playback_state == TVS_PLAYBACK_RESUMED) {
        TVS_LOG_PRINTF("already started 1, state %d\n", g_tvs_playback_state);
        do_unlock();
        return ret;
    }
    if (g_tvs_playback_state == TVS_PLAYBACK_STOPPED) {
        TVS_LOG_PRINTF("song stop , state %d\n", g_tvs_playback_state);
        do_unlock();
        return ret;

    }

    if (g_tvs_playback_state == TVS_PLAYBACK_NEW_SOURCE) {

        TVS_LOG_PRINTF("call tvs_media_player_start_play\n");
        ret = tvs_media_player_start_play(g_player_token);
    } else {
        if (!is_current_media_end() && !alarm_rings) {
            set_player_state(TVS_PLAYBACK_RESUMED);
            tvs_media_player_inner_upload_song_status(TVS_PLAYBACK_RESUMED);
            TVS_LOG_PRINTF("call tvs_media_player_resume_play\n");
            ret = tvs_media_player_resume_play(g_player_token);
        }
    }
    do_unlock();
    return ret;
}

static int tvs_media_player_inner_set_source_1(const char *url, const char *token, unsigned int offset, bool start_now)
{
    if (url == NULL || token == NULL || strlen(url) == 0 || strlen(token) == 0) {
        return 0;
    }
    do_lock();


#if ARREARS_ENABLE

    if (arrears_type && strstr(url, "stream.qqmusic.qq.com")) {	//下发了音频url 且 欠费类型不为0
        /* if (arrears_type && strstr(url, "stream.qqmusic.qq.com") && (!g_player_url || (g_player_url && strcmp(url, g_player_url)))) {	//下发了音频url 且 欠费类型不为0 */
        audio_play_disable = 1;		//关闭当前次tts播放
        printf("url is music ,forbid play-------%s-------%d\r\n", __func__, __LINE__);
        if (arrears_type == CLOSE_WITH_PROMPT) {
            //播放提示音
            extern int play_voice_prompt(const char *fname, u8 mode);
            play_voice_prompt("arrears.mp3", g_tvs_playback_state == TVS_PLAYBACK_STOPPED ? 0 : 1);

            if (g_tvs_playback_state != TVS_PLAYBACK_IDLE && g_tvs_playback_state != TVS_PLAYBACK_NEW_SOURCE && g_tvs_playback_state != TVS_PLAYBACK_BUFFER_UNDERRUN && g_tvs_playback_state != TVS_PLAYBACK_PAUSED) {
                if (g_tvs_playback_state == TVS_PLAYBACK_FINISHED) {
                    arrears_flag = 2;
                }

                tvs_executor_upload_media_player_state(TVS_PLAYBACK_STARTED);

                if (g_tvs_playback_state != TVS_PLAYBACK_STARTED && g_tvs_playback_state != TVS_PLAYBACK_RESUMED) {
                    tvs_executor_upload_media_player_state(TVS_PLAYBACK_STOPPED);
                    set_player_state(TVS_PLAYBACK_STOPPED);
                }
            }
        }
        do_unlock();
        return 0;

    }

#endif

    if (!is_current_media_end()) {
        set_player_state(TVS_PLAYBACK_STOPPED);
        tvs_media_player_stop_play(g_player_token);
    }

    release_last_source();

    g_player_offset = offset;

    g_player_token = strdup(token);
    g_player_url = strdup(url);

    set_player_state(TVS_PLAYBACK_NEW_SOURCE);

    tvs_media_player_set_source(g_player_url, token, offset);


    if (start_now) {
        //这个可以不用调用，因为在外层也会调用upload_start
        /* set_player_state(TVS_PLAYBACK_STARTED); */
        /* upload_player_started();	 */
        TVS_LOG_PRINTF("call tvs_media_player_start_play 1\n");
        tvs_media_player_start_play(token);
    }

    do_unlock();
    return 0;
}

int tvs_media_player_inner_set_source(const char *url, const char *token, unsigned int offset)
{
    return tvs_media_player_inner_set_source_1(url, token, offset, false);
}

int tvs_media_player_inner_set_source_and_play(const char *url, const char *token, unsigned int offset)
{
    return tvs_media_player_inner_set_source_1(url, token, offset, true);
}

void tvs_media_player_inner_init()
{
    TVS_LOCKER_INIT

    tvs_media_player_init();
}

void tvs_core_notify_media_player_on_play_finished(const char *token)
{
    TVS_LOG_PRINTF("call %s\n", __func__);
    do_lock();
    tvs_media_callback_finished(token);
    do_unlock();
}

void tvs_core_notify_media_player_on_play_stopped(int error_code, const char *token)
{
    TVS_LOG_PRINTF("call %s\n", __func__);
    do_lock();

    tvs_media_callback_stopped(error_code, token);
    do_unlock();
}

void tvs_core_notify_media_player_on_play_started(const char *token)
{
    TVS_LOG_PRINTF("call %s\n", __func__);
    do_lock();
    if (g_tvs_playback_state == TVS_PLAYBACK_STARTED || g_tvs_playback_state == TVS_PLAYBACK_RESUMED) {
        TVS_LOG_PRINTF("already started, state %d\n", g_tvs_playback_state);
        do_unlock();
        return;
    }
    tvs_playback_state state = TVS_PLAYBACK_STARTED;
    set_player_state(state);
    tvs_media_player_inner_upload_song_status(state);
    do_unlock();
}

void tvs_core_notify_media_player_on_play_paused(const char *token)
{
    TVS_LOG_PRINTF("call %s\n", __func__);
    do_lock();
    //改回pause, 上报改成stop
    if (g_tvs_playback_state == TVS_PLAYBACK_PAUSED) {
        /* if (g_tvs_playback_state == TVS_PLAYBACK_STOPPED) { */
        do_unlock();
        TVS_LOG_PRINTF("already paused\n");
        return;
    }
    tvs_playback_state state = TVS_PLAYBACK_PAUSED;
    /* tvs_playback_state state = TVS_PLAYBACK_STOPPED; */
    set_player_state(state);
    tvs_media_player_inner_upload_song_status(state);
    do_unlock();
}


