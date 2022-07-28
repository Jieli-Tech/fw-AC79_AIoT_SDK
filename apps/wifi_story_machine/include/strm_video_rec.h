#ifndef _STRM_VIDEO_REC_H_
#define _STRM_VIDEO_REC_H_

#include "system/includes.h"
#include "server/ctp_server.h"
#include "os/os_api.h"
#include "action.h"
#include "app_config.h"
#include "server/rt_stream_pkg.h"
#include "server/net_server.h"
#include "streaming_media_server/fenice_config.h"

enum NET_VIDEO_REC_STA {
    NET_VIDREC_NO_ERR = 0,
    NET_VIDREC_REQ_ERR = 0xA0,
    NET_VIDREC_STA_START,
    NET_VIDREC_STA_STARTING,
    NET_VIDREC_STA_STOP,
    NET_VIDREC_STA_STOPING,
    NET_VIDREC_ADD_NOTIFY,
    NET_VIDREC_DELECT_NOTIFY,
    NET_VIDREC_STATE_NOTIFY,
};

struct strm_video_hdl {
    enum NET_VIDEO_REC_STA state;
    struct server *video_rec0;
    struct server *video_rec1;
    struct server *video_display_0;
    struct server *video_display_1;

    u16 width;
    u16 height;
    u8 *video_buf;
    u8 *audio_buf;
    u32 video_buf_size;
    u32 audio_buf_size;
    u8 isp_scenes_status;
    u8 channel;

    int timer_handler;
    u32 dy_fr;
    u32 dy_fr_denom;
    u32 dy_bitrate;
    u32 fbuf_fcnt;
    u32 fbuf_ffil;
    u8 uvc_id;
    u8 video_id;
    u8 is_open;//标记RTSP是否被打开过
    u8 cap_image;
};

/*extern struct strm_video_hdl fv_rec_handler; */
extern void *get_strm_video_rec_handler(void);
extern int strm_video_rec_open(void);
extern int strm_video_rec_close(u8 close);//主要用于录像控制RTSP

#endif

