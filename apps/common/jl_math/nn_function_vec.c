#include"nn_function_vec.h"



//=========================================================================
// matrix_vector_real_xs8_ys8_zs32
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = sum(y[n] * x[n] / 2^q)  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
void vector_real_zs32_matrix_ys8_xs8(long *zptr, long *yptr, long *xptr, short len, short loop, short x_inc, short y_inc, short z_inc, char q)
//void matrix_vector_real_xs8_ys8_zs32(long *zptr, long*yptr, long*xptr,short len, short loop,short x_inc,short y_inc,short z_inc,char q)
{

    long config = VECZ_S32 |  VECY_S8 |  VECX_S8 | VEC_ACC | VEC_REAL_MUL_MODE;

    hwvec_exec(xptr, yptr, zptr, x_inc, y_inc, z_inc, len, loop, q, config, 0);

}



//=========================================================================
// vector_real_zs32_matrix_ys32_xs16
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = sum(y[n] * x[n] / 2^q)  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
void vector_real_zs32_matrix_ys32_xs16(long *zptr, long *yptr, long *xptr, short len, short loop, short x_inc, short y_inc, short z_inc, char q)
//void matrix_vector_real_xs8_ys8_zs32(long *zptr, long*yptr, long*xptr,short len, short loop,short x_inc,short y_inc,short z_inc,char q)
{

    long config = VECZ_S32 |  VECY_S32 |  VECX_S16 | VEC_ACC | VEC_REAL_MUL_MODE;

    hwvec_exec(xptr, yptr, zptr, x_inc, y_inc, z_inc, len, loop, q, config, 0);

}

//=========================================================================
// vector_real_zs32_matrix_ys32_xs8
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = sum(y[n] * x[n] / 2^q)  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
void vector_real_zs32_matrix_ys32_xs8(long *zptr, long *yptr, long *xptr, short len, short loop, short x_inc, short y_inc, short z_inc, char q)
//void matrix_vector_real_xs8_ys8_zs32(long *zptr, long*yptr, long*xptr,short len, short loop,short x_inc,short y_inc,short z_inc,char q)
{

    long config = VECZ_S32 |  VECY_S32 |  VECX_S8 | VEC_ACC | VEC_REAL_MUL_MODE;

    hwvec_exec(xptr, yptr, zptr, x_inc, y_inc, z_inc, len, loop, q, config, 0);

}




//=========================================================================
// vector_real_zs32_ys32_add_xs32
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = saturated( y[n] + x[n] )  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
void vector_real_zs32_ys32_add_xs32(void *zptr, void *yptr, void *xptr, short len)
{

    long config = VECZ_S32 |  VECY_S32 |  VECX_S32;

    vector_real_add(zptr, yptr, xptr, len, config);

}

//=========================================================================
// vector_real_zs32_ys32_add_xs16
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = saturated( y[n] + x[n] )  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
void vector_real_zs32_ys32_add_xs16(void *zptr, void *yptr, void *xptr, short len)
{

    long config = VECZ_S32 |  VECY_S32 |  VECX_S16;

    vector_real_add(zptr, yptr, xptr, len, config);

}

//=========================================================================
// vector_real_zs32_ys32_sub_xs32
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = saturated( y[n] - x[n] )  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
void vector_real_zs32_ys32_sub_xs32(void *zptr, void *yptr, void *xptr, short len)
{

    long config = VECZ_S32 |  VECY_S32 |  VECX_S32;

    vector_real_sub(zptr, yptr, xptr, len, config);

}




//vector_real_scale_s32
//=========================================================================
// vector_real_scale_s32
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = (x[n] * const_dat/ 2^q)  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
void vector_real_scale_s32(void *zptr, void *xptr, short len, char q, long const_dat)
{
    long config = VECZ_S32 |  VECY_S32 |  VECX_S32;
    vector_real_mul_const(zptr, xptr, len, q, const_dat, config);
}



//=========================================================================
// vector_real_zs32_ys32_mul_xs32
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = saturated(y[n] * x[n] / 2^q)  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
void vector_real_zs32_ys32_mul_xs32(void *zptr, void *yptr, void *xptr, short len, char q)
{
    long config = VECZ_S32 | VECY_S32 | VECX_S32 | VEC_CPY_REAL_OR_VEC;
    vector_real_mul(zptr, yptr, xptr, len, q, config);
}

//=========================================================================
// vector_real_zs32_ys16_mul_xs16
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = saturated(y[n] * x[n] / 2^q)  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
void vector_real_zs32_ys16_mul_xs16(void *zptr, void *yptr, void *xptr, short len, char q)
{
    long config = VECZ_S32 | VECY_S16 | VECX_S16 | VEC_CPY_REAL_OR_VEC;
    vector_real_mul(zptr, yptr, xptr, len, q, config);
}

//=========================================================================
// vector_real_zs32_ys32_mul_xs16
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = saturated(y[n] * x[n] / 2^q)  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
void vector_real_zs32_ys32_mul_xs16(void *zptr, void *yptr, void *xptr, short len, char q)
{
    long config = VECZ_S32 | VECY_S32 | VECX_S16 | VEC_CPY_REAL_OR_VEC;
    vector_real_mul(zptr, yptr, xptr, len, q, config);
}

