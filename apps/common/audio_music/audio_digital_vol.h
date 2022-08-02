#ifndef _AUDIO_DIGITAL_VOL_H_
#define _AUDIO_DIGITAL_VOL_H_

#include "generic/typedef.h"
#include "os/os_api.h"

/*************************自定义支持重入的数字音量调节****************************/
void *user_audio_digital_volume_open(u8 vol, u8 vol_max, u16 fade_step);
int user_audio_digital_volume_close(void *_d_volume);
u8 user_audio_digital_volume_get(void *_d_volume);
int user_audio_digital_volume_set(void *_d_volume, u8 vol);
int user_audio_digital_volume_reset_fade(void *_d_volume);
int user_audio_digital_volume_run(void *_d_volume, void *data, u32 len, u32 sample_rate, u8 ch_num);
void user_audio_digital_set_volume_tab(void *_d_volume, u16 *user_vol_tab, u8 user_vol_max);

#endif

