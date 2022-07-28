#ifndef _NET_VIDEO_REC_H_
#define _NET_VIDEO_REC_H_

#define	STRM_SOURCE_VIDEO0  0x1001
#define	STRM_SOURCE_VIDEO1  0x1002


#include "system/includes.h"
#include "server/video_server.h"
#include "video_rec.h"
#include "net_server.h"


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

enum APP_STATUS {
    APP_CAMERA_IN,
    APP_CAMERA_OUT,
};

struct _remote {
    struct vs_video_rec rec_info;
    int width;
    int height;
};

struct net_video_hdl {
    enum NET_VIDEO_REC_STA net_state;
    enum NET_VIDEO_REC_STA net_state_ch2;
    enum NET_VIDEO_REC_STA net_state1;
    enum NET_VIDEO_REC_STA net_state1_ch2;

    struct server *net_video_rec;
    struct server *net_video_rec1;
    u8 *net_v0_fbuf;
    u8 *net_v1_fbuf;
    u8 *audio_buf;
    void *priv;
    u8 net_video0_vrt_on;
    u8 net_video0_art_on;
    u8 net_video1_vrt_on;
    u8 net_video1_art_on;
    u8 videoram_mark;
    union video_req net_videoreq[2];
    struct server *net_disbuf_enc;
    enum NET_VIDEO_REC_STA net_state2;
    u8 *net_v2_fbuf;
    u32 total_frame;
    int timer_handler;
    u32 dy_fr;
    u32 dy_fr_denom;
    u32 fbuf_fcnt;
    u32 fbuf_ffil;
    u32 dy_bitrate;
    u8 uvc_id;
    u8 isp_scenes_status;
    struct _remote remote;
    void *cmd_fd;
    u8 channel;

    u8 is_open;//标记是否是私有协议
    u8 video_rec_err;//用在录像IMC打不开情况下,关闭文件,录像中把卡不能添加文件列表
    u8 video_id;//0前视,1后视
    u8 video_720P;//存储谁霸占了720P通道,1 录像,2 实时流,3 RTSP
    u8 cap_image;
};

extern int net_video_rec_get_fps(void) ;
extern int net_video_rec_get_audio_rate();
extern char *video_rec_finish_get_name(FILE *fd, int index, u8 is_emf);
extern int video_rec_finish_notify(char *path);
extern int video_rec_delect_notify(FILE *fd, int id);
extern int video_rec_err_notify(const char *method);
extern int video_rec_all_stop_notify(void);
extern int video_rec_control_start(void);
extern int video_rec_control_doing(void);
extern int video_rec_device_event_action(struct device_event *event);
extern int video_rec_sd_in_notify(void);
extern int video_rec_sd_out_notify(void);
extern int video_rec_state_notify(void);
extern int net_video_rec_stop_notify(void);
extern void net_video_rec_status_notify(void);
extern int video_rec_sd_event_ctp_notify(char state);
extern int video_rec_get_fps();
extern int net_pkg_get_video_size(int *width, int *height);
extern void *get_video_rec_handler(void);
extern void *get_net_video_rec_handler(void);
extern int video_rec_get_abr_from(u32 width);
extern int net_video_rec_state(void);
extern u8 net_video_rec_get_drop_fp(void);
extern u8 net_video_rec_get_lose_fram(void);
extern u8 net_video_rec_get_send_fram(void);
extern int user_net_video_rec_open(char forward);
extern int user_net_video_rec_close(char forward);
extern int user_net_video_rec_take_photo(int qua, void (*callback)(char *buf, int len));



#endif

