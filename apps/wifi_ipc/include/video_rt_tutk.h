#ifndef __VIDEO_RT_TUTK_H
#define __VIDEO_RT_TUTK_H




int video0_tutk_start(void *arg, u16 width, u16 height, u8 fps, u32 rate, u32 format);
int video0_tutk_stop(void *arg);

int video0_tutk_change_bits_rate(void *arg, u32 bits_rate);



int tutk_speak_init(void *hdl, u32 format, u32 rate);
int tutk_speak_uninit(void *hdl);


#endif