//=========================================================================
// vector_real_zs32_ys32_mul_xs8
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = saturated(y[n] * x[n] / 2^q)  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
void vector_real_zs32_ys32_mul_xs8(void *zptr, void *yptr, void *xptr, short len, char q)
{
    long config = VECZ_S32 |  VECY_S32 | VECX_S8 | VEC_CPY_REAL_OR_VEC;
    vector_real_mul(zptr, yptr, xptr, len, q, config);
}

//=========================================================================
// vector_real_zs32_ys8_mul_xs8
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = saturated(y[n] * x[n] / 2^q)  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
void vector_real_zs32_ys8_mul_xs8(void *zptr, void *yptr, void *xptr, short len, char q)
{
    long config = VECZ_S32 | VECY_S8 | VECX_S8 | VEC_CPY_REAL_OR_VEC;
    vector_real_mul(zptr, yptr, xptr, len, q, config);
}

//=========================================================================
// vector_real_zs32_sum_xs32
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// z_dw >= x_dw
//  -32 < Q < 32
// *zptr, *xptr:  s32/s16/s8
// z[n] = sum(x[n]) / 2^q  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
void vector_real_zs32_sum_xs32(void *zptr, void *xptr, short len, char q)
{
    long config = VECZ_S32 |  VECX_S32;
    vector_real_sum(zptr, xptr, len, q, config);
}





//=========================================================================
// vector_real_zs8_x32_mul_const
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = (x[n] * const_dat/ 2^q)  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//==
void vector_real_zs8_x32_mul_const(void *zptr, void *xptr, short len, char q, long const_dat)
{

    long config = VECZ_S8 | VECX_S32;

    vector_real_mul_const(zptr, xptr, len, q, const_dat, config);

}

//add
void vector_real_zs32_x32_mul_const(void *zptr, void *xptr, short len, char q, long const_dat)
{

    long config = VECZ_S32 | VECX_S32;

    vector_real_mul_const(zptr, xptr, len, q, const_dat, config);

}
//add
void vector_real_zs32_ys32_mac_xs32(void *zptr, void *xptr, short len, char q, long const_dat)
{
    long config;
    config = VECZ_S32 |  VECY_S32 |  VECX_S32;
    vector_real_mac_const(zptr, xptr, zptr, len, q, const_dat,  config);
}
//add
void vector_real_zs32_ys8_dot_product_xs8(void *zptr, void *yptr, void *xptr, short len, char q)
{
    long config;
    config = VECZ_S32 |  VECY_S8 |  VECX_S8;
    vector_real_dot_product(zptr, yptr, xptr, len, q,  config);
}

void vector_complex_zs32_ys32_add_xs32(void *zptr, void *yptr, void *xptr, short len)
{
    long config = VECZ_S32 | VECY_S32 | VECX_S32;
    vector_complex_add(zptr, yptr, xptr, len, config);
}

void vector_complex_zs32_ys32_sub_xs32(void *zptr, void *yptr, void *xptr, short len)
{
    long config = VECZ_S32 | VECY_S32 | VECX_S32;
    vector_complex_sub(zptr, yptr, xptr, len, config);
}


///===============================================================
///  complex mul  z = y .* x / 2^q
///===============================================================
void vector_complex_zs32_ys8_mul_xs8(void *zptr, void *yptr, void *xptr, short len, char q)
{
    long config = VECZ_S32 | VECY_S8 | VECX_S8;
    vector_complex_mul(zptr, yptr, xptr, len, q, config);
}


void vector_complex_zs32_ys32_mul_xs32(void *zptr, void *yptr, void *xptr, short len, char q)
{
    long config = VECZ_S32 | VECY_S32 | VECX_S32;
    vector_complex_mul(zptr, yptr, xptr, len, q, config);
}



void vector_complex_zs32_ys32_mul_conj_xs32(void *zptr, void *yptr, void *xptr, short len, char q)
{
    long config = VECZ_S32 | VECY_S32 | VECX_S32 | VECX_CONJ;
    vector_complex_mul(zptr, yptr, xptr, len, q, config);
}



void vector_complex_zs64_ys32_mul_conj_xs32(void *zptr, void *yptr, void *xptr, short len, char q)
{
    long config = VECZ_S64 | VECY_S32 | VECX_S32 | VECX_CONJ;
    vector_complex_mul(zptr, yptr, xptr, len, q, config);
}


/////===============================================================
/////  complex quadratic  z= x^2 / 2^q
/////===============================================================
//
void vector_complex_zs64_qdt_xs32(void *zptr, void *xptr, short len, char q)
{
    long config = VECZ_S64 | VECX_S32;
    vector_complex_qdt(zptr, xptr, len, q, config);
}

void vector_complex_zs32_qdt_xs32(void *zptr, void *xptr, short len, char q)
{
    long config = VECZ_S32 | VECX_S32;
    vector_complex_qdt(zptr, xptr, len, q, config);
}



