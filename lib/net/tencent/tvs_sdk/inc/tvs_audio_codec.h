#ifndef __TVS_AUDIO_CODEC_H__
#define __TVS_AUDIO_CODEC_H__

typedef int (*tvs_audio_codec_open)(int compress);

typedef void (*tvs_audio_codec_close)();

typedef int (*tvs_audio_codec_encode)(char *pcm_data, int size, char *codec_buffer, int buffe_size);

typedef struct {
    tvs_audio_codec_open do_open;
    tvs_audio_codec_close do_close;
    tvs_audio_codec_encode do_encode;
    int type;
    int src_buffer_size;
    int codec_buffer_size;
    int compress;
} tvs_audio_codec;

#endif
