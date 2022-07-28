#ifndef _DUI_API_H_
#define _DUI_API_H_

#include "generic/typedef.h"
#include "generic/list.h"

#define ONE_CACHE_BUF_LEN      1344

typedef struct  {
    struct list_head entry;
    u8 buf[1344];
    int len;
    u32 sessionid;
} DUI_AUDIO_INFO;

extern DUI_AUDIO_INFO *get_dui_audio_info(void);
extern void put_dui_audio_info(DUI_AUDIO_INFO *info);
extern void dui_media_speak_play(const char *url);
extern void dui_media_audio_play(const char *url);
extern void dui_media_audio_continue_play(const char *url);
extern void dui_media_audio_pause_play(const char *url);
extern void dui_media_audio_stop_play(const char *url);
extern void dui_volume_change_notify(int volume);
extern void dui_media_audio_resume_play(void);
extern void dui_media_audio_prev_play(void);
extern void dui_media_audio_next_play(void);
extern void dui_event_notify(int event, void *arg);
extern int dui_media_audio_play_seek(u32 time_elapse);
extern int dui_recorder_start(u16 sample_rate, u8 voice_mode, u8 use_vad);
extern int dui_recorder_stop(u8 voice_mode);

#endif
