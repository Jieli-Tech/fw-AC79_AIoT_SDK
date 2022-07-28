#include "system/includes.h"
#include "server/video_server.h"
#include "event/key_event.h"
#include "event/device_event.h"
#include "video_rec.h"
#include "video_system.h"
#include "action.h"
#include "app_config.h"
#include "asm/debug.h"
#include "asm/osd.h"
#include "app_database.h"
#include "storage_device.h"

#ifdef CONFIG_NET_ENABLE
#include "net_video_rec.h"
#endif


#define AUDIO_VOLUME	100

#ifndef CONFIG_VIDEO0_ENABLE
#undef VREC0_FBUF_SIZE
#define VREC0_FBUF_SIZE   0
#endif
#ifndef CONFIG_VIDEO1_ENABLE
#undef VREC1_FBUF_SIZE
#define VREC1_FBUF_SIZE   0
#endif
#ifndef CONFIG_VIDEO2_ENABLE
#undef VREC2_FBUF_SIZE
#define VREC2_FBUF_SIZE   0
#endif
#define LOCK_FILE_PERCENT   40    //0~100

#define NAME_FILE_BY_DATE   0	//使用RTC日期文件名


struct video_rec_hdl rec_handler;
#define __this 	(&rec_handler)

static const unsigned char video_osd_format_buf[] = "yyyy-nn-dd hh:mm:ss";
static const u16 rec_pix_w[] = {1280, 640};
static const u16 rec_pix_h[] = {720,  480};
static const char *rec_dir[][2] = {
#ifdef CONFIG_EMR_DIR_ENABLE
    {CONFIG_REC_DIR_0, CONFIG_EMR_REC_DIR_0},
    {CONFIG_REC_DIR_1, CONFIG_EMR_REC_DIR_1},
    {CONFIG_REC_DIR_2, CONFIG_EMR_REC_DIR_2},
#else
    {CONFIG_REC_DIR_0, CONFIG_REC_DIR_0},
    {CONFIG_REC_DIR_1, CONFIG_REC_DIR_1},
    {CONFIG_REC_DIR_2, CONFIG_REC_DIR_2},
#endif
};

static const char *rec_path[][2] = {
#ifdef CONFIG_EMR_DIR_ENABLE
    {CONFIG_REC_PATH_0, CONFIG_EMR_REC_PATH_0},
    {CONFIG_REC_PATH_1, CONFIG_EMR_REC_PATH_1},
    {CONFIG_REC_PATH_2, CONFIG_EMR_REC_PATH_2},
#else
    {CONFIG_REC_PATH_0, CONFIG_REC_PATH_0},
    {CONFIG_REC_PATH_1, CONFIG_REC_PATH_1},
    {CONFIG_REC_PATH_2, CONFIG_REC_PATH_2},
#endif
};

#ifdef CONFIG_VIDEO_720P
#define CAP_IMG_SIZE (150*1024)
#else
#define CAP_IMG_SIZE (40*1024)
#endif

static int video_rec_start();
static int video_rec_stop(u8 close);
static int video_rec_device_event_handler(struct device_event *event);
static int video_rec_sd_in();
static int video_rec_sd_out();
static int video_rec_get_abr(u32 width);




#ifdef CONFIG_NET_ENABLE
/******************************用于网络实时流*************************************/
extern char *video_rec_finish_get_name(FILE *fd, int index, u8 is_emf); //index：video0则0，video1则1，video2则2
extern int video_rec_finish_notify(char *path);
extern int video_rec_delect_notify(FILE *fd, int id);
extern int video_rec_err_notify(const char *method);
extern int video_rec_state_notify(void);
extern int video_rec_start_notify(void);
extern int video_rec_all_stop_notify(void);
extern int net_video_rec_event_notify(void);
extern int net_video_rec_event_stop(void);
extern int net_video_rec_event_start(void);
extern char *get_net_video_rec0_video_buf(int *buf_size);
extern char *get_net_video_rec1_video_buf(int *buf_size);
extern char *get_net_video_rec2_video_buf(int *buf_size);
extern char *get_net_video_rec_audio_buf(int *buf_size);
extern char *get_net_video_isc_buf(int *buf_size);
extern void net_video_server_init(void);
extern int net_video_event_hander(void *e);
#else
int video_rec_finish_notify(char *path)
{
    return 0;
}
int video_rec_delect_notify(FILE *fd, int id)
{
    return 0;
}
int video_rec_err_notify(const char *method)
{
    return 0;
}
int video_rec_state_notify(void)
{
    return 0;
}
int video_rec_start_notify(void)
{
    return 0;
}
int video_rec_all_stop_notify(void)
{
    return 0;
}
int net_video_rec_event_notify(void)
{
    return 0;
}
int net_video_rec_event_stop(void)
{
    return 0;
}
int net_video_rec_event_start(void)
{
    return 0;
}
char *get_net_video_rec0_video_buf(int *buf_size)
{
    *buf_size = 0;
    return NULL;
}
char *get_net_video_rec1_video_buf(int *buf_size)
{
    *buf_size = 0;
    return NULL;
}
char *get_net_video_rec2_video_buf(int *buf_size)
{
    *buf_size = 0;
    return NULL;
}
char *get_net_video_rec_audio_buf(int *buf_size)
{
    *buf_size = 0;
    return 0;
}
char *get_net_video_isc_buf(int *buf_size)
{
    *buf_size = 0;
    return NULL;
}
void net_video_server_init(void)
{
}
void net_video_event_hander(void *e)
{
}
#endif

#ifdef CONFIG_NET_ENABLE
#if (defined CONFIG_NO_SDRAM_ENABLE) //开wifi,没有sdram 录像buff使用实时流buff
#undef AUDIO_BUF_SIZE
#undef VREC0_FBUF_SIZE
#define AUDIO_BUF_SIZE 		NET_AUDIO_BUF_SIZE
#define VREC0_FBUF_SIZE 	NET_VREC0_FBUF_SIZE
#endif
#endif

