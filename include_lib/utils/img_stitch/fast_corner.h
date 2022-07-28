
#ifndef _FAST_CORNER_H
#define _FAST_CORNER_H

//#define windon_size	7
//#define Max_x_off	5
//#define Max_y_off	13
#define hw_acc		1

/*#include<stdio.h>
#include<stdint.h>
#include<string.h>*/
/*#include "fs.h"*/
void fast_corner_match_init(int max_corner_count, int fast_corner_mode, unsigned int *dirsx_src_match_, unsigned int *dirsy_src_match_,
                            unsigned int *dirsx_dst_match_, unsigned int *dirsy_dst_match_, unsigned int *simi_match_, unsigned int *flag_match_, unsigned int *corner_x_, unsigned int *corner_y_, int offset_th_, int nms_windon_size_, int Max_off_y);
void fast_corner_match_deinit();
int my_get_fast_corner(unsigned char *image, unsigned int *fast_img, int rows, int cols, unsigned long threshold, unsigned int  *dirsx, unsigned int *dirsy);
int fast_corner_match(unsigned char *img_src, unsigned int  *dirsx_src, unsigned int *dirsy_src, unsigned char *img_dst, unsigned int  *dirsx_dst, unsigned int *dirsy_dst, int width, int count_src, int count_dst, unsigned long th, unsigned int *simi, int offset);
void array_sort(unsigned int *src_x, unsigned int *src_y, unsigned int *dst_x, unsigned int *dst_y, unsigned int *simi, int length);
#endif
