#ifndef _YUV_TO_RGB_H
#define _YUV_TO_RGB_H

#include "generic/typedef.h"

u16 rgb_24_to_565(u8 R, u8 G, u8 B);

//yuvBuffer_in:YUV输入源数据缓存区, rgbBuffer_out:rgb输出缓存区，width/height:分辨率宽高, be:大小端存储:1大端,0小端
void yuv420p_quto_rgb24(unsigned char *yuvBuffer_in, unsigned char *rgbBuffer_out, int width, int height);
void yuv420p_quto_rgb565(unsigned char *yuvBuffer_in, unsigned char *rgbBuffer_out, int width, int height, char be);
void yuv422p_quto_rgb565(unsigned char *yuvBuffer_in, unsigned char *rgbBuffer_out, int width, int height, char be);
void yuv444p_quto_rgb565(unsigned char *yuvBuffer_in, unsigned char *rgbBuffer_out, int width, int height, char be);

//rgb565:rgb565输入源数据缓存区, yuv420p:YUV数据输出缓存区, width/height:分辨率宽高, be:大小端存储:1大端,0小端
int rgb565_to_yuv420p(unsigned char *rgb565, unsigned char *yuv420p, int width, int height, char be);

//rgb24:rgb24输入源数据缓存区, yuv420p:YUV数据输出缓存区, width/height:分辨率宽高
int rgb24_to_yuv420p(unsigned char *rgb24, unsigned char *yuv420p, int width, int height);

/**********RGB565转RG24************/
void RGB565_to_RGB888(unsigned char *rgb565, unsigned char *rgb888, int width, int height);
/**********RGB888转RGYUV420***********/
void RGB888_to_YUV420(u8 *rgb_buf, u8 *yuv_buf, u16 width, u16 heigh);

#endif
