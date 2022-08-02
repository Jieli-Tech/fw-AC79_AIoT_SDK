#include "tvs_jsons.h"
#include "os_wrapper.h"

#include "tvs_executor_service.h"
#include "tvs_alert_interface.h"
#include "tvs_media_player_inner.h"
#include "tvs_system_interface.h"
#include "tvs_config.h"
#include "tvs_api_config.h"

#define TVS_LOG_DEBUG_MODULE  "TVS_JSON"
#include "tvs_log.h"

/******************  Context  *********************/
// 播放状态的上下文
#define TVS_CONTEXT_PLAYBACK_STATE   \
			"{" \
			    "\"header\": {" \
			        "\"namespace\": \"AudioPlayer\"," \
			        "\"name\": \"PlaybackState\"" \
			    "}," \
			    "\"payload\": {" \
			        "\"token\": \"\"," \
			        "\"offsetInMilliseconds\": 1000," \
			        "\"playerActivity\": \"PAUSED\"" \
			    "}" \
			"}"

// TTS状态的上下文
#define TVS_CONTEXT_SPEAK_STATE   \
			"{" \
				"\"header\": {" \
					"\"namespace\": \"SpeechSynthesizer\"," \
					"\"name\": \"SpeechState\"" \
				"}," \
				"\"payload\": {" \
					"\"token\": \"...\"," \
					"\"offsetInMilliseconds\": 1000," \
					"\"playerActivity\": \"FINISHED\"" \
				"}" \
			"}"

// ASR的上下文
#define TVS_CONTEXT_SPEAK_RECOGNIZE_STATE   \
			"{\"header\":{\"namespace\":\"SpeechRecognizer\",\"name\":\"RecognizerState\"},\"payload\":{\"wakeword\":\"\",\"isNoWakeEnabled\":false}}"

// 提醒/闹钟上下文
#define TVS_CONTEXT_ALART_STATE   \
			"{" \
				"\"header\": {" \
					"\"namespace\": \"Alerts\"," \
					"\"name\": \"AlertsState\"" \
				"}," \
				"\"payload\": {" \
					"\"allAlerts\": []," \
					"\"activeAlerts\": []" \
				"}" \
			"}"

// 音量上下文
#define TVS_CONTEXT_VULUME_STATE   \
			"{" \
				"\"header\": {" \
					"\"namespace\": \"Speaker\"," \
					"\"name\": \"VolumeState\"" \
				"}," \
				"\"payload\": {" \
					"\"volume\": 50," \
					"\"muted\": false" \
				"}" \
			"}"

// 情景模式上下文（比如儿童模式等）
#define TVS_CONTEXT_MODE   \
			"{" \
				"\"header\": {" \
					"\"namespace\": \"TvsProfileInformation\"," \
					"\"name\": \"ProfileState\"" \
				"}," \
				"\"payload\": {" \
					"\"mainMode\": \"\"" \
				"}" \
			"}"

// UI上下文，包括ASR结果等，附带这个上下文，才会有ASR结果返回
#define TVS_CONTEXT_UI  \
	"{\"header\":{\"namespace\":\"TvsUserInterface\",\"name\":\"ShowState\"},\"payload\":{\"isEnabled\":true,\"version\":\"v2\"}}"

/******************  PlaybackController  *********************/

#define TVS_PLAY_CONTROL_JSON_NAME_NEXT  "NextCommandIssued"
#define TVS_PLAY_CONTROL_JSON_NAME_PREV  "PreviousCommandIssued"

#define TVS_PLAY_CONTROL_JSON  \
		"{" \
			"\"context\": []," \
			"\"event\": {"  \
		        "\"header\": {" \
		            "\"namespace\": \"PlaybackController\"," \
		            "\"name\": \"NextCommandIssued\"," \
		            "\"messageId\": \"...\"" \
		        "}," \
		        "\"payload\": {}" \
		    "}" \
		"}"

/******************  SpeechRecognizer  *********************/
#define RECORD_FORMAT_SPEEX_16K     "AUDIO_SPEEX_L16_RATE_16000_CHANNELS_1"

#define RECORD_FORMAT_SPEEX_8K      "AUDIO_SPEEX_L16_RATE_8000_CHANNELS_1"

#define RECORD_FORMAT_PCM_16K       "AUDIO_L16_RATE_16000_CHANNELS_1"

