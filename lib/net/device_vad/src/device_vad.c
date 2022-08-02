#include <string.h>
#include "fvad.h"
#include "printf.h"

// default , do not modify
#define VAD_FRAME_MS (80) // 80ms
#define VAD_INPUT_MS (20) // 20ms
#define VAD_CACHE_SIZE (VAD_FRAME_MS/VAD_INPUT_MS)
#define VAD_VOICE_NUM_IN_FRAME (2) // if a frame which is  80ms len,  have >= 20ms voice, detect speaking
// can change
#define VAD_IGNORE_HOTWORD_TIME (2 * VAD_FRAME_MS) // 2*80 ms
#define VAD_SPEAKING_SILENCE_VALID_TIME  (6 * VAD_FRAME_MS) // 6*80 ms
#define VAD_VOICE_VALID_TIME (8 * VAD_FRAME_MS) // 5*80 ms

/** add by tcq*/
typedef unsigned char u8;
typedef unsigned short u16;

typedef enum _duer_vad_status_enum {
    VAD_WAKEUP,
    VAD_SPEAKING,
    VAD_PLAYING,
} duer_vad_status_enum_t;

typedef struct {
    Fvad *vad;
    int count;
    int voice_cache[VAD_CACHE_SIZE];
    // for VAD_WAKEUP status
    int vad_wakeup_total_len;// wakeup len
    int vad_wakeup_voice_len;// wakeup voice len
    // for VAD_SPEAKING status
    int vad_speaking_silence_len;// speaking silence len

    u8 vad_status;
    u8 vad_status_before;

    // human finish speak
    u8 vad_stop_speak;
    int vad_sample_rate;

    u16 vad_speaking_silence_valid_time;
    u16 vad_voice_valid_time;
} vad_t;

static int init_fvad(Fvad *vad, int sample_rate)
{
    if (vad == NULL) {
        printf("vad is NULL\n");
        return -1;
    }

    if (fvad_set_mode(vad, 3) < 0) {
        printf(" vad set mode fail\n");
        fvad_free(vad);
        return -1;
    }

    if (fvad_set_sample_rate(vad, sample_rate) < 0) {
        printf("vad set sample rate fail\n");
        fvad_free(vad);
        return -1;
    }

    return 0;
}

static const char *const g_vad_status_string[] = {
    "wakeup",
    "speaking",
    "playing",
};

static void vad_set_status(vad_t *vad, int status)
{
    //printf("%s -> %s\n",g_vad_status_string[vad_status_before],g_vad_status_string[vad_status]);

    if (VAD_WAKEUP == status) {
        vad->vad_wakeup_total_len = 0;
        vad->vad_wakeup_voice_len = 0;
        vad->vad_stop_speak = 0;
        vad->count = 0;
    } else if (VAD_SPEAKING == status) {
        vad->vad_speaking_silence_len = 0;
    }

    vad->vad_status_before = vad->vad_status;
    vad->vad_status = status;

    // human start speak
    if ((VAD_WAKEUP == vad->vad_status_before) && (VAD_SPEAKING == vad->vad_status)) {
        printf("[VAD] start speak\n");
    }

    // human stop speak
    if ((VAD_SPEAKING == vad->vad_status_before) && (VAD_PLAYING == vad->vad_status)) {
        printf("[VAD] stop speak\n");
        vad->vad_stop_speak = 1;
    }

    // cloud vad work
    if ((VAD_WAKEUP == vad->vad_status_before) && (VAD_PLAYING == vad->vad_status)) {
        printf("[VAD] cloud vad work\n");
    }
}

void vad_set_playing(void *vad)
{
    vad_set_status(vad, VAD_PLAYING);
}

