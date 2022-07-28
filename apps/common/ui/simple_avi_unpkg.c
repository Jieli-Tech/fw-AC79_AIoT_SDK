#include "generic/typedef.h"
#include "fs/fs.h"
#include "simple_avi_unpkg.h"
#include "os/os_api.h"
#include "app_config.h"


#define UNPKG_S_ERR() do {printf("err : %s  %d\n",__func__,__LINE__);return -1;}while(0)
#define UNPKG_U_ERR() do {printf("err : %s  %d\n",__func__,__LINE__);return 0;}while(0)
#define AVI_DBG()	printf("--->%s %d \n",__func__,__LINE__);

#define UNPKG_AVI_ERR() do {\
        printf("\n\nmake sure avi_net_playback_unpkg_init/avi_net_preview_unpkg_init func is used before use this function ,"\
        "and use avi_net_unpkg_exit func int the end!!!!\n\n");}while(0);

#define FD_READ_CHECK(len,idx)\
	if(len != sizeof(idx)){\
        fseek(fd, 0, SEEK_SET);\
        UNPKG_S_ERR();\
	}

#define FD_LEN_CHECK(offset,flen)\
	if(offset > flen){\
        fseek(fd, 0, SEEK_SET);\
		printf("offset = %d , flen = %d \n",offset,flen);\
        UNPKG_S_ERR();\
	}

#define IDX_00DC   ntohl(0x30306463)
#define IDX_01WB   ntohl(0x30317762)
#define IDX_00WB   ntohl(0x30307762)

#define REC_CYC_FILE_TIME 	10	 //分钟
#define AVI_AUDIO_NUM_TABLE		(REC_CYC_FILE_TIME * 60 * 2 + 128)

struct avi_head_str {
    UNPKG_JL_AVI_HEAD file_head;
    FILE *fd;
    u8 flag;
    u32 flen;
    u32 vdframe_cnt;
    u32 vdframe_offset_cnt;
    u32 adframe_cnt;
    u32 adframe_offset_cnt;
    u32 movi_addr;

    u32 audio_chunk_num;
    u32 video_chunk_num;

    u32 last_ad_num;
    u32 last_ad_sum;
    u32 last_vd_num;
    u32 last_vd_sum;

    u32 idx1_addr;
    u32 per_adframe_time;//每帧音频时间
    float video_num_coefficient;
    float audio_num_coefficient;
    u8 audio_num_buff[AVI_AUDIO_NUM_TABLE];//存储音频每帧偏移量，1s2帧，3分钟360帧
    u8 *video_num_buff;
};
static u8 avi_open_file_num = 0;
static struct avi_head_str avi_head_s[2];//0 preview , 1 playback

static int avi_get_video_audio_chunk_num(FILE *fd, u8 state);

static struct avi_head_str *avi_get_head_handler(u8 state)
{
    struct avi_head_str *avi_info = NULL;

    if (!state) {
        avi_info = &avi_head_s[0];
    } else {
        avi_info = &avi_head_s[1];
    }

    return avi_info;
}

static int avi_read_head(FILE *fd, u8 state)
{
    int len;
    struct avi_head_str *avi_info = avi_get_head_handler(state);
    memset(avi_info, 0, sizeof(struct avi_head_str));

    if (fd == NULL) {
        UNPKG_S_ERR();
    }

    if (avi_info->flag && avi_info->fd == fd) {
        return 0;
    }
    if (fseek(fd, 0, SEEK_SET)) {
        UNPKG_S_ERR();
    }
    len = fread(&avi_info->file_head, sizeof(UNPKG_JL_AVI_HEAD), 1, fd);
    if (len != sizeof(UNPKG_JL_AVI_HEAD)) {
        fseek(fd, 0, SEEK_SET);
        UNPKG_S_ERR();
    }
    avi_info->flen = flen(fd);
    avi_info->fd = fd;
    avi_info->flag = true;
    avi_info->movi_addr = 512 - 4;
    fseek(fd, 0, SEEK_SET);
    return 0;
}


int avi_net_playback_unpkg_init(FILE *fd, u8 state)//state : 0 preview , 1 playback
{
    if (fd) {
        avi_read_head(fd, state);
        avi_get_video_audio_chunk_num(fd, state);
        avi_open_file_num++;
        return 0;
    }
    return -1;
}
int avi_net_preview_unpkg_init(FILE *fd, u8 state)
{
    if (fd) {
        avi_read_head(fd, state);
        avi_open_file_num++;
        return 0;
    }
    return -1;
}
int avi_net_unpkg_exit(FILE *fd, u8 state)
{
    struct avi_head_str *avi_info = avi_get_head_handler(state);
    if (fd) {
        if (avi_open_file_num == 0) {
            avi_open_file_num = 0;
        } else {
            avi_open_file_num--;
        }
        if (avi_info->video_num_buff) {
            free(avi_info->video_num_buff);
            avi_info->video_num_buff = NULL;
        }
        /*memset(avi_info,0,sizeof(struct avi_head_str));*/
        return 0;
    }
    return -1;
}

int is_vaild_avi_file(FILE *fd, u8 state)
{
    int riff = ntohl(0x52494646);//"riff"
    int avi = ntohl(0x41564920);//"avi "
    int err;
    int fsize;
    struct avi_head_str *avi_info = avi_get_head_handler(state);

    if (!fd) {
        UNPKG_S_ERR();
    }
    if (avi_info->flag && avi_info->fd == fd) {
        goto get_data;
    } else {
        UNPKG_AVI_ERR();
        UNPKG_S_ERR();
    }

get_data:
    if (avi_info->file_head.avih.dwTotalFrames == 0 || avi_info->file_head.file_size == 0) {
        UNPKG_U_ERR();
    }
    fsize = flen(fd);
    if (avi_info->file_head.file_size > fsize) {
        printf("avi file is destroy !!!\n\n");
        return 0;
    }
    if (avi_info->file_head.riff == riff || avi_info->file_head.file_type == avi) {
        return 1;
    }
    return 0;
}