#define RECORD_FORMAT_PCM_8K        "AUDIO_PCM_L16_RATE_8000_CHANNELS_1"

// 智能语音对话请求
#define TVS_RECOGNIZE_JSON  \
		"{" \
			"\"context\": []," \
			"\"event\": {" \
				"\"header\": {" \
					"\"namespace\": \"SpeechRecognizer\"," \
					"\"name\": \"Recognize\"," \
					"\"messageId\": \"...\"," \
					"\"dialogRequestId\": \"...\"" \
				"}," \
				"\"payload\": {" \
					"\"profile\": \"NEAR_FIELD\"," \
					"\"format\": \"\"," \
					"\"initiator\": {" \
						"\"type\": \"TAP\"," \
						"\"payload\": {" \
							"\"wakeWordIndices\": {" \
								"\"startIndexInSamples\": 0," \
								"\"endIndexInSamples\": 0" \
							"}," \
							"\"token\": \"\"" \
						"}" \
					"}" \
				"}" \
			"}" \
		"}"


// 明确语义请求
#define TVS_SEMANTIC_JSON  \
		"{" \
		    "\"context\": []," \
		    "\"event\": {" \
		        "\"header\": {" \
		            "\"namespace\": \"TvsSemanticRecognizer\"," \
		            "\"name\": \"Recognize\"," \
		            "\"messageId\": \"\"," \
		            "\"dialogRequestId\": \"\"" \
		        "}," \
		        "\"payload\": {" \
		            "\"semantic\": \"\"" \
		        "}" \
		    "}" \
		"} "

// 智能文本识别请求
#define TVS_TEXTRECOGNIZER_JSON  \
		"{" \
		    "\"context\": []," \
		    "\"event\": {" \
		        "\"header\": {" \
		            "\"namespace\": \"TvsTextRecognizer\"," \
		            "\"name\": \"Recognize\"," \
		            "\"messageId\": \"\"," \
		            "\"dialogRequestId\": \"\"" \
		        "}," \
		        "\"payload\": {" \
		            "\"text\": \"\"" \
		        "}" \
		    "}" \
		"} "


/******************  event upload  *********************/
#define TVS_SPEAKER_UPLOAD_VOLUME  "VolumeChanged"

// TTS状态上报
#define TVS_SPEECH_STATE_UPLOAD_JSON  \
	"{" \
		"\"context\": []," \
		"\"event\": {" \
			"\"header\": {" \
				"\"namespace\": \"SpeechSynthesizer\"," \
				"\"name\": \"SpeechStarted\"," \
				"\"messageId\": \"\"" \
			"}," \
			"\"payload\": {" \
				"\"token\": \"\"" \
			"}" \
		"}" \
	"}"

// 扬声器状态上报
#define TVS_SPEAKER_UPLOAD_JSON  \
		"{" \
			"\"context\": []," \
		    "\"event\": {" \
		        "\"header\": {" \
		            "\"namespace\": \"Speaker\"," \
		            "\"name\": \"\"," \
		            "\"messageId\": \"\"" \
		        "}," \
		        "\"payload\": {" \
		         	"\"volume\":50," \
		            "\"muted\": false" \
		        "}" \
		    "}" \
		"}"

#define TVS_AUDIOPLAYER_UPLOAD_FINISH  "PlaybackFinished"

// 流媒体播放状态上报
#define TVS_AUDIOPLAYER_UPLOAD_JSON  \
				"{" \
					"\"context\": []," \
					"\"event\": {" \
						"\"header\": {" \
							"\"namespace\": \"AudioPlayer\"," \
							"\"name\": \"\"," \
							"\"messageId\": \"\"" \
						"}," \
						"\"payload\": {" \
							"\"token\": \"\"," \
							"\"offsetInMilliseconds\": 1000" \
						"}" \
					"}" \
				"}"

// 情景模式状态上报
#define TVS_MODE_UPLOAD_JSON  \
				"{" \
					"\"context\": []," \
				    "\"event\": {" \
				        "\"header\": {" \
				            "\"namespace\": \"TvsModeControl\"," \
				            "\"name\": \"SwitchModeSucceeded\"," \
				            "\"messageId\": \"\"" \
				       "}," \
				        "\"payload\": {" \
				            "\"srcMode\": \"\"," \
				            "\"dstMode\": \"\"" \
				        "}" \
				    "}" \
				"}"

