#ifndef  __SIMPLE_AVI_UNPKG_H__
#define  __SIMPLE_AVI_UNPKG_H__
#include "generic/typedef.h"
#include "fs/fs.h"


typedef u32 FOURCC;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned char BOOL;

typedef struct __avi_unpkg_info {
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
} AVI_UNPKG_INFO;


typedef struct unpkg_avi_mainheader {
    FOURCC fcc;   // 必须为‘avih’
    u32  cb;    // 本数据结构的大小，不包括最初的8个字节（fcc和cb两个域）
    u32  dwMicroSecPerFrame;   // 视频帧间隔时间（以毫秒为单位）
    u32  dwMaxBytesPerSec;     // 这个AVI文件的最大数据率
    u32  dwPaddingGranularity; // 数据填充的粒度
    u32  dwFlags;         // AVI文件的全局标记，比如是否含有索引块等
    u32  dwTotalFrames;   // 总帧数
    u32  dwInitialFrames; // 为交互格式指定初始帧数（非交互格式应该指定为0）
    u32  dwStreams;       // 本文件包含的流的个数
    u32  dwSuggestedBufferSize; // 建议读取本文件的缓存大小（应能容纳最大的块）
    u32  dwWidth;         // 视频图像的宽（以像素为单位）
    u32  dwHeight;        // 视频图像的高（以像素为单位）
    u32  dwReserved[4];   // 保留
}  UNPKG_AVI_MAINHEADER;

typedef struct unpkg_avi_streamheader {
    FOURCC fcc;  // 必须为‘strh’
    u32  cb;   // 本数据结构的大小，不包括最初的8个字节（fcc和cb两个域）
    FOURCC fccType;    // 流的类型：‘auds’（音频流）、‘vids’（视频流）、‘mids’（MIDI流）、‘txts’（文字流）
    FOURCC fccHandler; // 指定流的处理者，对于音视频来说就是解码器
    u32  dwFlags;    // 标记：是否允许这个流输出？调色板是否变化？
    u16   wPriority;  // 流的优先级（当有多个相同类型的流时优先级最高的为默认流）
    u16   wLanguage;
    u32  dwInitialFrames; // 为交互格式指定初始帧数
    u32  dwScale;   // 这个流使用的时间尺度
    u32  dwRate;
    u32  dwStart;   // 流的开始时间
    u32  dwLength;  // 流的长度（单位与dwScale和dwRate的定义有关）
    u32  dwSuggestedBufferSize; // 读取这个流数据建议使用的缓存大小
    u32  dwQuality;    // 流数据的质量指标（0 ~ 10,000）
    u32  dwSampleSize; // Sample的大小
    struct {
        u16 left;
        u16 top;
        u16 right;
        u16 bottom;
    }  rcFrame;  // 指定这个流（视频流或文字流）在视频主窗口中的显示位置
    // 视频主窗口由AVIMAINHEADER结构中的dwWidth和dwHeight决定
}
UNPKG_AVI_STREAMHEADER;



typedef struct UNPKG_BIT_MAPINFOHEADER {
    FOURCC  fcc;				//必须为"strf"
    u32 strf_size;

    u32 biSize;					//本结构所占字节数
    u32 biWidth;				//位图宽度，以像素为单位
    u32 biHeight;				//位图高度，以像素为单位
    u16 biPlanes;				//目标设备级别，必须为1
    u16 biBitCount;				//每个像素的位数
    u32 biCompression;			//位图压缩类型,例如MJPG
    u32 biSizeImage;			//位图大小，以字节为单位
    u32 biXPelsPerMeter;		//位图水平分辨率
    u32 biYPelsPerMeter;		//位图垂直分辨率
    u32 biClrUsed;				//位图实际使用的调色板的颜色数
    u32 biClrImportant;			//位图显示过程中重要的颜色数
}  UNPKG_BIT_MAPINFOHEADER;


typedef struct UNPKG_WAVE_FORMATEX {
    FOURCC  fcc;				//必须为"strf"
    u32 strf_size;

    u16  wFormatTag;
    u16  nChannels;			//设置音频文件的通道数量，对于单声道的声音，此此值为1。对于立体声，此值为2.
    u32 nSamplesPerSec;     //采样率
    u32 nAvgBytesPerSec;    //设置请求的平均数据传输率，单位byte/s。这个值对于创建缓冲大小是很有用的。一般设置为每秒的大小
    u16  nBlockAlign;		//以字节为单位设置块对齐。块对齐是指最小数据的原子大小。
    //如果wFormatTag= WAVE_FORMAT_PCM，nBlockAlign为(nChannels*wBitsPerSample) / 8
    u16  wBitsPerSample;	//根据wFormatTag的类型设置每个样本的位深（即每次采样样本的大小，以bit为单位）
    u16  cbSize;			//额外信息的大小，以字节为单位
}  UNPKG_WAVE_FORMATEX;

