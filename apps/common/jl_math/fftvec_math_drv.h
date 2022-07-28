#ifndef __FFTVEC_MATH_DRV_H_
#define __FFTVEC_MATH_DRV_H_

typedef struct  {
    unsigned long fft_con;
    unsigned long fft_iadr;
    unsigned long fft_oadr;
    unsigned long fft_null;
} hwfft_ctx_t;

#define  COMPLEX_FFT10_CONFIG   ((2<<28)|(10<<16)|(1<<8)|(1<<13)|(0<<12)|(0<<4)|(0<<2)|(0<<1))  // res x 1
#define  COMPLEX_IFFT10_CONFIG  ((2<<28)|(10<<16)|(1<<8)|(1<<13)|(0<<12)|(0<<4)|(1<<2)|(0<<1))  // res x 1.125
#define  COMPLEX_FFT20_CONFIG   ((2<<28)|(20<<16)|(2<<8)|(1<<13)|(0<<12)|(0<<4)|(0<<2)|(0<<1))  // res x 1
#define  COMPLEX_IFFT20_CONFIG  ((3<<28)|(20<<16)|(2<<8)|(1<<13)|(0<<12)|(0<<4)|(1<<2)|(0<<1))  // res x 1.125
#define  COMPLEX_FFT15_CONFIG   ((2<<28)|(15<<16)|(0<<8)|(1<<13)|(1<<12)|(0<<4)|(0<<2)|(0<<1))  // res x 1
#define  COMPLEX_IFFT15_CONFIG  ((2<<28)|(15<<16)|(0<<8)|(1<<13)|(1<<12)|(0<<4)|(1<<2)|(0<<1))  // res x 1.875
#define  COMPLEX_FFT30_CONFIG   ((2<<28)|(30<<16)|(1<<8)|(1<<13)|(1<<12)|(0<<4)|(0<<2)|(0<<1))  // res x 1
#define  COMPLEX_IFFT30_CONFIG  ((3<<28)|(30<<16)|(1<<8)|(1<<13)|(1<<12)|(0<<4)|(1<<2)|(0<<1))  // res x 1.875
#define  COMPLEX_FFT32_CONFIG   ((2<<28)|(32<<16)|(5<<8)|(0<<13)|(0<<12)|(1<<4)|(0<<2)|(0<<1))  // res x 1
#define  COMPLEX_IFFT32_CONFIG  ((4<<28)|(32<<16)|(5<<8)|(0<<13)|(0<<12)|(1<<4)|(1<<2)|(0<<1))  // res x 1
#define  COMPLEX_FFT40_CONFIG   ((2<<28)|(40<<16)|(3<<8)|(1<<13)|(0<<12)|(0<<4)|(0<<2)|(0<<1))  // res x 1
#define  COMPLEX_IFFT40_CONFIG  ((4<<28)|(40<<16)|(3<<8)|(1<<13)|(0<<12)|(0<<4)|(1<<2)|(0<<1))  // res x 1.25
#define  COMPLEX_FFT60_CONFIG   ((2<<28)|(60<<16)|(2<<8)|(1<<13)|(1<<12)|(0<<4)|(0<<2)|(0<<1))  // res x 1
#define  COMPLEX_IFFT60_CONFIG  ((2<<28)|(60<<16)|(2<<8)|(1<<13)|(1<<12)|(0<<4)|(1<<2)|(0<<1))  // res x 1.875
#define  COMPLEX_FFT64_CONFIG   ((2<<28)|(64<<16)|(6<<8)|(0<<13)|(0<<12)|(2<<4)|(0<<2)|(0<<1))  // res x 1
#define  COMPLEX_IFFT64_CONFIG  ((3<<28)|(64<<16)|(6<<8)|(0<<13)|(0<<12)|(0<<4)|(1<<2)|(0<<1))  // res x 1
#define  COMPLEX_FFT80_CONFIG   ((2<<28)|(80<<16)|(4<<8)|(1<<13)|(0<<12)|(1<<4)|(0<<2)|(0<<1))  // res x 1
#define  COMPLEX_IFFT80_CONFIG  ((3<<28)|(80<<16)|(4<<8)|(1<<13)|(0<<12)|(0<<4)|(1<<2)|(0<<1))  // res x 1.25
#define  COMPLEX_FFT120_CONFIG  ((2<<28)|(120<<16)|(3<<8)|(1<<13)|(1<<12)|(0<<4)|(0<<2)|(0<<1))  // res x 1  1778 cyc
#define  COMPLEX_IFFT120_CONFIG ((3<<28)|(120<<16)|(3<<8)|(1<<13)|(1<<12)|(0<<4)|(1<<2)|(0<<1))  // res x 1.875
#define  COMPLEX_FFT128_CONFIG  ((2<<28)|(128<<16)|(7<<8)|(0<<13)|(0<<12)|(2<<4)|(0<<2)|(0<<1))  // res x 1 1660 cyc
#define  COMPLEX_IFFT128_CONFIG ((4<<28)|(128<<16)|(7<<8)|(0<<13)|(0<<12)|(1<<4)|(1<<2)|(0<<1))  // res x 1
#define  COMPLEX_FFT160_CONFIG  ((2<<28)|(160<<16)|(5<<8)|(1<<13)|(0<<12)|(1<<4)|(0<<2)|(0<<1))  // res x 1  2189
#define  COMPLEX_IFFT160_CONFIG ((4<<28)|(160<<16)|(5<<8)|(1<<13)|(0<<12)|(0<<4)|(1<<2)|(0<<1))  // res x 1.25
#define  COMPLEX_FFT240_CONFIG  ((2<<28)|(240<<16)|(4<<8)|(1<<13)|(1<<12)|(1<<4)|(0<<2)|(0<<1))  // res x 1  3336
#define  COMPLEX_IFFT240_CONFIG ((4<<28)|(240<<16)|(4<<8)|(1<<13)|(1<<12)|(1<<4)|(1<<2)|(0<<1))  // res x 1.875
#define  COMPLEX_FFT256_CONFIG  ((2<<28)|(256<<16)|(8<<8)|(0<<13)|(0<<12)|(3<<4)|(0<<2)|(0<<1))  // res x 1  3456
#define  COMPLEX_IFFT256_CONFIG ((3<<28)|(256<<16)|(8<<8)|(0<<13)|(0<<12)|(0<<4)|(1<<2)|(0<<1))  // res x 1
#define  COMPLEX_FFT320_CONFIG  ((2<<28)|(320<<16)|(6<<8)|(1<<13)|(0<<12)|(2<<4)|(0<<2)|(0<<1))  // res x 1  4133
#define  COMPLEX_IFFT320_CONFIG ((3<<28)|(320<<16)|(6<<8)|(1<<13)|(0<<12)|(0<<4)|(1<<2)|(0<<1))  // res x 1.25
#define  COMPLEX_FFT480_CONFIG  ((2<<28)|(480<<16)|(5<<8)|(1<<13)|(1<<12)|(2<<4)|(0<<2)|(0<<1))  // res x 1  8023
#define  COMPLEX_IFFT480_CONFIG ((3<<28)|(480<<16)|(5<<8)|(1<<13)|(1<<12)|(0<<4)|(1<<2)|(0<<1))  // res x 1.875
#define  COMPLEX_FFT512_CONFIG  ((2<<28)|(512<<16)|(9<<8)|(0<<13)|(0<<12)|(3<<4)|(0<<2)|(0<<1))  // res x 1  7780
#define  COMPLEX_IFFT512_CONFIG ((4<<28)|(512<<16)|(9<<8)|(0<<13)|(0<<12)|(1<<4)|(1<<2)|(0<<1))  // res x 1
#define  COMPLEX_FFT960_CONFIG  ((2<<28)|(960<<16)|(6<<8)|(1<<13)|(1<<12)|(2<<4)|(0<<2)|(0<<1))  // res x 1  15469
#define  COMPLEX_IFFT960_CONFIG ((4<<28)|(960<<16)|(6<<8)|(1<<13)|(1<<12)|(1<<4)|(1<<2)|(0<<1))  // res x 1.875
#define  COMPLEX_FFT1024_CONFIG  ((2<<28)|(1024<<16)|(10<<8)|(0<<13)|(0<<12)|(4<<4)|(0<<2)|(0<<1))  // res x 1 16262
#define  COMPLEX_IFFT1024_CONFIG ((3<<28)|(1024<<16)|(10<<8)|(0<<13)|(0<<12)|(0<<4)|(1<<2)|(0<<1))  // res x 1
#define  COMPLEX_FFT2048_CONFIG  ((2<<28)|(2048<<16)|(11<<8)|(0<<13)|(0<<12)|(4<<4)|(0<<2)|(0<<1))  // res x 1
#define  COMPLEX_IFFT2048_CONFIG ((4<<28)|(2048<<16)|(11<<8)|(0<<13)|(0<<12)|(1<<4)|(1<<2)|(0<<1))  // res x 1


