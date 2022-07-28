#include "sys_common.h"
#include "system/spinlock.h"
#include "os/os_api.h"
#include "app_config.h"
#include "os/os_api.h"
#include "stream_core.h"
#include "asm/uart.h"
#include "server/video_server.h"
#include "rt_stream_pkg.h"
#include "video_buf_config.h"


#define VIDEO_PKBUFF_SIZE 	(NET_VREC0_FBUF_SIZE / 2)
#if NET_AUDIO_BUF_SIZE
#define AUDIO_PKBUFF_SIZE 	(NET_AUDIO_BUF_SIZE / 2)
#endif

#define jl_ntohl(x) (u32)((((u32)(x))>>24) | ((((u32)(x))>>8)&0xff00) | (((u32)(x))<<24) | ((((u32)(x))&0xff00)<<8))

#define JL_ENDF     	jl_ntohl(0x56185719)
#define JPEG_HEAD 		0xE0FFD8FF
#define JL_000DC		jl_ntohl(0x30306463)
#define JL_001WB		jl_ntohl(0x30317762)


struct strm_path {
    char *path;
    char *mode;
};

struct strm_ptl_info {
    u8 kill: 1;
    u8 vd_use: 1;
    u8 ad_use: 1;
    u8 err: 1;
    u8 init: 1;
    u8 copy: 1;
    struct strm_path spath;
    void *ptl_fd;
    OS_SEM sem;
    u8 *video_pkbuff;
    u8 *audio_pkbuff;
    u32 vd_len;
    u32 ad_len;
    int pid;
};
struct strm_ptl_info strm_ptl_info_ = {0};
#define strm_ptl (&strm_ptl_info_)


/*********************************************************************************/
static int stream_protocol_open(const char *path, const char *mode)
{
    strm_ptl->ptl_fd = stream_open(path, mode);
    if (!strm_ptl->ptl_fd) {
        printf("stream_protocol_open err !!!\n");
        return -1;
    }
    return 0;
}

static int stream_protocol_write(u8 type, void *buf, u32 len)
{
    u32 *len_ptr = (u32 *)buf;
    if (!strm_ptl->ptl_fd) {
        return 0;
    } else if (len <= 8) {
        return len;
    }
    if (*(len_ptr + 2) == JPEG_HEAD) {
        buf += 8;
        len -= 8;
    }
    len &= ~(0xf << 28);
    len |= type << 28;
    return stream_write(strm_ptl->ptl_fd, buf, len);
}

static int stream_protocol_ctrl(u32 cmd, u32 arg)
{
    if (!strm_ptl->ptl_fd) {
        return 0;
    }
    return stream_ctrl(strm_ptl->ptl_fd, cmd, arg);
}

static int stream_protocol_close(void)
{
    int ret;
    if (!strm_ptl->ptl_fd) {
        return 0;
    }
    ret = stream_close(strm_ptl->ptl_fd);
    strm_ptl->ptl_fd = NULL;
    return ret;
}
/*********************************************************************************/

int stream_packet_cb(u8 type, u8 *buf, u32 size)
{
    u32 *head = (u32 *)buf;
    if (!strm_ptl->init || strm_ptl->err || strm_ptl->kill) {
        return 0;
    } else if ((size <= 8) && (*head == JL_ENDF)) {
        return size;
    }

    if (type == VIDEO_REC_JPEG_TYPE_VIDEO) {
        if (size > VIDEO_PKBUFF_SIZE) {
            printf("VDPKBUFF_SIZE no enough !!!\n");
            return 0;
        }
        if ((*head == JL_000DC && *(head + 2) == JPEG_HEAD) || *(head + 2) == JPEG_HEAD) {
            buf += 8;
            size -= 8;
        }
        if (!strm_ptl->vd_use && !strm_ptl->kill) {
            strm_ptl->copy = true;
            strm_ptl->vd_use = true;
            strm_ptl->vd_len = size;
            memcpy(strm_ptl->video_pkbuff, buf, size);
            strm_ptl->copy = false;
            os_sem_post(&strm_ptl->sem);
        }
    } else if (type == VIDEO_REC_PCM_TYPE_AUDIO) {
#ifdef AUDIO_PKBUFF_SIZE
        if (size > AUDIO_PKBUFF_SIZE) {
            printf("ADPKBUFF_SIZE no enough !!!\n");
            return 0;
        }
        if (!strm_ptl->ad_use && !strm_ptl->kill) {
            strm_ptl->copy = true;
            strm_ptl->ad_use = true;
            strm_ptl->ad_len = size;
            memcpy(strm_ptl->audio_pkbuff, buf, size);
            strm_ptl->copy = false;
            os_sem_post(&strm_ptl->sem);
        }
#endif
    }
    return size;
}

