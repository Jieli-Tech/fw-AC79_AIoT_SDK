/**
* @file  tvs_directives_processor.c
* @brief 解析后台下发的各种指令
* @date  2019-5-10
*
*/

#define TVS_LOG_DEBUG   1
#include "time.h"
#include "tvs_log.h"
#include "tvs_jsons.h"
#include "tvs_directives_processor.h"
#include "tvs_alert_interface.h"
#include "tvs_executor_service.h"
#include "tvs_common_def.h"

// 监听ASR结果
static tvs_callback_on_asr_result g_tvs_callback_on_asr_result = NULL;

static void on_recv_directives_SpeechSynthesizer(char *name, cJSON *payload, tvs_directives_infos *infos)
{
    if (name != NULL && 0 == strcasecmp(name, "Speak")) {
        infos->type = TVS_DIRECTIVES_TYPE_SPEAK;
        cJSON *token = cJSON_GetObjectItem(payload, "token");
        if (NULL != token && token->valuestring != NULL && strlen(token->valuestring) != 0) {
            infos->token = token->valuestring;
            //TVS_LOG_PRINTF("speak token %s\n", infos->token);
        }
    }

}

static void on_recv_directives_SpeechRecognizer(char *name, cJSON *payload, tvs_directives_infos *infos, cJSON *header)
{
    if (name != NULL && 0 == strcasecmp(name, "StopCapture")) {
        infos->type = TVS_DIRECTIVES_TYPE_STOP_CAPTURE;
        cJSON *dialog_id = cJSON_GetObjectItem(header, "dialogRequestId");

        if (NULL != dialog_id && dialog_id->valuestring != NULL && strlen(dialog_id->valuestring) != 0) {
            infos->dialog_id = (dialog_id->valuestring);
            TVS_LOG_PRINTF("StopCapture dialog %s\n", infos->dialog_id);
        }
    } else if (name != NULL && 0 == strcasecmp(name, "ExpectSpeech")) {
        infos->type = TVS_DIRECTIVES_TYPE_EXPECT_SPEECH;
    }
}

static void on_recv_directives_TvsModeControl(char *name, cJSON *payload, tvs_directives_infos *infos)
{
    if (name != NULL && 0 == strcasecmp(name, "SwitchMode")) {
        infos->type = TVS_DIRECTIVES_TYPE_SWITCH_MODE;

        cJSON *srcMode = cJSON_GetObjectItem(payload, "srcMode");

        if (NULL != srcMode && srcMode->valuestring != NULL && strlen(srcMode->valuestring) != 0) {
            infos->src_mode = srcMode->valuestring;
            TVS_LOG_PRINTF("tvs srcMode %s\n", infos->src_mode);
        }

        cJSON *dstMode = cJSON_GetObjectItem(payload, "dstMode");
        if (NULL != dstMode && dstMode->valuestring != NULL && strlen(dstMode->valuestring) != 0) {
            infos->dst_mode = dstMode->valuestring;
            TVS_LOG_PRINTF("tvs dstMode %s\n", infos->dst_mode);
        }
    }
}

static void on_recv_directives_Alerts(char *name, cJSON *payload, tvs_directives_infos *infos)
{
    int ret = 0;
    cJSON *token = cJSON_GetObjectItem(payload, "token");
    if (token == NULL || token->valuestring == NULL) {
        return;
    }

    if (name != NULL && 0 == strcasecmp(name, "SetAlert")) {
        ret = tvs_alert_new(payload);
        TVS_LOG_PRINTF("new alert %s, ret %d\n", token->valuestring, ret);
        tvs_executor_upload_alert_new(ret == 0, token->valuestring);

    } else if (name != NULL && 0 == strcasecmp(name, "DeleteAlert")) {

        ret = tvs_alert_delete(token->valuestring);
        TVS_LOG_PRINTF("delete alert %s, ret %d\n", token->valuestring, ret);

        tvs_executor_upload_alert_delete(ret == 0, token->valuestring);
    } else {
        return;
    }
}