// 用于同步本地状态到云端
#define TVS_SYNC_STATE_UPLOAD_JSON  \
	"{" \
		"\"context\": []," \
		"\"event\": {" \
			"\"header\": {" \
				"\"namespace\": \"System\"," \
				"\"name\": \"SynchronizeState\"," \
				"\"messageId\": \"\"" \
			"}," \
			"\"payload\": {}" \
		"}" \
	"}"

// 闹钟/提醒状态上报
#define TVS_ALERT_UPLOAD_JSON  \
	"{\"context\": [],\"event\":{\"header\":{\"namespace\":\"Alerts\",\"name\":\"AlertStarted\",\"messageId\":\"\"},\"payload\":{\"token\":\"\"}}}"

// 设备端对PUSH的回应
#define TVS_PUSH_UPLOAD_JSON  \
	"{\"context\":[],\"event\":{\"header\":{\"namespace\":\"TvsPushInterface\",\"name\":\"TerminalSyncMessage\",\"messageId\":\"\"},\"payload\":{\"message\":\"\"}}}"

// 闹钟/提醒设置结果上报
#define TVS_ALERT_PROCESS_JSON  \
	"{\"context\":[],\"event\":{\"header\":{\"namespace\":\"Alerts\",\"name\":\"SetAlertSucceeded\",\"messageId\":\"\"},\"payload\":{\"token\":\"\"}}}"

// 设备端对PUSH的ACK
#define TVS_PUSH_ACK_JSON  \
	"{\"context\":[],\"event\":{\"header\":{\"namespace\":\"TvsPushInterface\",\"name\":\" Acknowledgement\",\"messageId\":\"\"},\"payload\":{\"tokens\":[]}}}"

// 文本转语音的请求
#define TVS_TTS_JSON \
	"{\"event\":{\"header\":{\"namespace\":\"TvsBasicAbility\",\"name\":\"TextToSpeech\",\"messageId\":\"\",\"dialogRequestId\":\"\"},\"payload\":{\"text\":\"\"}}}"

//异常上报
#define TVS_EXCEPTION_REPORT_JSON  \
				"{" \
					"\"context\": []," \
					"\"event\": {" \
						"\"header\": {" \
							"\"namespace\": \"System\"," \
							"\"name\": \"ExceptionEncountered\"," \
							"\"messageId\": \"\"" \
						"}," \
						"\"payload\": {" \
							"\"unparsedDirective\": \"ExceptionEncountered\"," \
							"\"error\": {" \
								"\"type\":\"0\"," \
								"\"message\":\"\"" \
							"}" \
						"}" \
					"}" \
				"}"


char *do_create_msg_id()
{
    char *msg_id = TVS_MALLOC(60);
    if (msg_id == NULL) {
        return NULL;
    }
    memset(msg_id, 0, 60);

    sprintf(msg_id, "FREERTOS_2019%u", (unsigned int)os_wrapper_get_time_ms());
    return msg_id;
}

static cJSON *get_TvsProfileInformation_context()
{
    cJSON *root = cJSON_Parse(TVS_CONTEXT_MODE);
    cJSON *payload = NULL;
    do {
        payload = cJSON_GetObjectItem(root, "payload");
        if (NULL == payload) {
            break;
        }
        const char *mode = tvs_system_get_current_mode();

        if (mode == NULL || strlen(mode) == 0) {
            mode = "NORMAL";
        }

        cJSON_ReplaceItemInObject(payload, "mainMode", cJSON_CreateString(mode));

    } while (0);

    return root;
}


static cJSON *get_Volume_context()
{
    cJSON *root = NULL;
    int volume = 50;
    cJSON *payload = NULL;
    int ret = -1;
    volume = tvs_media_player_get_volume();

    do {
        root = cJSON_Parse(TVS_CONTEXT_VULUME_STATE);
        if (NULL == root) {
            break;
        }

        payload = cJSON_GetObjectItem(root, "payload");
        if (NULL == payload) {
            break;
        }


        cJSON_ReplaceItemInObject(payload, "volume", cJSON_CreateNumber(volume));

        ret = 0;
    } while (0);

    if (ret != 0 && root != NULL) {
        cJSON_Delete(root);
        root = NULL;
    }
    return root;
}