int avi_is_has_audio(FILE *fd, u8 state)
{
    int err;
    int auds = ntohl(0x61756473);//"auds"
    struct avi_head_str *avi_info = avi_get_head_handler(state);

    if (!fd) {
        UNPKG_S_ERR();
    }
    if (avi_info->flag && avi_info->fd == fd) {
        goto get_data;
    } else {
        UNPKG_AVI_ERR();
        UNPKG_S_ERR();
    }

get_data:
    if (avi_info->file_head.aud_strh.fccType == auds && avi_info->file_head.aud_strh.dwLength > 0) {
        return 0;
    }
    return -1;
}

int avi_get_fps(FILE *fd, u8 state)
{
    int fps;
    struct avi_head_str *avi_info = avi_get_head_handler(state);
    if (!fd) {
        UNPKG_S_ERR();
    }
    if (avi_info->flag && avi_info->fd == fd) {
        goto get_data;
    } else {
        UNPKG_AVI_ERR();
        UNPKG_S_ERR();
    }

get_data:
    fps = avi_info->file_head.vid_strh.dwRate;
//    printf("file fps : %d  \n",fps);
    return fps;
}

int avi_get_file_time(FILE *fd, u8 state)
{
    int time;
    struct avi_head_str *avi_info = avi_get_head_handler(state);
    if (!fd) {
        UNPKG_S_ERR();
    }
    if (avi_info->flag && avi_info->fd == fd) {
        goto get_data;
    } else {
        UNPKG_AVI_ERR();
        UNPKG_S_ERR();
    }

get_data:
    time = 1000000 / avi_info->file_head.avih.dwMicroSecPerFrame;
    time = avi_info->file_head.avih.dwTotalFrames / (time > 0 ? time : avi_info->file_head.avih.dwTotalFrames);
    time = time > 0 ? time : 1;
    /*printf("file time : %d \n", time);*/
    return time;
}

int avi_get_width_height(FILE *fd, void *__info, u8 state)
{
    AVI_UNPKG_INFO *info = (AVI_UNPKG_INFO *)__info;
    int err;
    struct avi_head_str *avi_info = avi_get_head_handler(state);
    if (!fd) {
        UNPKG_S_ERR();
    }
    if (avi_info->flag && avi_info->fd == fd) {
        goto get_data;
    } else {
        UNPKG_AVI_ERR();
        UNPKG_S_ERR();
    }

get_data:
    info->height = avi_info->file_head.avih.dwHeight;
    info->length = avi_info->file_head.avih.dwWidth;
    /*printf("height : %d , length : %d  \n",info->height,info->length);*/
    return 0;
}

int avi_get_audio_sample_rate(FILE *fd, u8 state)
{
    int err;
    int rate;
    struct avi_head_str *avi_info = avi_get_head_handler(state);
    if (!fd) {
        UNPKG_S_ERR();
    }
    if (avi_info->flag && avi_info->fd == fd) {
        goto get_data;
    } else {
        UNPKG_AVI_ERR();
        UNPKG_S_ERR();
    }

get_data:
    rate = avi_info->file_head.aud_strh.dwRate;
    /*if(!avi_info->audio_chunk_num)*/
    /*return 0;*/
//    printf("rate : %d  \n",rate);
    return rate;

}

int avi_audio_base_to_get_video_frame(u32 audio_num, u8 state) //线性补偿,audio_num:0,1....n
{
    double vd_num;
    struct avi_head_str *avi_info = avi_get_head_handler(state);
    if (avi_info->video_num_coefficient < 0.000001) {
        return 0;
    }
    audio_num += 1;
    vd_num = (double)(avi_info->video_num_coefficient * audio_num);
    return (int)vd_num;
}
int avi_video_base_to_get_audio_frame(int vd_num, u8 state) //线性补偿,vd_num:0,1.....n
{
    double ad_num;
    struct avi_head_str *avi_info = avi_get_head_handler(state);
    if (avi_info->audio_num_coefficient < 0.000001) {
        return 0;
    }
    vd_num += 1;
    ad_num = (double)(avi_info->audio_num_coefficient * vd_num);
    return (int)ad_num;
}

static int avi_get_audio_perfram_size(FILE *fd, int addr, u8 state)
{
    AVI_INDEX_INFO index_info = {0};
    int index_info_len = sizeof(AVI_INDEX_INFO);
    int i;
    int cnt = 0;
    int len;

    if (fd == NULL) {
        UNPKG_S_ERR();
    }

read_vd:
    if (ftell(fd) > (flen(fd) - sizeof(AVI_INDEX_INFO))) {
        return -1;
    }
    if (fseek(fd, addr + cnt * index_info_len, SEEK_SET)) {
        UNPKG_S_ERR();
    }
    len = fread(&index_info, index_info_len, 1, fd);
    if (len != index_info_len) {
        UNPKG_S_ERR();
    }
    if ((index_info.dwChunkId == 0 && index_info.dwOffset == 0 && index_info.dwSize == 0)
        || (index_info.dwChunkId != IDX_00DC && index_info.dwChunkId != IDX_01WB && index_info.dwChunkId != IDX_00WB)) {
        return 0;
    }
    if (index_info.dwSize == 0) {
        cnt++;
        goto read_vd;
    }
    if (index_info.dwChunkId == IDX_01WB || index_info.dwChunkId == IDX_00WB) {
        return index_info.dwSize;
    }
    cnt++;
    goto read_vd;
    return -1;
}

