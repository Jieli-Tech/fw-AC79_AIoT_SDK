#ifndef __TVS_JSONS_H_20190311__
#define __TVS_JSONS_H_20190311__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "cJSON_common/cJSON.h"
#include "stdbool.h"
#include "tvs_common_def.h"

char *get_speech_recognize_request_body(bool use_speex, bool use_8k, char *dialog_id,
                                        tvs_api_recognizer_type type, char *wakeword, int startMs, int endMs);

char *get_speaker_volume_upload_body(int volume);

char *do_create_msg_id();

char *get_control_request_body(int type, void *pparam, void *param2);

char *get_mode_changed_upload(const char *src, const char *dst);

char *get_sync_state_upload_body();

char *get_media_player_upload_body(int state);

char *get_push_ack_body(const char *token);

char *get_push_upload_body(const char *push_text);

char *get_alart_process_body(bool new_alert, bool succeed, const char *token);

char *get_alert_trigger_body(bool trigger, const char *token);

char *get_exception_report_body(int type, const char *message);


#endif

