/**
* @file  tvs_audio_provider.c
* @brief TVS SDK语音数据提供者
* @date  2019-5-10
* 接收外部传入的PCM数据，进行speex编码，为智能语音模块提供语音数据
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "os_wrapper.h"
#include "tvs_audio_provider.h"
#include "tvs_data_buffer.h"
#include "tvs_threads.h"
#include "generic/circular_buf.h"

#define TVS_LOG_DEBUG_MODULE  "AUDPROVIDER"
#include "tvs_log.h"
#include "tvs_audio_codec.h"
#include "tvs_speex_codec.h"
#include "tvs_audio_recorder_thread.h"
#include "tvs_audio_recorder_interface.h"
#include "tvs_config.h"
#include "tvs_exception_report.h"
#include "tvs_tts_player.h"
#include "tvs_api_impl.h"


#define PROVIDER_DEBUG   0

#define CODEC_BUFFER_SIZE   12 * 1024

static cbuffer_t g_codec_fifo;
static char g_codec_buffer[CODEC_BUFFER_SIZE];
static bool g_writer_end = true;

TVS_LOCKER_DEFINE

static tvs_api_callback_on_provider_reader_stop g_callback_on_reader_stop = NULL;

extern void tvs_speech_set_end_time(int time);

void tvs_audio_provider_set_callback(tvs_api_callback_on_provider_reader_stop cb)
{
    g_callback_on_reader_stop = cb;
}

static void clear_codec_buffer()
{
    cbuf_clear(&g_codec_fifo);
}

static void clear_buffer()
{
    clear_codec_buffer();
}

static void init_buffer()
{
    cbuf_init(&g_codec_fifo, g_codec_buffer, CODEC_BUFFER_SIZE);
}

static int read_buffer(cbuffer_t *buffer, char *data_buf, int data_buf_len)
{
    return cbuf_read(buffer, data_buf, data_buf_len);
}

static int get_size(cbuffer_t *buffer)
{
    return cbuf_get_data_size(buffer);
}

static int write_buffer(cbuffer_t *buffer, char *data_buf, int data_buf_len)
{
    return cbuf_write(buffer, data_buf, data_buf_len);
}

int tvs_audio_provider_write(char *data, int data_size)
{
    do_lock();

    if (g_writer_end) {
        // 调用了writer_end或者没有调用writer_begin
        do_unlock();
        return 0;
    }

    int ret = write_buffer(&g_codec_fifo, data, data_size);

    do_unlock();

    if (!ret) {
        TVS_LOG_PRINTF("speex codec fifo is full\n");
        tvs_exception_report_start(EXCEPTION_AUDIO_ENCODER, "speex codec fifo is full", strlen("speex codec fifo is full"));
    }

    return ret;
}

int tvs_audio_provider_writer_begin()
{
    // 注意，如果不调用writer end，直接调用writer begin，也应该先把上buffer数据清除掉
    TVS_LOG_PRINTF("call writer begin, session %d\n", tvs_core_get_current_session_id());

    do_lock();

    g_writer_end = false;

    init_buffer();

    do_unlock();

    return 0;
}

void tvs_audio_provider_writer_end()
{
    do_lock();
    if (g_writer_end) {
        do_unlock();
        return;
    }
    g_writer_end = true;
    do_unlock();
}

int tvs_audio_provider_reader_begin(int session)
{
    TVS_LOG_PRINTF("audio reader begin, session %d\n", session);
    return 0;
}

static void notify_listener(int session_id, int error)
{
    tvs_audio_recorder_thread_notify_reader_end(session_id, error);
    if (g_callback_on_reader_stop != NULL) {
        g_callback_on_reader_stop(session_id, error);
    }
}

void tvs_audio_provider_reader_end(int session_id, int error)
{
    int session = tvs_core_get_current_session_id();
    if (session != session_id) {
        TVS_LOG_PRINTF("audio reader end but session is stop, cur %d, target %d\n", session, session_id);
        return;
    }

    TVS_LOG_PRINTF("audio reader end, session %d\n", session_id);

    do_lock();
    // reader停止，且session并未过期，需要将codec buffer中的数据清除一遍
    clear_codec_buffer();
    do_unlock();

    tvs_speech_set_end_time(os_wrapper_get_time_ms());
    notify_listener(session_id, error);
}

int tvs_audio_provider_read(int session_id, char *buffer, int buffer_size)
{
    int session = tvs_core_get_current_session_id();
    if (session != session_id) {
        TVS_LOG_PRINTF("audio reader write but session is stop, cur %d, target %d\n", session, session_id);
        return 0;
    }

    do_lock();

    int size = get_size(&g_codec_fifo);

    if (size < buffer_size) {
        if (g_writer_end) {
            // 读取最后一点数据
            //TVS_LOG_PRINTF("read_buffer want last %d, cur size %d, session id %d\n", size, g_codec_fifo.len, session_id);
            read_buffer(&g_codec_fifo, buffer, size);
            do_unlock();
            return size;
        } else {
            do_unlock();
            // 数据不够，下次再读了
            return TVS_AUDIO_PROVIDER_ERROR_READ_TOO_FAST;
        }
    }

    //TVS_LOG_PRINTF("read_buffer want %d, cur size %d, session id %d\n", buffer_size, g_codec_fifo.len, session_id);

    read_buffer(&g_codec_fifo, buffer, buffer_size);

    do_unlock();

    return buffer_size;
}

static void rec_provider_callback_on_reader_stop(int session_id, int error)
{
    int cur_session_id = tvs_core_get_current_session_id();

    if (cur_session_id == session_id && !g_writer_end) {
        TVS_LOG_PRINTF("%s, session id %d, cur %d, error %d\n", __func__, session_id, cur_session_id, error);
        tvs_taskq_post(TC_RECORD_STOP);
    }
}

void tvs_audio_recorder_thread_start(int time_out_ms, int session_id)
{
    /* TVS_LOCKER_INIT */

    tvs_tts_player_close();

    tvs_audio_provider_writer_begin();

    tvs_audio_recorder_open(tvs_config_get_recorder_bitrate(), tvs_config_get_recorder_channels(),
                            0, 0, session_id);
}

void tvs_audio_recorder_thread_notify_reader_end(int session_id, int error)
{
    rec_provider_callback_on_reader_stop(session_id, error);
}

void tvs_audio_recorder_thread_stop()
{
    tvs_audio_provider_writer_end();
    tvs_audio_recorder_close(0, 0);
}

int tvs_audio_recorder_thread_init()
{
    TVS_LOCKER_INIT
    return 0;
}
