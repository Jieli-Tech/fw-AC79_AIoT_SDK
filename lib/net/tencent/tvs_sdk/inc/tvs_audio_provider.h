
#ifndef __TVS_AUDIO_PROVIDER_H__
#define __TVS_AUDIO_PROVIDER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "tvs_common_def.h"


typedef enum {
    TVS_AUDIO_PROVIDER_ERROR_NONE = TVS_API_AUDIO_PROVIDER_ERROR_NONE,
    TVS_AUDIO_PROVIDER_ERROR_OTHERS = TVS_API_AUDIO_PROVIDER_ERROR_OTHERS,
    TVS_AUDIO_PROVIDER_ERROR_STOP_CAPTURE = TVS_API_AUDIO_PROVIDER_ERROR_STOP_CAPTURE,
    TVS_AUDIO_PROVIDER_ERROR_TIME_OUT = TVS_API_AUDIO_PROVIDER_ERROR_TIME_OUT,
    TVS_AUDIO_PROVIDER_ERROR_NETWORK = TVS_API_AUDIO_PROVIDER_ERROR_NETWORK,
    TVS_AUDIO_PROVIDER_ERROR_READ_TOO_FAST = -1000,
} tvs_audio_provider_error;

void tvs_audio_provider_set_callback(tvs_api_callback_on_provider_reader_stop cb);

int tvs_audio_provider_writer_begin();

void tvs_audio_provider_writer_end();

int tvs_audio_provider_write(char *data, int data_size);


#ifdef __cplusplus
}
#endif

#endif
