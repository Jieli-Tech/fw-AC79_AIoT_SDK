#ifndef AUDIO_EFFECT_H
#define AUDIO_EFFECT_H

#include "spectrum/spectrum_fft.h"
#include "digital_vol.h"


union audio_effect_req {
    spectrum_fft_open_parm spectrum_fft;
    digital_vol_open_parm  digital_vol;
};


#endif