#ifdef CONFIG_NET_ENABLE
void *get_video_rec_handler(void)
{
    return (void *)&rec_handler;
}
int video_rec_get_fps()
{
#ifdef VIDEO_REC_FPS
    return VIDEO_REC_FPS;
#else
    return 0;
#endif
}

int video_rec_get_audio_sampel_rate(void)
{
#ifdef  VIDEO_REC_AUDIO_SAMPLE_RATE
    return VIDEO_REC_AUDIO_SAMPLE_RATE;
#else
    return 8000;
#endif
}
int video_rec_control_start(void)
{
    int err;
    err = video_rec_start();
    return err;
}

int video_rec_control_doing(void)
{
    int err;
    if (__this->state == VIDREC_STA_START) {
        err = video_rec_stop(0);
    } else {
        err = video_rec_start();
    }
    return err;
}

int video_rec_device_event_action(struct device_event *event)
{
    return video_rec_device_event_handler(event);
}

int video_rec_sd_in_notify(void)
{
    return video_rec_sd_in();
}

int video_rec_sd_out_notify(void)
{
    return video_rec_sd_out();
}

int video_rec_get_abr_from(u32 width)
{
    return video_rec_get_abr(width);
}
#endif
/******************************************************************/

static void video_rec_buf_alloc()
{
    int buf_size[] = {VREC0_FBUF_SIZE, VREC1_FBUF_SIZE, VREC2_FBUF_SIZE};
    int bfsize = 0;
#if (defined CONFIG_NO_SDRAM_ENABLE)
    __this->audio_buf = get_net_video_rec_audio_buf(&bfsize);//获取公用BUFF
#endif
    if (!__this->audio_buf && AUDIO_BUF_SIZE) {
        __this->audio_buf = malloc(AUDIO_BUF_SIZE);
        if (!__this->audio_buf) {
            log_d(">>>>>>>>>> audiobuf alloc err>>>>>>\n");
            return ;
        }
    }
    for (int i = 0; i < ARRAY_SIZE(buf_size); i++) {
        if (buf_size[i]) {
#if (defined CONFIG_NO_SDRAM_ENABLE)
            switch (i) {
            case 0:
                __this->video_buf[i] = get_net_video_rec0_video_buf(&bfsize);//获取公用BUFF
                break;
            case 1:
                __this->video_buf[i] = get_net_video_rec1_video_buf(&bfsize);//获取公用BUFF
                break;
            case 2:
                __this->video_buf[i] = get_net_video_rec2_video_buf(&bfsize);//获取公用BUFF
                break;
            }
#endif
            if (!__this->video_buf[i]) {
                __this->video_buf[i] = malloc(buf_size[i]);
                if (!__this->video_buf[i]) {
                    log_d(">>>>>>>>>> videobuf alloc err>>>>>>\n");
                }
            }
        } else {
            __this->video_buf[i] = NULL;
        }
    }
}
static int video_rec_destroy()
{
    if (__this->state != VIDREC_STA_FORBIDDEN) {
        return -EFAULT;
    }

    for (int i = 0; i < CONFIG_VIDEO_REC_NUM; i++) {
#if (defined CONFIG_NO_SDRAM_ENABLE)
        int bfsize = 0;
        u8 *video_buf = NULL;
        switch (i) {
        case 0:
            video_buf = get_net_video_rec0_video_buf(&bfsize);//获取公用BUFF
            break;
        case 1:
            video_buf = get_net_video_rec1_video_buf(&bfsize);//获取公用BUFF
            break;
        case 2:
            video_buf = get_net_video_rec2_video_buf(&bfsize);//获取公用BUFF
            break;
        }
        if (video_buf == __this->video_buf[i] && __this->video_buf[i]) {
            __this->video_buf[i] = NULL;
        }
#endif
        if (__this->video_buf[i]) {
            free(__this->video_buf[i]);
            __this->video_buf[i] = NULL;
        }
    }
#if (defined CONFIG_NO_SDRAM_ENABLE)
    int bfsize = 0;
    u8 *audio_buf = get_net_video_rec_audio_buf(&bfsize);//获取公用BUFF
    if (audio_buf == __this->audio_buf && __this->audio_buf) {
        __this->audio_buf = NULL;
    }
#endif
    if (__this->audio_buf) {
        free(__this->audio_buf);
        __this->audio_buf = NULL;
    }

    if (__this->cap_buf) {
        free(__this->cap_buf);
        __this->cap_buf = NULL;
    }

    return 0;
}
static int __get_sys_time(struct sys_time *time)
{
    void *fd = dev_open("rtc", NULL);
    if (fd) {
        dev_ioctl(fd, IOCTL_GET_SYS_TIME, (u32)time);
        dev_close(fd);
        return 0;
    }

    return -EINVAL;
}
static const char *rec_file_name(int format)
{
#if NAME_FILE_BY_DATE
    struct sys_time time;
    static char file_name[MAX_FILE_NAME_LEN];

    if (__get_sys_time(&time) == 0) {
        if (format == VIDEO_FMT_AVI) {
            sprintf(file_name, "VID_%d%02d%02d_%02d%02d%02d.AVI",
                    time.year, time.month, time.day, time.hour, time.min, time.sec);
        } else if (format == VIDEO_FMT_MOV) {
            sprintf(file_name, "VID_%d%02d%02d_%02d%02d%02d.MOV",
                    time.year, time.month, time.day, time.hour, time.min, time.sec);
        } else if (format == VIDEO_FMT_MP4) {
            sprintf(file_name, "VID_%d%02d%02d_%02d%02d%02d.MP4",
                    time.year, time.month, time.day, time.hour, time.min, time.sec);
        }
        return file_name;
    }
#endif

    if (format == VIDEO_FMT_AVI) {
        return "VID_***.AVI";
    } else if (format == VIDEO_FMT_MP4) {
        return "VID_***.MP4";
    } else {
        return "VID_***.MOV";
    }
}

