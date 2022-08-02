#ifndef CPU_CONFIG_H
#define CPU_CONFIG_H

#define FAT_CACHE_NUM   32

#define VIDEO0_REC_FORMAT   VIDEO_FMT_AVI
#define VIDEO1_REC_FORMAT   VIDEO_FMT_AVI
#define VIDEO2_REC_FORMAT   VIDEO_FMT_AVI
#ifdef CONFIG_PSRAM_ENABLE
#define VREC0_FBUF_SIZE     (512*1024)
#define VREC1_FBUF_SIZE     (256*1024)
#define VREC2_FBUF_SIZE     (256 * 1024)
#define AUDIO_BUF_SIZE      (256*1024)
#else
#if (!defined CONFIG_NO_SDRAM_ENABLE && __SDRAM_SIZE__ >= (2 * 1024 * 1024))
#define VREC0_FBUF_SIZE     (500*1024)
#define VREC1_FBUF_SIZE     (0)
#define VREC2_FBUF_SIZE     (0)
#define AUDIO_BUF_SIZE      (64*1024)
#else
#define VREC0_FBUF_SIZE     (150*1024)
#define VREC1_FBUF_SIZE     (0)
#define VREC2_FBUF_SIZE     (0)
#define AUDIO_BUF_SIZE      (16*1024)
#endif
#endif

#define USB_CAMERA_BUF_SIZE (1 * 1024 * 1024) // + 512 * 1024)
#define CAMERA_CAP_BUF_SIZE (1 * 1024 * 1024)


#if (!defined CONFIG_NO_SDRAM_ENABLE && __SDRAM_SIZE__ >= (2 * 1024 * 1024))
#define NET_VREC0_FBUF_SIZE     (200*1024)
#define NET_VREC1_FBUF_SIZE     (0)
#define NET_AUDIO_BUF_SIZE      (64*1024)
#else
#define NET_VREC0_FBUF_SIZE     (100*1024)
#define NET_VREC1_FBUF_SIZE     (0)
#define NET_AUDIO_BUF_SIZE      (16*1024)
#endif



#ifndef CONFIG_VIDEO1_ENABLE
#undef VREC1_FBUF_SIZE
#define VREC1_FBUF_SIZE (0)
#endif

#ifndef CONFIG_VIDEO2_ENABLE
#undef VREC2_FBUF_SIZE
#define VREC2_FBUF_SIZE (0)
#endif



#endif
