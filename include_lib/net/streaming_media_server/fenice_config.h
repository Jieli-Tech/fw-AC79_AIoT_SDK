#ifndef _FENICE_CONFIG_H_
#define _FENICE_CONFIG_H_

/* Debug disabled */
#define ENABLE_DEBUG 1

/* Dump enabled */
//#define ENABLE_DUMP 1

/* Buffer Dump disabled */
#define ENABLE_DUMPBUFF 0

/* verbosity enabled */
#define ENABLE_VERBOSE 1

/* Define default directory string for Fenice A/V resources */
//#define FENICE_AVROOT_DIR_DEFAULT_STR "B:"//"avroot" //"B:/avroot"
#define FENICE_AVROOT_DIR_DEFAULT_STR "storage/sd1/C"//"avroot" //"B:/avroot"

/* Define max number of RTSP incoming sessions for Fenice */
#define FENICE_MAX_SESSION_DEFAULT 6

/* Define default RTSP listening port */
#define FENICE_RTSP_PORT_DEFAULT 554

/* Define to 1 if you have `alloca', as a function or macro. */
#define HAVE_ALLOCA 0

/* Define to 1 if you have <alloca.h> and it should be used (not on Ultrix).
   */
#define HAVE_ALLOCA_H 0

/* Define SCTP support */
//#define HAVE_SCTP_FENICE 1

/* Define to 1 if the system has the type `struct sockaddr_storage'. */
#define HAVE_STRUCT_SOCKADDR_STORAGE 1

/* Name of package */
#define PACKAGE "jl"

/* Version number of package */
#define FEN_VERSION "0.01"

#define BYTE_ORDER	LITTLE_ENDIAN

struct fenice_source_info {
    unsigned int type;
    unsigned int width;
    unsigned int height;
    unsigned int fps;
    unsigned int sample_rate;
    unsigned int channel_num;
};

struct fenice_config {
    char protocol[4];
    unsigned int port;
    int (*exit)(void);  /* 关闭底层硬件 */
    int (*setup)(void);  /* 开启底层硬件 */
    int (*get_video_info)(struct fenice_source_info *info);/*获取配置视频相关参数*/
    int (*get_audio_info)(struct fenice_source_info *info);/*获取配置音频参数*/
    int (*set_media_info)(struct fenice_source_info *info);/*设置前后视：0前视，1后视*/
};

#endif //_FENICE_CONFIG_H_