/*码率控制，根据具体分辨率设置*/
static int video_rec_get_abr(u32 width)
{
    /*视频码率kbps使用说明:
     码率：一帧图片以K字节为单位大小 * 帧率 * 8，比如：一帧图片为30KB，帧率为20帧，则码率为30*20*8=4800
     VGA图片大小说明：低等质量(小于20K)，中等质量(20K-40K)，高质量(大于40K，极限70K)
     720P图片大小说明：低等质量(小于50K)，中等质量(50k-100K)，高质量(大于100K，极限150K)
    */
    if (width <= 720) {
        /* return 8000; */
        return 4000;
    } else if (width <= 1280) {
        return 8000;
        /* return 10000; */
    } else if (width <= 1920) {
        return 14000;
    } else {
        return 18000;
    }
}
/*
 *根据录像不同的时间和分辨率，设置不同的录像文件大小
 */
static u32 video_rec_get_fsize(u8 cycle_time, u16 vid_width, int format)
{
    u32 fsize;

    if (cycle_time > 10 || cycle_time == 0) {
        cycle_time = 3;
    }

    fsize = video_rec_get_abr(vid_width) * cycle_time * 8250;

    if (format == VIDEO_FMT_AVI) {
        fsize = fsize + fsize / 4;
    }
    printf("fsize---->> %d \n\n", fsize);

    return fsize;
}

static int video_rec_cmp_fname(void *afile, void *bfile)
{
    int alen, blen;
    char afname[MAX_FILE_NAME_LEN];
    char bfname[MAX_FILE_NAME_LEN];

    if ((afile == NULL) || (bfile == NULL)) {
        return 0;
    }
    printf("video_rec_cmp_fname: %p, %p\n", afile, bfile);

    alen = fget_name(afile, (u8 *)afname, MAX_FILE_NAME_LEN);
    if (alen <= 0) {
        log_e("fget_name: afile=%x\n", afile);
        return 0;
    }
    ASCII_ToUpper(afname, alen);

    blen = fget_name(bfile, (u8 *)bfname, MAX_FILE_NAME_LEN);
    if (blen <= 0) {
        log_e("fget_name: bfile=%x\n", bfile);
        return 0;
    }
    ASCII_ToUpper(bfname, blen);

    printf("afname: %s, bfname: %s\n", afname, bfname);

    if (alen == blen && !strcmp(afname, bfname)) {
        return 1;
    }

    return 0;
}

static void video_rec_fscan_release(int lock_dir)
{
    printf("video_rec_fscan_release: %d\n", lock_dir);
    for (int i = 0; i < 3; i++) {
        if (__this->fscan[lock_dir][i]) {
            fscan_release(__this->fscan[lock_dir][i]);
            __this->fscan[lock_dir][i] = NULL;
        }
    }
}

static void video_rec_fscan_dir(int id, int lock_dir, const char *path)
{
    const char *str;
#ifdef CONFIG_EMR_DIR_ENABLE
    str = "-tMOVAVIMP4 -sn";
#else
    str = lock_dir ? "-tMOVAVIMP4 -sn -ar" : "-tMOVAVIMP4 -sn -a/r";
#endif
    if (__this->fscan[lock_dir][id]) {
        if (__this->old_file_number[lock_dir][id] == 0) {
            puts("--------delete_all_scan_file\n");
            fscan_release(__this->fscan[lock_dir][id]);
            __this->fscan[lock_dir][id] = NULL;
        }
    }

    if (!__this->fscan[lock_dir][id]) {
        __this->fscan[lock_dir][id] = fscan(path, str, 0);
        if (!__this->fscan[lock_dir][id]) {
            __this->old_file_number[lock_dir][id] = 0;
        } else {
            __this->old_file_number[lock_dir][id] = __this->fscan[lock_dir][id]->file_number;
        }
        __this->file_number[lock_dir][id] = __this->old_file_number[lock_dir][id];
        printf("fscan_dir: %d, file_number = %d\n", id, __this->file_number[lock_dir][id]);
    }
}

static FILE *video_rec_get_first_file(int id)
{
    int max_index = -1;
    int max_file_number = 0;
    int persent = __this->lock_fsize * 100 / __this->total_size;
    int lock_dir = !!(persent > LOCK_FILE_PERCENT);

    log_d("lock_file_persent: %d, %d, size: %dMB\n", id, persent, __this->lock_fsize / 1024);

#ifdef CONFIG_VIDEO0_ENABLE
    video_rec_fscan_dir(0, lock_dir, rec_path[0][lock_dir]);
#endif
    for (int i = 0; i < 3; i++) {
        if (__this->fscan[lock_dir][i]) {
            if (max_file_number < __this->file_number[lock_dir][i]) {
                max_file_number = __this->file_number[lock_dir][i];
                max_index = i;
            }
        }
    }

    if (max_index < 0) {
        return NULL;
    }
    if (max_index != id && id >= 0) {
        /* 查看优先删除的文件夹是否满足删除条件 */
        if (__this->file_number[lock_dir][id] + 3 > __this->file_number[lock_dir][max_index]) {
            max_index = id;
        }
    }


    log_d("fselect file from dir %d, %d\n", lock_dir, max_index);


    if (__this->fscan[lock_dir][max_index]) {
        FILE *f = fselect(__this->fscan[lock_dir][max_index], FSEL_FIRST_FILE, 0);
        if (f) {

            if (lock_dir == 0) {
                if (video_rec_cmp_fname(__this->file[max_index], f)) {
                    fclose(f);
                    return NULL;
                }
            } else {
                __this->lock_fsize -= flen(f) / 1024;
                log_d("lock fsize - = %d\n", __this->lock_fsize);
            }

            __this->file_number[lock_dir][max_index]--;
            __this->old_file_number[lock_dir][max_index]--;
            if (__this->old_file_number[lock_dir][max_index] == 0) {
                video_rec_fscan_release(lock_dir);
            }
        }
#ifdef CONFIG_NET_ENABLE
        video_rec_delect_notify(f, -1);
#endif
        return f;
    } else {
        log_e("fscan[%d][%d] err", lock_dir, max_index);
        return NULL;
    }
    return NULL;
}