static cJSON *get_UI_context()
{
    cJSON *root = NULL;

    int ret = -1;

    do {
        root = cJSON_Parse(TVS_CONTEXT_UI);
        if (NULL == root) {
            break;
        }

        ret = 0;
    } while (0);

    if (ret != 0 && root != NULL) {
        cJSON_Delete(root);
        root = NULL;
    }
    return root;
}

static cJSON *get_Recognizer_context(tvs_api_recognizer_type type, char *wakeword)
{
    if (type != TVS_RECOGNIZER_WAKEWORD) {
        return NULL;
    }

    if (wakeword == NULL || strlen(wakeword) == 0) {
        TVS_LOG_PRINTF("invalid wakeword!!!");
        return NULL;
    }

    cJSON *root = NULL;
    cJSON *payload = NULL;

    int ret = -1;

    do {
        root = cJSON_Parse(TVS_CONTEXT_SPEAK_RECOGNIZE_STATE);
        if (NULL == root) {
            break;
        }

        payload = cJSON_GetObjectItem(root, "payload");
        if (NULL == payload) {
            break;
        }

        cJSON_ReplaceItemInObject(payload, "wakeword", cJSON_CreateString(wakeword));

        ret = 0;
    } while (0);

    if (ret != 0 && root != NULL) {
        cJSON_Delete(root);
        root = NULL;
    }
    return root;
}

static cJSON *get_MediaPlayer_state_context()
{
    cJSON *root = NULL;
    cJSON *payload = NULL;
    int ret = -1;

    char *token_str = tvs_media_player_inner_get_token();

    if (token_str == NULL) {
        token_str = "";
    }

    const char *activity = tvs_media_player_inner_get_playback_state();

    long audio_play_offset_in_ms = tvs_media_player_inner_get_playback_offset();

    do {
        root = cJSON_Parse(TVS_CONTEXT_PLAYBACK_STATE);
        if (NULL == root) {
            break;
        }

        payload = cJSON_GetObjectItem(root, "payload");
        if (NULL == payload) {
            break;
        }


        cJSON_ReplaceItemInObject(payload, "token", cJSON_CreateString(token_str));
        cJSON_ReplaceItemInObject(payload, "playerActivity", cJSON_CreateString(activity));
        cJSON_ReplaceItemInObject(payload, "offsetInMilliseconds", cJSON_CreateNumber(audio_play_offset_in_ms));

        ret = 0;
    } while (0);

    if (ret != 0 && root != NULL) {
        cJSON_Delete(root);
        root = NULL;
    }

    return root;

}

static cJSON *get_Alert_context(bool need_alert)
{
    cJSON *root = NULL;
    int ret = -1;
    do {
        root = cJSON_Parse(TVS_CONTEXT_ALART_STATE);
        if (NULL == root) {
            break;
        }

        cJSON *payload = cJSON_GetObjectItem(root, "payload");
        if (NULL == payload) {
            break;
        }

        if (need_alert) {
            cJSON *alerts = tvs_alert_read_all();
            if (alerts != NULL) {
                cJSON_ReplaceItemInObject(payload, "allAlerts", alerts);

            }
        }
        ret = 0;
    } while (0);

    if (ret != 0 && root != NULL) {
        cJSON_Delete(root);
        root = NULL;
    }

    return root;
}

extern bool cJSON_Impl_HasObjectItem(const cJSON *object, const char *string);

static void append_context(cJSON *root, bool need_alert, tvs_api_recognizer_type type, char *wakeword)
{
    if (!cJSON_Impl_HasObjectItem(root, "context")) {
        return;
    }

    cJSON_DeleteItemFromObject(root, "context");
    cJSON *context = cJSON_CreateArray();
    if (context == NULL) {
        return;
    }

    cJSON *child = get_MediaPlayer_state_context();

    if (NULL != child) {
        cJSON_AddItemToArray(context, child);
    } else {
        TVS_LOG_PRINTF("add playback state failed\n");
    }

    child = get_Alert_context(need_alert);

    if (NULL != child) {
        cJSON_AddItemToArray(context, child);
    } else {
        TVS_LOG_PRINTF("add alert state failed\n");
    }

    child = get_Recognizer_context(type, wakeword);

    if (NULL != child) {
        cJSON_AddItemToArray(context, child);
    } else {
        TVS_LOG_PRINTF("no recognizer context!\n");
    }

    child = get_Volume_context();

    if (NULL != child) {
        cJSON_AddItemToArray(context, child);
    } else {
        TVS_LOG_PRINTF("add volume state failed\n");
    }

    child = get_TvsProfileInformation_context();

    if (NULL != child) {
        cJSON_AddItemToArray(context, child);
    } else {
        TVS_LOG_PRINTF("add tvs profile state failed\n");
    }

    if (tvs_config_will_print_asr_result()) {
        child = get_UI_context();
        if (NULL != child) {
            cJSON_AddItemToArray(context, child);
        } else {
            TVS_LOG_PRINTF("add tvs ui context failed\n");
        }
    }

    cJSON_AddItemToObject(root, "context", context);
}