// check voice per 20 ms
static void vad_check_voice(vad_t *vad, int voice)
{
    if (VAD_PLAYING == vad->vad_status) {
        return;
    }

    int total = 0;
    int voice_time = VAD_INPUT_MS;// 20ms
    int i = 0;

    // update vad cache
    for (i = (VAD_CACHE_SIZE - 1); i > 0; --i) {
        vad->voice_cache[i] = vad->voice_cache[i - 1];
    }

    vad->voice_cache[0] = voice;

    // check voice per 80 ms
    vad->count += voice_time;

    if (vad->count >= VAD_FRAME_MS) {
        for (i = 0; i < VAD_CACHE_SIZE ; ++i) {
            //printf("[%d] ",voice_cache[i]);
            total += vad->voice_cache[i];
        }

        //printf(" Frame %d\n", total);
        vad->count = 0;

        //vad status change:
        if (VAD_WAKEUP == vad->vad_status) {
            //printf("wakeup %d\t%d,%d,%d,%d\n",total,voice_cache[0],voice_cache[1],voice_cache[2],voice_cache[3]);
            vad->vad_wakeup_total_len += VAD_FRAME_MS;

            if (vad->vad_wakeup_total_len > VAD_IGNORE_HOTWORD_TIME) {
                if (total >= VAD_VOICE_NUM_IN_FRAME) {
                    vad->vad_wakeup_voice_len += VAD_FRAME_MS;
                    //printf("* %d ",total);
                } else {
                    vad->vad_wakeup_voice_len = 0;
                    //printf("_ %d\n",total);
                }

                if (vad->vad_wakeup_voice_len >= vad->vad_voice_valid_time) {
                    //printf("\nwake up -> speaking\n");
                    vad_set_status(vad, VAD_SPEAKING);
                }
            }
        } else if (VAD_SPEAKING == vad->vad_status) {
            if (total >= VAD_VOICE_NUM_IN_FRAME) {
                vad->vad_speaking_silence_len = 0;

                // check if have some silence in a speaking frame
                for (i = 0; i < VAD_CACHE_SIZE; ++i) {
                    if (vad->voice_cache[i] > 0) {
                        break;
                    }
                }

                int some_silence_time = i * VAD_INPUT_MS;

                if (some_silence_time > 0) {
                    vad->count += some_silence_time;
                    //printf("some_silence_time %d i:%d (%d,%d,%d,%d)\n", some_silence_time, i,
                    //     voice_cache[0], voice_cache[1], voice_cache[2], voice_cache[3]);
                }
            } else {
                vad->vad_speaking_silence_len += VAD_FRAME_MS;
            }

            if (vad->vad_speaking_silence_len >= vad->vad_speaking_silence_valid_time) {
                //printf("%d speaking -> stop\n", vad_speaking_silence_len);
                vad_set_status(vad, VAD_PLAYING);
            }
        }
    }
}

int vad_main(void *p, char *buff, int length)
{
    vad_t *vad = (vad_t *)p;

    // check 100ms voice
    if ((length % (vad->vad_sample_rate / 100 * 4)) != 0) {
        printf("vad input length error : %d\n", length);
        return vad->vad_status;
    }

    // init vad cache

    int voice = 0;
    int total = 0;
    int ret = 0;

    for (int i = 0; i < length; i += (vad->vad_sample_rate / 100 * 2)) {
        voice += fvad_process(vad->vad, (int16_t *)buff + i / 2, vad->vad_sample_rate / 100);
        total ++;

        if (2 == total) {
            // update vad cache
            ret += voice;
            vad_check_voice(vad, voice);
            // reset
            voice = 0;
            total = 0;
        }
    }

    /* return ret; */
    return vad->vad_status;
}

void *vad_init(int sample_rate, int voice_valid_time, int speaking_silence_valid_time)
{
    vad_t *vad = calloc(1, sizeof(vad_t));
    if (vad == NULL) {
        return NULL;
    }

    vad->vad = fvad_new();
    if (vad->vad == NULL) {
        free(vad);
        return NULL;
    }

    if (0 == speaking_silence_valid_time) {
        speaking_silence_valid_time = VAD_SPEAKING_SILENCE_VALID_TIME;
    }
    if (0 == voice_valid_time) {
        voice_valid_time = VAD_VOICE_VALID_TIME;
    }

    vad->vad_sample_rate = sample_rate;
    vad->vad_speaking_silence_valid_time = speaking_silence_valid_time;
    vad->vad_voice_valid_time = voice_valid_time;

    if (init_fvad(vad->vad, sample_rate) < 0) {
        printf("init vad fail\n");
        free(vad);
        return NULL;
    }

    vad_set_status(vad, VAD_WAKEUP);

    return vad;
}

void vad_free(void *p)
{
    vad_t *vad = (vad_t *)p;
    fvad_free(vad->vad);
    free(vad);
}

void vad_reset(void *p)
{
    vad_t *vad = (vad_t *)p;
    memset(vad->voice_cache, 0, sizeof(vad->voice_cache));
    vad_set_status(vad, VAD_WAKEUP);
}