static void video_rec_rename_file(int id, FILE *file, int fsize, int format)
{
    char file_name[32];

    __this->new_file[id] = NULL;

#ifndef CONFIG_JLFAT_ENABLE
    int err = fcheck(file);
    if (err) {
        puts("fcheck fail\n");
#ifdef CONFIG_NET_ENABLE
        video_rec_delect_notify(file, -1);
#endif
        fdelete(file);
        return;
    }
#endif

    int size = flen(file);
    int persent = (size / 1024) * 100 / (fsize / 1024);

    printf("rename file: persent=%d, %d,%d\n", persent, size >> 20, fsize >> 20);

    if (persent >= 90 && persent <= 110) {
        sprintf(file_name, "%s%s", rec_dir[id][0] + strlen(CONFIG_ROOT_PATH) - 1, rec_file_name(format));

        printf("fmove: %d, %d, %s\n", id, format, file_name);

#ifdef CONFIG_NET_ENABLE
        video_rec_delect_notify(file, -1);
#endif

        int err = fmove(file, file_name, &__this->new_file[id], 1, strlen(file_name));
        if (err == 0) {
            fseek(__this->new_file[id], fsize, SEEK_SET);
            fseek(__this->new_file[id], 0, SEEK_SET);
            return;
        }
        puts("fmove_file_faild\n");
    }
#ifdef CONFIG_NET_ENABLE
    video_rec_delect_notify(file, -1);
#endif

    fdelete(file);
}
static int video_rec_create_file(int id, u32 fsize, int format, const char *path)
{
    FILE *file;
    int try_cnt = 0;
    char file_path[64];

    sprintf(file_path, "%s%s", path, rec_file_name(format));

    printf("id:%d , fopen: %s, min space %dMB\n", id, file_path, fsize >> 20);

    do {
        file = fopen(file_path, "w+");
        if (!file) {
            log_e("fopen faild\n");
            break;
        }
#ifdef CONFIG_NET_ENABLE
        goto __exit;//在写第一帧时候再seek整个文件大小
#endif
        if (fseek(file, fsize, SEEK_SET)) {
            goto __exit;
        }
        log_e("fseek faild\n");
        fdelete(file);

    } while (++try_cnt < 2);

    return -EIO;

__exit:
    fseek(file, 0, SEEK_SET);
    /*__this->new_file_size[id] = fsize;*/
    __this->new_file_size[id] = 0;
    __this->new_file[id] = file;

    return 0;
}

static int video_rec_del_old_file()
{
    int i;
    int err;
    FILE *file;
    int fsize[3] = {0, 0, 0};
    u32 cur_space;
    u32 need_space = 0;
    int cyc_time = db_select("cyc");
    int format[3] = { VIDEO0_REC_FORMAT, VIDEO1_REC_FORMAT, VIDEO2_REC_FORMAT};
    printf("cyc_time : %d\n", cyc_time);

#ifdef CONFIG_VIDEO0_ENABLE
    if (!__this->new_file[0]) {
        int res = db_select("res");
        res = (res > 1 || res < 0) ? 1 : res;
        fsize[0] =  video_rec_get_fsize(cyc_time, rec_pix_w[res], VIDEO0_REC_FORMAT);
        need_space += fsize[0];
    }
#endif

    err = fget_free_space(CONFIG_ROOT_PATH, &cur_space);
    if (err) {
        return err;
    }

    printf("space: %dMB, %dMB\n", cur_space / 1024, need_space / 1024 / 1024);

    if (cur_space >= 3 * (need_space / 1024)) {
        for (i = 0; i < 3; i++) {
            if (fsize[i] != 0) {
                err = video_rec_create_file(i, fsize[i], format[i], rec_path[i][0]);
                if (err) {
                    return err;
                }
            }
        }
        return 0;
    }


    while (1) {
        if (cur_space >= (need_space / 1024) * 3) {
            break;
        }
        file = video_rec_get_first_file(-1);
        if (!file) {
            return -ENOMEM;
        }
        fdelete(file);
        fget_free_space(CONFIG_ROOT_PATH, &cur_space);
    }

    for (i = 0; i < 3; i++) {
        if (fsize[i] != 0) {
            file = video_rec_get_first_file(i);
            if (file) {
                video_rec_rename_file(i, file, fsize[i], format[i]);
            }
            if (!__this->new_file[i]) {
                err = video_rec_create_file(i, fsize[i], format[i], rec_path[i][0]);
                if (err) {
                    return err;
                }
            }
        }
    }

    return 0;
}
/*
 * 判断SD卡是否挂载成功和簇大小是否大于32K
 */