//LAST_DIV  FFT_NU  LOG2    FFT5   FFT3 fft4_sDIV4 IFF  REAL
#define  REAL_FFT20_CONFIG     ((3<<28)|(20<<16)|(1<<8)|(1<<13)|(0<<12)|(0<<4)|(0<<2)|(1<<1))  // res x 1      180
#define  REAL_IFFT20_CONFIG    ((3<<28)|(20<<16)|(1<<8)|(1<<13)|(0<<12)|(0<<4)|(1<<2)|(1<<1))  // res x 1.125
#define  REAL_FFT30_CONFIG     ((3<<28)|(30<<16)|(0<<8)|(1<<13)|(1<<12)|(0<<4)|(0<<2)|(1<<1))  // res x 1      238
#define  REAL_IFFT30_CONFIG    ((3<<28)|(30<<16)|(0<<8)|(1<<13)|(1<<12)|(0<<4)|(1<<2)|(1<<1))  // res x 1.875
#define  REAL_FFT40_CONFIG     ((3<<28)|(40<<16)|(2<<8)|(1<<13)|(0<<12)|(0<<4)|(0<<2)|(1<<1))  // res x 1      292
#define  REAL_IFFT40_CONFIG    ((4<<28)|(40<<16)|(2<<8)|(1<<13)|(0<<12)|(0<<4)|(1<<2)|(1<<1))  // res x 1.25
#define  REAL_FFT60_CONFIG     ((3<<28)|(60<<16)|(1<<8)|(1<<13)|(1<<12)|(0<<4)|(0<<2)|(1<<1))  // res x 1      516
#define  REAL_IFFT60_CONFIG    ((4<<28)|(60<<16)|(1<<8)|(1<<13)|(1<<12)|(0<<4)|(1<<2)|(1<<1))  // res x 1.875
#define  REAL_FFT64_CONFIG     ((3<<28)|(64<<16)|(5<<8)|(0<<13)|(0<<12)|(1<<4)|(0<<2)|(1<<1))  // res x 1      485
#define  REAL_IFFT64_CONFIG    ((3<<28)|(64<<16)|(5<<8)|(0<<13)|(0<<12)|(0<<4)|(1<<2)|(1<<1))  // res x 1
#define  REAL_FFT80_CONFIG     ((3<<28)|(80<<16)|(3<<8)|(1<<13)|(0<<12)|(1<<4)|(0<<2)|(1<<1))  // res x 1     635
#define  REAL_IFFT80_CONFIG    ((3<<28)|(80<<16)|(3<<8)|(1<<13)|(0<<12)|(0<<4)|(1<<2)|(1<<1))  // res x 1.25
#define  REAL_FFT120_CONFIG    ((3<<28)|(120<<16)|(2<<8)|(1<<13)|(1<<12)|(0<<4)|(0<<2)|(1<<1))  // res x 1  934  cyc
#define  REAL_IFFT120_CONFIG   ((3<<28)|(120<<16)|(2<<8)|(1<<13)|(1<<12)|(0<<4)|(1<<2)|(1<<1))  // res x 1.875
#define  REAL_FFT128_CONFIG    ((3<<28)|(128<<16)|(6<<8)|(0<<13)|(0<<12)|(2<<4)|(0<<2)|(1<<1))  // res x 1 912 cyc
#define  REAL_IFFT128_CONFIG   ((4<<28)|(128<<16)|(6<<8)|(0<<13)|(0<<12)|(0<<4)|(1<<2)|(1<<1))  // res x 1
#define  REAL_FFT160_CONFIG    ((3<<28)|(160<<16)|(4<<8)|(1<<13)|(0<<12)|(1<<4)|(0<<2)|(1<<1))  // res x 1  1133
#define  REAL_IFFT160_CONFIG   ((4<<28)|(160<<16)|(4<<8)|(1<<13)|(0<<12)|(0<<4)|(1<<2)|(1<<1))  // res x 1.25
#define  REAL_FFT240_CONFIG    ((3<<28)|(240<<16)|(3<<8)|(1<<13)|(1<<12)|(1<<4)|(0<<2)|(1<<1))  // res x 1  2155
#define  REAL_IFFT240_CONFIG   ((4<<28)|(240<<16)|(3<<8)|(1<<13)|(1<<12)|(1<<4)|(1<<2)|(1<<1))  // res x 1.875
#define  REAL_FFT256_CONFIG    ((3<<28)|(256<<16)|(7<<8)|(0<<13)|(0<<12)|(3<<4)|(0<<2)|(1<<1))  // res x 1  2061
#define  REAL_IFFT256_CONFIG   ((3<<28)|(256<<16)|(7<<8)|(0<<13)|(0<<12)|(0<<4)|(1<<2)|(1<<1))  // res x 1
#define  REAL_FFT320_CONFIG    ((3<<28)|(320<<16)|(5<<8)|(1<<13)|(0<<12)|(2<<4)|(0<<2)|(1<<1))  // res x 1  2686
#define  REAL_IFFT320_CONFIG   ((3<<28)|(320<<16)|(5<<8)|(1<<13)|(0<<12)|(0<<4)|(1<<2)|(1<<1))  // res x 1.25
#define  REAL_FFT480_CONFIG    ((3<<28)|(480<<16)|(4<<8)|(1<<13)|(1<<12)|(2<<4)|(0<<2)|(1<<1))  // res x 1  4073
#define  REAL_IFFT480_CONFIG   ((3<<28)|(480<<16)|(4<<8)|(1<<13)|(1<<12)|(0<<4)|(1<<2)|(1<<1))  // res x 1.875
#define  REAL_FFT512_CONFIG    ((3<<28)|(512<<16)|(8<<8)|(0<<13)|(0<<12)|(3<<4)|(0<<2)|(1<<1))  // res x 1  4239
#define  REAL_IFFT512_CONFIG   ((4<<28)|(512<<16)|(8<<8)|(0<<13)|(0<<12)|(0<<4)|(1<<2)|(1<<1))  // res x 1
#define  REAL_FFT960_CONFIG    ((3<<28)|(960<<16)|(5<<8)|(1<<13)|(1<<12)|(2<<4)|(0<<2)|(1<<1))  // res x 1  9480
#define  REAL_IFFT960_CONFIG   ((4<<28)|(960<<16)|(5<<8)|(1<<13)|(1<<12)|(0<<4)|(1<<2)|(1<<1))  // res x 1.875
#define  REAL_FFT1024_CONFIG   ((3<<28)|(1024<<16)|(9<<8)|(0<<13)|(0<<12)|(4<<4)|(0<<2)|(1<<1))// res x 1  9333
#define  REAL_IFFT1024_CONFIG  ((3<<28)|(1024<<16)|(9<<8)|(0<<13)|(0<<12)|(0<<4)|(1<<2)|(1<<1))// res x 1
#define  REAL_FFT2048_CONFIG   ((3<<28)|(2048<<16)|(10<<8)|(0<<13)|(0<<12)|(4<<4)|(0<<2)|(1<<1))  // res x 1 19351
#define  REAL_IFFT2048_CONFIG  ((4<<28)|(2048<<16)|(10<<8)|(0<<13)|(0<<12)|(0<<4)|(1<<2)|(1<<1))  // res x 1

