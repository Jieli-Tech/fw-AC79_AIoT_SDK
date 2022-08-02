
#ifndef _ABNORMAL_OFF_H_
#define _ABNORMAL_OFF_H_

#include <stdarg.h>
#include "generic/typedef.h"
#include "os/os_api.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/prot/ethernet.h"
#include <streaming_media_server/fenice_config.h>
#include <streaming_media_server/fenice/socket.h>
#include <streaming_media_server/fenice/schedule.h>
#include "streaming_media_server/fenice/rtsp.h"


typedef struct _FENICE_TRANSFER {
    unsigned int type;  /* app层控制采用的协议类型--TCP/默认 */
    unsigned int port;  /* app层配置的端口号 */
    int (*exit)(void);  /* 关闭底层硬件 */
    int (*setup)(void);  /* 开启底层硬件 */
    int (*get_video_info)(struct fenice_source_info *info);/*获取配置视频相关参数*/
    int (*get_audio_info)(struct fenice_source_info *info);/*获取配置音频参数*/
    int (*set_media_info)(struct fenice_source_info *info);/*设置前后视：0前视，1后视*/
} FENICE_TRANSFER;


struct __RTSP_GBV {
    u32 __is_live;
    int eventloop_req_stop_addr;
    int eventloop_req_stop_flag;
    int num_conn;
    int conn_count;
    int stop_schedule;
    tsocket main_fd;
    tsocket main_sctp_fd;
    int session_connect_cnt;
    int video_rec_flag;
    u32 rtsp_play_flag;
    int fenice_close_hardware_flag;
    int dis_cli_break_flag;
    int stream_media_server_pid;
    int schedule_do_thread_pid;


    OS_MUTEX rtp_fd_mutex;
//FENICE_BANORMAL_OFF
    unsigned char flag;
    unsigned char pend_flag;
    unsigned int accept_num;
    OS_SEM sem;
    OS_SEM accept_sem;
    OS_MUTEX mutex;
    OS_MUTEX select_mutex;
//rtsp
    schedule_list sched[ONE_FORK_MAX_CONNECTION];
    RTSP_buffer *rtsp_list;
    SD_descr *SD_global_list;

    FENICE_TRANSFER fenice_transfer;
};


#endif