void fill_request_header_id(cJSON *header, char *dialog_id)
{
    char *msg_id = NULL;

    if (dialog_id == NULL) {
        msg_id = do_create_msg_id();
    } else {
        msg_id = dialog_id;
    }

    cJSON_ReplaceItemInObject(header, "messageId", cJSON_CreateString(msg_id));
    if (cJSON_GetObjectItem(header, "dialogRequestId") != NULL) {
        cJSON_ReplaceItemInObject(header, "dialogRequestId", cJSON_CreateString(msg_id));
    }
    if (dialog_id == NULL) {
        TVS_FREE(msg_id);
    }
}

static cJSON *get_request_body(cJSON *root, bool need_alert, char *dialog_id, tvs_api_recognizer_type type, char *wakeword)
{
    cJSON *event = NULL;
    cJSON *header = NULL;

    int ret = -1;

    do {
        if (NULL == root) {
            break;
        }

        event = cJSON_GetObjectItem(root, "event");
        if (NULL == event) {
            break;
        }

        header = cJSON_GetObjectItem(event, "header");
        if (NULL == header) {
            break;
        }

        fill_request_header_id(header, dialog_id);

        append_context(root, need_alert, type, wakeword);

        ret = 0;
    } while (0);

    if (root != NULL && ret != 0) {
        cJSON_Delete(root);
        root = NULL;
    }

    return root;
}

cJSON *get_events_request_body(const char *json_template)
{
    cJSON *root = cJSON_Parse(json_template);
    if (NULL == root) {
        return NULL;
    }

    return get_request_body(root, false, NULL, TVS_RECOGNIZER_TAP, NULL);
}

cJSON *get_events_request_body_with_alert(const char *json_template, char *dialog_id, tvs_api_recognizer_type type, char *wakeword)
{
    cJSON *root = cJSON_Parse(json_template);
    if (NULL == root) {
        return NULL;
    }

    return get_request_body(root, true, dialog_id, type, wakeword);
}

static char *get_play_control_request_body(bool next)
{
    cJSON *root = get_events_request_body(TVS_PLAY_CONTROL_JSON);
    if (NULL == root) {
        return NULL;
    }

    if (!next) {
        do {
            cJSON *event = cJSON_GetObjectItem(root, "event");
            if (NULL == event) {
                break;
            }

            cJSON *header = cJSON_GetObjectItem(event, "header");
            if (NULL == header) {
                break;
            }

            cJSON_ReplaceItemInObject(header, "name", cJSON_CreateString(TVS_PLAY_CONTROL_JSON_NAME_PREV));
        } while (0);
    }

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return body;
}

static char *get_media_player_upload_inner(const char *name)
{
    cJSON *root = get_events_request_body(TVS_AUDIOPLAYER_UPLOAD_JSON);
    if (NULL == root) {
        return NULL;
    }


    do {
        const char *token = tvs_media_player_inner_get_token();
        if (NULL == token) {
            token = "";
        }

        cJSON *event = cJSON_GetObjectItem(root, "event");
        if (NULL == event) {
            break;
        }

        cJSON *header = cJSON_GetObjectItem(event, "header");
        if (NULL == header) {
            break;
        }

        cJSON *payload = cJSON_GetObjectItem(event, "payload");
        if (NULL == payload) {
            break;
        }

        cJSON_ReplaceItemInObject(header, "name", cJSON_CreateString(name));

        cJSON_ReplaceItemInObject(payload, "token", cJSON_CreateString(token));

#if ARREARS_ENABLE
        if (!arrears_type) {	//如果没欠费了 正常流程
            arrears_flag = 0;
        }

        cJSON_ReplaceItemInObject(payload, "offsetInMilliseconds", cJSON_CreateNumber(arrears_flag ? 0 : tvs_media_player_inner_get_offset()));
        arrears_flag = (arrears_flag > 0) ? --arrears_flag : 0;
#else
        cJSON_ReplaceItemInObject(payload, "offsetInMilliseconds", cJSON_CreateNumber(tvs_media_player_inner_get_offset()));
#endif
    } while (0);

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return body;
}

