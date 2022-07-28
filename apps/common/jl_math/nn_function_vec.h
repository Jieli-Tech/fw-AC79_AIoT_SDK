#ifndef __NN_FUNCTION_VEC_H_
#define __NN_FUNCTION_VEC_H_


#include "stdint.h"
#include "includes.h"
#include "fftvec_math_drv.h"




void vector_real_zs32_matrix_ys8_xs8(long *zptr, long *yptr, long *xptr, short len, short loop, short x_inc, short y_inc, short z_inc, char q);
void vector_real_zs32_matrix_ys32_xs16(long *zptr, long *yptr, long *xptr, short len, short loop, short x_inc, short y_inc, short z_inc, char q);
void vector_real_zs32_matrix_ys32_xs8(long *zptr, long *yptr, long *xptr, short len, short loop, short x_inc, short y_inc, short z_inc, char q);


void vector_real_zs32_ys32_add_xs32(void *zptr, void *yptr, void *xptr, short len);
void vector_real_zs32_ys32_add_xs16(void *zptr, void *yptr, void *xptr, short len);
void vector_real_zs32_ys32_sub_xs32(void *zptr, void *yptr, void *xptr, short len);

void vector_real_scale_s32(void *zptr, void *xptr, short len, char q, long const_dat);

void vector_real_zs32_ys32_mul_xs32(void *zptr, void *yptr, void *xptr, short len, char q);
void vector_real_zs32_ys16_mul_xs16(void *zptr, void *yptr, void *xptr, short len, char q);
void vector_real_zs32_ys32_mul_xs16(void *zptr, void *yptr, void *xptr, short len, char q);
void vector_real_zs32_ys32_mul_xs8(void *zptr, void *yptr, void *xptr, short len, char q);
void vector_real_zs32_ys8_mul_xs8(void *zptr, void *yptr, void *xptr, short len, char q);


void vector_real_zs32_sum_xs32(void *zptr, void *xptr, short len, char q);


void vector_complex_zs32_ys32_add_xs32(void *zptr, void *yptr, void *xptr, short len);
void vector_complex_zs32_ys32_sub_xs32(void *zptr, void *yptr, void *xptr, short len);

void vector_complex_zs32_ys8_mul_xs8(void *zptr, void *yptr, void *xptr, short len, char q);
void vector_complex_zs32_ys32_mul_xs32(void *zptr, void *yptr, void *xptr, short len, char q);
void vector_complex_zs32_ys32_mul_conj_xs32(void *zptr, void *yptr, void *xptr, short len, char q);

void vector_complex_zs64_ys32_mul_conj_xs32(void *zptr, void *yptr, void *xptr, short len, char q);

void vector_complex_zs64_qdt_xs32(void *zptr, void *xptr, short len, char q);

void vector_complex_zs32_qdt_xs32(void *zptr, void *xptr, short len, char q);

void vector_complex_zs32_ys32_mac_xs32(void *zptr, void *yptr, void *xptr, short len, char q);

void vector_complex_zs32_ys32_mac_conj_xs32(void *zptr, void *yptr, void *xptr, short len, char q);

void vector_complex_zs32_ys32_msc_xs32(void *zptr, void *yptr, void *xptr, short len, char q);


void vector_real_zs32_x32_max_scale(long *zptr,  long *xptr, short len, char q, long const_dat);

void vector_real_zs64_x64_max_scale(long  long *xptr,  long long max_const, long len);

void vector_real_zs32_x32_min_scale(long *zptr,  long *xptr, short len, char q, long const_dat);
void vector_real_zs64_x64_min_scale(long  long *xptr,  long long min_const, long len);

void vector_real_smooth_zs64_xs64(long long *zptr,  long long *xptr,  unsigned long len, char q, long lamda);

void vector_real_smooth_zs32_xs32(long *zptr,  long *xptr,  unsigned long len, char q, long lamda);

void vector_complex_smooth_zs32_xs32(long *zptr,  long *xptr,  unsigned long len, char q, long lamda);

void complex_abs_s32(unsigned long *zptr, long *xptr, unsigned long len);


void vector_real_zs8_x32_mul_const(void *zptr, void *xptr, short len, char q, long const_dat);

//add
void vector_real_zs32_x32_mul_const(void *zptr, void *xptr, short len, char q, long const_dat);
//add
void vector_real_zs32_ys32_mac_xs32(void *zptr, void *xptr, short len, char q, long const_dat);
//add
void vector_real_zs32_ys8_dot_product_xs8(void *zptr, void *yptr, void *xptr, short len, char q);








void vector_real_zs8_linear_ys8_xs8(
    int8_t *pV, int8_t *pM, int32_t *bias,
    const int dim_vec_x, const int dim_vec_y,
    const int wt_x, const int wt_y,
    int shift_bias, int multiplier,
    int8_t *pOut, int32_t *ip_temp);

void vector_real_zs8_depwise_ys8_xs8(
    int8_t *data_in, int8_t *wt, int32_t *bias,
    const int dim_vec_x, const int dim_vec_y,
    int shift_bias, int multiplier,
    int8_t *data_out, int32_t *ip_temp);




#endif

