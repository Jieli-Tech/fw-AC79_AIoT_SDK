
#ifndef __TVS_AUDIO_RECORDER_THREAD_H__
#define __TVS_AUDIO_RECORDER_THREAD_H__

void tvs_audio_recorder_thread_start(int time_out_ms, int session_id);

void tvs_audio_recorder_thread_stop();

void tvs_audio_recorder_thread_notify_reader_end(int session_id, int error);


#endif
