//////////////////////////////////////////////
//net net net net net net
//
//test
//
//test
//
//
//
//
//
//////////////////////////////////////////////
//
//
//

#include "system/includes.h"
#include "server/video_server.h"
#include "video_rec.h"
#include "video_system.h"
#include "action.h"
#include "app_config.h"
#include "asm/debug.h"
#include "asm/osd.h"
#include "app_database.h"
#include "storage_device.h"




#include "stream_core.h"



#include "IOTCAPIs.h"
#include "AVAPIs.h"
#include "P2PCam/AVIOCTRLDEFs.h"
#include "P2PCam/AVFRAMEINFO.h"


#define LOW_QUA   500
#define MID_QUA   800
#define HIGH_QUA  1000

#define VIDEO_STREAM_CHANNEL  0x3
#define AUDIO_STREAM_CHANNEL  0x2

static int AuthCallBackFn(char *viewAcc, char *viewPwd)
{
#if 0
    if (strcmp(viewAcc, tinfo.username) == 0 && strcmp(viewPwd, tinfo.password) == 0) {
        printf("\n %s suc\n", __func__);
        return 1;
    }
    printf("\n tutk_srv.username = %s viewAcc = %s\n", tinfo.username, viewAcc);
    printf("\n tutk_srv.password = %s viewPwd = %s \n", tinfo.password, viewPwd);
#endif
    return 1;
}


#define PCM_TYPE_AUDIO      1
#define JPEG_TYPE_VIDEO     2
#define H264_TYPE_VIDEO     3
#define PREVIEW_TYPE        4
#define DATE_TIME_TYPE      5
#define MEDIA_INFO_TYPE     6
#define PLAY_OVER_TYPE      7
#define GPS_INFO_TYPE       8
#define NO_GPS_DATA_TYPE    9
#define G729_TYPE_AUDIO    10


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

extern void *tutk_session_cli_get(void);



void tutk_debug_info()
{
    struct tutk_client_info *cinfo = (struct tutk_client_info *)tutk_session_cli_get();

    if (!cinfo) {
        return;
    }


    int frame_num = 0;
    int resend_size = 0;
    int ret = avServGetResendFrmCount(cinfo->streamindex, &frame_num);
    printf("ret=%d  frame_num:%d\n", ret, frame_num);
    ret = avServGetResendSize(cinfo->streamindex, &resend_size);

    printf("ret=%d  resize:%d\n", ret, resend_size);
    float rate = avResendBufUsageRate(cinfo->streamindex);
    printf("rate=%4.2f\n", rate);

}



static void *xopen(const char *path, void *arg)
{
    log_d("\n\n\nxopen\n\n");
    int nResend = -1;

    struct tutk_client_info *cinfo = (struct tutk_client_info *)tutk_session_cli_get();

    if (!cinfo) {
        return NULL;
    }

    log_d("cinfo->session_id=%d\n", cinfo->session_id);
    cinfo->streamindex = avServStart3(cinfo->session_id
                                      , AuthCallBackFn
                                      , 5
                                      , 0
                                      , VIDEO_STREAM_CHANNEL
                                      , &nResend);
    if (cinfo->streamindex < 0) {
        log_e("tutk stream open fail streamindex=%d\n", cinfo->streamindex);
        return NULL;
    }


    /* avServSetResendSize(cinfo->streamindex, 512); //kb */

    int frame_num = 0;
    int resend_size = 0;
    int ret = avServGetResendFrmCount(cinfo->streamindex, &frame_num);
    printf("ret=%d  frame_num:%d\n", ret, frame_num);
    ret = avServGetResendSize(cinfo->streamindex, &resend_size);

    printf("ret=%d  resize:%d\n", ret, resend_size);
    log_d("cinfo xopen end\n\n");

    return cinfo;

}

