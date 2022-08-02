#ifndef  __RT_STREAM_PKG_H__
#define  __RT_STREAM_PKG_H__

#include "fs/fs.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#define PCM_AUDIO_TYPE      1
#define JPEG_VIDEO_TYPE     2
#define ONLINE_PING_TYPE    3
#define RESET_REQ_TYPE  	4
#define RESET_RSP_TYPE  	5

#define FAST_PLAY_MAKER (1<<6)
#define LAST_FREG_MAKER (1<<7)

//#define NET_PKG_TEST
struct raw_stream_info {
    char *udp_send_buf;  //udp发送缓冲
    u32 seq;//seq自加
    u32 type;//视频类型
};

enum {
    VIDEO_FORWARD = 0x0,
    VIDEO_BEHIND,
};

struct frm_head {
    u8 type;
    u8 res;
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

struct __JPG_INFO {
    u32 src_w;
    u32 src_h;
    u32 buf_len;
    u8  *buf;
};

#define CHECK_CODE 0x88
#define CHECK_CODE_NUM 32
#define UDP_SEND_SIZE_MAX   500
#endif  /*RT_STREAM_PKG_H*/