static char *get_semantic(const char *semantic)
{
    cJSON *root = get_events_request_body(TVS_SEMANTIC_JSON);
    if (NULL == root) {
        return NULL;
    }

    do {
        cJSON *event = cJSON_GetObjectItem(root, "event");
        if (NULL == event) {
            break;
        }

        cJSON *header = cJSON_GetObjectItem(event, "header");
        if (NULL == header) {
            break;
        }

        cJSON *payload = cJSON_GetObjectItem(event, "payload");
        if (NULL == payload) {
            break;
        }

        cJSON_ReplaceItemInObject(payload, "semantic", cJSON_CreateString(semantic == NULL ? "" : semantic));
    } while (0);

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return body;
}

static char *get_tts_request(tvs_api_tts_param *tts_param)
{
    if (tts_param == NULL) {
        return NULL;
    }

    cJSON *root = get_events_request_body(TVS_TTS_JSON);
    if (NULL == root) {
        return NULL;
    }

    do {
        cJSON *event = cJSON_GetObjectItem(root, "event");
        if (NULL == event) {
            break;
        }

        cJSON *header = cJSON_GetObjectItem(event, "header");
        if (NULL == header) {
            break;
        }

        cJSON *payload = cJSON_GetObjectItem(event, "payload");
        if (NULL == payload) {
            break;
        }

        cJSON_ReplaceItemInObject(payload, "text", cJSON_CreateString(tts_param->tts_text == NULL ? "" : tts_param->tts_text));

        if (!(tts_param->timbre == NULL || strlen(tts_param->timbre) == 0)) {
            cJSON_AddStringToObject(payload, "timbre", tts_param->timbre);
        }

        if (tts_param->volume >= 0 && tts_param->volume <= 100) {
            cJSON_AddNumberToObject(payload, "volume", tts_param->volume);
        }

        if (tts_param->speed >= 0 && tts_param->speed <= 100) {
            cJSON_AddNumberToObject(payload, "speed", tts_param->speed);
        }

        if (tts_param->pitch >= 0 && tts_param->pitch <= 100) {
            cJSON_AddNumberToObject(payload, "pitch", tts_param->pitch);
        }
    } while (0);

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return body;
}

char *get_alert_trigger_body(bool trigger, const char *token)
{
    cJSON *root = get_events_request_body(TVS_ALERT_UPLOAD_JSON);
    if (NULL == root) {
        return NULL;
    }

    do {
        cJSON *event = cJSON_GetObjectItem(root, "event");
        if (NULL == event) {
            break;
        }

        cJSON *header = cJSON_GetObjectItem(event, "header");
        if (NULL == header) {
            break;
        }

        cJSON *payload = cJSON_GetObjectItem(event, "payload");
        if (NULL == payload) {
            break;
        }
        if (!trigger) {
            cJSON_ReplaceItemInObject(header, "name", cJSON_CreateString("AlertStopped"));
        }

        cJSON_ReplaceItemInObject(payload, "token", cJSON_CreateString(token));
    } while (0);

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return body;
}


char *get_alart_process_body(bool new_alert, bool succeed, const char *token)
{
    cJSON *root = get_events_request_body(TVS_ALERT_PROCESS_JSON);
    if (NULL == root) {
        return NULL;
    }

    do {
        cJSON *event = cJSON_GetObjectItem(root, "event");
        if (NULL == event) {
            break;
        }

        cJSON *header = cJSON_GetObjectItem(event, "header");
        if (NULL == header) {
            break;
        }

        cJSON *payload = cJSON_GetObjectItem(event, "payload");
        if (NULL == payload) {
            break;
        }

        char *name = NULL;

        if (new_alert) {
            if (succeed) {
                name = "SetAlertSucceeded";
            } else {
                name = "SetAlertFailed";
            }
        } else {
            if (succeed) {
                name = "DeleteAlertSucceeded";
            } else {
                name = "DeleteAlertFailed";
            }
        }

        cJSON_ReplaceItemInObject(header, "name", cJSON_CreateString(name));

        cJSON_ReplaceItemInObject(payload, "token", cJSON_CreateString(token));

    } while (0);

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return body;
}

