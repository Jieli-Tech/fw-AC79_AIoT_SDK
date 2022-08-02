#ifndef __TVS_DERIT_PROCESSOR_H_FEAWEFAWEFAF__
#define __TVS_DERIT_PROCESSOR_H_FEAWEFAWEFAF__

typedef enum {
    TVS_DIRECTIVES_TYPE_UNKNOWN,
    TVS_DIRECTIVES_TYPE_SPEAK,
    TVS_DIRECTIVES_TYPE_AUDIO_PLAY_CLEAR_QUEUE,
    TVS_DIRECTIVES_TYPE_REMOTE_AUDIO_PLAY,
    TVS_DIRECTIVES_TYPE_AUDIO_PLAY,
    TVS_DIRECTIVES_TYPE_AUDIO_PLAY_STOP,
    TVS_DIRECTIVES_TYPE_STOP_CAPTURE,
    TVS_DIRECTIVES_TYPE_SET_ALART,
    TVS_DIRECTIVES_TYPE_REMOVE_ALART,
    TVS_DIRECTIVES_TYPE_EXPECT_SPEECH,
    TVS_DIRECTIVES_TYPE_SET_VOLUME,
    TVS_DIRECTIVES_TYPE_ADJUST_VOLUME,
    TVS_DIRECTIVES_TYPE_SWITCH_MODE,
    TVS_DIRECTIVES_TYPE_SEMANTIC,
    TVS_DIRECTIVES_TYPE_TERMINALSYNC,
    TVS_DIRECTIVES_TYPE_TVS_CONTROL,
    TVS_DIRECTIVES_TYPE_UNBIND,
} tvs_directives_type;

typedef struct {
    tvs_directives_type type;
    char *play_url;
    char *token;
    char *dialog_id;
    int volume;
    char *src_mode;
    char *dst_mode;
    int play_offset;
    char *semantic;
    bool free_semantic;
    long long unbind_times;
} tvs_directives_infos;

typedef struct {
    int session_id;
    bool expect_speech;
    bool has_media;
} tvs_directives_params;


void on_recv_tvs_directives(const char *directives, bool down_channel, tvs_directives_params *params);

void tvs_directives_set_asr_callback(void *callback);

#endif
