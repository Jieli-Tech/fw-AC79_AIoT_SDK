#ifndef __TVS_SPEEX_API_H___
#define __TVS_SPEEX_API_H___

int tvs_speex_encode(char *pcm, int pcm_size, char *codec_buffer, int codec_buffer_size);

int tvs_speex_enc_open(int compress);

void tvs_speex_enc_close();

#endif
