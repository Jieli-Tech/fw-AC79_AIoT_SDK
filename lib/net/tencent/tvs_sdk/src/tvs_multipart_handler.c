
#include "tvs_multipart_handler.h"
#include "mongoose.h"
#define TVS_LOG_DEBUG_MODULE  "MULTH"

#include "tvs_log.h"
#include "tvs_directives_handler.h"
#include "tvs_state.h"
#include "tvs_core.h"

#include "tvs_tts_player.h"
#include "tvs_api_config.h"

void tvs_speech_save_put_data(void *data, int len);

static void tvs_multi_on_recv_binary_begin(void *multipart_info, bool down_channel)
{
    if (down_channel) {
        return;
    }
    TVS_LOG_PRINTF("%s\n", __func__);

    tvs_multi_param *param = tvs_multipart_parser_get_param(multipart_info);
    if (param != NULL) {
        param->has_tts = 1;
        //TVS_LOG_PRINTF("%s, has tts %d\n", __func__, param->has_tts);
        param->tts_bytes = 0;
    }
#if ARREARS_ENABLE
    if (audio_play_disable) {
        audio_play_disable = 0;
        return ;
    }
#endif
    tvs_state_set(TVS_STATE_SPEECH_PLAYING, param->control_type, 0);
    tvs_tts_player_open();
}

static void tvs_multi_on_recv_binary_chunked(void *multipart_info, bool down_channel, const char *bin, int bin_len)
{
    //TVS_LOG_PRINTF("%s %d %d\n", __func__, bin_len, down_channel);

    if (down_channel) {
        return;
    }


    tvs_multi_param *param = tvs_multipart_parser_get_param(multipart_info);
    if (param != NULL) {
        param->tts_bytes += bin_len;
    }

    tvs_speech_save_put_data((char *)bin, bin_len);

    tvs_tts_player_write((char *)bin, bin_len);
}

static void tvs_multi_on_recv_binary_end(void *multipart_info, bool down_channel)
{
    if (down_channel) {
        return;
    }

    TVS_LOG_PRINTF("%s\n", __func__);

    tvs_multi_param *param = tvs_multipart_parser_get_param(multipart_info);
    if (param != NULL && param->has_tts) {
        tvs_tts_player_no_more_data();
    }
}

static void tvs_multi_on_all_complete(void *multipart_info, bool down_channel)
{
    //TVS_LOG_PRINTF("%s\n", __func__);
}

static void tvs_multi_on_recv_metadata(void *multipart_info, bool down_channel, const char *name, const char *metadata, int metadata_len)
{
    tvs_multi_param *param = tvs_multipart_parser_get_param(multipart_info);
    tvs_directives_params *dir_param = NULL;
    if (param != NULL) {
        dir_param = &param->directives_param;
    }

    tvs_directives_parse_metadata(name, metadata, metadata_len, down_channel, dir_param);
}

void tvs_init_multipart_callback(tvs_multipart_callback *multipart_callback)
{
    memset(multipart_callback, 0, sizeof(tvs_multipart_callback));
    multipart_callback->on_all_complete = tvs_multi_on_all_complete;
    multipart_callback->on_recv_binary_begin = tvs_multi_on_recv_binary_begin;
    multipart_callback->on_recv_binary_chunked = tvs_multi_on_recv_binary_chunked;
    multipart_callback->on_recv_binary_end = tvs_multi_on_recv_binary_end;
    multipart_callback->on_recv_metadata = tvs_multi_on_recv_metadata;
}

void tvs_release_multipart_parser(void **multipart_info, bool down_channel)
{
    if (multipart_info == NULL) {
        return;
    }

    tvs_multi_param *param = tvs_multipart_parser_get_param(*multipart_info);
    if (param != NULL && param->has_tts) {
        tvs_tts_player_no_more_data();
    }

    if (multipart_info != NULL && *multipart_info != NULL) {
        tvs_multipart_parser_release(*multipart_info);
        *multipart_info = NULL;
    }
}

void tvs_multipart_process_chunked(void **multipart_info, tvs_multipart_callback *multipart_callback,
                                   tvs_multi_param *param, void *ev_data, bool jump, bool down_channel)
{
    struct http_message *hm = (struct http_message *)ev_data;
    struct mg_str *s = NULL;

    if (*multipart_info == NULL) {
        int buffer_len = 150;
        char *boundary = TVS_MALLOC(buffer_len);
        memset(boundary, 0, buffer_len);
        if ((s = mg_get_http_header(hm, "Content-Type")) != NULL &&
            s->len >= 9 && strncmp(s->p, "multipart", 9) == 0) {
            boundary[0] = boundary[1] = '-';
            int boundary_len = mg_http_parse_header(s, "boundary", boundary + 2, buffer_len - 2);
            if (boundary_len != 0) {
                //TVS_LOG_PRINTF("%s - %s\n", __func__, boundary);
                *multipart_info = tvs_multipart_parser_init(boundary, multipart_callback, param, down_channel);
            }
        }
        TVS_FREE(boundary);
    }

    if (*multipart_info != NULL && hm->body.len > 0) {
        int len = hm->body.len;
        char *body = (char *)hm->body.p;
        if (jump && len > 2) {
            for (int i = 0; i < len; i++) {
                if (body[i] == '-') {
                    body += i;
                    len -= i;
                    break;
                }
            }
        }
        tvs_multipart_parser_excute(*multipart_info, body, len);
    }

}

void tvs_multipart_handler_init()
{

}