static void on_recv_directives_AudioPlayer(char *name, cJSON *payload, tvs_directives_infos *infos)
{
    if (name == NULL) {
        return;
    }
    if (0 == strcasecmp(name, "Play")) {
        infos->type = TVS_DIRECTIVES_TYPE_AUDIO_PLAY;
        cJSON *audioItem = cJSON_GetObjectItem(payload, "audioItem");
        if (audioItem != NULL) {
            cJSON *stream = cJSON_GetObjectItem(audioItem, "stream");
            if (NULL != stream) {
                cJSON *url = cJSON_GetObjectItem(stream, "url");
                if (NULL != url && url->valuestring != NULL && strlen(url->valuestring) != 0) {
                    infos->play_url = (url->valuestring);
                    TVS_LOG_PRINTF("play url %s, length %d\n", infos->play_url, (int)strlen(infos->play_url));
                }

                cJSON *token = cJSON_GetObjectItem(stream, "token");
                if (NULL != token && token->valuestring != NULL && strlen(token->valuestring) != 0) {
                    infos->token = (token->valuestring);
                    TVS_LOG_PRINTF("play token %s, length %d\n", infos->token, (int)strlen(infos->token));
                }

                cJSON *offsetInMilliseconds = cJSON_GetObjectItem(stream, "offsetInMilliseconds");
                if (NULL != offsetInMilliseconds) {
                    /* infos->play_offset = (offsetInMilliseconds->valueint / 1000); */
                    infos->play_offset = offsetInMilliseconds->valueint;
                    TVS_LOG_PRINTF("play offset %d\n", infos->play_offset);
                }
            }
        }
    } else if (0 == strcasecmp(name, "ClearQueue")) {
        infos->type = TVS_DIRECTIVES_TYPE_AUDIO_PLAY_CLEAR_QUEUE;
    } else if (0 == strcasecmp(name, "Stop")) {
        infos->type = TVS_DIRECTIVES_TYPE_AUDIO_PLAY_STOP;
    }
}

static void on_recv_directives_Speaker(char *name, cJSON *payload, tvs_directives_infos *infos)
{

    if (name == NULL) {
        return;
    }
    if (0 == strcasecmp(name, "SetVolume")) {
        infos->type = TVS_DIRECTIVES_TYPE_SET_VOLUME;

        cJSON *volume = cJSON_GetObjectItem(payload, "volume");
        if (NULL != volume) {
            infos->volume = volume->valueint;
            TVS_LOG_PRINTF("set volume %d\n", infos->volume);
        }
    } else if (0 == strcasecmp(name, "AdjustVolume")) {
        infos->type = TVS_DIRECTIVES_TYPE_ADJUST_VOLUME;

        cJSON *volume = cJSON_GetObjectItem(payload, "volume");
        if (NULL != volume) {
            infos->volume = volume->valueint;
            TVS_LOG_PRINTF("adjust volume %d\n", infos->volume);
        }
    }
}

static void do_process_push(const char *type, const char *text, const char *token, tvs_directives_infos *infos)
{
    cJSON *root = cJSON_Parse(text);

    cJSON *child = NULL;
    cJSON *semantic = NULL;

    if (strcmp(type, "moshiqiehuankongzhi-1111182340950216704") == 0) {
        child = cJSON_GetObjectItem(root, "childControlInfo");

        semantic = cJSON_GetObjectItem(child, "semantic");

        if (semantic != NULL) {
            infos->semantic = cJSON_PrintUnformatted(semantic);
            infos->free_semantic = true;
            infos->type = TVS_DIRECTIVES_TYPE_SEMANTIC;
        }
    } else if (strcmp(type, "tvs_common_terminalsync") == 0) {
        infos->type = TVS_DIRECTIVES_TYPE_TERMINALSYNC;
        infos->semantic = (char *)text;
        infos->token = (char *)(token);
    } else if (strcmp(type, "{\"domain\":\"device_domain\",\"intent\":\"device_unbind\"}") == 0) {
        //cJSON不支持long long,直接转换
        char *ptime = strstr(text, "unbind_time");
        if (ptime != NULL) {
            infos->type = TVS_DIRECTIVES_TYPE_UNBIND;
            infos->unbind_times = atoll(ptime + 13);
            time_t seconds = infos->unbind_times / 1000;
//			TVS_LOG_PRINTF("Push unbind times:%s\n",asctime(localtime( &seconds)));
        } else {
            TVS_LOG_PRINTF("Push unbind error!!!\n");
        }
    }

    cJSON_Delete(root);
}

