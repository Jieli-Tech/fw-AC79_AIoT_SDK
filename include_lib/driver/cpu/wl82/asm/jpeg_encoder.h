/*******************************************************************************************
 File Name: jpeg_encode.h
 Version: 1.00
 Discription:
 Author:yulin deng
 Email :flowingfeeze@163.com
 Date: 11 2013
 Copyright:(c)JIELI  2011  @ , All Rights Reserved.
*******************************************************************************************/
#ifndef  __JPEG_ENCODER_H__
#define  __JPEG_ENCODER_H__

#include "typedef.h"
#include "jpeg_codec.h"
#include "asm/jpeg_abr.h"
#include "video/video.h"

/*
#define JPEG0_SFR_BEGIN  &JPG0_CON0
#define JPEG1_SFR_BEGIN  &JPG1_CON0


typedef struct JPEG_SFR
{
volatile u32 CON0            ;
volatile u32 CON1            ;
volatile u32 CON2            ;
volatile u32 YDCVAL          ;        //write only
volatile u32 UDCVAL          ;        //write only
volatile u32 VDCVAL          ;        //write only
volatile u32 YPTR0           ;        //write only
volatile u32 UPTR0           ;        //write only
volatile u32 VPTR0           ;        //write only
volatile u32 YPTR1           ;        //write only
volatile u32 UPTR1           ;        //write only
volatile u32 VPTR1           ;        //write only
volatile u32 BADDR           ;        //write only
volatile u32 BCNT            ;
volatile u32 MCUCNT          ;
volatile u32 PRECNT          ;        //write only
volatile u32 YUVLINE         ;        //write only
volatile u32 CFGRAMADDR      ;        //write only
//u32 rev[0x1c00-0x12] ;
volatile u32 CFGRAMVAL       ;  //0x1c00*4
volatile u32 PTR_NUM         ;
}JPEG_SFR ;
*/


#define JPEG_DEFAULT_EXIF_LEN        0xA8//208
//extern unsigned char const jpeg_file_header[624 + JPEG_ADD_LEN];




#define FRAME_INT 1
#define JPEG_MODLE_NUM  1   // jpeg 编码模块个数

#define JPEG_ENC_INT_EN     1//是否允许JPEG编码中断

#define JPEG_ENCODE_BITCNT  0x1000//0x800 //位流buffer大小

#define JPEG_INT_EN()   reg->CON0 |= BIT(2)|BIT(3)
#define JPEG_INT_DIS()  reg->CON0 &=~(BIT(2)|BIT(3))

#define JPEG_FTYPE_NORMAL		1
#define JPEG_FTYPE_SKIP			2

#define BUILD_DYNAMIC_HUFFMAN   0
#define ENABLE_JPEG_ABR			1
#define ENABLE_ZOOM_ENC			0

#define JPEGENC_KSTART      0x80
#define JPEGENC_RESET       0x40
#define JPEGENC_BITS_FULL   0x20
#define JPEGENC_STATUS      0xf

//#define GET_JPEGENC_STATUS(hd) ((hd)->state & 0xf)
//#define SET_JPEGENC_STATUS(hd, x) (hd)->state = (((hd)->state & 0xf0) | ((x) & 0xf))
#define SMP_ENABLE     1

enum {
    JPEG_SEEK_SET = 0x01,
    JPEG_SEEK_CUR = 0x02,
};


enum {
    JPEGENC_UNINIT = 0x0,
    JPEGENC_INIT,
    JPEGENC_IDLE,
    JPEGENC_STOP,
    JPEGENC_RUNNING,
    JPEGENC_STOPING,
    JPEGENC_FRAME_END,
    JPEGENC_FRAME_ERR,
};

#if BUILD_DYNAMIC_HUFFMAN
enum {
    HUFFMAN_NO_INIT = 0x0,
    HUFFMAN_OPEN,
    HUFFMAN_CLOSE,
};
#endif

/*
typedef struct FILEHEAD {
    u16  YQT_DCT[0x40] ;
    u16  UVQT_DCT[0x40];
    u8   filedata[138] ;
} qtfilehead_t ;
*/
typedef struct jpg_q_table qtfilehead_t;

struct fill_frame {
    u8 forbidden;
    volatile u8 enable;
    volatile u8 cri;
    int timer;
    int one_sec_timer;
    u32 msecs;
    u32 one_sec_msecs;
    u32 secs;
    u32 interval;
    u32 cont;
    u32 fbase;
    u32 one_sec_fbase;
    u32 fnum;
    u32 cnt_fnum;
    u32 fps;
};


struct jpeg_encode_info {
    u16 width;
    u16 height;
    u32 kbps;
    u8  fps;
    u8  source; //imc / buffer
    u8  fmt; // yuv420 yuv422 yuv444
    u8  mode;// 编码模式 0 -- 连续编码模式   1 -- 编一张。
    u8  bits_mode;
    u8  q_val;
    u8  vbuf_num;
    struct jpg_q_table *qt;
};

