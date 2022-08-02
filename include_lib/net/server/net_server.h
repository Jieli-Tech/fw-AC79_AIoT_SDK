#ifndef  __NET_SERVER_H__
#define  __NET_SERVER_H__



#include "fs/fs.h"


#include "lwip/sockets.h"
#include "lwip/netdb.h"
enum {
    GET_MEDIA_INFO,
    GET_VIDEO_PREVEIW,
    PLAY_VIDEO_START,
    PLAY_VIDEO_PAUSE,
    PLAY_VIDEO_FF,
    PLAY_VIDEO_FR,
    PLAY_VIDEO_STOP,
};

enum {
    PREVIEW = 0x0,
    THUS,
};
struct preview {
    u8 type;
    char (*filename)[64];
    u8 *buffer;
    size_t buffer_len;
    size_t data_size;
    FILE *fd;
    u16 weight;
    u16 height;
    u32 num;
    u16 timeinv;
    u32 offset;

};

struct _playback {
    char file_name[64];
    u32 vedio_inv;
    u32 weight;
    u32 height;
    u8  type;
    u32 msec;
};


struct net_req {
    u32 isforward;
    char dir[36];
    struct preview pre;
    struct _playback playback;

};

enum {
    NONE = 0x0,
    VID_JPG,
    VIDEO,
    JPG,
};

struct rt_stream_app_info {
    u32 width;
    u32 height;
    u8 type;
    u8 fps;
    void *priv;
};



void FILE_LIST_ADD(u32 status, const char *__path, u8 create_file);
int FILE_LIST_INIT(u32 flag);
int FILE_LIST_EXIT(void);
void FILE_LIST_INIT_SMALL(u32 file_num);
void FILE_LIST_IN_MEM(u32 flag);
void FILE_LIST_TASK_INIT(void);
int FILE_INITIND_CHECK();
void FILE_CHANGE_ATTR(const char *fname, char attr);
void FILE_REMOVE_ALL();
void FILE_DELETE(char *__fname, u8 create_file);
int FILE_GEN_SYNC(const char *filename, const char *dir);

void FILE_GEN(void);

int virfile_reg(void);
int vf_list(u8 type, u8 isforward, char *dir);

int video_preview_post_msg(struct net_req *req);
int  video_playback_post_msg(struct net_req *req);
int preview_init(u16 port, int callback(void *priv, u8 *data, size_t len));
int playback_init(u16 port, int callback(void *priv, u8 *data, size_t len));
void preview_uninit();
void playback_uninit();
int playback_disconnect_cli(struct sockaddr_in *dst_addr);
int video_preview_and_thus_disconnect(struct sockaddr_in *dst_addr);
int playback_cli_pause(struct sockaddr_in *dst_addr);
int playback_cli_continue(struct sockaddr_in *dst_addr);
int playback_cli_fast_play(struct sockaddr_in *dst_addr, u32 speed);
int video_cli_slide(struct sockaddr_in *dst_addr, u8 direct);
void net_video_rec_fmt_notify(void);

unsigned short DUMP_PORT();
unsigned short FORWARD_PORT();
unsigned short BEHIND_PORT();
const char *get_rec_path_1();
const char *get_rec_path_2();
const char *get_root_path();



#endif  /*NET_SERVER_H*/