#define  UNPKG_HEAD_JUNK_SIZE      (512 - ( (6 * 4) + sizeof(UNPKG_AVI_MAINHEADER) + ((3 * 4 + sizeof(UNPKG_AVI_STREAMHEADER)) * 2 + \
                                sizeof(UNPKG_BIT_MAPINFOHEADER) + sizeof(UNPKG_WAVE_FORMATEX)) + (2 * 4) + 12) % 512 )

typedef struct __JL_AVI_HEAD {

    FOURCC  riff;  // 必须为‘RIFF’
    u32 file_size;
    u32 file_type; // "AVI "

    FOURCC  list_hdrl;  // 必须为"LIST"
    u32 hdrl_size;
    FOURCC  hdrl;   //必须为"hdrl"

    UNPKG_AVI_MAINHEADER avih;

    FOURCC  list_strl_vid;  // 必须为"LIST"
    u32 strl_vidsize;
    FOURCC  strl_vid;   //必须为"strl"
    UNPKG_AVI_STREAMHEADER vid_strh;
    UNPKG_BIT_MAPINFOHEADER vid_strf;

    FOURCC  list_strl_aud;  // 必须为"LIST"
    u32 strl_audsize;
    FOURCC  strl_aud;   //必须为"strl"
    UNPKG_AVI_STREAMHEADER aud_strh;
    UNPKG_WAVE_FORMATEX aud_strf;

    FOURCC fcc_head_junk;  //必须为"JUNK"
    u32 head_junk_len;
    u8 head_junk[UNPKG_HEAD_JUNK_SIZE];
    u32 list;
    u32 len ;
    u32 movi ;
    //u8 movi[8];

} UNPKG_JL_AVI_HEAD;


//索引表
typedef struct _avioldindex_entry {
    DWORD dwChunkId; // 表征本数据块的四字符码
    DWORD dwFlags; // 说明本数据块是不是关键帧、 是不是‘rec ’ 列表等信息
    DWORD dwOffset; // 本数据块在文件中的偏移量
    DWORD dwSize; // 本数据块的大小
} AVI_INDEX_INFO; // 这是一个数组！ 为每个媒体数据块都定义一个索引信息


int avi_get_width_height(FILE *fd, void *__info, u8 state);
int avi_get_audio_sample_rate(FILE *fd, u8 state);
int is_vaild_avi_file(FILE *fd, u8 state);
int avi_preview_unpkg_init(FILE *fd, u8 state);
int avi_playback_unpkg_init(FILE *fd, u8 state);
int avi_unpkg_exit(FILE *fd, u8 state);
int avi_send_next_video_frame(FILE *fd, u8 mode, u8 state);
int avi_send_next_audio_frame(FILE *fd, u8 mode, u8 state);
int avi_send_one_video_audio_frame(FILE *fd, u8 mode, u8 state);
int avi_get_fps(FILE *fd, u8 state);
int avi_get_file_time(FILE *fd, u8 state);
int avi_seek_time_shaft_preview_set(FILE *fd, int offset_ms, u8 state); //返回值：这帧的JPEG数据长度，设置完成之后立即读取JPEG数据
int avi_video_get_frame(FILE *fd, int offset_num, u8 *buf, u32 buf_len, u8 state);
int avi_audio_get_frame(FILE *fd, int offset_num, u8 *buf, u32 buf_len, u8 state);
int avi_get_video_num(FILE *fd, int offset_ms, u8 state); //返回seq
int avi_is_has_audio(FILE *fd, u8 state);
int avi_get_audio_chunk_num(FILE *fd, u8 state); //获取整个视频的音频帧数
int avi_get_video_chunk_num(FILE *fd, u8 state);
int avi_get_audio_timestanp(FILE *fd, u8 state);
int avi_audio_base_to_get_video_frame(u32 audio_num, u8 state);
int avi_video_base_to_get_audio_frame(int vd_num, u8 state);
int avi_video_set_frame_addr(FILE *fd, int offset_num, u8 state);

#endif  /*SIMPLE_MOV_UNPKG_H*/