void tvs_directives_set_asr_callback(void *callback)
{
    g_tvs_callback_on_asr_result = callback;
}

static void on_recv_directives_TvsUserInterface(char *name, cJSON *payload, tvs_directives_infos *infos)
{
    if (name == NULL) {
        return;
    }
    if (0 == strcasecmp(name, "ASRShow")) {
        cJSON *data = cJSON_GetObjectItem(payload, "data");
        if (data != NULL) {
            cJSON *isFinal = cJSON_GetObjectItem(data, "isFinal");
            cJSON *text = cJSON_GetObjectItem(data, "text");

            if (isFinal != NULL && text != NULL) {
                char *asr_res = text->valuestring == NULL ? "" : text->valuestring;

                if (g_tvs_callback_on_asr_result != NULL) {
                    g_tvs_callback_on_asr_result(asr_res, isFinal->valueint != 0);
                }

                if (isFinal->valueint) {
                    TVS_LOG_PRINTF("get ASR Result text %s, isFinal %d\n", asr_res, isFinal->valueint);
                }
            }
        }
    } else if (0 == strcasecmp(name, "Show")) {
        cJSON *errMsg = cJSON_GetObjectItem(payload, "errMsg");
        if (errMsg != NULL && errMsg->valuestring != NULL && strlen(errMsg->valuestring) > 0) {
            TVS_LOG_PRINTF("get TvsUserInterface error msg %s\n", errMsg->valuestring);
        }
    }
}

static void on_recv_directives_DeviceControl(char *name, cJSON *payload, tvs_directives_infos *infos)
{
    if (name == NULL) {
        return;
    }
    if (0 == strcasecmp(name, "Control")) {
        infos->semantic = cJSON_PrintUnformatted(payload);
        infos->free_semantic = true;
        infos->type = TVS_DIRECTIVES_TYPE_TVS_CONTROL;
    }
}


static void on_recv_directives_TvsPushInterface(char *name, cJSON *payload, tvs_directives_infos *infos)
{
    if (name == NULL) {
        return;
    }

    if (0 != strcasecmp(name, "TransparentMessage")) {
        return;
    }

    cJSON *messages = cJSON_GetObjectItem(payload, "messages");

    if (messages != NULL && messages->type == cJSON_Array && cJSON_GetArraySize(messages) > 0) {
        cJSON *child = cJSON_GetArrayItem(messages, 0);

        if (child != NULL) {
            cJSON *text = cJSON_GetObjectItem(child, "text");
            char *text_str = text == NULL ? "" : text->valuestring == NULL ? "" : text->valuestring;
            TVS_LOG_PRINTF("get push text %s\n", text_str);

            cJSON *type = cJSON_GetObjectItem(child, "type");
            char *type_str = type->valuestring == NULL ? "" : type->valuestring;

            cJSON *token = cJSON_GetObjectItem(child, "token");
            char *token_str = token->valuestring == NULL ? "" : token->valuestring;

            do_process_push(type_str, text_str, token_str, infos);

            tvs_executor_send_push_ack(token_str);
        }
    }


}

