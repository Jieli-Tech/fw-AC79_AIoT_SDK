
#ifndef IMG_STITCH_H
#define IMG_STITCH_H
#include<stdlib.h>
#include"fast_corner.h"
#include"matrix_process.h"
#include"img_transform.h"
enum img_stitch_status {
    success = 1,
    corner_count_err,
    match_count_err,
    mapping_err,
    direction_err,
    out_w_err,
    force_exit_err,
    dist_err,
};
void debug_corner_info();
int get_current_w();
void img_stitch_init(unsigned char *img, int width, int height, unsigned char *output_img, int w_out, int h_out, int outh_off, int Max_corner_count, int Fast_corner_mode, unsigned long Fast_th, unsigned long Match_th, int offset_th, int nms_windon_size, int Max_filter_th_x, int Max_filter_th_y, int Frame_stride, int Adjustment_enable, int max_off, int Fast_num_target,
                     int Fast_num_target_min, int Fast_num_target_max, unsigned long Fast_th_min, unsigned long Fast_th_max, unsigned long *diff_ths, int *strengths, unsigned char *Fast_ths, int NMS_mode);
void img_stitch_reinit(unsigned char *img, int width, int height, unsigned char *output_img, int w_out, int h_out, int outh_off, int Max_corner_count, int Fast_corner_mode, unsigned long Fast_th, unsigned long Match_th, int Adjustment_enable, int Fast_num_target,
                       int Fast_num_target_min, int Fast_num_target_max, unsigned long Fast_th_min, unsigned long Fast_th_max, unsigned long *diff_ths, int *strengths, unsigned char *Fast_ths, int NMS_mode);
void img_stitch_deinit();
int img_stitch_process(unsigned char *src_img, unsigned char *dst_img, unsigned char *output_img, unsigned char flag_force_stitch, unsigned char flag_force_exit);
int post_process(unsigned char *src, unsigned char *dst, int width, int height, int center_x, int center_y);
#endif
