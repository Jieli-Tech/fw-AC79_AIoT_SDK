
#ifndef __TVS_AUDIO_RECORDER_H__
#define __TVS_AUDIO_RECORDER_H__

int tvs_audio_recorder_open(int bitrate, int channels, int period_size, int period_count, int session_id);

void tvs_audio_recorder_close(int error, int session_id);

int tvs_audio_recorder_read(char *recorder_buffer, int buffer_size, int session_id);

int tvs_audio_recorder_init();

#endif