static int storage_device_available()
{
    struct vfs_partition *part;

    if (storage_device_ready() == 0) {
        return false;
    } else {
        part = fget_partition(CONFIG_ROOT_PATH);
        printf("part_fs_attr: %x\n", part->fs_attr);
        if (part->clust_size < 32 || (part->fs_attr & F_ATTR_RO)) {
            return false;
        }
        __this->total_size = part->total_size;
    }

    return true;
}
static void video_rec_close_file(int dev_id)
{
    if (!__this->file[dev_id]) {
        return;
    }
#ifdef CONFIG_NET_ENABLE
    char is_emf = 0;
    char *path = video_rec_finish_get_name(__this->file[dev_id], dev_id, is_emf);
#endif

    fclose(__this->file[dev_id]);
    __this->file[dev_id] = NULL;

#ifdef CONFIG_NET_ENABLE
    if (path) { //必须关闭文件之后才能调用，否则在读取文件信息不全！！！
        video_rec_finish_notify(path);
    }
#endif
}
static int video0_rec_stop(u8 close)
{
    union video_req req;
    int err;

    log_d("video0_rec_stop\n");

    if (__this->video_rec0) {
        req.rec.channel = 0;
        req.rec.state = VIDEO_STATE_STOP;
        err = server_request(__this->video_rec0, VIDEO_REQ_REC, &req);
        if (err != 0) {
#ifdef CONFIG_NET_ENABLE
            video_rec_err_notify("VIDEO_REC_ERR");
#endif
            printf("\nstop rec err 0x%x\n", err);
            return VREC_ERR_V0_REQ_STOP;
        }
    }

    video_rec_close_file(0);

    if (close) {
        if (__this->video_rec0) {
            server_close(__this->video_rec0);
            __this->video_rec0 = NULL;
        }
    }

    return 0;
}
static int take_photo(void)
{
    int err;
#ifdef CONFIG_USR_VIDEO_ENABLE
    extern int user_video_rec0_open(void);
    extern int user_video_rec_take_photo(void);
    extern int user_video_rec0_close(void);
    err = user_video_rec0_open();
    if (err) {
        return -EINVAL;
    }
    os_time_dly(5);
    user_video_rec_take_photo();
    user_video_rec0_close();
    return 0;
#endif
    return -EINVAL;;
}
int video_rec_take_photo(void)
{
    struct server *server = NULL;
    union video_req req = {0};
    char *path;
    char buf[48];
    char name_buf[20];
    int err;

    if (__this->state == VIDREC_STA_START) {
        server = __this->video_rec0;
        if (!server) {
            printf("waring :video not open\n");
            return -EINVAL;
        }
        req.icap.quality = VIDEO_MID_Q;
        req.icap.buf_size = CAP_IMG_SIZE;
        req.icap.buf = malloc(CAP_IMG_SIZE);
        if (!req.icap.buf) {
            goto error;
        }
        req.rec.text_osd = NULL;
        req.rec.graph_osd = NULL;
        req.icap.text_label = NULL;
        req.icap.file_name = name_buf;
        req.icap.path = CAMERA0_CAP_PATH"IMG_****.jpg";
        path = CAMERA0_CAP_PATH;
        err = server_request(server, VIDEO_REQ_IMAGE_CAPTURE, &req);
        if (err != 0) {
            puts("\n\n\ntake photo err\n\n\n");
            goto error;
        }
        sprintf(buf, "%s%s", path, req.icap.file_name);
        printf("%s\n\n", buf);
        if (req.icap.buf) {
            free(req.icap.buf);
        }
        return 0;

error:
        if (req.icap.buf) {
            free(req.icap.buf);
        }
        return -EINVAL;
    } else if (__this->state != VIDREC_STA_STARTING && __this->state != VIDREC_STA_STOPING) {
        sys_timeout_add_to_task("sys_timer", NULL, take_photo, 10);
#if 0
#ifdef CONFIG_USR_VIDEO_ENABLE
        extern int user_video_rec0_open(void);
        extern int user_video_rec_take_photo(void);
        extern int user_video_rec0_close(void);
        err = user_video_rec0_open();
        if (err) {
            return -EINVAL;
        }
        os_time_dly(5);
        user_video_rec_take_photo();
        user_video_rec0_close();
        return 0;
#endif
#endif
    }
    return -EINVAL;
}

/*
 *注意：循环录像的时候，虽然要重新传参，但是要和start传的参数保持一致！！！
 */