#define FFT_SAME_ADR  (1<<0)



typedef struct {
    unsigned long  vector_con;
    unsigned long  vector_xadr;
    unsigned long  vector_yadr;
    unsigned long  vector_zadr;
    unsigned long  vector_config0;
    unsigned long  vector_config1;
    unsigned long  vector_config2;
    unsigned long  null;
} hwvec_ctx_t;


#define  VECX_S32           (2<<6)
#define  VECX_S16           (1<<6)
#define  VECX_S8            (0<<6)

#define  VECY_S32           (2<<8)
#define  VECY_S16           (1<<8)
#define  VECY_S8            (0<<8)

#define  VECZ_S64           (3<<10)
#define  VECZ_S32           (2<<10)
#define  VECZ_S16           (1<<10)
#define  VECZ_S8            (0<<10)


#define  VEC_MIN            (1<<20)
#define  VEC_MAX            (1<<19)
#define  VEC_CPX_QDT        (1<<18)
#define  VEC_CONST_MUL      (1<<17)
#define  VEC_RES_NEG_ZERO   (1<<16)
#define  VEC_MAC32          (1<<15)
#define  VEC_QDT            (1<<14)
#define  VECY_CONJ          (1<<13)
#define  VECX_CONJ          (1<<12)

#define  VEC_ACC            (1<<5)
#define  VEC_SUB            (1<<4)