extern void tvs_directives_handle(tvs_directives_infos *directives, bool down_channel, tvs_directives_params *params);

// 处理event通道/下行通道下发的json指令
void on_recv_tvs_directives(const char *json, bool down_channel, tvs_directives_params *params)
{
    cJSON *root = NULL;

    tvs_directives_infos *infos = NULL;

    bool do_print = true;

    do {
        root = cJSON_Parse(json);
        if (NULL == root) {
            TVS_LOG_PRINTF("parse root failed\n");
            break;
        }

        cJSON *directive = cJSON_GetObjectItem(root, "directive");

        if (NULL == directive) {
            TVS_LOG_PRINTF("parse directive failed\n");
            break;
        }

        cJSON *header = cJSON_GetObjectItem(directive, "header");

        if (NULL == header) {
            TVS_LOG_PRINTF("parse header failed\n");
            break;
        }

        cJSON *name = cJSON_GetObjectItem(header, "name");

        if (NULL == name) {
            TVS_LOG_PRINTF("parse name failed\n");
            break;
        }

        cJSON *namespace = cJSON_GetObjectItem(header, "namespace");

        if (NULL == namespace) {
            TVS_LOG_PRINTF("parse namespace failed\n");
            break;
        }

        infos = TVS_MALLOC(sizeof(tvs_directives_infos));
        if (NULL == infos) {
            TVS_LOG_PRINTF("create tvs_directives_infos OOM\n");
            break;
        }
        memset(infos, 0, sizeof(tvs_directives_infos));

        cJSON *payload = cJSON_GetObjectItem(directive, "payload");

        if (NULL != payload) {
            if (namespace->valuestring != NULL) {
                if (0 == strcasecmp(namespace->valuestring, "AudioPlayer")) {
                    on_recv_directives_AudioPlayer(name->valuestring, payload, infos);
                } else if (0 == strcasecmp(namespace->valuestring, "SpeechSynthesizer")) {
                    on_recv_directives_SpeechSynthesizer(name->valuestring, payload, infos);
                } else if (0 == strcasecmp(namespace->valuestring, "SpeechRecognizer")) {
                    on_recv_directives_SpeechRecognizer(name->valuestring, payload, infos, header);
                } else if (0 == strcasecmp(namespace->valuestring, "Alerts")) {
                    on_recv_directives_Alerts(name->valuestring, payload, infos);
                } else if (0 == strcasecmp(namespace->valuestring, "Speaker")) {
                    on_recv_directives_Speaker(name->valuestring, payload, infos);
                } else if (0 == strcasecmp(namespace->valuestring, "TvsModeControl")) {
                    on_recv_directives_TvsModeControl(name->valuestring, payload, infos);
                } else if (0 == strcasecmp(namespace->valuestring, "TvsPushInterface")) {
                    on_recv_directives_TvsPushInterface(name->valuestring, payload, infos);
                } else if (0 == strcasecmp(namespace->valuestring, "TvsUserInterface")) {
                    on_recv_directives_TvsUserInterface(name->valuestring, payload, infos);
                    do_print = false;
                } else if (0 == strcasecmp(namespace->valuestring, "DeviceControl")) {
                    on_recv_directives_DeviceControl(name->valuestring, payload, infos);
                }
            }
        } else {
            TVS_LOG_PRINTF("no payload\n");
        }

        if (do_print) {
            TVS_LOG_PRINTF("directives from %s : %s\n", down_channel ? "down_channel" : "event_channel", json == NULL ? "" : json);
        }

    } while (0);


    tvs_directives_handle(infos, down_channel, params);

    if (infos != NULL && infos->semantic != NULL && infos->free_semantic) {
        TVS_FREE(infos->semantic);
        infos->semantic = NULL;
    }

    if (infos != NULL) {
        TVS_FREE(infos);
    }

    if (NULL != root) {
        cJSON_Delete(root);
        root = NULL;
    }
}