static int xsend(void *fd, char *data, int len, unsigned int flag)
{
    int ret = 0;
    struct tutk_client_info *cinfo = (struct tutk_client_info *)fd;
    /* log_d("111cinfo->streamindex=%d\n", cinfo->streamindex); */
    struct frm_head frame_head = {0};
    static u32 v_seq = 0, a_seq = 0;

    //  put_buf(data,8);

    /* printf("flag:%d\n", flag); */
    if (flag == H264_TYPE_VIDEO) {
        frame_head.type = H264_TYPE_VIDEO;
        frame_head.seq = (v_seq++);
        frame_head.timestamp += 0;
    } else if (flag == PCM_TYPE_AUDIO) {
        frame_head.type = PCM_TYPE_AUDIO;
        frame_head.seq = (a_seq++);
        frame_head.timestamp += 0;
        return len;
    } else if (flag == JPEG_TYPE_VIDEO) {
        frame_head.type = JPEG_TYPE_VIDEO;
        frame_head.seq = (v_seq++);
        frame_head.timestamp += 0;
    } else {
        putchar('E');
    }

    frame_head.frm_sz = len;
    do {
        if (cinfo->streamtask_kill) {
            return 0;

        }


        ret = avSendFrameData(cinfo->streamindex, data, len, &frame_head, sizeof(struct frm_head));
        if (ret < 0) {
            if (ret == AV_ER_EXCEED_MAX_SIZE) {

#if 0
                puts("\n\n-------AV_ER_EXCEED_MAX_SIZE------\n\n") ;

                int frame_num = 0;
                int resend_size = 0;
                int ret = avServGetResendFrmCount(cinfo->streamindex, &frame_num);
                printf("ret=%d  frame_num:%d\n", ret, frame_num);
                ret = avServGetResendSize(cinfo->streamindex, &resend_size);

                printf("ret=%d  resize:%d\n", ret, resend_size);
                float rate = avResendBufUsageRate(cinfo->streamindex);
                printf("rate=%4.2f\n", rate);

                puts("\n\n-------AV_ER_EXCEED_MAX_SIZE------\n\n") ;
#endif
                return len;

            }
            log_e("\n\n\n tutk_send err ret=%d\n", ret);
            return 0;

        }

    } while (ret != AV_ER_NoERROR);

    /* putchar('k'); */
    return len;
}

static void xclose(void *fd)
{
    struct tutk_client_info *cinfo = (struct tutk_client_info *)fd;

    log_d("cinfo->session_id=%d\n", cinfo->session_id);
    avServStop(cinfo->streamindex);
    log_d("\n\n\nxclose\n\n\n");
}

REGISTER_NET_VIDEO_STREAM_SUDDEV(tutk_video_stream_sub) = {
    .name = "tutk",
    .open = xopen,
    .write = xsend,
    .close = xclose,
};







#include "os/os_api.h"
#include "system/init.h"
#include "asm/includes.h"
#include "IOTCAPIs.h"
#include "AVAPIs.h"
#include "P2PCam/AVIOCTRLDEFs.h"
#include "P2PCam/AVFRAMEINFO.h"
#include "server/audio_server.h"
#include "server/server_core.h"
#include "generic/circular_buf.h"
#include "server/ctp_server.h"




#define VIDEO_STREAM_CHANNEL  0x3
#define AUDIO_STREAM_CHANNEL  0x2



#define PCM_TYPE_AUDIO      1
#define JPEG_TYPE_VIDEO     2
#define H264_TYPE_VIDEO     3
#define PREVIEW_TYPE        4
#define DATE_TIME_TYPE      5
#define MEDIA_INFO_TYPE     6
#define PLAY_OVER_TYPE      7
#define GPS_INFO_TYPE       8
#define NO_GPS_DATA_TYPE    9
#define G729_TYPE_AUDIO    10




#define CACHE_BUF_LEN (8 * 1024)

struct {
    struct server *dec_server;
    u8 *cache_buf;
    cbuffer_t save_cbuf;
    volatile u8 run_flag;
} audio_hdl;

#define __this (&audio_hdl)





static int tutk_vfs_fread(void *file, void *data, u32 len)
{
    u32 rlen;

    do {
        rlen = cbuf_read(&__this->save_cbuf, data, len);
        if (rlen == len) {
            /* put_buf(data, 32); */
            break;
        }

        if (!__this->run_flag) {
            return 0;
        }
        /* putchar('E'); */
    } while (__this->run_flag);

    return len;
}

static int tutk_vfs_fclose(void *file)
{
    return 0;
}

static int tutk_vfs_flen(void *file)
{
    return 0;
}

static const struct audio_vfs_ops tutk_vfs_ops = {
    .fwrite = NULL,
    .fread  = tutk_vfs_fread,
    .fclose = tutk_vfs_fclose,
    .flen   = tutk_vfs_flen,
};

