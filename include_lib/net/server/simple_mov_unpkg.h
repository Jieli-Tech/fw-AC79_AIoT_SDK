#ifndef  __SIMPLE_MOV_UNPKG_H__
#define  __SIMPLE_MOV_UNPKG_H__
#include "generic/typedef.h"
#include "fs/fs.h"

struct __mov_unpkg_info {
    u32 *stsz_tab;
    u32 *stco_tab;
    u32 *audio_stco_tab;
    u32 audio_block_size;
    u32 length;
    u32 height;
    u32 scale;
    u32 durition;
    u32 sample_rate;
    u32 video_sample_count;
    u32 sample_duration;
    u32 audio_chunk_num;
};


int read_stts(FILE *fp, struct __mov_unpkg_info *info);
int read_time_scale_dur(FILE *file_fp, struct __mov_unpkg_info *info);
int read_height_and_length(FILE *file_fp, struct __mov_unpkg_info *info);

int get_audio_sample_rate(FILE *fp);
int get_chunk_offset(u32 *stco_tab, u32 count);
u32 *get_chunk_index_table(FILE *fp, struct __mov_unpkg_info *info);
int get_audio_sample_count(FILE *fp);
int get_audio_chunk_offset_entry(FILE *fp);
u32 *get_audio_chunk_table(FILE *fp, struct __mov_unpkg_info *info);
int get_audio_chunk_offset(u32 *audio_stco_tab, u32 count);
u32 *get_sample_index_table(FILE *fp, struct __mov_unpkg_info *info);
int get_sample_size(u32 *stsz_table, u32 count);
int is_vaild_mov_file(FILE *fp);
int is_has_audio(FILE *fp);


#endif  /*SIMPLE_MOV_UNPKG_H*/