static int avi_other_find_index(FILE *fd, u8 state)
{
    int offset;
    int index_addr;
    int idx1 = ntohl(0x69647831);//"index1"
    int hdrl = ntohl(0x6864726c);//"hdrl"
    int avih = ntohl(0x61766968);//"avih"
    int list = ntohl(0x4c495354);//"list"
    int strl = ntohl(0x7374726c);//"strl"
    int strh = ntohl(0x73747268);//"strh"
    int strf = ntohl(0x73747266);//"strf"
    int junk = ntohl(0x4a554e4b);//"junk"
    int movi = ntohl(0x6d6f7669);//"movi"
    int len;
    int nidx_len;
    int nidx_idx;
    int err;
    int addr;
    float vd_presec_ms;
    float ad_presec_ms;
    struct avi_head_str *avi_info = avi_get_head_handler(state);
    u32 offset_addr;
    u32 fd_len;

    offset_addr = 16;
    fseek(fd, 0, SEEK_END);

    avi_info->flen = flen(fd);
    if (avi_info->flen <= 0 /* || avi_info->flen == 32768*/) { //文件系统BUG，读取方式是"rb"时候flen = 32768
        return -1;
    }
    fd_len = avi_info->flen;
    fseek(fd, offset_addr, SEEK_SET);
    len = fread(&nidx_len, sizeof(nidx_len), 1, fd);
    FD_READ_CHECK(len, nidx_len);
    avi_info->file_head.hdrl_size = nidx_len;

    offset_addr += sizeof(nidx_len);
    fseek(fd, offset_addr, SEEK_SET);
    len = fread(&nidx_idx, sizeof(nidx_idx), 1, fd);
    FD_READ_CHECK(len, nidx_idx);
    /*printf("---> next index = 0x%x , offset = 0x%x, line = %d\n",nidx_idx, ftell(fd),__LINE__);*/
    if (nidx_idx != hdrl) {
        UNPKG_S_ERR();
    }
    /*printf("--->hdrl \n");*/
    /*printf("---> next index = 0x%x , offset = 0x%x, line = %d\n",nidx_idx, ftell(fd),__LINE__);*/
    len = fread(&nidx_idx, sizeof(nidx_idx), 1, fd);
    FD_READ_CHECK(len, nidx_idx);
    /*printf("---> next index = 0x%x , offset = 0x%x, line = %d\n",nidx_idx, ftell(fd),__LINE__);*/
    if (avih != nidx_idx) {
        UNPKG_S_ERR();
    }
    len = fread(&nidx_len, sizeof(nidx_len), 1, fd);
    FD_READ_CHECK(len, nidx_len);
    /*printf("--->avih , nidx_len = %d\n",nidx_len);*/
    fread(&avi_info->file_head.avih.dwMicroSecPerFrame, nidx_len, 1, fd);

    /*printf("---> next index = 0x%x , offset = 0x%x, line = %d\n",nidx_idx, ftell(fd),__LINE__);*/
    len = fread(&nidx_idx, sizeof(nidx_idx), 1, fd);
    FD_READ_CHECK(len, nidx_idx);
    /*printf("---> next index = 0x%x , offset = 0x%x, line = %d\n",nidx_idx, ftell(fd),__LINE__);*/
    if (list != nidx_idx) {
        UNPKG_S_ERR();
    }

    /*printf("--->list video\n");*/
    fread(&avi_info->file_head.strl_vidsize, sizeof(int), 1, fd);

    len = fread(&nidx_idx, sizeof(nidx_idx), 1, fd);
    FD_READ_CHECK(len, nidx_idx);
    /*printf("---> next index = 0x%x , offset = 0x%x, line = %d\n",nidx_idx, ftell(fd),__LINE__);*/
    if (strl != nidx_idx) {
        UNPKG_S_ERR();
    }
    /*printf("--->strl\n");*/
    len = fread(&nidx_idx, sizeof(nidx_idx), 1, fd);
    FD_READ_CHECK(len, nidx_idx);
    if (strh != nidx_idx) {
        UNPKG_S_ERR();
    }
    len = fread(&nidx_len, sizeof(nidx_len), 1, fd);
    FD_READ_CHECK(len, nidx_len);
    fread(&avi_info->file_head.vid_strh.fccType, nidx_len, 1, fd);
    /*printf("---> next index = 0x%x , offset = 0x%x, line = %d\n",nidx_idx, ftell(fd),__LINE__);*/

    /*printf("--->strf\n");*/
    len = fread(&nidx_idx, sizeof(nidx_idx), 1, fd);
    FD_READ_CHECK(len, nidx_idx);
    /*printf("---> next index = 0x%x , offset = 0x%x, line = %d\n",nidx_idx, ftell(fd),__LINE__);*/
    if (strf != nidx_idx) {
        UNPKG_S_ERR();
    }
    len = fread(&nidx_len, sizeof(nidx_len), 1, fd);
    FD_READ_CHECK(len, nidx_len);
    fread(&avi_info->file_head.vid_strf.biSize, nidx_len, 1, fd);
    /*printf("---> next index = 0x%x , offset = 0x%x, line = %d\n",nidx_idx, ftell(fd),__LINE__);*/

    offset_addr = ftell(fd);
    while (1) {
        FD_LEN_CHECK(offset_addr, fd_len);
        fseek(fd, offset_addr, SEEK_SET);
        len = fread(&nidx_idx, sizeof(nidx_idx), 1, fd);
        FD_READ_CHECK(len, nidx_idx);
        offset_addr += sizeof(nidx_idx);
        if (list == nidx_idx) {
            break;
        }
        fseek(fd, offset_addr, SEEK_SET);
        FD_LEN_CHECK(offset_addr, fd_len);
        len = fread(&nidx_len, sizeof(nidx_len), 1, fd);
        FD_READ_CHECK(len, nidx_len);
        offset_addr += sizeof(nidx_idx);
        offset_addr += nidx_len;
    }
    /*printf("--->list audio\n");*/
    fread(&avi_info->file_head.strl_audsize, sizeof(int), 1, fd);

    len = fread(&nidx_idx, sizeof(nidx_idx), 1, fd);
    FD_READ_CHECK(len, nidx_idx);
    if (strl != nidx_idx) {
        UNPKG_S_ERR();
    }
    /*printf("--->strl audio\n");*/
    len = fread(&nidx_idx, sizeof(nidx_idx), 1, fd);
    FD_READ_CHECK(len, nidx_idx);
    if (strh != nidx_idx) {
        UNPKG_S_ERR();
    }
    len = fread(&nidx_len, sizeof(nidx_len), 1, fd);
    FD_READ_CHECK(len, nidx_len);
    fread(&avi_info->file_head.aud_strh.fccType, nidx_len, 1, fd);

    /*printf("---> next index = 0x%x , offset = 0x%x, line = %d\n",nidx_idx, ftell(fd),__LINE__);*/
    /*printf("--->strf audio\n");*/
    len = fread(&nidx_idx, sizeof(nidx_idx), 1, fd);
    FD_READ_CHECK(len, nidx_idx);
    if (strf != nidx_idx) {
        UNPKG_S_ERR();
    }
    len = fread(&nidx_len, sizeof(nidx_len), 1, fd);
    FD_READ_CHECK(len, nidx_len);
    fread(&avi_info->file_head.aud_strf.wFormatTag, nidx_len, 1, fd);
    /*printf("---> next index = 0x%x , offset = 0x%x, line = %d\n",nidx_idx, ftell(fd),__LINE__);*/

    offset_addr = ftell(fd);
    while (1) {
        FD_LEN_CHECK(offset_addr, fd_len);
        fseek(fd, offset_addr, SEEK_SET);
        len = fread(&nidx_idx, sizeof(nidx_idx), 1, fd);
        FD_READ_CHECK(len, nidx_idx);
        offset_addr += sizeof(nidx_idx);
        if (idx1 == nidx_idx) {
            offset_addr -= sizeof(nidx_len);
            fseek(fd, offset_addr, SEEK_SET);
            break;
        }
        FD_LEN_CHECK(offset_addr, fd_len);
        fseek(fd, offset_addr, SEEK_SET);
        len = fread(&nidx_len, sizeof(nidx_len), 1, fd);
        FD_READ_CHECK(len, nidx_len);
        offset_addr += sizeof(nidx_len);
        if (list == nidx_idx) {
            FD_LEN_CHECK(offset_addr, fd_len);
            fseek(fd, offset_addr, SEEK_SET);
            len = fread(&nidx_idx, sizeof(nidx_idx), 1, fd);
            FD_READ_CHECK(len, nidx_idx);
            if (movi == nidx_idx) {
                avi_info->movi_addr =  ftell(fd) - 4;
            }
        }
        offset_addr += nidx_len;
    }
    printf("get index ok ,avi_info->movi_addr = 0x%x\n", avi_info->movi_addr);
    return 0;
}


