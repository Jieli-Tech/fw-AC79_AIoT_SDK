#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tvs_core.h"
#include "tvs_api/tvs_api.h"
#include "tvs_audio_recorder_thread.h"
#include "tvs_executor_service.h"
#include "tvs_audio_provider.h"
#include "tvs_preference.h"
#include "tvs_directives_processor.h"
#include "tvs_down_channel.h"

#include "tvs_log.h"
#include "tvs_threads.h"
#include "tvs_iplist.h"
#include "tvs_ip_provider.h"
#include "mongoose.h"
#include "tvs_http_client.h"


static const char *g_state_str[] = {"idle",
                                    "preparing",
                                    "recognizing",
                                    "busy",
                                    "speech playing",
                                    "complete"
                                   };

TVS_LOCKER_DEFINE

int tvs_api_start_recognize_inner(tvs_api_recognizer_type type, const char *wakeword, int startIndexInSamples, int endIndexInSamples)
{
    int ret = 0;
    if ((ret = tvs_is_executor_valid_ex(TVS_EXECUTOR_CMD_SPEECH, false)) != 0) {
        return ret;
    }

    int sessionid = tvs_core_new_session_id();
    tvs_audio_recorder_thread_start(15 * 1000, sessionid);
    return tvs_core_start_recognize(sessionid, type, wakeword, startIndexInSamples, endIndexInSamples);
}

static int tvs_api_start_recognize_inner1()
{
    return tvs_api_start_recognize_inner(TVS_RECOGNIZER_TAP, NULL, 0, 0);
}

int tvs_api_start_recognize_ex(bool use_8k_bitrate, bool use_speex)
{
    return tvs_api_start_recognize_inner1();
}

int tvs_api_start_recognize()
{
    return tvs_api_start_recognize_inner1();
}

int tvs_api_stop_recognize()
{
    tvs_audio_provider_writer_end();

    tvs_audio_recorder_thread_stop();
    return 0;
}

int tvs_api_stop_all_activity()
{

    tvs_audio_provider_writer_end();

    tvs_audio_recorder_thread_stop();

    tvs_executor_stop_all_activities();

    return 0;
}

int tvs_api_set_asr_callback(tvs_callback_on_asr_result callback)
{
    tvs_directives_set_asr_callback(callback);
    return 0;
}

int tvs_api_playcontrol_next()
{
    return tvs_core_play_control_next();
}

int tvs_api_playcontrol_previous()
{
    return tvs_core_play_control_prev();
}

int tvs_api_on_voice_wakeup(int session_id, const char *wakeword, int startIndexInSamples, int endIndexInSamples)
{
    return tvs_core_start_recognize(session_id, TVS_RECOGNIZER_WAKEWORD, wakeword, startIndexInSamples, endIndexInSamples);
}

int tvs_api_start_text_recognize(const char *text)
{
    return 0;
}

int tvs_api_send_semantic(const char *semantic)
{
    return tvs_core_send_semantic(semantic);
}

int tvs_api_start_text_to_speech(char *text)
{
    tvs_api_tts_param param;
    memset(&param, 0, sizeof(tvs_api_tts_param));
    param.tts_text = text;
    param.pitch = -1;
    param.volume = -1;
    param.speed = -1;
    param.timbre = NULL;

    return tvs_core_start_text_to_speech(&param);
}

int tvs_api_start_text_to_speech_ex(tvs_api_tts_param *tts_param)
{
    return tvs_core_start_text_to_speech(tts_param);
}

int tvs_api_init(tvs_api_callback *api_callbacks, tvs_default_config *config, tvs_product_qua *produce_qua)
{
    TVS_LOCKER_INIT

    return tvs_core_initialize(api_callbacks, config, produce_qua);
}

void tvs_api_set_env(tvs_api_env env)
{

}

void tvs_api_set_sandbox(bool open_sandbox)
{

}

tvs_mode tvs_api_get_current_mode()
{
    return TVS_MODE_NORMAL;
}

void tvs_api_log_enable(bool enable)
{
    tvs_core_enable_log(enable);
}

int tvs_api_start()
{
    return 0;
}

int tvs_api_stop()
{
    return 0;
}

void tvs_event_push_ack(const char *token)
{

}

void tvs_event_push_text(const char *event_msg)
{
    tvs_executor_send_push_text((char *)event_msg);
}

const char *tvs_get_states_name(tvs_recognize_state state)
{
    if (state >= sizeof(g_state_str) / sizeof(char *)) {
        return "unknown";
    }

    return g_state_str[(int)state];
}

int tvs_api_new_session_id()
{
    return tvs_core_new_session_id();
}

int tvs_api_get_current_session_id()
{
    return tvs_core_get_current_session_id();
}

int tvs_api_audio_provider_writer_begin()
{
    return tvs_audio_provider_writer_begin();
}

void tvs_api_audio_provider_writer_end()
{
    tvs_audio_provider_writer_end();
}

int tvs_api_audio_provider_write(const char *audio_data, int size)
{
    return tvs_audio_provider_write((char *)audio_data, size);
}

int tvs_api_audio_provider_listen(tvs_api_callback_on_provider_reader_stop callback)
{
    tvs_audio_provider_set_callback(callback);
    return 0;
}

