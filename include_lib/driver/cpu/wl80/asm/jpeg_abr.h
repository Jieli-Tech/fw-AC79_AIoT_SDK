#ifndef _JPEG_ABR_H
#define _JPEG_ABR_H
#include "typedef.h"

typedef struct jpeg_abr_fd {
    u8 fps;
    u32 kbps;
    u32 mbFactor;
    u32 frames;
    u32 cur_abr;
    u32 total_kbytes;
    u8 q_step;
    s8 max_q;
    s8 min_q;
    s8 q;
} mabr_t;


// init , 输入帧率，目标码率，宽高，初始q， 最大q, 最小q, 每次调整q的步长；推荐q步长为<=3; 太大容易出现跳跃，或者画面变化太剧烈
mabr_t *jpeg_abr_init(u32 fps, u32 abr_kbps, u32 w, u32 h, u32 init_q, u32 max_q, u32 min_q, u32 q_step);
// 输入前一帧的大小（KBytes), 返回新的一帧Q值；
int jpeg_abr_update(mabr_t *h, u32 pre_kbytes);

// deinit
void jpeg_abr_uninit(mabr_t *jpeg_abr);

void jpeg_abr_reset(mabr_t *jpeg_abr, u32 abr_kbps);

int jpeg_abr_check_remain_space(mabr_t *h, u32 size);
#endif