/////===============================================================
///  complex mac  z = z + y*x /2^q
/////===============================================================

void vector_complex_zs32_ys32_mac_xs32(void *zptr, void *yptr, void *xptr, short len, char q)
{

    long config = VECZ_S32 | VECY_S32 | VECX_S32;
    vector_complex_mac32(zptr, yptr, xptr, len, q, config);

}

///===============================================================
///  complex mac  z = z + y*conj(x)/2^q
///===============================================================
void vector_complex_zs32_ys32_mac_conj_xs32(void *zptr, void *yptr, void *xptr, short len, char q)
{
    long config = VECZ_S32 | VECY_S32 | VECX_S32 | VECX_CONJ;
    vector_complex_mac32(zptr, yptr, xptr, len, q, config);
}


void vector_complex_zs32_ys32_msc_xs32(void *zptr, void *yptr, void *xptr, short len, char q)
{
    long config = VECZ_S32 | VECY_S32 | VECX_S32;
    vector_complex_msc32(zptr, yptr, xptr, len, q, config);
}

///===============================================================
///  real mac  z =max( x, const) / 2^Q;
///===============================================================
void vector_real_zs32_x32_max_scale(long *zptr,  long *xptr, short len, char q, long const_dat)
{
    long config = VECZ_S32 | VECX_S32;
    vector_real_max(zptr, xptr, len, q, config, const_dat);
}


void vector_real_zs64_x64_max_scale(long  long *xptr,  long long max_const, long len)
{
    long long buf64;
    unsigned long  cnt;

    for (cnt = 0; cnt < len; cnt++) {
        buf64 = *xptr;
        if (buf64  < max_const) {
            *xptr =     max_const ;
        }
        xptr++;
    }
}

///===============================================================
///  real mac  z =min( x, const) / 2^Q;
///===============================================================
void vector_real_zs32_x32_min_scale(long *zptr,  long *xptr, short len, char q, long const_dat)
{
    long config = VECZ_S32 | VECX_S32;
    vector_real_min(zptr, xptr, len, q, config, const_dat);
}

void vector_real_zs64_x64_min_scale(long  long *xptr,  long long min_const, long len)
{
    long long buf64;
    unsigned long  cnt;

    for (cnt = 0; cnt < len; cnt++) {
        buf64 = *xptr;
        if (buf64  >= min_const) {
            *xptr =     min_const ;
        }
        xptr++;
    }
}



///===============================================================
///  z= z*lamda + (（ (1<<q) - lamda ） *  x) >> q
///  z =x + ((z- x)*lamda ) >> q
///===============================================================
void vector_real_smooth_zs64_xs64(long long *zptr,  long long *xptr,  unsigned long len, char q, long lamda)
{
    long long buf64;
    unsigned long cnt;
    for (cnt = 0; cnt < len; cnt++) {
        buf64 = *xptr++;
        buf64 = (buf64 + ((*zptr - buf64) *  lamda)) >> q;
        *zptr++ = buf64 ;
    }

}


void vector_real_smooth_zs32_xs32(long *zptr,  long *xptr,  unsigned long len, char q, long lamda)
{
    long config;
    config = VECZ_S32 |  VECY_S32 |  VECX_S32;
    vector_real_zs32_ys32_sub_xs32(zptr, zptr, xptr, len);

    vector_real_mac_const(zptr, zptr, xptr, len, q, lamda,  config);
}




void vector_complex_smooth_zs32_xs32(long *zptr,  long *xptr,  unsigned long len, char q, long lamda)
{
    long config;
    config = VECZ_S32 |  VECY_S32 |  VECX_S32;
    vector_real_zs32_ys32_sub_xs32(zptr, zptr, xptr, len * 2);

    vector_real_mac_const(zptr, zptr, xptr, len * 2, q, lamda,  config);
}




//=====================================
//  z[0] = (x[0]^2 + x[1]^2)^0.5
//  z[1] = (x[2]^2 + x[3]^2)^0.5
//...
//  z[n] = (x[2*n]^2 + x[2*n+1]^2)^0.5
// result effective precision : 22bit
//=====================================


void complex_abs_s32(unsigned long *zptr, long *xptr, unsigned long len)
{
    unsigned long cnt;
    long long d64;
    long long s64;
    long tmp;
    unsigned char sel;
    long inv_b = 20376027;
    for (cnt = 0; cnt < len ; cnt ++) {
        tmp = *xptr++;
        s64 = ((long long) * xptr++ << 32) | (long long)((tmp) & 0xffffffff);
        sel = (0x01 << 2) | (3);
        __asm__ volatile("%0 = copex(%1) (%2)" : "=r"(d64) : "r"(s64), "i"(sel));
        tmp = d64 >> 32;
        d64 = (long long)tmp *  inv_b;
        __asm__ volatile("%0 = %1>>>25(round)" : "=r"(tmp) : "r"(d64));
        //*zptr++ = d64 >>25;
        *zptr++ = tmp;//d64 >>25;
    }
}