static void tvs_api_test_set_asr(const char *env)
{
    if (env == NULL) {
        return;
    }

    int bitrate = 8000;
    if (strcmp(env, "speex_8000") == 0) {
        bitrate = 8000;
    } else if (strcmp(env, "speex_16000") == 0) {
        bitrate = 16000;
    } else if (strcmp(env, "pcm_8000") == 0) {
        bitrate = 8000;
    } else if (strcmp(env, "pcm_16000") == 0) {
        bitrate = 16000;
    } else {
        bitrate = 8000;
    }

    TVS_LOG_PRINTF("recognize param set speex enable, bitrate %d\n", bitrate);

    tvs_preference_set_number_value(TVS_PREF_ARS_BITRATE, bitrate);
}

static void tvs_api_test_set_env(const char *env)
{
    if (env == NULL) {
        return;
    }

    int env_i = TVS_API_ENV_TEST;
    char *name = "";
    if (strcmp(env, "test") == 0) {
        env_i = TVS_API_ENV_TEST;
        name = "test";
    } else if (strcmp(env, "exp") == 0) {
        env_i = TVS_API_ENV_EXP;
        name = "exp";
    } else if (strcmp(env, "dev") == 0) {
        env_i = TVS_API_ENV_DEV;
        name = "dev";
    } else {
        env_i = TVS_API_ENV_NORMAL;
        name = "normal";
    }

    TVS_LOG_PRINTF("auth param set env %s\n", name);

    tvs_preference_set_number_value(TVS_PREF_ENV_TYPE, env_i);
}

static void tvs_api_test_set_sanbox(const char *open)
{
    if (open == NULL) {
        return;
    }
    bool b_open = false;
    if (strcmp(open, "open") == 0) {
        b_open = true;
    } else if (strcmp(open, "close") == 0) {
        b_open = false;
    } else {
        return;
    }

    TVS_LOG_PRINTF("sandbox param set %s\n", open);

    tvs_preference_set_number_value(TVS_PREFERENCE_SANDBOX, b_open);
}

static void tvs_api_test_iplist(const char *value)
{
    if (value == NULL) {
        return;
    }
    int ip = tvs_ip_provider_convert_ip_str(value);

    tvs_ip_provider_on_ip_invalid(ip);
}

void tvs_echo_test_add_error_ip(const char *ip_str);

void tvs_echo_test_remove_error_ip(const char *ip_str);

/*static char* base64_decode(const char* src, int src_len) {
	if (src == NULL || src_len <= 0) {
		return NULL;
	}

	int dst_len = src_len + 1;

	char* dst = TVS_MALLOC(dst_len);
	if (NULL == dst) {
		return NULL;
	}

	memset(dst, 0, dst_len);

	mg_base64_decode((const unsigned char *)src, src_len, dst);

	return dst;
}

void tvs_tts_test(const char* src) {
	int src_len = src == NULL ? 0 : strlen(src);

	char* dst = base64_decode(src, src_len);

	tvs_api_start_text_to_speech(dst);
}*/

extern void tvs_echo_test_print();
extern void tvs_echo_test_clear();
extern void tvs_speech_test_set_connect_disable(bool disable);

void tvs_test_get_traffic()
{
    unsigned long traffic = tvs_http_client_get_total_send_recv_size();

    TVS_LOG_PRINTF("total http traffic: %lu bytes\n", traffic);
}

void tvs_test_speech_connection(const char *value)
{
    if (value == NULL) {
        return;
    }

    if (strcmp(value, "enable") == 0) {
        tvs_speech_test_set_connect_disable(false);
    } else if (strcmp(value, "disable") == 0) {
        tvs_speech_test_set_connect_disable(true);
    }
}

void tvs_api_test_case(const char *cmd, const char *value)
{
    if (cmd == NULL) {
        return;
    }

    if (strcmp(cmd, "env") == 0) {
        tvs_api_test_set_env(value);
    } else if (strcmp(cmd, "reco") == 0) {
        tvs_api_test_set_asr(value);
    } else if (strcmp(cmd, "sandbox") == 0) {
        tvs_api_test_set_sanbox(value);
    } else if (strcmp(cmd, "ipcheck") == 0) {
        // 测试IP失效策略
        tvs_api_test_iplist(value);
    } else if (strcmp(cmd, "break") == 0) {
        tvs_down_channel_break();
    } else if (strcmp(cmd, "echoadd") == 0) {
        tvs_echo_test_add_error_ip(value);
    } else if (strcmp(cmd, "echodel") == 0) {
        tvs_echo_test_remove_error_ip(value);
    } else if (strcmp(cmd, "echoclear") == 0) {
        tvs_echo_test_clear();
    } else if (strcmp(cmd, "echoprint") == 0) {
        tvs_echo_test_print();
    }  else if (strcmp(cmd, "speechcon") == 0) {
        tvs_test_speech_connection(value);
    } else if (strcmp(cmd, "iplist") == 0) {
        tvs_executor_start_iplist(false);
    } else if (strcmp(cmd, "traffic") == 0) {
        tvs_test_get_traffic();
    } else if (strcmp(cmd, "tts") == 0) {
        //tvs_tts_test(value);
    } else if (strcmp(cmd, "mem") == 0) {
#if 0
        os_wrapper_print_mem();
#endif
    } else {
        TVS_LOG_PRINTF("invalid test case, cmd %s, valud %s", cmd, value == NULL ? "" : value);
    }
}

