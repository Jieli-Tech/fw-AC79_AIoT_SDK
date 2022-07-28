// *INDENT-OFF*

#include "app_config.h"

/******************************************************************
 *系统跑sdram或者ddr，配置在sdk_ld.c
 *系统不带sdram，配置在sdk_ld_sfc.c
 *****************************************************************/
#ifdef CONFIG_NO_SDRAM_ENABLE
#include "sdk_ld_sfc.c"
#else
#include "sdk_ld_sdram.c"
#endif