static u32 avi_find_index_addr(FILE *fd, u8 state)
{
    int offset;
    int index_addr;
    int idx1 = ntohl(0x69647831);//"index1"
    int len;
    int err;
    int addr;
    float vd_presec_ms;
    float ad_presec_ms;
    struct avi_head_str *avi_info = avi_get_head_handler(state);

    struct idex1_str {
        int str;
        int len;
    } idex1_head = {0};

    if (fd == NULL) {
        UNPKG_U_ERR();
    }

    if (avi_info->flag && avi_info->fd == fd) {
        goto get_data;
    } else {
        UNPKG_AVI_ERR();
        UNPKG_S_ERR();
    }

get_data:
    if (avi_info->idx1_addr) {
        return avi_info->idx1_addr;
    }

    // 2 . 读取AVI文件头
    if (!avi_info->flag || avi_info->fd != fd) {
        UNPKG_U_ERR();
    }

    index_addr = avi_info->file_head.len - 4;//音视频数据内容大小
    addr  = sizeof(UNPKG_JL_AVI_HEAD) + index_addr;

    //3 . 跳转到索引表处
    if (fseek(fd, addr, SEEK_SET)) {
        fseek(fd, 0, SEEK_SET);
        UNPKG_U_ERR();
    }

read_index:
    //4 . 读取"inde1"块的头和整个索引表的长度
    len = fread(&idex1_head, sizeof(struct idex1_str), 1, fd);
    addr += sizeof(struct idex1_str);
    if (len != sizeof(struct idex1_str)) {
        fseek(fd, 0, SEEK_SET);
        UNPKG_U_ERR();
    }

    if (idex1_head.str != idx1) {
        fseek(fd, 0, SEEK_SET);
        if (!avi_other_find_index(fd, state)) {
            addr = ftell(fd);
            /*printf("--->get addr = 0x%x\n",addr);*/
            goto read_index;
        }
        UNPKG_U_ERR();
    }

    if (!avi_info->per_adframe_time) {
        len = avi_get_audio_perfram_size(fd, addr, state);
        if (len > 0) {
            avi_info->per_adframe_time = ((float)len / (avi_info->file_head.aud_strh.dwRate * avi_info->file_head.aud_strf.nBlockAlign)) * 1000;
            ad_presec_ms = (float)((float)len / (avi_info->file_head.aud_strh.dwRate * avi_info->file_head.aud_strf.nBlockAlign)) * 1000;
            vd_presec_ms = (float)avi_info->file_head.avih.dwMicroSecPerFrame / 1000;
            avi_info->video_num_coefficient = (float)ad_presec_ms / vd_presec_ms;
            avi_info->audio_num_coefficient = (float)vd_presec_ms / ad_presec_ms;
            /*printf("ad_presec_ms : %f , %f , %f \n",ad_presec_ms,vd_presec_ms,avi_info->video_num_coefficient);*/
        } else {
            printf("get AVI audio frame err!!!!!  \n");
            avi_info->per_adframe_time = ((float)8184 / (8000 * 2)) * 1000; //ms
        }
    }
    //5 . 返回第一个索引表地址
    avi_info->idx1_addr = addr;

    //6 . 恢复文件指针
    fseek(fd, 0, SEEK_SET);
    return avi_info->idx1_addr;
}