static int video0_rec_savefile()
{
    union video_req req = {0};
    int err;

    if (!__this->file[0]) {
        return -ENOENT;
    }

    int res = db_select("res");
    res = (res > 1 || res < 0) ? 1 : res;

    int isc_size = 0;
    req.rec.isc_sbuf = get_net_video_isc_buf(&isc_size);
    req.rec.sbuf_size = isc_size;
    req.rec.channel = 0;
    req.rec.width 	= rec_pix_w[res];
    req.rec.height 	= rec_pix_h[res];
    req.rec.format 	= VIDEO0_REC_FORMAT;
    req.rec.state 	= VIDEO_STATE_SAVE_FILE;
    req.rec.file    = __this->file[0];

#ifdef CONFIG_NET_ENABLE
    req.rec.audio.sample_rate = video_rec_get_audio_sampel_rate();
#else
    req.rec.audio.sample_rate = 8000;
#endif
    req.rec.audio.channel = 1;
    req.rec.pkg_mute.aud_mute = !db_select("mic");

    req.rec.rec_small_pic   = 0;
    err = server_request(__this->video_rec0, VIDEO_REQ_REC, &req);
    if (err != 0) {
        log_e("rec0_save_file: err=%d\n", err);
        return err;
    }

    return 0;
}
static int video_rec_savefile(int dev_id)
{
    int i;
    int err;
    int post_msg = 0;
    union video_req req;
    printf(">>>>>> video_rec_savefile : 0x%x \n\n", dev_id);

    if (__this->state != VIDREC_STA_START) {
        return 0;
    }

    if (__this->need_restart_rec) {
        log_d("need restart rec");
        video_rec_stop(0);
        video_rec_start();
        return 0;
    }

    log_d("\nvideo_rec_start_new_file: %d\n", dev_id);

    video_rec_close_file(dev_id);

    if (__this->new_file[dev_id] == NULL) {
        err = video_rec_del_old_file();
        if (err) {
            goto __err;
        }
        post_msg = 1;
    }
    __this->file[dev_id]     = __this->new_file[dev_id];
    __this->new_file[dev_id] = NULL;

#ifdef CONFIG_VIDEO0_ENABLE
    if (dev_id == 0) {
        err = video0_rec_savefile();
        if (err) {
            goto __err;
        }
    }
#endif

    __this->state = VIDREC_STA_START;

#ifdef CONFIG_NET_ENABLE
    video_rec_state_notify();
#endif
    printf("rec_savefile ok .....\n\n");
    return 0;

__err:
#ifdef CONFIG_VIDEO0_ENABLE
    err = video0_rec_stop(0);
    if (err) {
        printf("\nsave wrong0 %x\n", err);
    }
#endif
    __this->state = VIDREC_STA_STOP;
    printf("rec_savefile err .....\n\n");
    return -EFAULT;
}
static void rec_dev_server_event_handler(void *priv, int argc, int *argv)
{
    /*
     *该回调函数会在录像过程中，写卡出错被当前录像APP调用，例如录像过程中突然拔卡
     */
    switch (argv[0]) {
    case VIDEO_SERVER_UVM_ERR:
        log_e("APP_UVM_DEAL_ERR\n");
        break;
    case VIDEO_SERVER_PKG_ERR:
        log_e("video_server_pkg_err\n");
        if (__this->state == VIDREC_STA_START) {
#ifdef CONFIG_NET_ENABLE
            video_rec_err_notify("VIDEO_REC_ERR");
            net_video_rec_event_stop();
#endif
            video_rec_stop(0);
#ifdef CONFIG_NET_ENABLE
            net_video_rec_event_start();
#endif
        }
        break;
    case VIDEO_SERVER_PKG_END:
        if (db_select("cyc") > 0) {
            video_rec_savefile((int)priv);
        } else {
#ifdef CONFIG_NET_ENABLE
            net_video_rec_event_stop();
#endif
            video_rec_stop(0);
#ifdef CONFIG_NET_ENABLE
            net_video_rec_event_start();
#endif
        }
        break;
    default :
        log_e("unknow rec server cmd %x , %x!\n", argv[0], (int)priv);
        break;
    }
}
#ifdef CONFIG_VIDEO0_ENABLE
static int video0_rec_start()
{
    int err;
    union video_req req = {0};
    struct video_text_osd text_osd = {0};
    struct video_graph_osd graph_osd;
    u16 max_one_line_strnum;

    puts("start_video_rec0\n");
    if (!__this->video_rec0) {
#ifdef CONFIG_UVC_VIDEO2_ENABLE
        __this->video_rec0 = server_open("video_server", "video2.0");
#else
        __this->video_rec0 = server_open("video_server", "video0.0");
#endif
        if (!__this->video_rec0) {
            return VREC_ERR_V0_SERVER_OPEN;
        }
        server_register_event_handler(__this->video_rec0, (void *)0, rec_dev_server_event_handler);
    }

    /*db_update("res",1);
    db_update("cyc",1);
    db_flush();*/
    int res = db_select("res");
    res = (res > 1 || res < 0) ? 1 : res;

    int isc_size = 0;
    req.rec.isc_sbuf = get_net_video_isc_buf(&isc_size);
    req.rec.sbuf_size = isc_size;
#ifdef CONFIG_UVC_VIDEO2_ENABLE
    req.rec.uvc_id = __this->uvc_id;
#endif
#ifdef CONFIG_VIDEO_REC_PPBUF_MODE
    req.rec.bfmode = VIDEO_PPBUF_MODE;
#endif
#ifdef  CONFIG_VIDEO_SPEC_DOUBLE_REC_MODE
    req.rec.wl80_spec_mode = VIDEO_WL80_SPEC_DOUBLE_REC_MODE;
#endif
#ifdef CONFIG_UI_ENABLE
    req.rec.picture_mode = 1;//整帧图片模式
#endif
    req.rec.channel     = 0;
    req.rec.camera_type = VIDEO_CAMERA_NORMAL;
    req.rec.width 	    = rec_pix_w[res];
    req.rec.height 	    = rec_pix_h[res];
    req.rec.format 	    = VIDEO0_REC_FORMAT;
    req.rec.state 	    = VIDEO_STATE_START;
    req.rec.file        = __this->file[0];
#ifdef CONFIG_NET_ENABLE
    req.rec.fsize = __this->new_file_size[0];
#endif

    req.rec.quality     = VIDEO_LOW_Q;
    req.rec.fps 	    = 0 ;
#ifdef CONFIG_NET_ENABLE
    req.rec.real_fps 	= video_rec_get_fps();
#else
    req.rec.real_fps 	= 0;
#endif
#ifdef CONFIG_NET_ENABLE
    req.rec.audio.sample_rate = video_rec_get_audio_sampel_rate();
#else
    req.rec.audio.sample_rate = 8000;
#endif
    req.rec.audio.channel 	= 1;
    req.rec.audio.volume    = AUDIO_VOLUME;
    req.rec.audio.buf = __this->audio_buf;
    req.rec.audio.buf_len = AUDIO_BUF_SIZE;
    req.rec.pkg_mute.aud_mute = !db_select("mic");

    /*
    *码率，I帧和P帧比例，必须是偶数（当录MOV的时候才有效）,
    *roio_xy :值表示宏块坐标， [6:0]左边x坐标 ，[14:8]右边x坐标，[22:16]上边y坐标，[30:24]下边y坐标,写0表示1个宏块有效
    * roio_ratio : 区域比例系数
    */
    req.rec.abr_kbps = video_rec_get_abr(req.rec.width);
    req.rec.IP_interval = 0;
    /*感兴趣区域为下方 中间 2/6 * 4/6 区域，可以调整
    	感兴趣区域qp 为其他区域的 70% ，可以调整
    */
    req.rec.roi.roio_xy = (req.rec.height * 5 / 6 / 16) << 24 | (req.rec.height * 3 / 6 / 16) << 16 | (req.rec.width * 5 / 6 / 16) << 8 | (req.rec.width) * 1 / 6 / 16;
    req.rec.roi.roi1_xy = (req.rec.height * 11 / 12 / 16) << 24 | (req.rec.height * 4 / 12 / 16) << 16 | (req.rec.width * 11 / 12 / 16) << 8 | (req.rec.width) * 1 / 12 / 16;
    req.rec.roi.roi2_xy = 0;
    req.rec.roi.roi3_xy = (1 << 24) | (0 << 16) | ((req.rec.width / 16) << 8) | 0;
    req.rec.roi.roio_ratio = 256 * 70 / 100 ;
    req.rec.roi.roio_ratio1 = 256 * 90 / 100;
    req.rec.roi.roio_ratio2 = 0;
    req.rec.roi.roio_ratio3 = 256 * 80 / 100;


    text_osd.font_w = OSD_DEFAULT_WIDTH;//必须16对齐
    text_osd.font_h = OSD_DEFAULT_HEIGHT;//必须16对齐
    text_osd.text_format = video_osd_format_buf;

    /*text_osd.text_format = NULL;//关闭OSD时间水印*/

    text_osd.x = (req.rec.width - text_osd.font_w * strlen(video_osd_format_buf) + 15) / 16 * 16;
    text_osd.y = (req.rec.height - text_osd.font_h + 15) / 16 * 16;
    text_osd.osd_yuv = 0xe20095;
    if (db_select("dat") > 0) {
        req.rec.text_osd = &text_osd;
    }

    req.rec.cycle_time = db_select("cyc");
    if (req.rec.cycle_time > 10 || req.rec.cycle_time <= 0) {
        req.rec.cycle_time = 3;
    }

    req.rec.buf = __this->video_buf[0];
    req.rec.buf_len = VREC0_FBUF_SIZE;
    req.rec.rec_small_pic   = 0;
    req.rec.cycle_time = req.rec.cycle_time * 60;
    printf(">>>>>>>video rec cyc time : %d min\n\n", req.rec.cycle_time / 60);
    err = server_request(__this->video_rec0, VIDEO_REQ_REC, &req);
    if (err != 0) {
        puts("\n\n\nstart rec err\n\n\n");
        return VREC_ERR_V0_REQ_START;
    }
    __this->state = VIDREC_STA_START;
    return 0;
}
#endif
static int video_rec_start()
{
    int err;
    u32 clust;
    u8 cnt = 0;
    u8 state = __this->state;
    int buf_size[] = {VREC0_FBUF_SIZE, VREC1_FBUF_SIZE, VREC2_FBUF_SIZE};

    __this->char_wait = 0;
    __this->need_restart_rec = 0;

    if (__this->state == VIDREC_STA_START) {
        return 0;
    }

    log_d("(((((( video_rec_start: in\n");

    if (!storage_device_available()) {
        return 0;
    }

    /*
     * 申请录像所需要的音频和视频帧buf
     */
    video_rec_buf_alloc();

    /*
     * 判断SD卡空间，删除旧文件并创建新文件
     */
redo:
    err = video_rec_del_old_file();
    if (err) {
        cnt++;
        if (cnt < 3) {
            log_d("retry ... \n\n");
            os_time_dly(50);
            goto redo;
        }
        log_e("start free space err\n");
        return VREC_ERR_START_FREE_SPACE;
    }

    for (int i = 0; i < 3; i++) {
        __this->file[i] = __this->new_file[i];
        __this->new_file[i] = NULL;
    }
#ifdef CONFIG_VIDEO0_ENABLE
    err = video0_rec_start();
    if (err) {
        video0_rec_stop(0);
        return err;
    }
#endif
    __this->state = VIDREC_STA_START;
    log_d("video_rec_start: out )))))))\n");
    return 0;
}
static void video0_rec_close()
{
    if (__this->video_rec0) {
        server_close(__this->video_rec0);
        __this->video_rec0 = NULL;
    }
}
static int video_rec_storage_device_ready(void *p)
{
    __this->sd_wait = 0;
#ifdef CONFIG_ENABLE_VLIST
    FILE_LIST_INIT(1);
#endif

    if ((int)p == 1) {
#ifdef CONFIG_NET_ENABLE
        video_rec_start_notify();//先停止网络实时流再录像,录像完毕再通知APP
#else
        video_rec_start();
#endif
    }
    return 0;
}
static int video_rec_stop(u8 close)
{
    int err;
    __this->need_restart_rec = 0;

    if (__this->state != VIDREC_STA_START) {
        return 0;
    }

    puts("\nvideo_rec_stop\n");

    __this->state = VIDREC_STA_STOPING;

#ifdef CONFIG_VIDEO0_ENABLE
    err = video0_rec_stop(close);
    if (err) {
        puts("\nstop0 err\n");
    }
#endif
    __this->state = VIDREC_STA_STOP;
    puts("video_rec_stop: exit\n");
    return 0;
}
static int video_rec_close()
{
#ifdef CONFIG_VIDEO0_ENABLE
    video0_rec_close();
#endif
    return 0;
}
static int video_rec_init()
{
    int err = 0;
    __this->sd_wait = wait_completion(storage_device_ready, video_rec_storage_device_ready, (void *)1, NULL);
    return err;
}
static int video_rec_uninit()
{
    int err;
    if (__this->state == VIDREC_STA_START) {
        return -EFAULT;
    }
    if (__this->sd_wait) {
        wait_completion_del(__this->sd_wait);
        __this->sd_wait = 0;
    }
    if (__this->char_wait) {
        wait_completion_del(__this->char_wait);
        __this->char_wait = 0;
    }
    if (__this->state == VIDREC_STA_START) {
        err = video_rec_stop(1);
    }
    video_rec_close();
    video_rec_fscan_release(0);
    video_rec_fscan_release(1);
    __this->state = VIDREC_STA_FORBIDDEN;
    __this->lan_det_setting = 0;
    return 0;
}

