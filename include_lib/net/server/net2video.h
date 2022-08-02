#ifndef __NET2VIDEO__H
#define __NET2VIDEO__H
struct cmd_ctl {
    int len;
    int cmd;
    char *data;
};

struct photo_parm {
    u32 width;
    u32 height;
    u32 quality;
    char *databuffer;
    u32 datalen;
};



int video_rt_tcp_server_init2(int port, int (*callback)(int, char *, int, void *));

void video_rt_tcp_server_uninit2();

int cmd_send(struct cmd_ctl *cinfo);

void video_disconnect_all_cli();


#define IPCIOC_STREAM_ON                _IOW('U', 0, sizeof(unsigned int))
#define IPCIOC_STREAM_OFF               _IOW('U', 1, sizeof(unsigned int))
#define IPCIOC_REQBUFS                  _IOW('U', 2, sizeof(unsigned int))
#define IPCIOC_DQBUF                    _IOW('U', 3, sizeof(unsigned int))
#define IPCIOC_QBUF                     _IOW('U', 4, sizeof(unsigned int))
#define IPCIOC_SEND_CMD                     _IOW('U', 5, sizeof(unsigned int))
#define IPCIOC_SEND_CMD_ASYNC                     _IOW('U', 6, sizeof(unsigned int))
#define IPCIOC_CONNECT                     _IOW('U', 7, sizeof(unsigned int))
#define IPCIOC_DISCONNECT                     _IOW('U', 8, sizeof(unsigned int))
#define IPCIOC_SET_NET                     _IOW('U', 9, sizeof(unsigned int))
#define IPCIOC_RESET_NET                 _IOW('U', 10, sizeof(unsigned int))



#define VCAM_SET_VIDEO_PRARM                _IOW('U', 16, sizeof(unsigned int))
#define VCAM_SET_CYC_TIME                _IOW('U', 17, sizeof(unsigned int))
#define VCAM_IMAGE_PICTURE                _IOW('U', 18, sizeof(unsigned int))
#define VCAM_SET_LOCAL_CYC_TIME                _IOW('U', 19, sizeof(unsigned int))

#define IPCIOC_AUDIO_STREAM_ON                _IOW('U',11, sizeof(unsigned int))
#define IPCIOC_AUDIO_STREAM_OFF               _IOW('U', 12, sizeof(unsigned int))
#define IPCIOC_AUDIO_REQBUFS                  _IOW('U', 13, sizeof(unsigned int))
#define IPCIOC_AUDIO_DQBUF                    _IOW('U', 14, sizeof(unsigned int))
#define IPCIOC_AUDIO_QBUF                     _IOW('U', 15, sizeof(unsigned int))



enum cmd_num {
    CMD_UNKNOWN_CMD           = -1000,
    CMD_TIMEOUT_CMD           = -9999,
#if 0
    CMD_SET_VIDEO_SAVE_FILE_FAIL   = -9,
    CMD_GET_IMAGE_PICTURE_FAIL               =  -8,
    CMD_SET_VIDEO_REC_PRARM_FAIL               =  -7,
    CMD_GET_DISPLAY_RESO_FAIL               =  -6,
    CMD_STOP_H264_STREAM_FAIL = -5,
    CMD_STOP_JPEG_STREAM_FAIL = -4,
    CMD_GET_FPS_FAIL          = -3,
    CMD_OPEN_JPEG_STREAM_FAIL = -2,
    CMD_OPEN_H264_STREAM_FAIL = -1,
#endif
    CMD_NO_ERR                =  0,
    CMD_OPEN_H264_STREAM      =  1,
    CMD_OPEN_JPEG_STREAM      =  2,
    CMD_GET_FPS               =  3,
    CMD_STOP_JPEG_STREAM      =  4,
    CMD_STOP_H264_STREAM      =  5,
    CMD_GET_DISPLAY_RESO               =  6,
    CMD_SET_VIDEO_REC_PRARM               =  7,
    CMD_GET_IMAGE_PICTURE               =  8,
    CMD_SET_VIDEO_SAVE_FILE   = 9,
    CMD_SWITCH_TO_PHOTO_MODE  = 10,
    CMD_SWITCH_TO_VIDEO_MODE  = 11,
    CMD_TAKE_PHOTO            = 12,
    CMD_TAKE_PHOTO1            = 13,
    CMD_SET_JPEG_RESO         = 14,
};
extern const struct device_operations ipc_dev_ops;




#endif