static int avi_audio_get_offset_cnt(FILE *fd, int num, u8 state) //第一帧从1开始，...
{
    int i;
    int addr = 0;
    struct avi_head_str *avi_info = avi_get_head_handler(state);

    if (!avi_info->flag || avi_info->fd != fd) {
        UNPKG_AVI_ERR();
        UNPKG_S_ERR();
    }
    if (num > avi_info->audio_chunk_num) {
        return -1;
    }
    if (num <= 1) {
        avi_info->last_ad_num = 0;
        avi_info->last_ad_sum = 0;
        num = 1;
    }
    if (num < avi_info->last_ad_num) {
        for (i = 0; i < num; i++) { //任意帧
            addr += avi_info->audio_num_buff[i];
        }
    } else {
        addr += avi_info->last_ad_sum;
        for (i = avi_info->last_ad_num; i < num; i++) { //升序帧，第一帧从1开始，...
            addr += avi_info->audio_num_buff[i];
        }
    }
    avi_info->last_ad_num = num;
    avi_info->last_ad_sum = addr;
    return addr;
}
static int avi_video_get_offset_cnt(FILE *fd, int num, u8 state) //第一帧从1开始，...
{
    int i;
    int addr = 0;
    struct avi_head_str *avi_info = avi_get_head_handler(state);

    if (!avi_info->flag || avi_info->fd != fd) {
        UNPKG_AVI_ERR();
        UNPKG_S_ERR();
    }

    if (num <= 1) {
        avi_info->last_vd_num = 0;
        avi_info->last_vd_sum = 0;
        num = 1;
        return 0;
    }

    if (num > avi_info->video_chunk_num && !state) {  //回放模式
        return -1;
    }

    if (num < avi_info->last_vd_num) {
        for (i = 0; i < num; i++) { //任意帧
            addr += avi_info->video_num_buff[i];
        }
    } else {
        addr += avi_info->last_vd_sum;
        for (i = avi_info->last_vd_num; i < num; i++) { //升序帧，第一帧从1开始，...
            addr += avi_info->video_num_buff[i];
        }
    }
    avi_info->last_vd_num = num;
    avi_info->last_vd_sum = addr;
    return addr;
}

