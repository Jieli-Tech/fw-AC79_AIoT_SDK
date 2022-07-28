
#ifndef _IMG_TRANSFORM_H
#define _IMG_TRANSFORM_H
//#include<stdint.h>
/*#include "fs.h"*/
#define interpolate1_process 1

void img_transform_init(unsigned char *src, unsigned char *output_img, int width, int height, int w_out, int h_out, int h_off, int Frame_stride, int Adjustment_enable);
void img_transform_reinit(unsigned char *src, unsigned char *output_img, int width, int height, int w_out, int h_out, int h_off, int Frame_stride, int Adjustment_enable);
int warpAffine(unsigned char *src, unsigned char *dst, float tran_matrix[6], int width_src, int height_src, int width_dst, int height_dst, unsigned char mode);
int post_process_(unsigned char *src, unsigned char *dst, int width, int height, int center_x, int center_y);
int get_current_w(void);

#endif