/*
 *录像的状态机,进入录像app后就是跑这里
 */
static int video_rec_state_machine(struct application *app, enum app_state state, struct intent *it)
{
    int err = 0;
    int len;

    switch (state) {
    case APP_STA_CREATE:
        log_d("\n >>>>>>> video_rec: create\n");

        memset(__this, 0, sizeof(struct video_rec_hdl));

        video_rec_buf_alloc();
        __this->state = VIDREC_STA_IDLE;

        break;
    case APP_STA_START:
        if (!it) {
            break;
        }
        switch (it->action) {
        case ACTION_VIDEO_REC_MAIN:
            puts("ACTION_VIDEO_REC_MAIN\n");
            video_rec_init();
            net_video_server_init();
            break;
        case ACTION_VIDEO_REC_SET_CONFIG:
            break;
        case ACTION_VIDEO_REC_CHANGE_STATUS:
            break;
        }
        break;
    case APP_STA_PAUSE:
        puts("--------app_rec: APP_STA_PAUSE\n");
        video_rec_fscan_release(0);
        video_rec_fscan_release(1);
        break;
    case APP_STA_RESUME:
        puts("--------app_rec: APP_STA_RESUME\n");
        break;
    case APP_STA_STOP:
        puts("--------app_rec: APP_STA_STOP\n");
        if (__this->state == VIDREC_STA_START) {
            video_rec_stop(0);
        }
#ifdef CONFIG_NET_ENABLE
        video_rec_all_stop_notify();
#endif
        if (video_rec_uninit()) {
            err = 1;
            break;
        }
        break;
    case APP_STA_DESTROY:
        puts("--------app_rec: APP_STA_DESTROY\n");
        if (video_rec_destroy()) {
            err = 2;
            break;
        }
        f_free_cache(CONFIG_ROOT_PATH);
        log_d("<<<<<<< video_rec: destroy\n");
        break;
    }

    return err;
}


