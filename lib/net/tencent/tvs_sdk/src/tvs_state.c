#include "tvs_log.h"

#include "tvs_core.h"
#include "tvs_media_player_inner.h"
#include "tvs_audio_track_interface.h"
#include "tvs_state.h"
#include "tvs_threads.h"

extern tvs_api_callback g_tvs_core_callback;

static tvs_recognize_state g_tvs_core_state = TVS_STATE_IDLE;

bool g_is_busy = false;

TVS_LOCKER_DEFINE

void tvs_state_start_work(bool working, int type, int error)
{
    if (working) {
        tvs_state_set(TVS_STATE_PREPARING, type, error);
    } else {
        tvs_state_set(TVS_STATE_IDLE, type, error);
    }
}

void tvs_state_set(int new_state, int type, int error)
{
    do_lock();
    if (g_tvs_core_state == new_state) {
        do_unlock();
        return;
    }

    int old_state = g_tvs_core_state;
    g_tvs_core_state = new_state;

    do_unlock();

    tvs_api_state_param state_param = {0};
    state_param.error = error;
    state_param.control_type = type;

    TVS_LOG_PRINTF("state changed, type %d, from %d to %d, error %d\n", type, old_state, new_state, error);

    if (g_tvs_core_callback.on_state_changed != NULL) {
        g_tvs_core_callback.on_state_changed(old_state, new_state, &state_param);
    }

    if (old_state == TVS_STATE_IDLE && new_state != TVS_STATE_IDLE) {
        // enter working state
        tvs_media_player_inner_pause_play();
    } else if (old_state != TVS_STATE_IDLE && new_state == TVS_STATE_IDLE) {
        // return idle state
        tvs_media_player_inner_start_play();
    }
}

void tvs_state_manager_init()
{

    TVS_LOCKER_INIT

}