static u8 buffer[8192 + 128];
static void tutk_audio_task(void *arg)
{

    int ret = 0;
    int len = 0;

    int nResend = -1;
    unsigned int frmNo;
    int outBufSize = 0;
    int outFrmSize = 0;
    int outFrmInfoSize = 0;
    struct frm_head frameInfo;
    int ioType = 0;
    int srvType = 0;



    struct tutk_client_info *cinfo = (struct tutk_client_info *)arg;


    printf("cinfo->session_id=%d\n", cinfo->session_id);

    cinfo->speakindex = avClientStart2(cinfo->session_id, "asdsadasdsa", "123213213", 20, &srvType, 0x2, &nResend);
    if (cinfo->speakindex < 0) {
        printf("%s   %d\n", __func__, __LINE__);
        return;
    }

    while (1) {
        if (cinfo->speakkill) {
            break;
        }

agin:
        ret = avRecvFrameData2(cinfo->speakindex, buffer, 8192, &outBufSize, &outFrmSize, (char *)&frameInfo, sizeof(struct frm_head), &outFrmInfoSize, &frmNo);
        if (ret < 0) {
            if (ret == AV_ER_DATA_NOREADY) {
                msleep(50);
                goto agin;
            }
            return ;
        }

        len = cbuf_write(&__this->save_cbuf, buffer, ret);
        if (len == 0) {
            putchar('B');
            cbuf_clear(&__this->save_cbuf);
        }
    }

    printf("end tutk_audio_task\n");


}

//在ctp_cmd.c cmd_put_close_speak_stream关闭
int close_audio_channel(void *arg)
{


    struct tutk_client_info *cinfo = (struct tutk_client_info *)arg;

    union audio_req req = {0};


    if (!cinfo->speakvaild) {
        return -1;
    }

    avClientStop(cinfo->speakindex);
    cinfo->speakkill = 1;
    thread_kill(&cinfo->speak_task_pid, KILL_WAIT);
    cinfo->speakkill = 0;


    if (!__this->run_flag) {
        return 0;
    }

    __this->run_flag = 0;

    cinfo->speakvaild  = 0;

    if (__this->dec_server) {
        req.dec.cmd = AUDIO_DEC_STOP;
        server_request(__this->dec_server, AUDIO_REQ_DEC, &req);
        server_close(__this->dec_server);
        __this->dec_server = NULL;
    }

    if (__this->cache_buf) {
        free(__this->cache_buf);
        __this->cache_buf = NULL;
    }

    return 0;
}
//在ctp_cmd.c cmd_put_open_speak_stream  打开
int open_audio_channel(void *arg)
{
    int err;

    struct tutk_client_info *cinfo = (struct tutk_client_info *)arg;
    union audio_req req = {0};
    char task_name[64];

    if (__this->run_flag) {
        return -1;
    }

    if (cinfo->speakvaild) {
        printf("audio is same open , plase close \n");
        return -1;

    }


    if (!__this->dec_server) {
        __this->dec_server = server_open("audio_server", "dec");
        if (!__this->dec_server) {
            goto __err;
        }
    }

    __this->cache_buf = (u8 *)malloc(CACHE_BUF_LEN);
    if (__this->cache_buf == NULL) {
        goto __err;
    }
    cbuf_init(&__this->save_cbuf, __this->cache_buf, CACHE_BUF_LEN);


    __this->run_flag = 1;


    memset(&req, 0, sizeof(union audio_req));

    req.dec.cmd             = AUDIO_DEC_OPEN;
    req.dec.volume          = 100;
    req.dec.output_buf      = NULL;
    req.dec.output_buf_len  = 512;
    req.dec.channel         = 1;
    req.dec.sample_rate     = 8000;
    req.dec.priority        = 1;
    req.dec.vfs_ops         = &tutk_vfs_ops;
    req.dec.dec_type 		= "pcm";
    req.dec.sample_source   = "dac";

    err = server_request(__this->dec_server, AUDIO_REQ_DEC, &req);
    if (err) {
        goto __err;
    }

    req.dec.cmd = AUDIO_DEC_START;
    err = server_request(__this->dec_server, AUDIO_REQ_DEC, &req);
    if (err) {
        goto __err;
    }


    sprintf(task_name, "tutk_audio_task%d", cinfo->session_id);
    thread_fork(task_name, 22, 0x1000, 0, &cinfo->speak_task_pid, tutk_audio_task, cinfo);



    cinfo->speakvaild = 1;


    return 0;


__err:
    if (__this->dec_server) {
        server_close(__this->dec_server);
        __this->dec_server = NULL;
    }
    if (__this->cache_buf) {
        free(__this->cache_buf);
        __this->cache_buf = NULL;
    }

    __this->run_flag = 0;

    return -1;

}

