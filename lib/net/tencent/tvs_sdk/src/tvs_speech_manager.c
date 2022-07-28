#include "tvs_http_client.h"
#include "tvs_http_manager.h"
#include "tvs_speech_manager.h"
#include "tvs_multipart_handler.h"
#include "os_wrapper.h"
#include "tvs_config.h"
#include "tvs_audio_recorder_interface.h"
#include "tvs_jsons.h"

#define TVS_LOG_DEBUG_MODULE  "SPEECH"
#include "tvs_log.h"

#include "tvs_audio_track_interface.h"
#include "tvs_audio_provider.h"

#include "tvs_state.h"
#include "tvs_threads.h"
#include "tvs_exception_report.h"

#define RECORDER_BUFFER_SIZE   512

#define SAVE_TTS_DATA   0

#define ERROR_NETWORK_CONNECT    -100

#define TVS_DEBUG_PRINTF(fmt, arg...) \
	do {							\
		if (1) SYS_LOG("[TVS_D][%s:%ld]" fmt, TVS_LOG_DEBUG_MODULE, os_wrapper_get_time_ms(), ##arg); \
	} while (0)

typedef struct {
    int send_size;
    bool recorder_open;
    char *recorder_buffer;
    int recorder_buffer_size;
    int error;
    int record_start_time;
    bool recording;
    bool stop_capture;
    void *multipart_info;
    tvs_multipart_callback multipart_callback;
    char *bounder_buffer;
    tvs_speech_manager_config *config;
    char *dialog_id;
    int record_type;
    tvs_multi_param multi_param;
} tvs_speech_param;

int tvs_audio_provider_reader_begin(int session_id);

void tvs_audio_provider_reader_end(int session_id, int error);

int tvs_audio_provider_read(int session_id, char *buffer, int buffer_size);

static bool g_recv_stop_capture = false;

static char *g_stop_capture_dialog_id = NULL;

TVS_LOCKER_DEFINE

static void *g_expect_timer = NULL;

static bool g_test_set_connect_disable = false;

#if SAVE_TTS_DATA
#include "fs/fatfs/ff.h"

static FIL tts_fp;

static void tvs_speech_save_start()
{
    f_unlink("0:/tts123.mp3");
    int ret = f_open(&tts_fp, "0:/tts123.mp3", FA_CREATE_ALWAYS | FA_WRITE);

    if (ret != FR_OK) {
        TVS_LOG_PRINTF("open fail\n");
        return;
    }
}

void tvs_speech_save_put_data(void *data, int len)
{
    unsigned int has_write;

    f_write(&tts_fp, data, len, &has_write);

    if (len != has_write) {
        TVS_LOG_PRINTF("write fail\n");
        return;
    }
}

static void tvs_speech_save_stop()
{
    f_close(&tts_fp);
}
#else
static void tvs_speech_save_start()
{

}

void tvs_speech_save_put_data(void *data, int len)
{

}

static void tvs_speech_save_stop()
{
}

#endif

void stop_expect_speech_timer()
{
    do_lock();
    if (g_expect_timer != NULL) {
        TVS_LOG_PRINTF("stop expect speech timer\n");
        os_wrapper_stop_timer(g_expect_timer);
        g_expect_timer = NULL;
    }
    do_unlock();
}

void start_expect_speech_timer(void *func, int time)
{
    do_lock();

    os_wrapper_start_timer(&g_expect_timer, func, time, false);
    TVS_LOG_PRINTF("start expect speech timer, handle %p\n", g_expect_timer);

    do_unlock();
}

static int start_recorder(tvs_http_client_param *param)
{
    tvs_speech_param *speech_param = (tvs_speech_param *)param->user_data;

    if (speech_param->recorder_open) {
        return 0;
    }

    if (0 == tvs_audio_provider_reader_begin(speech_param->multi_param.directives_param.session_id)) {
        TVS_LOG_PRINTF("start recorder\n");
        speech_param->recorder_open = true;
        speech_param->record_start_time = os_wrapper_get_time_ms();
        return 0;
    } else {
        return -1;
    }
}

static void stop_recorder(tvs_http_client_param *param, int reason)
{
    tvs_speech_param *speech_param = (tvs_speech_param *)param->user_data;

    if (!speech_param->recorder_open) {
        return;
    }

    tvs_state_set(TVS_STATE_BUSY, TVS_CONTROL_SPEECH, 0);

    TVS_LOG_PRINTF("stop recorder\n");
    tvs_audio_provider_reader_end(speech_param->multi_param.directives_param.session_id, reason);
    speech_param->recorder_open = false;
}

void tvs_speech_test_set_connect_disable(bool disable)
{
    g_test_set_connect_disable = disable;

    TVS_LOG_PRINTF("test set speech connetion disable %d", disable);
}

void tvs_speech_callback_on_connect(void *connection, int error_code, tvs_http_client_param *param)
{
    tvs_speech_param *speech_param = (tvs_speech_param *)param->user_data;

    if (g_test_set_connect_disable) {
        // 测试用
        error_code = 113;
    }

    if (error_code == 0) {
        //TVS_LOG_PRINTF("server connected\n");
        if (start_recorder(param) != 0) {
            speech_param->error = -1;
        }
    } else {
        speech_param->error = -1;
        TVS_LOG_PRINTF("%s, error %d\n", __func__, error_code);
        tvs_audio_provider_reader_end(speech_param->multi_param.directives_param.session_id, TVS_AUDIO_PROVIDER_ERROR_NETWORK);
    }

}

void tvs_speech_callback_on_response(void *connection, int ret_code, const char *response, int response_len, tvs_http_client_param *param)
{
    if (ret_code == 200 || ret_code == 204) {
        //TVS_LOG_PRINTF("speech response 200ok\n");
    } else {
        TVS_LOG_PRINTF("speech response %d, body %.*s\n", ret_code, response_len, response);
        char ch_temp[40] = {0};
        snprintf(ch_temp, sizeof(ch_temp), "http response code error:%d", ret_code);
        tvs_exception_report_start(EXCEPTION_SPEECH, ch_temp, strlen(ch_temp));
    }
}

void tvs_speech_callback_on_close(void *connection, int by_server, tvs_http_client_param *param)
{
    //TVS_LOG_PRINTF("%s\n", __func__);
}

int tvs_speech_callback_on_start_request(void *connection, const char *path, const char *host,
        const char *extra_header, const char *payload, tvs_http_client_param *param)
{
    //TVS_LOG_PRINTF("send payload %p - %s\n", connection, payload);
    tvs_http_send_multipart_start((struct mg_connection *)connection, path, strlen(path), host, strlen(host), payload);
    return 0;
}

void tvs_speech_callback_on_recv_chunked(void *connection, void *ev_data, const char *chunked, int chunked_len, tvs_http_client_param *param)
{
    tvs_speech_param *speech_param = (tvs_speech_param *)param->user_data;
    stop_recorder(param, TVS_AUDIO_PROVIDER_ERROR_STOP_CAPTURE);

    tvs_multipart_process_chunked(&speech_param->multipart_info, &speech_param->multipart_callback, &speech_param->multi_param, ev_data, false, false);
}

void tvs_speech_callback_on_loop_end(tvs_http_client_param *param)
{

    tvs_speech_param *speech_param = (tvs_speech_param *)param->user_data;

    stop_recorder(param, 0);

    tvs_release_multipart_parser(&speech_param->multipart_info, false);

    if (speech_param->recorder_buffer != NULL) {
        TVS_FREE(speech_param->recorder_buffer);
        speech_param->recorder_buffer = NULL;
    }
}

static void set_stop_capture_param(bool stop, char *dialog_id)
{
    do_lock();
    g_recv_stop_capture = stop;
    if (g_stop_capture_dialog_id != NULL) {
        TVS_FREE(g_stop_capture_dialog_id);
        g_stop_capture_dialog_id = NULL;
    }

    if (dialog_id != NULL) {
        g_stop_capture_dialog_id = strdup(dialog_id);
    }
    do_unlock();

}
int tvs_speech_manager_stop_capture(char *dialog_id)
{
    set_stop_capture_param(true, dialog_id);

    return 0;
}

bool tvs_speech_callback_on_loop(void *connection, tvs_http_client_param *param)
{
    tvs_speech_param *speech_param = (tvs_speech_param *)param->user_data;

    if (speech_param->error != 0) {
        TVS_LOG_PRINTF("get error 1\n");
        return true;
    }

    int ret = 0;

    if (speech_param->recorder_open) {

        do_lock();
        bool stop_capture = false;
        if (g_recv_stop_capture && strcmp(g_stop_capture_dialog_id, speech_param->dialog_id) == 0) {
            stop_capture = true;
        }
        do_unlock();

        if (stop_capture) {
            stop_recorder(param, TVS_AUDIO_PROVIDER_ERROR_STOP_CAPTURE);
        } else if (os_wrapper_get_time_ms() - speech_param->record_start_time > 20 * 1000) {
            stop_recorder(param, TVS_AUDIO_PROVIDER_ERROR_TIME_OUT);
        } else {
            if (speech_param->recorder_buffer == NULL) {
                speech_param->recorder_buffer = TVS_MALLOC(RECORDER_BUFFER_SIZE);
                speech_param->recorder_buffer_size = RECORDER_BUFFER_SIZE;
                //TVS_LOG_PRINTF("init recorder buffer\n");

                if (speech_param->recorder_buffer == NULL) {
                    TVS_LOG_PRINTF("init recorder buffer OOM\n");
                    return true;
                }
            }
            ret = tvs_audio_provider_read(speech_param->multi_param.directives_param.session_id, speech_param->recorder_buffer, speech_param->recorder_buffer_size);
            speech_param->recording = true;

            if (ret > 0) {
                speech_param->send_size += ret;
                tvs_http_send_multipart_data((struct mg_connection *)connection, speech_param->recorder_buffer, ret);
            } else if (TVS_AUDIO_PROVIDER_ERROR_READ_TOO_FAST == ret) {
                //TVS_LOG_PRINTF("recorder wait next time\n");
                // 下次再读
                os_wrapper_sleep(20);
            } else {
                TVS_LOG_PRINTF("recorder read end, ret %d\n", ret);
                stop_recorder(param, TVS_AUDIO_PROVIDER_ERROR_NONE);
            }
        }
    } else {
        if (speech_param->recording) {
            TVS_LOG_PRINTF("recorder read end, data size %d\n", speech_param->send_size);
            tvs_http_send_multipart_end((struct mg_connection *)connection);
            speech_param->recording = false;
        }
    }

    // go on
    return false;
}

int tvs_speech_manager_init()
{
    TVS_LOCKER_INIT

    return 0;
}

int tvs_speech_manager_start(tvs_speech_manager_config *speech_config,
                             tvs_http_client_callback_exit_loop should_exit_func,
                             tvs_http_client_callback_should_cancel should_cancel,
                             void *exit_param,
                             bool *expect_speech,
                             bool *connected)
{

    if (speech_config == NULL) {
        TVS_LOG_PRINTF("speech error for invalid speech config\n");
        return -1;
    }
    set_stop_capture_param(false, NULL);

    tvs_http_client_callback cb = { 0 };
    cb.cb_on_chunked = tvs_speech_callback_on_recv_chunked;
    cb.cb_on_close = tvs_speech_callback_on_close;
    cb.cb_on_connect = tvs_speech_callback_on_connect;
    cb.cb_on_loop = tvs_speech_callback_on_loop;
    cb.cb_on_request = tvs_speech_callback_on_start_request;
    cb.cb_on_response = tvs_speech_callback_on_response;
    cb.cb_on_loop_end = tvs_speech_callback_on_loop_end;
    cb.exit_loop = should_exit_func;
    cb.exit_loop_param = exit_param;
    cb.should_cancel = should_cancel;

    tvs_http_client_config config = { 0 };
    tvs_http_client_param http_param = { 0 };
    tvs_speech_param speech_param = { 0 };
    speech_param.config = speech_config;
    speech_param.multi_param.directives_param.session_id = speech_config->session_id;
    speech_param.multi_param.control_type = TVS_CONTROL_SPEECH;

    int bitrate = speech_config->bitrate;

    char *url = tvs_config_get_event_url();
    speech_param.dialog_id = do_create_msg_id();
    char *payload = get_speech_recognize_request_body(true, bitrate == 8000, speech_param.dialog_id,
                    speech_config->type, speech_config->wakeword, speech_config->startIndexInSamples, speech_config->endIndexInSamples);

    if (NULL == payload) {
        if (NULL != speech_param.dialog_id) {
            TVS_FREE(speech_param.dialog_id);
            speech_param.dialog_id = NULL;
        }
        TVS_LOG_PRINTF("speech error for invalid payload\n");
        return -1;
    }

    tvs_init_multipart_callback(&speech_param.multipart_callback);
    tvs_speech_save_start();

    tvs_state_set(TVS_STATE_RECOGNIZNG, TVS_CONTROL_SPEECH, 0);

    int ret = tvs_http_client_request(url, NULL, payload, &speech_param, &config, &cb, &http_param, true);

    tvs_speech_save_stop();

    TVS_LOG_PRINTF("speech ret %d, has tts %d, audio %d, tts %d, payload %s\n",
                   ret,
                   speech_param.multi_param.has_tts,
                   speech_param.send_size,
                   speech_param.multi_param.tts_bytes,
                   payload);

    if (NULL != speech_param.dialog_id) {
        TVS_FREE(speech_param.dialog_id);
        speech_param.dialog_id = NULL;
    }

    set_stop_capture_param(false, NULL);

    TVS_FREE(payload);

    if (expect_speech != NULL) {
        *expect_speech = speech_param.multi_param.directives_param.expect_speech;
    }

    if (connected != NULL) {
        *connected = http_param.connected;
    }

    return ret;
}