char *get_control_request_body(int type, void *pparam, void *param2)
{
    if (TVS_EXECUTOR_CMD_PLAY_PREV == type) {
        return get_play_control_request_body(false);
    } else if (TVS_EXECUTOR_CMD_PLAY_NEXT == type) {
        return get_play_control_request_body(true);
    } else if (TVS_EXECUTOR_CMD_PLAY_FINISH == type) {
        return get_media_player_upload_inner(TVS_AUDIOPLAYER_UPLOAD_FINISH);
    } else if (TVS_EXECUTOR_CMD_SEMANTIC == type) {
        return get_semantic((char *)pparam);
    } else if (TVS_EXECUTOR_CMD_TTS == type) {
        return get_tts_request((tvs_api_tts_param *)param2);
    } else {
        return NULL;
    }
}

static char *get_speaker_upload_body(const char *name, int volume)
{
    cJSON *root = get_events_request_body(TVS_SPEAKER_UPLOAD_JSON);
    if (NULL == root) {
        return NULL;
    }

    do {
        cJSON *event = cJSON_GetObjectItem(root, "event");
        if (NULL == event) {
            break;
        }

        cJSON *header = cJSON_GetObjectItem(event, "header");
        if (NULL == header) {
            break;
        }

        cJSON *payload = cJSON_GetObjectItem(event, "payload");
        if (NULL == payload) {
            break;
        }

        cJSON_ReplaceItemInObject(header, "name", cJSON_CreateString(name));

        cJSON_ReplaceItemInObject(payload, "volume", cJSON_CreateNumber((double)volume));
    } while (0);

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return body;
}

char *get_mode_changed_upload(const char *src, const char *dst)
{
    cJSON *root = get_events_request_body(TVS_MODE_UPLOAD_JSON);
    if (NULL == root) {
        return NULL;
    }

    do {
        cJSON *event = cJSON_GetObjectItem(root, "event");
        if (NULL == event) {
            break;
        }

        cJSON *header = cJSON_GetObjectItem(event, "header");
        if (NULL == header) {
            break;
        }

        cJSON *payload = cJSON_GetObjectItem(event, "payload");
        if (NULL == payload) {
            break;
        }

        cJSON_ReplaceItemInObject(payload, "srcMode", cJSON_CreateString(src));
        cJSON_ReplaceItemInObject(payload, "dstMode", cJSON_CreateString(dst));
    } while (0);

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return body;
}

char *get_speaker_volume_upload_body(int volume)
{
    return get_speaker_upload_body(TVS_SPEAKER_UPLOAD_VOLUME, volume);
}