#define  VEC_REAL_ADDSUB_MODE            (0<<0)
#define  VEC_REAL_MUL_MODE               (1<<0)
#define  VEC_CPX_MUL_MODE                (2<<0)
#define  VEC_QDT_MODE                    (3<<0)


// FIX_ME,借用硬件无用到的BIT
#define  VEC_CPY_REAL_OR_VEC             (1<<31)        //区分复制长度是实数还是向量, just for mark,
#define  VEC_ZPTR_CPY                    (1<<30)        //是否需要复制输入ZPTR, just for mark,
#define  VEC_ZPTR_CPY_DOT                (1<<29)        //区分ZPTR输出结果复制长度是一个点还是向量, just for mark,


void hwfft_exec_batch(hwfft_ctx_t *ctx, int nbatch);
void hwfft_exec_once(hwfft_ctx_t *ctx);
int hwfft_get_nfft(unsigned long fft_config);
//0---forward  1---backward
int hwfft_get_backward(unsigned long fft_config);
//0---complex  1---real
int hwfft_get_real(unsigned long fft_config);
int hwfft_check_fft_config(unsigned long fft_config, int nfft, int backward, int real);
void hwfft_fill_fft_ctx(hwfft_ctx_t *ctx, unsigned long fft_config, long *iadr, long *oadr);
void hwfft_ifft_scale(long *iadr, int nfft);
void hwfft_fft_i32(hwfft_ctx_t *ctx);
void hwfft_ifft_i32(hwfft_ctx_t *ctx);
void hwfft_fft_f32(hwfft_ctx_t *ctx, float scale);
void hwfft_ifft_f32(hwfft_ctx_t *ctx, float scale);