struct jpeg_encoder_fd {
    void *parent;
    struct jpeg_encode_info info;
#if SMP_ENABLE
    spinlock_t lock;
#endif
    u8 parent_state;
    volatile u8 active;
    u8 std_huffman;
    u8 encoder_running;
    u8 encoder_reset;
    u8 enable_abr;
    u8 enable_dyhuffman;
    u8 hori_sample;
    u8 vert_sample;
    u8 sample_rate;
    volatile u8 state;
    volatile u8 mcu_pnd;
    volatile u32 mcu_cnt;
    volatile u32 mcu_line_cnt;
    volatile u16 mcu_line_set;//分行编码
    volatile u32 encode_mcu;
    volatile u32 bits_cnt;
    volatile u32 file_size;
    u32 buf_len;

    struct video_cap_buffer cap_buffer;
    u8 *data;
    u8 *exif;
    int exif_size;
    u8 *thumbnails;
    int thumb_len;
    int  head_len;
    int f_div_count;
    void *priv;
    void *irq_priv;
    void *fb;
    int (*irq_handler)(void *priv, enum mjpg_irq_code code, enum mjpg_frame_type type);
    //void *fbpipe;
    const struct mjpg_user_ops *ops;
    OS_SEM sem_stop ;
    OS_SEM sem_dhuffman;
    OS_SEM sem_mcu_pnd;
    void *dync_huffan_fh;
#if ENABLE_JPEG_ABR
    struct jpeg_abr_fd *abr_fd;//自适应q值运算数据
#endif
    struct list_head entry;

    u8 bfmode;
};


static inline u8 GET_ENCODER_STATE(struct jpeg_encoder_fd *fd)
{
    u8 state;
#if SMP_ENABLE
    spin_lock(&fd->lock);
#endif
    state = fd->state & 0xf;
#if SMP_ENABLE
    spin_unlock(&fd->lock);
#endif
    return state;
}

static inline void SET_ENCODER_STATE(struct jpeg_encoder_fd *fd, u8 state)
{
#if SMP_ENABLE
    spin_lock(&fd->lock);
#endif
    fd->state = (fd->state & 0xf0) | (state & 0xf);
    __asm_csync();
#if SMP_ENABLE
    spin_unlock(&fd->lock);
#endif
}

extern int jpeg_manual_encoder_init(struct jpeg_encoder_fd *fd, void *arg);
extern int jpeg_encoder_init(struct jpeg_encoder_fd *fd, void *arg, enum jpeg_enc_mode mode);
extern int jpeg_encoder_uninit(struct jpeg_encoder_fd *fd);
int jpeg_encoder_start(void *_fd);
int get_jpeg_encoder_s_attr(void *_fd, struct mjpg_s_attr *attr);
int set_jpeg_encoder_s_attr(void *_fd, struct mjpg_s_attr *attr);
int get_jpeg_encoder_d_attr(void *_fd, struct mjpg_d_attr *attr);
int set_jpeg_encoder_d_attr(void *_fd, struct mjpg_d_attr *attr);
int set_jpeg_encoder_fps(void *_fh, int fps);
int set_jpeg_encoder_sbuf(void *_fh, struct video_cap_buffer *b);
int set_jpeg_encoder_user_ops(void *_fh, void *priv, const struct mjpg_user_ops *ops);
int set_jpeg_encoder_handler(void *_fh, void *priv, int (*handler)(void *, enum mjpg_irq_code, enum mjpg_frame_type));
int jpeg_encoder_stop(void *_fh);
int jpeg_encoder_wait_stop(void *_fh);
int jpeg_encoder_restart(void *_fd);
int jpeg_encoder_manual_start(void *_fd, struct YUV_frame_data *input_frame, u8 *bits_buf, int buf_len);
int jpeg_encoder_reset_bits_rate(void *_fd, u32 bits_rate);
int jpeg_encoder_reset(void *_fd, u8 init_hw);
int jpeg_encoder_pause(void *_fd);
int jpeg_encoder_close(void *_fd);
int jpeg_encoder_image_start(void *_fd, void *arg, u8 for_stream, u8 wait_complet);
int jpeg_encoder_image_wait_complet(void *_fd, void *arg);
int jpeg_encoder_check_space(void *_fh, u32 size);
int jpeg_encoder_mcu_line_kick(void *_fd);
int jpeg_encoder_in_irq_set_stop(void *_fh);
int set_jpeg_encoder_use_muc_line(void *_fh, int line);
int set_jpeg_encoder_next_line(void *_fh);

int encoder_dhuffman_irq_handler(struct jpeg_encoder_fd *fd);
int encoder_speed_irq_handler(struct jpeg_encoder_fd *fd);
int encoder_bits_irq_handler(struct jpeg_encoder_fd *fd);
int encoder_mcu_irq_handler(struct jpeg_encoder_fd *fd);


#if BUILD_DYNAMIC_HUFFMAN
extern void *huffman_init(struct jpeg_encoder_fd *fd);
extern int huffman_uninit(void *fh);
extern int update_huffman_info(void *fh, void *reg, u8 *buf);
extern void update_huff_freq_data(void *fh, void *reg);
extern int reset_dynamic_huffman(void *fh);
#endif

#endif