static int stream_protocol_task(void *p)
{
    int ret;
    int to = 20;

    os_sem_create(&strm_ptl->sem, 0);
    strm_ptl->video_pkbuff = malloc(VIDEO_PKBUFF_SIZE);
    if (!strm_ptl->video_pkbuff) {
        printf("err : no mem for vd pkbuff !!!\n");
        strm_ptl->err = true;
        goto exit;
    }
#ifdef AUDIO_PKBUFF_SIZE
    strm_ptl->audio_pkbuff = malloc(AUDIO_PKBUFF_SIZE);
    if (!strm_ptl->audio_pkbuff) {
        printf("err : no mem for ad pkbuff !!!\n");
        strm_ptl->err = true;
        goto exit;
    }
#endif

    ret = stream_protocol_open(strm_ptl->spath.path, strm_ptl->spath.mode);
    if (ret) {
        printf("stream_protocol_open err, path : %s \n", strm_ptl->spath.path);
        strm_ptl->err = true;
        goto exit;
    }
    strm_ptl->init = true;
    printf("stream_protocol run \n");
    while (1) {
        ret = os_sem_pend(&strm_ptl->sem, 10);
        if (strm_ptl->kill) {
            break;
        }
        if (!ret) {
            if (strm_ptl->vd_use && strm_ptl->ad_use) {
                os_sem_set(&strm_ptl->sem, 0);
            }
            if (strm_ptl->vd_use) {
                ret = stream_protocol_write(JPEG_TYPE_VIDEO, strm_ptl->video_pkbuff, strm_ptl->vd_len);
                strm_ptl->vd_use = false;
                if (!ret) {
                    strm_ptl->err = true;
                    printf("err : stream_protocol_write, kill task !!!\n");
                    break;
                }
            }
#ifdef AUDIO_PKBUFF_SIZE
            if (strm_ptl->ad_use) {
                ret = stream_protocol_write(PCM_TYPE_AUDIO, strm_ptl->audio_pkbuff, strm_ptl->ad_len);
                strm_ptl->ad_use = false;
                if (!ret) {
                    strm_ptl->err = true;
                    printf("err : stream_protocol_write, kill task !!!\n");
                    break;
                }
            }
#endif
        }
    }

exit:
    strm_ptl->kill = true;
    while (strm_ptl->copy && to--) {
        os_time_dly(1);
    }
    stream_protocol_close();
    if (strm_ptl->video_pkbuff) {
        free(strm_ptl->video_pkbuff);
        strm_ptl->video_pkbuff = NULL;
    }
    if (strm_ptl->audio_pkbuff) {
        free(strm_ptl->audio_pkbuff);
        strm_ptl->audio_pkbuff = NULL;
    }
    if (os_sem_valid(&strm_ptl->sem)) {
        os_sem_del(&strm_ptl->sem, 0);
    }
    return 0;
}
int stream_protocol_task_create(char *path, char *mode)
{
    int ret;
    int to = 100;
    if (strm_ptl->init) {
        printf("stream_protocol_task already open \n");
        return 0;
    }

    strm_ptl->spath.path = path;
    strm_ptl->spath.mode = mode;
    strm_ptl->ptl_fd = NULL;
    strm_ptl->kill = 0;
    strm_ptl->err = 0;
    strm_ptl->vd_use = 0;
    strm_ptl->ad_use = 0;
    strm_ptl->pid = 0;
    ret = thread_fork("stream_protocol_task", 16, 1024, 0, &strm_ptl->pid, stream_protocol_task, NULL);
    if (ret) {
        printf("stream_protocol_task create err !!!\n");
        return -1;
    }
    while (!strm_ptl->init && !strm_ptl->err && --to) {
        os_time_dly(1);
    }
    if (strm_ptl->err || to == 0) {
        ret = -EINVAL;
    }
    if (ret) {
        printf("stream_protocol_task kill req = %d\n", ret);
        strm_ptl->kill = true;
        os_sem_post(&strm_ptl->sem);
        thread_kill(&strm_ptl->pid, KILL_WAIT);
        strm_ptl->init = false;
        strm_ptl->kill = false;
    }
    return ret;
}
int stream_protocol_task_kill(void)
{
    if (strm_ptl->init) {
        strm_ptl->kill = true;
        os_sem_post(&strm_ptl->sem);
        printf("stream_protocol task kill \n");
        thread_kill(&strm_ptl->pid, KILL_WAIT);
        strm_ptl->init = false;
        strm_ptl->kill = false;
    }
    return 0;
}