static int avi_get_video_audio_chunk_num(FILE *fd, u8 state) //获取整个视频的音视频帧数
{
    int num = 0;
    int cnt = 0;
    AVI_INDEX_INFO index_info = {0};
    int index_info_len = sizeof(AVI_INDEX_INFO);
    int addr;
    int len;
    int start = 0, end = 0;
    int vdstart = 0, vdend = 0;
    struct avi_head_str *avi_info = avi_get_head_handler(state);

    if (avi_info->audio_chunk_num) {
        return avi_info->audio_chunk_num;
    }

    addr = (int)avi_find_index_addr(fd, state);
    if (addr <= 0) {
        UNPKG_S_ERR();
    }
    if (fseek(fd, addr + cnt * index_info_len, SEEK_SET)) {
        UNPKG_S_ERR();
    }

    if (!avi_info->video_num_buff) {
        avi_info->video_num_buff = zalloc(avi_info->file_head.avih.dwTotalFrames);//回放模式
    }
    if (!avi_info->video_num_buff) {
        printf("%s no mem err !!!!!\n", __func__);
        UNPKG_S_ERR();
    }

    for (;;) {
        if (ftell(fd) >= avi_info->flen) {
            break;
        }
        len = fread(&index_info, index_info_len, 1, fd);
        if (len != index_info_len) {
            UNPKG_S_ERR();
        }
        if ((index_info.dwChunkId == 0 && index_info.dwOffset == 0 && index_info.dwSize == 0)
            || (index_info.dwChunkId != IDX_00DC && index_info.dwChunkId != IDX_01WB && index_info.dwChunkId != IDX_00WB)) {
            break;
        }

        if (index_info.dwChunkId == IDX_01WB || index_info.dwChunkId == IDX_00WB) {
            start = cnt + num;
            if (num < sizeof(avi_info->audio_num_buff)) {
                avi_info->audio_num_buff[num] =  start - end;
            }
            end = start;
            num++;
            avi_info->audio_chunk_num = num;
        } else {
            vdstart = cnt + num;
            if (cnt < avi_info->file_head.avih.dwTotalFrames) {
                avi_info->video_num_buff[cnt] = vdstart - vdend;
            }
            vdend = vdstart;
            cnt++;
        }
    }
    avi_info->video_chunk_num = cnt;
    printf("audio_chunk_num num : %d \n", num);
    printf("video_chunk_num num : %d \n", cnt);
    return num;
}
int avi_get_video_num(FILE *fd, int offset_ms, u8 state)
{
    int cnt;
    struct avi_head_str *avi_info = avi_get_head_handler(state);
    if (!avi_info->flag || avi_info->fd != fd) {
        UNPKG_AVI_ERR();
        UNPKG_S_ERR();
    }
    cnt = offset_ms / (avi_info->file_head.avih.dwMicroSecPerFrame / 1000); //计算对应帧数
    return cnt;
}
int avi_get_audio_chunk_num(FILE *fd, u8 state) //获取整个视频的音频帧数
{
    struct avi_head_str *avi_info = avi_get_head_handler(state);
    return avi_info->audio_chunk_num;
}
int avi_get_video_chunk_num(FILE *fd, u8 state) //获取整个视频的音频帧数
{
    struct avi_head_str *avi_info = avi_get_head_handler(state);
    return avi_info->video_chunk_num;
}
//读对应视频帧数
int avi_video_get_frame(FILE *fd, int offset_num, u8 *buf, u32 buf_len, u8 state)
{
    int movi = ntohl(0x6d6f7669);//"movi"
    u32 addr;
    int len;
    int all_len;
    int idx_send_cnt;
    int idx_offset;
    AVI_INDEX_INFO index_info = {0};
    int index_info_len = sizeof(AVI_INDEX_INFO);
    int i;
    int cnt = 0;
    int offset_addr;
    struct avi_head_str *avi_info = avi_get_head_handler(state);
    struct idex1_str {
        int str;
        int len;
    } idex1_head = {0};

    if (fd == NULL) {
        UNPKG_S_ERR();
    }
    addr = avi_find_index_addr(fd, state);
    if (!addr) {
        UNPKG_S_ERR();
    }
    avi_info->vdframe_offset_cnt = offset_num;//计算对应帧数
    if (avi_info->vdframe_offset_cnt > avi_info->file_head.avih.dwTotalFrames) {
        /*avi_info->vdframe_offset_cnt = avi_info->file_head.avih.dwTotalFrames;*/
        return -1;
    }
    cnt = avi_video_get_offset_cnt(avi_info->fd, avi_info->vdframe_offset_cnt, state);
    /*printf("video cnt : %d , offset_num : %d \n",cnt,offset_num);*/
    if (cnt < 0) {
        return -1;
    }
read_vd:
    offset_addr = addr + cnt * index_info_len;
    if (offset_addr > avi_info->flen) {
        return 0;
    }
    if (fseek(fd, offset_addr, SEEK_SET)) {
        UNPKG_S_ERR();
    }
    len = fread(&index_info, index_info_len, 1, fd);
    if (len != index_info_len) {
        UNPKG_S_ERR();
    }
    if ((index_info.dwChunkId == 0 && index_info.dwOffset == 0 && index_info.dwSize == 0)
        || (index_info.dwChunkId != IDX_00DC && index_info.dwChunkId != IDX_01WB && index_info.dwChunkId != IDX_00WB)) {
        printf("end of video AVI file ... \n");
        return 0;
    }
    if (index_info.dwSize == 0) {
        cnt++;
        goto read_vd;
    }
    if (index_info.dwChunkId == IDX_00DC) {
        idx_offset = index_info.dwOffset + avi_info->movi_addr + sizeof(struct idex1_str);
        if (offset_addr > avi_info->flen || offset_addr < 0) {
            UNPKG_S_ERR();
        }
        if (fseek(fd, idx_offset, SEEK_SET)) {
            printf("idx_offset : %d , index_info.dwOffset : %d \n", idx_offset, index_info.dwOffset);
            UNPKG_S_ERR();
        }
        all_len  = 0;
    } else {
        cnt++;
        goto read_vd;
    }
    /*printf("video frame len : %d \n", index_info.dwSize);*/
    if (index_info.dwSize > buf_len) {
        printf("\n\n\n!!!!!err video data buff len not enough , need : %d ,but buflen is %d !!!!!!!!!!!\n\n", index_info.dwSize, buf_len);
        /*ASSERT(0, "!err video data buff len not enough");*/
        return 0;
    }
    /*printf("--->idx_offset = 0x%x ,index_info.dwOffset = 0x%x, index_info.dwSize = %d\n",idx_offset,index_info.dwOffset,index_info.dwSize);*/

    all_len = fread(buf, index_info.dwSize, 1, fd);

    /*printf("video frame all_len : %d \n", all_len);*/
    if (all_len != index_info.dwSize) {
        UNPKG_S_ERR();
    }
    avi_info->vdframe_cnt = offset_num;
    return all_len;
}
int avi_video_set_frame_addr(FILE *fd, int offset_num, u8 state)
{
    int movi = ntohl(0x6d6f7669);//"movi"
    u32 addr;
    int len;
    int all_len;
    int idx_send_cnt;
    int idx_offset;
    AVI_INDEX_INFO index_info = {0};
    int index_info_len = sizeof(AVI_INDEX_INFO);
    int i;
    int cnt = 0;
    int offset_addr;
    struct avi_head_str *avi_info = avi_get_head_handler(state);
    struct idex1_str {
        int str;
        int len;
    } idex1_head = {0};

    if (fd == NULL) {
        UNPKG_S_ERR();
    }
    addr = avi_find_index_addr(fd, state);
    if (!addr) {
        UNPKG_S_ERR();
    }
    avi_info->vdframe_offset_cnt = offset_num;//计算对应帧数
    if (avi_info->vdframe_offset_cnt > avi_info->file_head.avih.dwTotalFrames) {
        avi_info->vdframe_offset_cnt = avi_info->file_head.avih.dwTotalFrames;
        /*UNPKG_S_ERR();*/
    }
    cnt = avi_video_get_offset_cnt(avi_info->fd, offset_num, state);
    if (cnt < 0) {
        return -1;
    }
//    printf("cnt : %d ,vdframe_offset_cnt : %d\n",cnt,avi_info->vdframe_offset_cnt);
read_vd:
    offset_addr = addr + cnt * index_info_len;
    if (offset_addr > avi_info->flen) {
        return 0;
    }
    if (fseek(fd, offset_addr, SEEK_SET)) {
        UNPKG_S_ERR();
    }
    len = fread(&index_info, index_info_len, 1, fd);
    if (len != index_info_len) {
        UNPKG_S_ERR();
    }
    if ((index_info.dwChunkId == 0 && index_info.dwOffset == 0 && index_info.dwSize == 0)
        || (index_info.dwChunkId != IDX_00DC && index_info.dwChunkId != IDX_01WB && index_info.dwChunkId != IDX_00WB)) {
        printf("end of video AVI file ... \n");
        return 0;
    }
    if (index_info.dwSize == 0) {
        cnt++;
        goto read_vd;
    }
    if (index_info.dwChunkId == IDX_00DC) {
        idx_offset = index_info.dwOffset + avi_info->movi_addr + sizeof(struct idex1_str);
        if (offset_addr > avi_info->flen || offset_addr < 0) {
            UNPKG_S_ERR();
        }
        if (fseek(fd, idx_offset, SEEK_SET)) {
            UNPKG_S_ERR();
        }
    } else {
        cnt++;
        /*avi_info->adframe_cnt++;*/
        goto read_vd;
    }
    return index_info.dwSize;
}


