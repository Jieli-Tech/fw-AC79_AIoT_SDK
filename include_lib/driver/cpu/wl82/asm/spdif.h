#ifndef AUDIO_SPDIF_H
#define AUDIO_SPDIF_H

#include "generic/typedef.h"

#define SPDIF_IN_PORT_A      BIT(0)    //PC7
#define SPDIF_IN_PORT_B      BIT(1)    //PC8
#define SPDIF_IN_PORT_C      BIT(2)    //PA7
#define SPDIF_IN_PORT_D      BIT(3)    //PA8

#define SPDIF_IN_IOMAP_A     ( BIT(4) | BIT(0))  //input_ch 8
#define SPDIF_IN_IOMAP_B     ( BIT(4) | BIT(1))  //input_ch 9
#define SPDIF_IN_IOMAP_C     ( BIT(4) | BIT(2))  //input_ch 10
#define SPDIF_IN_IOMAP_D     ( BIT(4) | BIT(3))  //input_ch 11

#define SPDIF_CHANNEL_NUMBER      2
#define SPDIF_DATA_DAM_LEN  160
#define SPDIF_INFO_LEN      6

#define SYNCWORD1 0xF872
#define SYNCWORD2 0x4E1F
#define BURST_HEADER_SZIE 0x8

enum IEC61937DataType {
    IEC61937_AC3                 = 0x01,
    IEC61937_MPEG1_LAYER1        = 0x04,
    IEC61937_MPEG1_LAYER23       = 0x05,
    IEC61937_MPEG2_EXT           = 0x06,
    IEC61937_MPEG2_AAC           = 0x07,
    IEC61937_MPEG2_LAYER1_LSF    = 0x08,
    IEC61937_MPEG2_LAYER2_LSF    = 0x09,
    IEC61937_MPEG2_LAYER3_LSF    = 0x0A,
    IEC61937_DTS1                = 0x0B,
    IEC61937_DTS2                = 0x0C,
    IEC61937_DTS3                = 0x0D,
    IEC61937_ATRAC               = 0x0E,
    IEC61937_ATRAC3              = 0x0F,
    IEC61937_ATRACX              = 0x10,
    IEC61937_DTSHD               = 0x11,
    IEC61937_WMAPRO              = 0x12,
    IEC61937_MPEG2_AAC_LSF_2048  = 0x13,
    IEC61937_MPEG2_AAC_LSF_4096  = 0x14,
    IEC61937_EAC3                = 0x15,
};

struct spdif_platform_data {
    u8 spdif_port;
    u8 width;
    u8 width_24_to_16;
    u16 points;
    u32 input_port;
};

int audio_spdif_slave_open(const struct spdif_platform_data *pd);
int audio_spdif_slave_start(void);
void audio_spdif_slave_stop(void);
void audio_spdif_slave_close(void);
void audio_spdif_slave_switch_port(u8 spidf_port, u32 input_port);
void spdif_irq_handler(void);
void spdif_set_data_handler(void *priv, void (*handler)(void *, u8 *data, int len));

#endif