char *get_speech_recognize_request_body(bool use_speex, bool use_8k, char *dialog_id,
                                        tvs_api_recognizer_type type, char *wakeword, int startMs, int endMs)
{

    char *type_str = "TAP";

    switch (type) {
    case TVS_RECOGNIZER_PRESS_AND_HOLD:
        type_str = "PRESS_AND_HOLD";
        break;
    case TVS_RECOGNIZER_WAKEWORD:
        type_str = "WAKEWORD";
        break;
    default:
        type = TVS_RECOGNIZER_TAP;
        break;
    }

    cJSON *root =  get_events_request_body_with_alert(TVS_RECOGNIZE_JSON, dialog_id, type, wakeword);
    if (NULL == root) {
        return NULL;
    }


    do {
        cJSON *event = cJSON_GetObjectItem(root, "event");
        if (NULL == event) {
            break;
        }


        cJSON *payload = cJSON_GetObjectItem(event, "payload");
        if (NULL == payload) {
            break;
        }

        if (!use_speex) {
            if (use_8k) {
                cJSON_ReplaceItemInObject(payload, "format", cJSON_CreateString(RECORD_FORMAT_PCM_8K));
            } else {
                cJSON_ReplaceItemInObject(payload, "format", cJSON_CreateString(RECORD_FORMAT_PCM_16K));
            }
        } else {
            if (use_8k) {
                cJSON_ReplaceItemInObject(payload, "format", cJSON_CreateString(RECORD_FORMAT_SPEEX_8K));
            } else {
                cJSON_ReplaceItemInObject(payload, "format", cJSON_CreateString(RECORD_FORMAT_SPEEX_16K));
            }
        }

        cJSON *initiator = cJSON_GetObjectItem(payload, "initiator");
        if (NULL == initiator) {
            break;
        }

        cJSON_ReplaceItemInObject(initiator, "type", cJSON_CreateString(type_str));

        if (type == TVS_RECOGNIZER_WAKEWORD) {

            cJSON *initiator_payload = cJSON_GetObjectItem(initiator, "payload");
            if (NULL == initiator_payload) {
                break;
            }

            cJSON *wakeWordIndices = cJSON_GetObjectItem(initiator_payload, "wakeWordIndices");
            if (NULL == wakeWordIndices) {
                break;
            }

            cJSON_ReplaceItemInObject(wakeWordIndices, "startIndexInSamples", cJSON_CreateNumber(startMs));
            cJSON_ReplaceItemInObject(wakeWordIndices, "endIndexInSamples", cJSON_CreateNumber(endMs));
        }

    } while (0);

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return body;

}

char *get_sync_state_upload_body()
{
    cJSON *root = get_events_request_body_with_alert(TVS_SYNC_STATE_UPLOAD_JSON, NULL, TVS_RECOGNIZER_TAP, NULL);
    if (NULL == root) {
        return NULL;
    }

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return body;
}

char *get_media_player_upload_body(int state)
{

    const char *name = get_player_upload_state(state);
    if (name == NULL || strlen(name) == 0) {
        return NULL;
    }

    return get_media_player_upload_inner(name);
}

char *get_push_upload_body(const char *push_text)
{
    cJSON *root = get_events_request_body(TVS_PUSH_UPLOAD_JSON);
    if (NULL == root) {
        return NULL;
    }

    do {
        cJSON *event = cJSON_GetObjectItem(root, "event");
        if (NULL == event) {
            break;
        }

        cJSON *header = cJSON_GetObjectItem(event, "header");
        if (NULL == header) {
            break;
        }

        cJSON *payload = cJSON_GetObjectItem(event, "payload");
        if (NULL == payload) {
            break;
        }

        cJSON_ReplaceItemInObject(payload, "message", cJSON_CreateString(push_text));
    } while (0);

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return body;
}

char *get_push_ack_body(const char *token)
{
    cJSON *root = get_events_request_body(TVS_PUSH_ACK_JSON);
    if (NULL == root) {
        return NULL;
    }

    do {
        cJSON *event = cJSON_GetObjectItem(root, "event");
        if (NULL == event) {
            break;
        }

        cJSON *header = cJSON_GetObjectItem(event, "header");
        if (NULL == header) {
            break;
        }

        cJSON *payload = cJSON_GetObjectItem(event, "payload");
        if (NULL == payload) {
            break;
        }

        cJSON *token_arr = cJSON_GetObjectItem(payload, "tokens");

        if (NULL == token_arr) {
            break;
        }

        cJSON_AddItemToArray(token_arr, cJSON_CreateString(token));

    } while (0);

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return body;
}

char *get_exception_report_body(int type, const char *message)
{
    cJSON *root = get_events_request_body(TVS_EXCEPTION_REPORT_JSON);
    if (NULL == root) {
        return NULL;
    }

    do {
        cJSON *event = cJSON_GetObjectItem(root, "event");
        if (NULL == event) {
            break;
        }

        cJSON *header = cJSON_GetObjectItem(event, "header");
        if (NULL == header) {
            break;
        }

        cJSON *payload = cJSON_GetObjectItem(event, "payload");
        if (NULL == payload) {
            break;
        }

        cJSON *error = cJSON_GetObjectItem(payload, "error");

        if (NULL == error) {
            break;
        }
        char tmp_buf[10] = {0};
        cJSON_ReplaceItemInObject(error, "type", cJSON_CreateString(itoa(type, tmp_buf, 10)));

        cJSON_ReplaceItemInObject(error, "message", cJSON_CreateString(message));

    } while (0);

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return body;
}