/*
 *录像app的按键响应函数
 */
static int video_rec_key_event_handler(struct key_event *key)
{
    int err;

    switch (key->action) {
    case KEY_EVENT_CLICK:
        switch (key->value) {
        case KEY_OK:
            printf("video_rec_key_ok: %d\n", __this->state);
            break;
        case KEY_MENU:
            break;
        case KEY_MODE:
#ifdef CONFIG_NET_ENABLE
            video_rec_control_doing();
#endif
            puts("rec key mode\n");
            break;
        case KEY_UP:
            break;
        case KEY_DOWN:
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    return false;
}

static int video_rec_sd_in()
{
#ifdef CONFIG_NET_ENABLE
    video_rec_sd_event_ctp_notify(1);
#endif
#ifdef CONFIG_NET_ENABLE
    net_video_rec_status_notify();
#endif
    __this->lock_fsize_count = 0;

    return 0;
}

static int video_rec_sd_out()
{
    video_rec_fscan_release(0);
    video_rec_fscan_release(1);

    if (__this->sd_wait == 0) {
        __this->sd_wait = wait_completion(storage_device_ready,
                                          video_rec_storage_device_ready, (void *)1, NULL);
    }
#ifdef CONFIG_NET_ENABLE
    video_rec_sd_event_ctp_notify(0);
#endif
    return 0;
}
/*
 *录像app的设备响应函数
 */
static int video_rec_device_event_handler(struct device_event *event)
{
    int err;
    struct intent it;
    u8 *tpye;

    if (!ASCII_StrCmp(event->arg, "usb_host*", 9)) {
        switch (event->event) {
        case DEVICE_EVENT_IN:
            tpye = (u8 *)event->value;
            if (strstr(tpye, "uvc")) {
                __this->uvc_id = tpye[3] - '0';
            }
            printf("UVC or msd_storage online : %s, id=%d\n", tpye, __this->uvc_id);
            break;
        case DEVICE_EVENT_OUT:
            tpye = (u8 *)event->value;
            if (strstr(tpye, "uvc")) {
                __this->uvc_id = tpye[3] - '0';
            }
            printf("UVC or msd_storage offline : %s, id=%d\n", tpye, __this->uvc_id);
            if (__this->state == VIDREC_STA_START) {
                video_rec_stop(1);
            }
#ifdef CONFIG_NET_ENABLE
            net_video_rec_event_stop();
#endif
            break;
        }
    } else if (!ASCII_StrCmp(event->arg, "sd*", 4)) {
        switch (event->event) {
        case DEVICE_EVENT_IN:
            video_rec_sd_in();
            break;
        case DEVICE_EVENT_OUT:
            if (!fdir_exist(CONFIG_STORAGE_PATH)) {
                video_rec_sd_out();
            }
            break;
        }
    } else if (!ASCII_StrCmp(event->arg, "sys_power", 7)) {
        switch (event->event) {
        case DEVICE_EVENT_POWER_CHARGER_IN:
            puts("\n\ncharger in\n\n");
            break;
        case DEVICE_EVENT_POWER_CHARGER_OUT:
            puts("charger out\n");
            break;
        case DEVICE_EVENT_POWER_PERCENT:;//电池电量
            int battery_val = event->value;
            printf("battery_val = %d \n", battery_val);
            break;
        }
    } else if (!ASCII_StrCmp(event->arg, "parking", 7)) {
        switch (event->event) {
        case DEVICE_EVENT_IN:
            puts("parking on\n");	//parking on
            return true;
        case DEVICE_EVENT_OUT://parking off
            puts("parking off\n");
            return true;
        }
    } else if (!strcmp(event->arg, "camera0_err")) {
        log_e("camera0_err\n");
        if (__this->state == VIDREC_STA_START) {
            video_rec_stop(0);
            video_rec_start();
        }
#ifdef CONFIG_NET_ENABLE
        net_video_rec_event_start();
#endif

    }

    return false;
}

/*录像app的事件总入口*/
static int video_rec_event_handler(struct application *app, struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        return video_rec_key_event_handler((struct key_event *)event->payload);
    case SYS_DEVICE_EVENT:
        return video_rec_device_event_handler((struct device_event *)event->payload);
    case SYS_NET_EVENT:
        net_video_event_hander((void *)event->payload);
        return true;
    default:
        return false;
    }
}

static const struct application_operation video_rec_ops = {
    .state_machine  = video_rec_state_machine,
    .event_handler 	= video_rec_event_handler,
};

REGISTER_APPLICATION(app_video_rec) = {
    .name 	= "video_rec",
    .ops 	= &video_rec_ops,
    .state  = APP_STA_DESTROY,
};







