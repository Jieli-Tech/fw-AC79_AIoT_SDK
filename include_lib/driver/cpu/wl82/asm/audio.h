#ifndef CPU_AUDIO_H
#define CPU_AUDIO_H

#include "typedef.h"
#include "asm/ladc.h"
#include "asm/dac.h"
#include "asm/iis.h"
#include "asm/plnk.h"
#include "asm/src.h"
// #include "asm/eq.h"
#include "asm/spdif.h"

struct audio_pf_data {
    const struct adc_platform_data *adc_pf_data;
    const struct dac_platform_data *dac_pf_data;
    const struct iis_platform_data *iis0_pf_data;
    const struct iis_platform_data *iis1_pf_data;
    const struct plnk_platform_data *plnk0_pf_data;
    const struct plnk_platform_data *plnk1_pf_data;
    const struct spdif_platform_data *spdif_pf_data;
};

#endif
