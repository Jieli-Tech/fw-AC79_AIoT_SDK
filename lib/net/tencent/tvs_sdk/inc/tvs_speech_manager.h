#ifndef __TVS_SPEECH_MANAGER_H__
#define __TVS_SPEECH_MANAGER_H__

#include "tvs_audio_recorder_interface.h"
#include "tvs_http_client.h"
#include "tvs_common_def.h"

typedef struct {
    int session_id;
    int bitrate;
    tvs_api_recognizer_type type;
    int startIndexInSamples;
    int endIndexInSamples;
    char wakeword[50];
} tvs_speech_manager_config;

int tvs_speech_manager_init();

int tvs_speech_manager_stop_capture(char *dialog_id);

int tvs_speech_manager_start(tvs_speech_manager_config *speech_config,
                             tvs_http_client_callback_exit_loop should_exit_func,
                             tvs_http_client_callback_should_cancel should_cancel,
                             void *exit_param,
                             bool *expect_speech,
                             bool *connected);

void stop_expect_speech_timer();
void start_expect_speech_timer(void *func, int time);


#endif
