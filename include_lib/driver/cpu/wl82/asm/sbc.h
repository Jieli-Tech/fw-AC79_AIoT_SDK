/*****************************************************************
>file name : media/include/audio/sbc.h
>author : lichao
>create time : Thu 29 Nov 2018 04:35:31 PM CST
*****************************************************************/

#ifndef _SBC_H_
#define _SBC_H_


#define AUDIO_DEC_ORIG_CH       0
#define AUDIO_DEC_L_CH          1
#define AUDIO_DEC_R_CH          2
#define AUDIO_DEC_MONO_LR_CH    3


#define SBC_SYNCWORD            0x9C
#define MSBC_SYNCWORD           0xAD

#define MSBC_BLOCKS             15
/*sbc音频采样率*/
#define SBC_FREQ_16000          0x0
#define SBC_FREQ_32000          0x1
#define SBC_FREQ_44100          0x2
#define SBC_FREQ_48000          0x3

/*sbc block参数*/
#define SBC_BLK_4               0x0
#define SBC_BLK_8               0x1
#define SBC_BLK_12              0x2
#define SBC_BLK_16              0x3

/*sbc 声道参数*/
#define SBC_MODE_MONO           0x0
#define SBC_MODE_DUAL_CHANNEL   0x1
#define SBC_MODE_STEREO         0x2
#define SBC_MODE_JOINT_STEREO   0x3

/*subbands 参数*/
#define SBC_SB_4                0x0
#define SBC_SB_8                0x1

/*allocation*/
#define SBC_AM_LOUDNESS         0x0
#define SBC_AM_SNR              0x1

struct sbc_info {
    u8 frequency;
    u8 blocks;
    u8 subbands;
    u8 mode;
    u8 allocation;
    u8 bitpool;
    u8 endian;
};

#define MSBC_FRAME_POINTS       120
#define MSBC_FRAME_LEN          58

#if __BYTE_ORDER == __LITTLE_ENDIAN
typedef struct _rtp_header {
    u16 cc: 4;
    u16 x: 1;
    u16 p: 1;
    u16 v: 2;

    u16 pt: 7;
    u16 m: 1;

    u16 sequence_number;
    u32 timestamp;
    u32 ssrc;
    u32 csrc[0];
} _GNU_PACKED_ rtp_header;

#elif __BYTE_ORDER == __BIG_ENDIAN

typedef struct _rtp_header {
    unsigned v: 2;
    unsigned p: 1;
    unsigned x: 1;
    unsigned cc: 4;

    unsigned m: 1;
    unsigned pt: 7;

    u16 sequence_number;
    u32 timestamp;
    u32 ssrc;
    u32 csrc[0];
} _GNU_PACKED_ rtp_header;

#else
#error "Unknown byte order"
#endif

#endif
