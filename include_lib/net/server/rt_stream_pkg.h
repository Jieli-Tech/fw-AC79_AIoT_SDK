#ifndef  __RT_STREAM_PKG_H__
#define  __RT_STREAM_PKG_H__

#include "fs/fs.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"


#define PCM_TYPE_AUDIO      1
#define JPEG_TYPE_VIDEO     2
#define H264_TYPE_VIDEO     3
#define PREVIEW_TYPE        4
#define DATE_TIME_TYPE      5
#define MEDIA_INFO_TYPE  	6
#define PLAY_OVER_TYPE      7
#define GPS_INFO_TYPE	    8
#define NO_GPS_DATA_TYPE	9
#define G729_TYPE_AUDIO    10


#define FAST_PLAY_MAKER (1<<6)
#define LAST_FREG_MAKER (1<<7)


//#define NET_PKG_TEST
struct rt_stream_info {
    struct sockaddr_in addr;
    void *fd;
    char *udp_send_buf;  //udp发送缓冲
    u32 seq;//seq自加
    u32 type;//视频类型
    u16  port;
    u8 cb_flag;
};



enum {
    VIDEO_FORWARD = 0x0,
    VIDEO_BEHIND,
};

struct frm_head {
    u8 type;
    u8 res;
#if 0
    u8 sample_seq;
#endif
    u16 payload_size;
    u32 seq;
    u32 frm_sz;
    u32 offset;
    u32 timestamp;
} __attribute__((packed));

struct media_info {
    u16 length;
    u16 height;
    u32 fps;
    u32 audio_rate;
    u32 dur_time; //总时间
    char filename[0]; //添加变量在filename之前
};






#endif  /*RT_STREAM_PKG_H*/