void fast_i2f(const int *in, float *out, int N, float scale);
void fast_f2i(const float *in, int *out, int N, float scale);


//////////////////////////////////////////////////////////////////////////
//VECTOR

void hwvec_exec(
    void *xptr,
    void *yptr,
    void *zptr,
    short x_inc,
    short y_inc,
    short z_inc,
    short nlen,
    short nloop,
    char q,
    long config,
    long const_dat
);

void vector_complex_add(void *zptr, void *yptr, void *xptr, short len, long config);
void vector_complex_sub(void *zptr, void *yptr, void *xptr, short len, long config);
void vector_complex_mul(void *zptr, void *yptr, void *xptr, short len, char q, long config);
void vector_complex_mac32(void *zptr, void *yptr, void *xptr, short len, char q, long config);
void vector_complex_msc32(void *zptr, void *yptr, void *xptr, short len, char q, long config);
void vector_complex_dot_product(void *zptr, void *yptr, void *xptr, short len, char q, long config);
void vector_complex_qdt(void *zptr, void *xptr, short len, char q, long config);
void vector_complex_qdtsum(void *zptr, void *xptr, short len, char q, long config);
void vector_real_add(void *zptr, void *yptr, void *xptr, short len, long config);
void vector_real_sub(void *zptr, void *yptr, void *xptr, short len, long config);
void vector_real_mul(void *zptr, void *yptr, void *xptr, short len, char q, long config);
void vector_real_mac32(void *zptr, void *yptr, void *xptr, short len, char q, long config);
void vector_real_msc32(void *zptr, void *yptr, void *xptr, short len, char q, long config);
void vector_real_dot_product(void *zptr, void *yptr, void *xptr, short len, char q, long config);
void vector_real_sum(void *zptr, void *xptr, short len, char q, long config);
void vector_real_qdt(void *zptr, void *xptr, short len, char q, long config);
void vector_real_qdtsum(void *zptr, void *xptr, short len, char q, long config);
void vector_real_mac_const(void *zptr, void *yptr, void *xptr, short len, char q, long const_dat, long config);
void vector_real_msc_const(void *zptr, void *yptr, void *xptr, short len, char q, long const_dat, long config);
void vector_real_mul_const(void *zptr, void *xptr, short len, char q, long const_dat, long config);
void vector_real_max(void *zptr, void *xptr, short len, char q, long config, long const_dat);
void vector_real_min(void *zptr, void *xptr, short len, char q, long config, long const_dat);

#endif
