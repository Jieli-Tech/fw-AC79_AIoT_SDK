#ifndef ARCH_SDMMC_H
#define ARCH_SDMMC_H


#include "sdio/sdmmc.h"

struct sdmmc_platform_data {
    char port;
    u8 irq;
    u8 data_width;
    u8 priority;
    u8 detect_mode;
    u8 detect_io;
    u8 detect_io_level;
    u8 detect_time_interval;
    u16 detect_timeout;
    u8 hd_enable;
    u32 speed;
    volatile u32 *sfr;
    int (*detect_func)(const struct sdmmc_platform_data *);
    void (*port_init)(const struct sdmmc_platform_data *, int mode);
    void (*power)(u8 on);
};

#define SD0_PLATFORM_DATA_BEGIN(data) \
	static const struct sdmmc_platform_data data = {


#define SD0_PLATFORM_DATA_END() \
	.irq 					= IRQ_SD0_IDX, \
    .sfr                    = &JL_SD0->CON0, \
	.port_init 				= sdmmc_0_port_init, \
	.detect_time_interval 	= 250, \
	.detect_timeout     	= 1000, \
};

extern void sdmmc_0_port_init(const struct sdmmc_platform_data *, int mode);
extern int sdmmc_0_clk_detect(const struct sdmmc_platform_data *);
extern int sdmmc_0_io_detect(const struct sdmmc_platform_data *);

extern int sdmmc_cmd_detect(const struct sdmmc_platform_data *);

extern void sd_set_power(u8 enable);


#define SD1_PLATFORM_DATA_BEGIN(data) \
	static const struct sdmmc_platform_data data = {


#define SD1_PLATFORM_DATA_END() \
	.irq 					= IRQ_SD1_IDX, \
    .sfr                    = &JL_SD1->CON0, \
	.port_init 				= sdmmc_1_port_init, \
	.detect_time_interval 	= 250, \
	.detect_timeout     	= 1000, \
};

extern void sdmmc_1_port_init(const struct sdmmc_platform_data *, int mode);
extern int sdmmc_1_clk_detect(const struct sdmmc_platform_data *);
extern int sdmmc_1_io_detect(const struct sdmmc_platform_data *);



extern const struct device_operations sd_dev_ops;



#endif