//读对应音频帧数
int avi_audio_get_frame(FILE *fd, int offset_num, u8 *buf, u32 buf_len, u8 state)  //offset_num : 1....n
{
    u32 addr;
    int len;
    int all_len;
    int movi = ntohl(0x6d6f7669);//"movi"
    int idx_send_cnt;
    int idx_offset;
    AVI_INDEX_INFO index_info = {0};
    int index_info_len = sizeof(AVI_INDEX_INFO);
    int i;
    int cnt = 0;
    int offset_addr;
    int last_addr = 0;
    struct avi_head_str *avi_info = avi_get_head_handler(state);
    struct idex1_str {
        int str;
        int len;
    } idex1_head = {0};
    if (fd == NULL) {
        UNPKG_S_ERR();
    }
    addr = avi_find_index_addr(fd, state);
    if (!addr) {
        UNPKG_S_ERR();
    }
    if (offset_num > avi_info->audio_chunk_num) {
        /*printf("Warnning : offset_num > avi_info->audio_chunk_num \n");*/
        return 0;
    }
    last_addr = ftell(fd);
    avi_info->adframe_offset_cnt = 0;
    cnt = avi_audio_get_offset_cnt(fd, offset_num, state);
    if (cnt < 0) {
        return -1;
    }
    /*printf("audio cnt : %d \n",cnt);*/
read_vd:
    offset_addr = addr + cnt * index_info_len;
    if (offset_addr > avi_info->flen) {
        fseek(fd, last_addr, SEEK_SET);
        return 0;
    }
    if (fseek(fd, offset_addr, SEEK_SET)) {
        fseek(fd, last_addr, SEEK_SET);
        UNPKG_S_ERR();
    }
    len = fread(&index_info, index_info_len, 1, fd);
    if (len != index_info_len) {
        fseek(fd, last_addr, SEEK_SET);
        UNPKG_S_ERR();
    }
    if ((index_info.dwChunkId == 0 && index_info.dwOffset == 0 && index_info.dwSize == 0)
        || (index_info.dwChunkId != IDX_00DC && index_info.dwChunkId != IDX_01WB && index_info.dwChunkId != IDX_00WB)) {
        fseek(fd, last_addr, SEEK_SET);
        /*printf("end of audio AVI file ... \n");*/
        return 0;
    }
    if (index_info.dwSize == 0) {
        cnt++;
        goto read_vd;
    }
    if (index_info.dwChunkId == IDX_01WB || index_info.dwChunkId == IDX_00WB) {
        idx_offset = index_info.dwOffset + avi_info->movi_addr + sizeof(struct idex1_str);
        if (offset_addr > avi_info->flen || offset_addr < 0) {
            UNPKG_S_ERR();
        }
        if (fseek(fd, idx_offset, SEEK_SET)) {
            fseek(fd, last_addr, SEEK_SET);
            UNPKG_S_ERR();
        }
        all_len  = 0;
    } else {
        cnt++;
        goto read_vd;
    }
    if (index_info.dwSize > buf_len) {
        printf("\n\n\n!!!!!err audio data buff len not enough , need : %d ,but buflen is %d !!!!!!!!!!!\n\n", index_info.dwSize, buf_len);
        /*ASSERT(0, "!err audio data buff len not enough");*/
        return 0;
    }
    all_len = fread(buf, index_info.dwSize, 1, fd);
    if (all_len != index_info.dwSize) {
        fseek(fd, last_addr, SEEK_SET);
        UNPKG_S_ERR();
    }
    avi_info->adframe_cnt++;
    fseek(fd, last_addr, SEEK_SET);
    return all_len;
}

#if 1
#include "asm/jpeg_codec.h"
#include "system/includes.h"
#include "os/os_api.h"
#include "yuv_to_rgb.h"
#define AVI_UNPKG_DECODE_TEST	0

