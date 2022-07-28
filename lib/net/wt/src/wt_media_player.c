#include "server/ai_server.h"
#include "vt_types.h"
#include "timer.h"
#include "os/os_api.h"
#include "vt_bk.h"
#include "vt_types.h"
#include "linux_camera.h"
#include "linux_player.h"

extern const struct ai_sdk_api wt_sdk_api;

struct player_obj {
    pfun_vt_player_state_cb state_cb;
    void *userp;
    pfun_vt_task_cb task_cb;
    void *task_ctx;
};

static struct player_obj player_object = {0};

static char current_url[640] = {0};
static vt_mediaplayer_state media_state = E_VTPLAYER_STATE_STOPED;

static int player_init_url(char *url)
{
    snprintf(current_url, sizeof(current_url), "%s", url);
    return 0;
}

static int player_play(void)
{
    media_state = E_VTPLAYER_STATE_PLAYING;
    return ai_server_event_url(&wt_sdk_api, current_url, AI_SERVER_EVENT_URL);
}

static int player_stop(void)
{
    media_state = E_VTPLAYER_STATE_STOPED;
    //ret = obj->task_cb(obj->task_ctx, E_VT_TASKS_EXITED);
    return ai_server_event_url(&wt_sdk_api, NULL, AI_SERVER_EVENT_STOP);
}

static int player_pause(void)
{
    media_state = E_VTPLAYER_STATE_PAUSED;
    return ai_server_event_url(&wt_sdk_api, NULL, AI_SERVER_EVENT_PAUSE);
}

static int player_resume(void)
{
    media_state = E_VTPLAYER_STATE_PLAYING;
    return ai_server_event_url(&wt_sdk_api, NULL, AI_SERVER_EVENT_CONTINUE);
}

static int ispalying(char *url)
{
    return media_state == E_VTPLAYER_STATE_PLAYING;
}

int player_set_state_cb(pfun_vt_player_state_cb state_cb, void *userp)
{
    player_object.state_cb = state_cb;
    player_object.userp = userp;
    printf("===player_set_state_cb called===\n");
    return 0;
}

void player_state_changed_notify(vt_mediaplayer_state state)
{
    media_state = state;
    if (player_object.state_cb) {
        player_object.state_cb(current_url, state, player_object.userp);
    }
}

static void player_task_cb(void *args)
{
    int ret = 0;
    struct player_obj *obj = (struct player_obj *)args;

    while (1) {
        /*通知SDK，线程已经running起来了，同时调用SDK的TASK任务接口*/
        ret = obj->task_cb(obj->task_ctx, E_VT_TASKS_RUNNING);
        /*函数返回了 <0 表明任务请求退出TASK*/
        if (ret < 0) {
            printf("player_task request exit\n");
            break;
        }
    }
    /*通知SDK，TASK确实退出了，用于状态确认*/
    ret = obj->task_cb(obj->task_ctx, E_VT_TASKS_EXITED);
    printf("player_task exit\n");
}

/*注册给SDK，SDK调用此接口启动TASK*/
static int player_start_task_cb(pfun_vt_task_cb taskcb, void *ctx)
{
    player_object.task_cb = taskcb;
    player_object.task_ctx = ctx;

    return thread_fork("wt_player_app", 20, 1024, 0, NULL, player_task_cb, (void *)&player_object);
}

static const media_player_ops_t mmedia_player_ops_t = {
    player_init_url,
    player_play,
    player_stop,
    player_pause,
    player_resume,
    ispalying,
    player_set_state_cb,
    player_start_task_cb
};

const media_player_ops_t *get_linux_player(void)
{
    return &mmedia_player_ops_t;
}