int storage_device_ready(void);
struct yuv_recv {
    volatile unsigned char *y;
    volatile unsigned char *u;
    volatile unsigned char *v;
    volatile int y_size;
    volatile int u_size;
    volatile int v_size;
    volatile int recv_size;
    volatile int size;
    volatile char complit;
    volatile int yuv_cb_cnt;
};
static int yuv_out_cb(void *priv, struct YUV_frame_data *p)
{
    struct yuv_recv *rec = (struct yuv_recv *)priv;
    u8 type = (p->pixformat == VIDEO_PIX_FMT_YUV444) ? 1 : ((p->pixformat == VIDEO_PIX_FMT_YUV422) ? 2 : 4);
    ++rec->yuv_cb_cnt;
    if (!rec->complit) {
        memcpy(rec->y + rec->y_size, p->y, p->width * p->data_height);
        memcpy(rec->u + rec->u_size, p->u, p->width * p->data_height / type);
        memcpy(rec->v + rec->v_size, p->v, p->width * p->data_height / type);
        rec->y_size += p->width * p->data_height;
        rec->u_size += p->width * p->data_height / type;
        rec->v_size += p->width * p->data_height / type;
        rec->recv_size +=  p->width * p->data_height + (p->width * p->data_height / type) * 2;
        if (rec->recv_size >= rec->size) {
            rec->complit = 1;
        }
    } else {
        printf("err in complit  = %d , type = %s \n", rec->complit, (type == 1) ? "YUV444" : (type == 2) ? "yuv422" : "YUV420");
    }
    return 0;
}
int avi_unpkg_decoder_to_yuv_example(void)
{
    int ret;
    char name[64];
    char *fbuf = NULL;
    char *yuv = NULL;
    char *cy, *cb, *cr;
    int fbuflen = 50 * 1024;
    FILE *fd = NULL;
    FILE *yuv_fd = NULL;
    int num = 0;
    int pix;
    char ytype;
    int yuv_len;

    while (!storage_device_ready()) {//等待sd文件系统挂载完成
        os_time_dly(2);
    }

    fbuf = malloc(fbuflen);
    if (!fbuf) {
        printf("no men fbuf err!!!\n");
        goto exit;
    }
    sprintf(name, "%sVID_001.AVI", CONFIG_ROOT_PATH);
    //sprintf(name, "%stest1.AVI", CONFIG_ROOT_PATH);
    fd = fopen(name, "r");
    if (!fd) {
        printf("avi file open err : %s !!!\n", name);
        goto exit;
    }
    ret = avi_net_playback_unpkg_init(fd, 1); //解码初始化,最多10分钟视频
    if (ret) {
        printf("avi_net_playback_unpkg_init err!!!\n");
        goto exit;
    }
redo:
    num = 0;
    while (1) {
#if 0
        now = timer_get_ms();
        if (last && (now - last) >= 1000) {
            printf("---> sec num = %d \n", num + 1);
            last = now;
        } else if (!last) {
            last = now;
        }
#endif
        ret = avi_video_get_frame(fd, ++num, fbuf, fbuflen, 1); //全回放功能获取帧
        if (ret > 0) {
            struct jpeg_image_info info = {0};
            struct jpeg_decode_req req = {0};
            u32 *head = (u32 *)fbuf;
            u8 *dec_buf = fbuf;
            u32 fblen = ret;
            if (*head == IDX_00DC || *head == IDX_01WB || *head == IDX_00WB) {
                fblen -= 8;
                dec_buf += 8;
            }
            info.input.data.buf = dec_buf;
            info.input.data.len = fblen;
            if (jpeg_decode_image_info(&info)) {//获取JPEG图片信息
                printf("jpeg_decode_image_info err\n");
                break;
            } else {
                switch (info.sample_fmt) {
                case JPG_SAMP_FMT_YUV444:
                    ytype = 1;
                    break;//444
                case JPG_SAMP_FMT_YUV420:
                    ytype = 4;
                    break;//420
                default:
                    ytype = 2;
                }
                pix = info.width * info.height;
                yuv_len = pix + pix / ytype * 2;
                if (!yuv) {
                    yuv = malloc(yuv_len);
                    if (!yuv) {
                        printf("yuv malloc err len : %d , width : %d , height : %d \n", yuv_len, info.width, info.height);
                        break;
                    }
                }
#if 1
                cy = yuv;
                cb = cy + pix;
                cr = cb + pix / ytype;

                req.input_type = JPEG_INPUT_TYPE_DATA;
                req.input.data.buf = info.input.data.buf;
                req.input.data.len = info.input.data.len;
                req.buf_y = cy;
                req.buf_u = cb;
                req.buf_v = cr;
                req.buf_width = info.width;
                req.buf_height = info.height;
                req.out_width = info.width;
                req.out_height = info.height;
                req.output_type = JPEG_DECODE_TYPE_DEFAULT;
                req.bits_mode = BITS_MODE_UNCACHE;
                req.dec_query_mode = TRUE;

                ret = jpeg_decode_one_image(&req);//JPEG转YUV解码
                if (ret) {
                    printf("jpeg decode err !!\n");
                    break;
                }
#else
                extern void *jpg_dec_open(struct video_format * f);
                extern int jpg_dec_input_data(void *_fh, void *data, u32 len);
                extern int jpg_dec_set_output_handler(void *_fh, void *priv, int (*handler)(void *, struct YUV_frame_data *));
                extern int jpg_dec_get_s_attr(void *_fh, struct jpg_dec_s_attr * attr);
                extern int jpg_dec_set_s_attr(void *_fh, struct jpg_dec_s_attr * attr);
                struct yuv_recv yuv_rec_data = {0};
                static void *fh = NULL;
                if (!fh) {
                    fh = jpg_dec_open(NULL);
                    if (!fh) {
                        printf("err in jpg_dec_open \n\n");
                        return 0;
                    }
                    struct jpg_dec_s_attr jpg_attr;
                    jpg_dec_get_s_attr(fh, &jpg_attr);
                    jpg_attr.max_o_width  = 320;
                    jpg_attr.max_o_height = 240;
                    jpg_dec_set_s_attr(fh, &jpg_attr);
                    jpg_dec_set_output_handler(fh, (void *)&yuv_rec_data, yuv_out_cb);
                }
                yuv_rec_data.y = yuv;
                yuv_rec_data.u = yuv_rec_data.y + pix;
                yuv_rec_data.v = yuv_rec_data.u + pix / ytype;
                yuv_rec_data.size = yuv_len;
                yuv_rec_data.recv_size = 0;
                yuv_rec_data.y_size = 0;
                yuv_rec_data.u_size = 0;
                yuv_rec_data.v_size = 0;
                yuv_rec_data.complit = 0;
                yuv_rec_data.yuv_cb_cnt = 0;
                ret = jpg_dec_input_data(fh, info.input.data.buf, info.input.data.len);
                if (ret) {
                    printf("jpg_dec_input_data waite err \n\n");
                    break;
                }

                if (!yuv_rec_data.complit) {
                    printf("yuv_cb_cnt=%d\n", yuv_rec_data.yuv_cb_cnt);
                    printf("err yuv_rec_data.complit , size=%d, recv_size=%d\n\n", yuv_rec_data.size, yuv_rec_data.recv_size);
                    break;
                }

#endif

#if AVI_UNPKG_DECODE_TEST
                //写卡例子，可以用软件来验证,当然可以得到YUV之后转RGB可用于显示，例子在jpeg_decode_test.c里的yuv4xxp_quto_rgb565调用方法
                sprintf(name, "%stest.yuv", CONFIG_ROOT_PATH);
                if (!yuv_fd) {
                    yuv_fd = fopen(name, "wb+");
                }
                if (yuv_fd) {
                    fwrite(yuv_fd, yuv, yuv_len);
                    printf("write num = %d \n", num);
                }
#endif
            }
        } else {
            break;
        }
    }
exit:
    avi_net_unpkg_exit(fd, 1);
#if AVI_UNPKG_DECODE_TEST
    if (yuv_fd) {
        fclose(yuv_fd);
    }
#endif
    if (fd) {
        fclose(fd);
    }
    if (yuv) {
        free(yuv);
    }
    if (fbuf) {
        free(fbuf);
    }
    return 0;
}
int avi_unpkg_example_init(void)
{
    thread_fork("avi_unpkg_example", 10, 1500, 64, 0, avi_unpkg_decoder_to_yuv_example, NULL);
    return 0;
}
#endif

