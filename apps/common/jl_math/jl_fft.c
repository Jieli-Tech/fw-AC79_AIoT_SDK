#include "system/includes.h"
#include "app_config.h"
#include "fftvec_math_drv.h"
#include "_kiss_fft_guts.h"


#if defined CONFIG_ASR_ALGORITHM || defined CONFIG_REVERB_MODE_ENABLE || defined CONFIG_AEC_ENC_ENABLE || defined CONFIG_M4A_DEC_ENABLE

static DEFINE_SPINLOCK(fft_lock);

#define  FFT_REAL       1                 //complex
#define  IFFT_DIV2      (3-FFT_REAL)
#define  FFT_ISR_IE     0

//软件fft算法
/* #define KISS_FFT */

#define SHR(a,shift)    ((a) >> (shift))

#define PSHR(a,shift)   ((a)>=0?(SHR((a)+(1<<(shift-1)),shift)):\
                            (SHR((a)+(1<<(shift-1))-1,shift)))

#if defined CONFIG_CPU_WL82 && defined CONFIG_ASR_ALGORITHM
SEC_USED(.fft_data) static int datain[1024];
SEC_USED(.fft_data) static int datain_y[1024];
SEC_USED(.fft_data) static int dataout[1024 + 2];
#else
#if defined CONFIG_VIDEO_ENABLE
SEC_USED(.fft_data) static int datain[1024 + 2];	//wma硬件fft解码需要改成2048
SEC_USED(.fft_data) static int *dataout = datain;
SEC_USED(.fft_data) static int datain_y[0];
#else
SEC_USED(.fft_data) static int datain[1024];
SEC_USED(.fft_data) static int dataout[1024 + 2];
SEC_USED(.fft_data) static int datain_y[0];
#endif
#endif

typedef struct fft_config_dat {
    unsigned long fft0_con;
    unsigned long fft0_iadr;
    unsigned long fft0_oadr;
    unsigned long fft0_null;
} fft_config_struct;

SEC_USED(.fft_data) static fft_config_struct fft_config;
SEC_USED(.fft_data) static hwvec_ctx_t g_vector_core_set;

static const struct {
    u32 block;
    u32 config;
} complex_tab[] = {
    { 128, COMPLEX_FFT128_CONFIG  },
    { 128, COMPLEX_IFFT128_CONFIG },
    { 256, COMPLEX_FFT256_CONFIG  },
    { 256, COMPLEX_IFFT256_CONFIG },
    { 512, COMPLEX_FFT512_CONFIG  },
    { 512, COMPLEX_IFFT512_CONFIG },
    { 1024, COMPLEX_FFT1024_CONFIG },
    { 1024, COMPLEX_IFFT1024_CONFIG},
    { 2048, COMPLEX_FFT2048_CONFIG },
    { 2048, COMPLEX_IFFT2048_CONFIG},
    { 10, COMPLEX_FFT10_CONFIG   },
    { 10, COMPLEX_IFFT10_CONFIG  },
    { 15, COMPLEX_FFT15_CONFIG   },
    { 15, COMPLEX_IFFT15_CONFIG  },
    { 20, COMPLEX_FFT20_CONFIG   },
    { 20, COMPLEX_IFFT20_CONFIG  },
    { 30, COMPLEX_FFT30_CONFIG   },
    { 30, COMPLEX_IFFT30_CONFIG  },
    { 32, COMPLEX_FFT32_CONFIG   },
    { 32, COMPLEX_IFFT32_CONFIG  },
    { 40, COMPLEX_FFT40_CONFIG   },
    { 40, COMPLEX_IFFT40_CONFIG  },
    { 60, COMPLEX_FFT60_CONFIG   },
    { 60, COMPLEX_IFFT60_CONFIG  },
    { 64, COMPLEX_FFT64_CONFIG   },
    { 64, COMPLEX_IFFT64_CONFIG  },
    { 80, COMPLEX_FFT80_CONFIG   },
    { 80, COMPLEX_IFFT80_CONFIG  },
    { 120, COMPLEX_FFT120_CONFIG  },
    { 120, COMPLEX_IFFT120_CONFIG },
    { 160, COMPLEX_FFT160_CONFIG  },
    { 160, COMPLEX_IFFT160_CONFIG },
    { 240, COMPLEX_FFT240_CONFIG  },
    { 240, COMPLEX_IFFT240_CONFIG },
    { 320, COMPLEX_FFT320_CONFIG  },
    { 320, COMPLEX_IFFT320_CONFIG },
    { 480, COMPLEX_FFT480_CONFIG  },
    { 480, COMPLEX_IFFT480_CONFIG },
    { 960, COMPLEX_FFT960_CONFIG  },
    { 960, COMPLEX_IFFT960_CONFIG },
};

static const struct {
    u32 block;
    u32 config;
} real_tab[] = {
    { 128, REAL_FFT128_CONFIG  },
    { 128, REAL_IFFT128_CONFIG },
    { 256, REAL_FFT256_CONFIG  },
    { 256, REAL_IFFT256_CONFIG },
    { 512, REAL_FFT512_CONFIG  },
    { 512, REAL_IFFT512_CONFIG },
    { 1024, REAL_FFT1024_CONFIG },
    { 1024, REAL_IFFT1024_CONFIG},
    { 2048, REAL_FFT2048_CONFIG },
    { 2048, REAL_IFFT2048_CONFIG},
    { 20, REAL_FFT20_CONFIG   },
    { 20, REAL_IFFT20_CONFIG  },
    { 30, REAL_FFT30_CONFIG   },
    { 30, REAL_IFFT30_CONFIG  },
    { 40, REAL_FFT40_CONFIG   },
    { 40, REAL_IFFT40_CONFIG  },
    { 60, REAL_FFT60_CONFIG   },
    { 60, REAL_IFFT60_CONFIG  },
    { 64, REAL_FFT64_CONFIG   },
    { 64, REAL_IFFT64_CONFIG  },
    { 80, REAL_FFT80_CONFIG   },
    { 80, REAL_IFFT80_CONFIG  },
    { 120, REAL_FFT120_CONFIG  },
    { 120, REAL_IFFT120_CONFIG },
    { 160, REAL_FFT160_CONFIG  },
    { 160, REAL_IFFT160_CONFIG },
    { 240, REAL_FFT240_CONFIG  },
    { 240, REAL_IFFT240_CONFIG },
    { 320, REAL_FFT320_CONFIG  },
    { 320, REAL_IFFT320_CONFIG },
    { 480, REAL_FFT480_CONFIG  },
    { 480, REAL_IFFT480_CONFIG },
    { 960, REAL_FFT960_CONFIG  },
    { 960, REAL_IFFT960_CONFIG },
};


#ifdef KISS_FFT

//是否使用软硬混合fft
#define SOFT_FFT_ENABLE

typedef struct kiss_config {
    kiss_fft_cfg forward;
    kiss_fft_cfg backward;
    int N;
    float *fft_in;
    float *fft_out;
} kiss_config_t;

static kiss_config_t *kiss_512;
static kiss_config_t *kiss_1024;


static u8 spin_is_lock()
{
    return fft_lock.rwlock;
}

//input: only real
//output: interleave real and image, size is (N/2+1)*2
__attribute__((always_inline))
void kiss_cfg_fft(void *table, int *in, int *out)
{
    kiss_config_t *t = (kiss_config_t *)table;

    float scale = 1;// / t->N;

    for (int i = 0; i < t->N; i++) {
        t->fft_in[i * 2] = in[i] * scale;
        t->fft_in[i * 2 + 1] = 0;
    }

    kiss_fft(t->forward, t->fft_in, t->fft_out);

    for (int i = 0; i < t->N + 2; i++) {
        out[i] = t->fft_out[i];
    }
}

//output: only real
//input: interleave real and image, size is (N/2+1)*2
__attribute__((always_inline))
void kiss_cfg_ifft(void *table, int *in, int *out)
{
    kiss_config_t *t = (kiss_config_t *)table;

    float scale;
    int i;
    scale = 1.0f / t->N;

    for (int i = 0; i < t->N / 2 + 1; i++) {
        t->fft_in[i * 2] = in[i * 2];
        t->fft_in[i * 2 + 1] = in[i * 2 + 1];
    }
    for (int i = t->N / 2 + 1; i < t->N; i++) {
        t->fft_in[i * 2] = t->fft_in[(t->N - i) * 2];
        t->fft_in[i * 2 + 1] = -t->fft_in[(t->N - i) * 2 + 1];
    }

    kiss_fft(t->backward, t->fft_in, t->fft_out);

    for (int i = 0; i < t->N; i++) {
        out[i] = t->fft_out[i * 2];
    }

    for (i = 0; i < t->N; i++) {
        out[i] *= scale;
    }

}


void kiss_config_uninit()
{
    if (kiss_512->fft_in) {
        free(kiss_512->fft_in);
        kiss_512->fft_in = NULL;
    }
    if (kiss_512->fft_out) {
        free(kiss_512->fft_out);
        kiss_512->fft_out = NULL;
    }
    kiss_fft_free(kiss_512->forward);
    kiss_fft_free(kiss_512->backward);
    free(kiss_512);

    if (kiss_1024->fft_in) {
        free(kiss_1024->fft_in);
        kiss_1024->fft_in = NULL;
    }
    if (kiss_1024->fft_out) {
        free(kiss_1024->fft_out);
        kiss_1024->fft_out = NULL;
    }
    kiss_fft_free(kiss_1024->forward);
    kiss_fft_free(kiss_1024->backward);
    free(kiss_1024);
}

void kiss_config_init()
{
    kiss_512 = (kiss_config_t *)calloc(1, sizeof(kiss_config_t));
    if (!kiss_512) {
        printf("%s, kiss_512 malloc fail\n\r", __func__);
        return;
    }
    kiss_1024 = (kiss_config_t *)calloc(1, sizeof(kiss_config_t));
    if (!kiss_1024) {
        printf("%s, kiss_1024 malloc fail\n\r", __func__);
        return;
    }

    kiss_512->N        	= 512;
    kiss_512->forward  	= kiss_fft_alloc(kiss_512->N, 0, NULL, NULL);
    kiss_512->backward 	= kiss_fft_alloc(kiss_512->N, 1, NULL, NULL);

    kiss_512->fft_in 	= (float *)calloc(1, sizeof(float) * kiss_512->N * 2);
    if (!kiss_512->fft_in) {
        printf("%s, kiss_512 fft_in malloc fail\n\r", __func__);
        return;
    }
    kiss_512->fft_out 	= (float *)calloc(1, sizeof(float) * kiss_512->N * 2);
    if (!kiss_512->fft_out) {
        printf("%s, kiss_512 fft_out malloc fail\n\r", __func__);
        return;
    }

    kiss_1024->N        	= 1024;
    kiss_1024->forward  	= kiss_fft_alloc(kiss_1024->N, 0, NULL, NULL);
    kiss_1024->backward 	= kiss_fft_alloc(kiss_1024->N, 1, NULL, NULL);

    kiss_1024->fft_in 	= (float *)calloc(1, sizeof(float) * kiss_1024->N * 2);
    if (!kiss_1024->fft_in) {
        printf("%s, kiss_1024 fft_in malloc fail\n\r", __func__);
        return;
    }
    kiss_1024->fft_out 	= (float *)calloc(1, sizeof(float) * kiss_1024->N * 2);
    if (!kiss_1024->fft_out) {
        printf("%s, kiss_1024 fft_out malloc fail\n\r", __func__);
        return;
    }

}

late_initcall(kiss_config_init);

#endif


__attribute__((always_inline))
static int vc_hw_fft(int *coef_in, int blockbit, int *coef_out, int real, int same)
{
    //init pi32v2 fft hardware
    int blocklen = 1 << blockbit;
    JL_FFT->CON = 0;

#ifdef CONFIG_CPU_WL82
    int i;

    if (real) {
        for (i = 0; i < ARRAY_SIZE(real_tab); i += 2) {
            if (blocklen == real_tab[i].block) {
                fft_config.fft0_con = real_tab[i].config | (same << 0);
                break;
            }
        }
        if (i >= ARRAY_SIZE(real_tab)) {
            return -1;
        }
    } else {
        for (i = 0; i < ARRAY_SIZE(complex_tab); i += 2) {
            if (blocklen == complex_tab[i].block) {
                fft_config.fft0_con = complex_tab[i].config | (same << 0);
                break;
            }
        }
        if (i >= ARRAY_SIZE(complex_tab)) {
            return -1;
        }
    }
#else
    fft_config.fft0_con = (blocklen << 16) | ((blockbit - 1 - real) << 8) | ((blockbit - real - 2) << 4) | (0 << 2) | (real << 1) | (same << 0);
#endif
    fft_config.fft0_iadr = (unsigned int)coef_in;
    fft_config.fft0_oadr = (unsigned int)coef_out;

    JL_FFT->CADR = (unsigned int)&fft_config;
    JL_FFT->CON = (1 << 8) | (0 << 6) | (FFT_ISR_IE << 2) | (0 << 1) | (1 << 0);
    while ((JL_FFT->CON & (1 << 7)) == 0);
    JL_FFT->CON |= (1 << 6);
    return 0;
}

__attribute__((always_inline))
static int vc_hw_ifft(int *coef_in, int blockbit, int *coef_out, int real, int same)
{
    //init pi32v2 fft hardware
    int blocklen = 1 << blockbit;
    JL_FFT->CON = 0;

#ifdef CONFIG_CPU_WL82
    int i;

    if (real) {
        for (i = 0; i < ARRAY_SIZE(real_tab); i += 2) {
            if (blocklen == real_tab[i + 1].block) {
                fft_config.fft0_con = real_tab[i + 1].config | (same << 0);
                break;
            }
        }
        if (i >= ARRAY_SIZE(real_tab)) {
            return -1;
        }
    } else {
        for (i = 0; i < ARRAY_SIZE(complex_tab); i += 2) {
            if (blocklen == complex_tab[i + 1].block) {
                fft_config.fft0_con = complex_tab[i + 1].config | (same << 0);
                break;
            }
        }
        if (i >= ARRAY_SIZE(complex_tab)) {
            return -1;
        }
    }
#else
    fft_config.fft0_con = (blocklen << 16) | ((blockbit - 1 - real) << 8) | ((3 - real) << 4) | (1 << 2) | (real << 1) | (same << 0);
#endif
    fft_config.fft0_iadr = (unsigned int)coef_in;
    fft_config.fft0_oadr = (unsigned int)coef_out;

    JL_FFT->CADR = (unsigned int)&fft_config;
    JL_FFT->CON = (1 << 8) | (0 << 6) | (FFT_ISR_IE << 2) | (0 << 1) | (1 << 0);
    while ((JL_FFT->CON & (1 << 7)) == 0);
    JL_FFT->CON |= (1 << 6);
    return 0;
}

/* SEC_USED(.fft_text) */
__attribute__((always_inline))
void jl_fft_1024(short *in, int *out)
{
    spin_lock(&fft_lock);
    for (int i = 0; i < 1024; i++) {
        datain[i] = in[i];
    }
    vc_hw_fft(datain, 10, dataout, 1, datain == dataout ? 1 : 0);
    memcpy(out, dataout, sizeof(int) * 1026);
    spin_unlock(&fft_lock);
}

/* SEC_USED(.fft_text) */
__attribute__((always_inline))
void jl_fft_1024_ex(int *in, int *out)
{
#ifdef SOFT_FFT_ENABLE
    if (spin_is_lock()) {
        kiss_cfg_fft(kiss_1024, in, out);
    } else
#endif
    {
        spin_lock(&fft_lock);
        memcpy(datain, in, sizeof(int) * 1024);
        vc_hw_fft(datain, 10, dataout, 1, datain == dataout ? 1 : 0);
        memcpy(out, dataout, sizeof(int) * 1026);
        spin_unlock(&fft_lock);
    }
}

/* SEC_USED(.fft_text) */
__attribute__((always_inline))
void jl_fft_512(int *in, int *out)
{
#ifdef SOFT_FFT_ENABLE
    if (spin_is_lock()) {
        kiss_cfg_fft(kiss_512, in, out);
    } else
#endif
    {
        spin_lock(&fft_lock);
        memcpy(datain, in, sizeof(int) * 512);
        vc_hw_fft(datain, 9, dataout, 1, datain == dataout ? 1 : 0);
        memcpy(out, dataout, sizeof(int) * 514);
        spin_unlock(&fft_lock);
    }
}

/* SEC_USED(.fft_text) */
__attribute__((always_inline))
int jl_ifft_1024(int *in, short *out)
{
    int i;
    int tData_tmp = 0, tData_max = 0, shift = 0;
    spin_lock(&fft_lock);
    memcpy(dataout, in, sizeof(int) * 1026);
    vc_hw_ifft(dataout, 10, datain, 1, dataout == datain ? 1 : 0);
    for (i = 0; i < 1024; i++) {
        tData_tmp = datain[i] > 0 ? datain[i] : (-datain[i]);

        if (tData_max < tData_tmp) {
            tData_max = tData_tmp;
        }
    }
    while (tData_max > 32767) {
        tData_max >>= 1;
        shift += 1;
    }
    for (i = 0; i < 1024; i += 4) {
        out[i] = PSHR(datain[i], shift);
        out[i + 1] = PSHR(datain[i + 1], shift);
        out[i + 2] = PSHR(datain[i + 2], shift);
        out[i + 3] = PSHR(datain[i + 3], shift);
    }
    spin_unlock(&fft_lock);
    return shift;
}

/* SEC_USED(.fft_text) */
__attribute__((always_inline))
void jl_ifft_1024_ex(int *in, int *out)
{
#ifdef SOFT_FFT_ENABLE
    if (spin_is_lock()) {
        kiss_cfg_ifft(kiss_1024, in, out);
    } else
#endif
    {
        spin_lock(&fft_lock);
        memcpy(dataout, in, sizeof(int) * 1026);
        vc_hw_ifft(dataout, 10, datain, 1, dataout == datain ? 1 : 0);
        memcpy(out, datain, sizeof(int) * 1024);
        spin_unlock(&fft_lock);
    }
}

/* SEC_USED(.fft_text) */
__attribute__((always_inline))
void jl_ifft_512(int *in, int *out)
{
#ifdef SOFT_FFT_ENABLE
    if (spin_is_lock()) {
        kiss_cfg_ifft(kiss_512, in, out);
    } else
#endif
    {
        spin_lock(&fft_lock);
        memcpy(datain, in, sizeof(int) * 514);
        vc_hw_ifft(datain, 9, dataout, 1, dataout == datain ? 1 : 0);
        memcpy(out, dataout, sizeof(int) * 512);
        spin_unlock(&fft_lock);
    }
}

__attribute__((always_inline))
static u32 find_pow(u32 value)
{
    u32 i = 0;

    while (value) {
        value >>= 1;
        if (++i > 11) {
            ASSERT(NULL, "pow <= 10");
        }
    }

    return i - 1;
}

__attribute__((always_inline))
void jl_fft_N(int *in, int *out, int fft_N)
{
    spin_lock(&fft_lock);
    memcpy(datain, in, sizeof(int) * fft_N);
    vc_hw_fft(datain, find_pow(fft_N), dataout, 1, dataout == datain ? 1 : 0);
    memcpy(out, dataout, sizeof(int) * (fft_N + 2));
    spin_unlock(&fft_lock);
}

__attribute__((always_inline))
void jl_ifft_N(int *in, int *out, int fft_N)
{
    spin_lock(&fft_lock);
    memcpy(dataout, in, sizeof(int) * (fft_N + 2));
    vc_hw_ifft(dataout, find_pow(fft_N), datain, 1, dataout == datain ? 1 : 0);
    memcpy(out, datain, sizeof(int) * fft_N);
    spin_unlock(&fft_lock);
}

__attribute__((always_inline))
void jl_complex_fft_n(int *in, int *out, int bits)
{
    int len = 1 << bits;
    spin_lock(&fft_lock);
    memcpy(datain, in, sizeof(int) * len);
    vc_hw_fft(datain, bits - 1, dataout, 0, datain == dataout ? 1 : 0);
    memcpy(out, dataout, sizeof(int) * len);
    spin_unlock(&fft_lock);
}

__attribute__((always_inline))
void jl_complex_ifft_n(int *in, int *out, int bits)
{
    int len = 1 << bits;
    spin_lock(&fft_lock);
    memcpy(datain, in, sizeof(int) * len);
    vc_hw_ifft(datain, bits - 1, dataout, 0, datain == dataout ? 1 : 0);
    memcpy(out, dataout, sizeof(int) * len);
    spin_unlock(&fft_lock);
}

#if defined CONFIG_NEW_M4A_DEC_ENABLE
int hw_fft_core(int cfg, int *coef_in, int *coef_out)
{
    int len;
    int *in = datain, *out = dataout;

    if (!coef_in || !coef_out) {
        return 0;
    }

    if (datain == dataout) {
        cfg |= BIT(0);
    } else {
        cfg &= ~BIT(0);
    }

    if (cfg & BIT(1)) {
        len = (cfg & 0xFFF0000) >> 16;
        if (cfg & BIT(2)) {
            len += 2;
            in = dataout;
            out = datain;
        }
    } else {
        len = (cfg & 0xFFF0000) >> (16 - 1);
    }

    spin_lock(&fft_lock);
    memcpy(in, coef_in, sizeof(int) * len);

    JL_FFT->CON = 0;
    fft_config.fft0_con = cfg;
    fft_config.fft0_iadr = (unsigned int)in;
    fft_config.fft0_oadr = (unsigned int)out;
    JL_FFT->CADR = (unsigned int)&fft_config;
    JL_FFT->CON = (1 << 8) | (0 << 6) | (FFT_ISR_IE << 2) | (0 << 1) | (1 << 0);
    while ((JL_FFT->CON & (1 << 7)) == 0);
    JL_FFT->CON |= (1 << 6);

    if (cfg & BIT(1)) {
        if (cfg & BIT(2)) {
            len -= 2;
        } else {
            len += 2;
        }
    }

    memcpy(coef_out, out, sizeof(int) * len);
    spin_unlock(&fft_lock);

    return 0;
}
#endif

unsigned int Get_FFT_Base(void)
{
    return JL_FFT_BASE;
}

__attribute__((always_inline)) inline
void fast_i2f(const int *in, float *out, int N, float scale)
{
    int tmp32_1;
    asm volatile(
        " 1: \n\t"
        " rep %1 { \n\t"
        " 	%0 = [%2++=4] \n\t"
        " 	%0 = itof(%0) \n\t"
        " 	%0 = %0 * %4 (f) \n\t"
        " 	[%3++=4] = %0 \n\t"
        " } \n\t"
        " if(%1>0) goto 1b \n\t"
        : "=&r"(tmp32_1),
        "=&r"(N),
        "=&r"(in),
        "=&r"(out),
        "=&r"(scale)
        : "0"(tmp32_1),
        "1"(N),
        "2"(in),
        "3"(out),
        "4"(scale)
        :);
}

__attribute__((always_inline)) inline
void fast_f2i(const float *in, int *out, int N, float scale)
{
    int tmp32_1;
    asm volatile(
        " 1: \n\t"
        " rep %1 { \n\t"
        " 	%0 = [%2++=4] \n\t"
        " 	%0 = %0 * %4 (f) \n\t"
        " 	%0 = ftoi(%0)(even) \n\t"
        " 	[%3++=4] = %0 \n\t"
        " } \n\t"
        " if(%1>0) goto 1b \n\t"
        : "=&r"(tmp32_1),
        "=&r"(N),
        "=&r"(in),
        "=&r"(out),
        "=&r"(scale)
        : "0"(tmp32_1),
        "1"(N),
        "2"(in),
        "3"(out),
        "4"(scale)
        :);
}

//hwfft_ctx_t结构体和FFT输入输出的数据地址一定要放在内部ram

__attribute__((always_inline)) inline
void hwfft_exec_batch(hwfft_ctx_t *ctx, int nbatch)
{
    spin_lock(&fft_lock);
    JL_FFT->CADR = (unsigned long)(ctx);
    //CLR    //FFT     //ie     //en
    JL_FFT->CON = (nbatch << 8) | (1 << 6) | (0 << 3) | (FFT_ISR_IE << 2) | (1 << 0);
    while ((JL_FFT->CON & (1 << 7)) == 0);
    JL_FFT->CON |= (1 << 6);
    spin_unlock(&fft_lock);
};

__attribute__((always_inline)) inline
void hwfft_exec_once(hwfft_ctx_t *ctx)
{
    spin_lock(&fft_lock);
    JL_FFT->CADR = (unsigned long)(ctx);
    //CLR    //FFT     //ie     //en
    JL_FFT->CON = (1 << 8) | (1 << 6) | (0 << 3) | (FFT_ISR_IE << 2) | (1 << 0);
    while ((JL_FFT->CON & (1 << 7)) == 0);
    JL_FFT->CON |= (1 << 6);
    spin_unlock(&fft_lock);
};

__attribute__((always_inline)) inline
int hwfft_get_nfft(unsigned long fft_config)
{
    int nfft = (fft_config >> 16) & 0xfff;
    return nfft;
};

//0---forward  1---backward
__attribute__((always_inline)) inline
int hwfft_get_backward(unsigned long fft_config)
{
    return ((fft_config & (1 << 2)) != 0);
};

//0---complex  1---real
__attribute__((always_inline)) inline
int hwfft_get_real(unsigned long fft_config)
{
    return ((fft_config & (1 << 1)) != 0);
};

__attribute__((always_inline)) inline
int hwfft_check_fft_config(unsigned long fft_config, int nfft, int backward, int real)
{
    if (hwfft_get_nfft(fft_config) != nfft) {
        printf("fft nfft mismatch!\n");
        return -1;
    }

    if (hwfft_get_backward(fft_config) != backward) {
        printf("fft backward direction mismatch!\n");
        return -1;
    }

    if (hwfft_get_real(fft_config) != real) {
        printf("fft real mode mismatch!\n");
        return -1;
    }
    return 0;
};

__attribute__((always_inline)) inline
void hwfft_fill_fft_ctx(hwfft_ctx_t *ctx, unsigned long fft_config, long *iadr, long *oadr)
{
    unsigned long config;

    if (iadr == oadr) {
        config = fft_config | FFT_SAME_ADR;
    } else {
        config = fft_config & (~FFT_SAME_ADR);
    }
    ctx->fft_con = config;
    ctx->fft_iadr = (unsigned long)iadr;
    ctx->fft_oadr = (unsigned long)oadr;
    ctx->fft_null = 0;
//    printf("config1:%d %x\n",config,ctx->fft_iadr);
};

__attribute__((always_inline)) inline
void hwfft_ifft_scale(long *iadr, int nfft)
{
    int scale;
    int q;
    long config = VECX_S32 | VECY_S32 | VECZ_S32;
//    printf("nfft:%d\n",nfft);

    if (nfft % 15 == 0) {
        scale = 1118481;
        q = 21;
        vector_real_mul_const(iadr, iadr, nfft, q, scale, config);
    } else if (nfft % 10 == 0) {
        scale = 1677722;//1864135;
        q = 21;
        vector_real_mul_const(iadr, iadr, nfft, q, scale, config);
    }
}

__attribute__((always_inline)) inline
void hwfft_fft_i32(hwfft_ctx_t *ctx)
{
    hwfft_exec_once(ctx);
};

__attribute__((always_inline)) inline
void hwfft_ifft_i32(hwfft_ctx_t *ctx)
{
    int real = hwfft_get_real(ctx->fft_con);
    int nfft = hwfft_get_nfft(ctx->fft_con);
    hwfft_exec_once(ctx);

    hwfft_ifft_scale((long *)ctx->fft_oadr, nfft * (2 - real));
};

__attribute__((always_inline)) inline
void hwfft_fft_f32(hwfft_ctx_t *ctx, float scale)
{
    int nfft = hwfft_get_nfft(ctx->fft_con);
    int real = hwfft_get_real(ctx->fft_con);
    fast_f2i((float *)ctx->fft_iadr, (int *)ctx->fft_iadr, nfft * (2 - real), scale);
    hwfft_fft_i32(ctx);
    fast_i2f((int *)ctx->fft_oadr, (float *)ctx->fft_oadr, nfft * (2 - real) + 2 * real, 1.0f / scale); // com  0  real 1
};

__attribute__((always_inline)) inline
void hwfft_ifft_f32(hwfft_ctx_t *ctx, float scale)
{
    int nfft = hwfft_get_nfft(ctx->fft_con);
    int real = hwfft_get_real(ctx->fft_con);

    fast_f2i((float *)ctx->fft_iadr, (int *)ctx->fft_iadr, nfft * (2 - real) + 2 * real, scale);

    hwfft_ifft_i32(ctx);

    fast_i2f((int *)ctx->fft_oadr, (float *)ctx->fft_oadr, nfft * (2 - real), 1.0f / scale);
};

//////////////////////////////////////////////////////////////////////////
//VECTOR
__attribute__((always_inline)) inline
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
)
{
    spin_lock(&fft_lock);

    char vec_cpy_real = 0;
    char vec_zptr_cpy_dot = 0;

    void *_xptr = &datain[0];
    void *_yptr = &datain_y[0];
    void *_zptr = &dataout[0];

    if (config & VEC_CPY_REAL_OR_VEC) {
        vec_cpy_real = 1;
        config &= ~VEC_CPY_REAL_OR_VEC;
        memcpy(_xptr, xptr, nlen * 4);
        memcpy(_yptr, yptr, nlen * 4);
    } else {
        memcpy(_xptr, xptr, nlen * 4 * 2);
        memcpy(_yptr, yptr, nlen * 4 * 2);
    }

    if (config & VEC_ZPTR_CPY) {
        config &= ~VEC_ZPTR_CPY;
        if (vec_cpy_real) {
            memcpy(_zptr, zptr, nlen * 4);
        } else {
            memcpy(_zptr, zptr, nlen * 4 * 2);
        }
    } else if (zptr == xptr) {
        _zptr = _xptr;
    } else if (zptr == yptr) {
        _zptr = _yptr;
    }

    if (config & VEC_ZPTR_CPY_DOT) {
        vec_zptr_cpy_dot = 1;
        config &= ~VEC_ZPTR_CPY_DOT;
    }

    g_vector_core_set.vector_con = config;
    g_vector_core_set.vector_config0 = (q << 24) | (nloop << 12) | (nlen);
    g_vector_core_set.vector_config1 = (z_inc << 20) | (y_inc << 10) | x_inc;
    //printf("nlen0:%d,nloop:%d,q:%d,config:%d,%d\n",nlen,nloop,q,config,const_dat);

    g_vector_core_set.vector_xadr = (unsigned long)_xptr;
    g_vector_core_set.vector_yadr = (unsigned long)_yptr;
    g_vector_core_set.vector_zadr = (unsigned long)_zptr;

    JL_FFT->CONST = const_dat;

    JL_FFT->CADR = (unsigned long)(&g_vector_core_set);
    /*printf("JL_FFT->CONST:%x\n",&g_vector_core_set);
    printf("JL_FFT->vector_xadr:%x\n",g_vector_core_set.vector_xadr);
    printf("JL_FFT->vector_yadr:%x\n",g_vector_core_set.vector_yadr);
    printf("JL_FFT->vector_zadr:%x\n",g_vector_core_set.vector_zadr);
    printf("JL_FFT->vector_config0:%x\n",g_vector_core_set.vector_config0);
    printf("JL_FFT->vector_config1:%x\n",g_vector_core_set.vector_config1);*/

    // nu      clr    vector     ie      en
    JL_FFT->CON = (1 << 8) | (1 << 6) | (1 << 3) | (FFT_ISR_IE << 2) | (1 << 0);

    while ((JL_FFT->CON & (1 << 7)) == 0);
    JL_FFT->CON |= (1 << 6);

    if (vec_zptr_cpy_dot) {
        *(int *)zptr = *(int *)_zptr;
    } else {
        memcpy(zptr, _zptr, vec_cpy_real ? nlen * 4 : nlen * 4 * 2);
    }

    spin_unlock(&fft_lock);
}



//=========================================================================
// vector_complex_add
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//
// x.real[0] = x[0], x.imge[0] = x[1]
// y.real[0] = y[0], y.imge[0] = y[1]
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = saturated( y[n] + x[n] )  // 0 <= n < len*2
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_complex_add(void *zptr, void *yptr, void *xptr, short len, long config)
{
    config = config;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, (len << 1), 1, 0, config, 0);
}

//=========================================================================
// vector_complex_sub
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//
// x.real[0] = x[0], x.imge[0] = x[1]
// y.real[0] = y[0], y.imge[0] = y[1]
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = saturated( y[n] - x[n] )  // 0 <= n < len*2
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_complex_sub(void *zptr, void *yptr, void *xptr, short len, long config)
{
    config = config | VEC_SUB;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, (len << 1), 1, 0, config, 0);
}

//=========================================================================
// vector_complex_mul
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//  -32 < Q < 31
// x.real[0] = x[0], x.imge[0] = x[1]
// y.real[0] = y[0], y.imge[0] = y[1]
// z.real[0] = z[0], z.imge[0] = z[1]
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = saturated(y[n]. * x[n]. / 2^q)  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_complex_mul(void *zptr, void *yptr, void *xptr, short len, char q, long config)
{
    config = config | VEC_CPX_MUL_MODE;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, (len << 0), 1, q, config, 0);
}

//=========================================================================
// vector_complex_mac32
// dw bit[1:0] : x_dw   2: s32
// dw bit[3:2] : y_dw   2: s32
// dw bit[5:6] : z_dw   2: s32
// y_dw == z_dw == x_dw == 2
//  -32 < Q < 31
// x.real[0] = x[0], x.imge[0] = x[1]
// y.real[0] = y[0], y.imge[0] = y[1]
// z.real[0] = z[0], z.imge[0] = z[1]
// *zptr, *yptr, *xptr:  s32
// z[n] = saturated(z. + (y[n]. * x[n]. / 2^q))  // 0 <= n < len*2
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_complex_mac32(void *zptr, void *yptr, void *xptr, short len, char q, long config)
{
    //config = VEC_MAC32 | VECZ_S32 | VECY_S32 | VECX_S32 | VEC_CPX_MUL_MODE;
    config = config | VEC_MAC32 | VECZ_S32 | VECY_S32 | VECX_S32 | VEC_CPX_MUL_MODE | VEC_ZPTR_CPY;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, (len << 0), 1, q, config, 0);
}

//=========================================================================
// vector_complex_msc32
// dw bit[1:0] : x_dw   2: s32
// dw bit[3:2] : y_dw   2: s32
// dw bit[5:6] : z_dw   2: s32
// y_dw == z_dw == x_dw == 2
//  -32 < Q < 31
// x.real[0] = x[0], x.imge[0] = x[1]
// y.real[0] = y[0], y.imge[0] = y[1]
// z.real[0] = z[0], z.imge[0] = z[1]
// *zptr, *yptr, *xptr:  s32
// z[n] = saturated(z[n]. - (y[n]. * x[n]. / 2^q))  // 0 <= n < len*2
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_complex_msc32(void *zptr, void *yptr, void *xptr, short len, char q, long config)
{
    config = config | VEC_MAC32 | VECZ_S32 | VECY_S32 | VECX_S32 | VEC_SUB | VEC_CPX_MUL_MODE;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, (len << 1), 1, q, config, 0);
}

//=========================================================================
// vector_complex_dot_product
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//  -32 < Q < 31
// x.real[0] = x[0], x.imge[0] = x[1]
// y.real[0] = y[0], y.imge[0] = y[1]
// z.real[0] = z[0], z.imge[0] = z[1]
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = saturated(sum(y[n]. * x[n]. / 2^q))  // 0 <= n < len*2
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_complex_dot_product(void *zptr, void *yptr, void *xptr, short len, char q, long config)
{
    config = config | VEC_ACC | VEC_CPX_MUL_MODE;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, (len << 1), 1, q, config, 0);
}

//=========================================================================
// vector_complex_qdt
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//  -32 < Q < 31
// x.real[0] = x[0], x.imge[0] = x[1]
// z.real[0] = z[0], z.imge[0] = z[1]
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = saturated((x[n].real^2 + x[n].imag^2) / 2^q)  // 0 <= n < len/2
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_complex_qdt(void *zptr, void *xptr, short len, char q, long config)
{
    config = config | VEC_CPX_QDT | VEC_QDT | VEC_QDT_MODE;
    hwvec_exec(xptr, 0, zptr, 0, 0, 0, (len >> 1), 1, q, config, 0);
}

//=========================================================================
// vector_complex_qdtsum
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   3: s64  2: s32  1: s16  0:s8
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = (x[0].real^2 + x[0].image^2 + ... + x[n].real^2+ x[n].image^2) / 2^q)  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_complex_qdtsum(void *zptr, void *xptr, short len, char q, long config)
{
    config = config | VEC_QDT | VEC_ACC | VEC_QDT_MODE;
    hwvec_exec(xptr, 0, zptr, 0, 0, 0, (len << 1), 1, q, config, 0);
}

//=========================================================================
// vector_real_add
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = saturated( y[n] + x[n] )  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_add(void *zptr, void *yptr, void *xptr, short len, long config)
{
    config = config;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, len, 1, 0, config, 0);
}

//=========================================================================
// vector_real_sub
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = saturated( y[n] - x[n] )  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_sub(void *zptr, void *yptr, void *xptr, short len, long config)
{
    config = config | VEC_SUB;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, len, 1, 0, config, 0);
}

//=========================================================================
// vector_real_mul
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = saturated(y[n] * x[n] / 2^q)  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_mul(void *zptr, void *yptr, void *xptr, short len, char q, long config)
{
    config = config | VEC_REAL_MUL_MODE;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, len, 1, q, config, 0);
}

//=========================================================================
// vector_real_mac
// dw bit[1:0] : x_dw   2: s32
// dw bit[5:6] : z_dw   2: s32
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = (z + y*x / 2^q)  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_mac32(void *zptr, void *yptr, void *xptr, short len, char q, long config)
{
    config = config | VEC_MAC32 | VECZ_S32 | VECY_S32 | VECX_S32 | VEC_REAL_MUL_MODE;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, len, 1, q, config, 0);
}

//=========================================================================
// vector_real_msc
// dw bit[1:0] : x_dw   2: s32
// dw bit[5:6] : z_dw   2: s32
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = (z - y*x / 2^q)  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_msc32(void *zptr, void *yptr, void *xptr, short len, char q, long config)
{
    config = config | VEC_MAC32 | VECZ_S32 | VECY_S32 | VECX_S32 | VEC_SUB | VEC_REAL_MUL_MODE;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, len, 1, q, config, 0);
}

//=========================================================================
// vector_real_dot_product
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[3:2] : y_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// y_dw >= z_dw > x_dw
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = sum(y[n] * x[n] / 2^q)  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_dot_product(void *zptr, void *yptr, void *xptr, short len, char q, long config)
{
    config = config | VEC_ACC | VEC_REAL_MUL_MODE | VEC_CPY_REAL_OR_VEC | VEC_ZPTR_CPY_DOT;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, len, 1, q, config, 0);
}

//=========================================================================
// vector_real_sum
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// z_dw >= x_dw
//  -32 < Q < 32
// *zptr, *xptr:  s32/s16/s8
// z[n] = sum(x[n]) / 2^q  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_sum(void *zptr, void *xptr, short len, char q, long config)
{
    config = config | VEC_ACC | VEC_QDT_MODE;
    hwvec_exec(xptr, 0, zptr, 0, 0, 0, len, 1, q, config, 0);
}

//=========================================================================
// vector_real_qdt
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16  0:s8
// z_dw >= x_dw
//  -32 < Q < 32
// *zptr, *xptr:  s32/s16/s8
// z[n] = x[n]^2 / 2^q  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_qdt(void *zptr, void *xptr, short len, char q, long config)
{
    config = config | VEC_QDT | VEC_QDT_MODE;
    hwvec_exec(xptr, 0, zptr, 0, 0, 0, len, 1, q, config, 0);
}

//=========================================================================
// vector_real_qdtsum
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32  1: s16
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = sum(x[n]^2) / 2^q  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_qdtsum(void *zptr, void *xptr, short len, char q, long config)
{
    config = config | VEC_QDT | VEC_ACC | VEC_QDT_MODE;
    hwvec_exec(xptr, 0, zptr, 0, 0, 0, len, 1, q, config, 0);
}

//=========================================================================
// vector_real_mac_const
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = x[n] + y[n] * const_dat / 2^q  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_mac_const(void *zptr, void *yptr, void *xptr, short len, char q, long const_dat, long config)
{
    config = config | VEC_CONST_MUL | VEC_REAL_MUL_MODE | VEC_CPY_REAL_OR_VEC;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, len, 1, q, config, const_dat);
}

//=========================================================================
// vector_real_msc_const
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] =  x[n] - y[n] * const_dat / 2^q  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_msc_const(void *zptr, void *yptr, void *xptr, short len, char q, long const_dat, long config)
{
    config = config | VEC_CONST_MUL | VEC_SUB | VEC_REAL_MUL_MODE;
    hwvec_exec(xptr, yptr, zptr, 0, 0, 0, len, 1, q, config, const_dat);
}

//=========================================================================
// vector_real_mul_const
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32
//  -32 < Q < 32
// *zptr, *yptr, *xptr:  s32/s16/s8
// z[n] = (x[n] * const_dat/ 2^q)  // 0 <= n < len
// z, y, x buf size must be(32bit x n),  z_buf can be y_buf or x_buf
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_mul_const(void *zptr, void *xptr, short len, char q, long const_dat, long config)
{
    config = config | VEC_CONST_MUL | VEC_QDT | VEC_QDT_MODE;
    hwvec_exec(xptr, 0, zptr, 0, 0, 0, len, 1, q, config, const_dat);
}

//=========================================================================
// vector_real_max
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32
//  -32 < Q < 32
//  z = x*cosnt /2^q
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_max(void *zptr, void *xptr, short len, char q, long config, long const_dat)
{
    config = config | VEC_MAX | VEC_CONST_MUL | VEC_QDT | VEC_QDT_MODE;
    hwvec_exec(xptr, 0, zptr, 0, 0, 0, len, 1, q, config, const_dat);
}

//=========================================================================
// vector_real_min
// dw bit[1:0] : x_dw   2: s32  1: s16  0:s8
// dw bit[5:6] : z_dw   2: s32
//  -32 < Q < 32
//  z = x*cosnt /2^q
//=========================================================================
__attribute__((always_inline)) inline
void vector_real_min(void *zptr, void *xptr, short len, char q, long config, long const_dat)
{
    config = config | VEC_MIN | VEC_CONST_MUL | VEC_QDT | VEC_QDT_MODE;
    hwvec_exec(xptr, 0, zptr, 0, 0, 0, len, 1, q, config, const_dat);
}

#endif

#if defined CONFIG_FPGA_TEST_ENABLE && !defined CONFIG_VIDEO_ENABLE

#include "os/os_api.h"
#include "init.h"

void REAL_FFT_FUN(int *data, int blockbit, int *oudata)
{
    vc_hw_fft(data, blockbit, oudata, 1, data == oudata ? 1 : 0);
}

void REAL_IFFT_FUN(int *data, int blockbit, int *oudata)
{
    vc_hw_ifft(data, blockbit, oudata, 1, data == oudata ? 1 : 0);
}

void COMPLEX_FFT_FUN(int *data, int blockbit, int *oudata)
{
    vc_hw_fft(data, blockbit, oudata, 0, data == oudata ? 1 : 0);
}

void COMPLEX_IFFT_FUN(int *data, int blockbit, int *oudata)
{
    vc_hw_ifft(data, blockbit, oudata, 0, data == oudata ? 1 : 0);
}

static const int real_fft_test_input_data_512[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 21, 20, -83, 58, -40, 140, -161, 110, -102, 80, -17, 41, -101, 26, 35, -54, -28, 180, -49, -180, 87, 70, 34, -222, 272, -129, 2, 44, 86, -186, 84, 32, -99, 126, -90, 44, -44, 94, -167, 55, 42, 1, -55, 65, -15, -79, 156, -64, -29, 9, 99, -125, -10, 133, -88, -6, 0, 5, 38, -65, 25, -64, 104, -79, -10, 28, -50, 105, -89, 92, -125, 159, -143, 27, 41, -25, 56, -135, 134, -108, 87, -26, 0, -35, 62, 18, -83, 41, 42, -66, -1, 69, -54, -14, 26, 48, -64, 5, 38, -4, -44, 74, -59, 10, 24, 1, -40, 23, 31, -81, 58, -2, -10, 0, 26, -44, 2, 25, -1, -30, 37, -27, 0, 5, 8, -13, -13, 37, -25, 13, -10, 23, -30, 22, -1, -10, 2, 4, -2, -3, 11, -10, 0, 4, 3, -10, 9, -2, -2, -2, 4, -1, -2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const int real_fft_test_output_data_real_512[] = {
    16, -15, 15, -17, 26, -39, 50, -51, 32, 7, -43, 45, -16, -17, 23, -17, 24, -44, 49, -24, -13, 45, -66, 89, -108, 116, -105, 84, -62, 32, 12, -50, 47, 1, -49, 49, -6, -43, 65, -66, 56, -32, -7, 39, -47, 36, -19, -2, 31, -63, 85, -93, 108, -133, 143, -118, 84, -87, 144, -208, 224, -189, 147, -128, 120, -96, 57, -37, 61, -110, 128, -80, -11, 75, -78, 48, -28, 3, 71, -191, 288, -305, 258, -221, 246, -327, 414, -452, 431, -398, 392, -369, 263, -116, 66, -174, 336, -418, 401, -357, 321, -271, 216, -212, 277, -323, 242, -45, -122, 116, 52, -226, 269, -148, -72, 294, -446, 496, -449, 285, 6, -372, 672, -793, 715, -487, 226, -92, 164, -326, 374, -266, 153, -156, 263, -421, 568, -591, 393, -52, -214, 290, -257, 287, -498, 837, -1075, 991, -594, 77, 383, -612, 327, 521, -1311, 1229, -380, -106, -581, 1888, -2622, 2366, -1699, 1257, -1084, 895, -529, 57, 313, -397, 220, -37, 51, -157, 113, 89, -177, -41, 439, -717, 718, -534, 347, -270, 308, -409, 498, -498, 373, -180, 32, 4, 38, -58, 14, 48, -84, 109, -155, 233, -351, 532, -725, 773, -562, 194, 76, -64, -199, 484, -517, 207, 197, -297, -22, 465, -727, 837, -953, 932, -417, -602, 1538, -1694, 934, 243, -1241, 1793, -1868, 1439, -612, -169, 432, -182, -193, 514, -872, 1058, -524, -755, 1863, -1893, 1041, -279, 131, -279, 279, -114, -20, 88, -168, 270, -355, 420, -474, 507, -516
};

static const int real_fft_test_output_data_img_512[] = {
    0, -5, 14, -22, 29, -28, 16, 9, -34, 43, -20, -21, 43, -30, 3, 9, -11, 30, -72, 111, -127, 123, -115, 104, -81, 46, -12, -11, 29, -44, 44, -10, -43, 70, -40, -19, 53, -42, 11, 18, -44, 66, -68, 41, -3, -31, 52, -72, 86, -85, 73, -67, 68, -59, 29, -5, 23, -75, 105, -72, 3, 49, -60, 55, -65, 83, -80, 49, -17, 34, -102, 172, -181, 123, -49, 3, 29, -85, 150, -164, 99, -11, -22, -23, 100, -146, 128, -66, 22, -23, 28, 6, -32, -55, 252, -417, 433, -339, 248, -200, 166, -132, 130, -154, 123, 18, -207, 293, -212, 59, -11, 151, -404, 636, -750, 718, -573, 374, -153, -62, 203, -174, -50, 381, -688, 861, -833, 633, -438, 418, -536, 604, -526, 389, -308, 327, -484, 777, -1067, 1169, -1061, 910, -887, 1009, -1137, 1074, -736, 284, 1, 27, -367, 1010, -1776, 2123, -1622, 733, -531, 1372, -2304, 2202, -1105, -10, 489, -525, 579, -784, 973, -952, 668, -273, 34, -63, 176, -141, 3, -26, 293, -573, 585, -306, -39, 235, -241, 150, -69, 68, -167, 332, -477, 507, -413, 285, -227, 248, -278, 269, -248, 255, -284, 318, -348, 331, -176, -119, 383, -395, 116, 268, -497, 427, -154, -8, -217, 741, -1159, 1200, -996, 784, -512, -6, 604, -725, 8, 1224, -2201, 2400, -1873, 961, 80, -1027, 1537, -1407, 919, -605, 653, -883, 1299, -2131, 3144, -3489, 2686, -1398, 712, -893, 1285, -1333, 1126, -971, 908, -817, 681, -553, 442, -328, 215, -108, 0
};

static const int complex_fft_test_output_data_256[] = {
    65280, 65536, -21115, 20602, -10685, 10172, -7207, 6694, -5467, 4955, -4423, 3912, -3727, 3214, -3229, 2716, -2855, 2343, -2564, 2053, -2332, 1820, -2142, 1629, -1982, 1470, -1847, 1335, -1731, 1220, -1631, 1119, -1543, 1031, -1465, 953, -1396, 884, -1334, 822, -1278, 766, -1227, 716, -1181, 669, -1139, 627, -1100, 588, -1064, 552, -1030, 519, -1000, 488, -972, 459, -944, 432, -919, 407, -896, 384, -874, 362, -853, 341, -833, 322, -814, 303, -797, 286, -781, 268, -764, 252, -749, 237, -735, 223, -721, 209, -708, 196, -695, 183, -683, 171, -671, 160, -660, 149, -650, 138, -639, 127, -629, 117, -620, 107, -610, 98, -601, 89, -593, 81, -584, 72, -576, 64, -568, 56, -560, 48, -553, 41, -546, 33, -539, 27, -532, 20, -525, 13, -518, 6, -512, 0, -506, -6, -500, -13, -494, -18, -488, -24, -482, -30, -477, -35, -471, -41, -466, -46, -461, -51, -456, -56, -451, -61, -446, -66, -441, -71, -436, -76, -432, -80, -427, -85, -423, -90, -418, -94, -414, -98, -410, -103, -405, -107, -401, -111, -397, -115, -393, -119, -389, -123, -385, -127, -381, -131, -377, -135, -373, -139, -370, -143, -366, -146, -362, -150, -358, -154, -355, -157, -351, -161, -348, -164, -344, -168, -340, -171, -337, -175, -334, -178, -331, -182, -327, -185, -324, -189, -320, -192, -317, -195, -313, -199, -310, -202, -307, -205, -304, -208, -301, -212, -297, -215, -294, -218, -291, -221, -288, -224, -284, -228, -281, -231, -278, -234, -275, -237, -272, -241, -269, -243, -265, -247, -262, -250, -259, -253, -256, -256, -253, -259, -250, -262, -247, -265, -243, -269, -241, -272, -237, -275, -234, -278, -231, -281, -228, -284, -224, -288, -221, -291, -218, -294, -215, -297, -212, -301, -208, -304, -205, -307, -202, -310, -199, -313, -195, -317, -192, -320, -189, -324, -185, -327, -182, -331, -178, -334, -175, -337, -171, -340, -168, -344, -164, -348, -161, -351, -157, -355, -154, -358, -150, -362, -146, -366, -143, -370, -139, -373, -135, -377, -131, -381, -127, -385, -123, -389, -119, -393, -115, -397, -111, -401, -107, -405, -103, -410, -98, -414, -94, -418, -90, -423, -85, -427, -80, -432, -76, -436, -71, -441, -66, -446, -61, -451, -56, -456, -51, -461, -46, -466, -41, -471, -35, -477, -30, -482, -24, -488, -18, -494, -13, -500, -6, -506, 0, -512, 6, -518, 13, -525, 20, -532, 27, -539, 33, -546, 41, -553, 48, -560, 56, -568, 64, -576, 72, -584, 81, -593, 89, -601, 98, -610, 107, -620, 117, -629, 127, -639, 138, -650, 149, -660, 160, -671, 171, -683, 183, -695, 196, -708, 209, -721, 223, -735, 237, -749, 252, -764, 268, -781, 286, -797, 303, -814, 322, -833, 341, -853, 362, -874, 384, -896, 407, -919, 432, -944, 459, -972, 488, -1000, 519, -1030, 552, -1064, 588, -1100, 627, -1139, 669, -1181, 716, -1227, 766, -1278, 822, -1334, 884, -1396, 953, -1465, 1031, -1543, 1119, -1631, 1220, -1731, 1335, -1847, 1470, -1982, 1629, -2142, 1820, -2332, 2053, -2564, 2343, -2855, 2716, -3229, 3214, -3727, 3912, -4423, 4955, -5467, 6694, -7207, 10172, -10685, 20602, -21115
};

static const int complex_fft_test_output_data_512[] = {
    261632, 262144, -83954, 82931, -42230, 41204, -28325, 27297, -21369, 20345, -17196, 16168, -14414, 13386, -12426, 11398, -10934, 9909, -9773, 8752, -8844, 7823, -8084, 7063, -7453, 6429, -6917, 5894, -6457, 5433, -6058, 5036, -5711, 4686, -5402, 4379, -5128, 4105, -4883, 3861, -4663, 3638, -4463, 3438, -4283, 3257, -4117, 3090, -3964, 2939, -3824, 2799, -3694, 2670, -3575, 2551, -3463, 2438, -3358, 2335, -3262, 2238, -3172, 2148, -3086, 2062, -3007, 1983, -2931, 1907, -2859, 1835, -2792, 1769, -2729, 1704, -2668, 1644, -2610, 1587, -2556, 1533, -2503, 1481, -2454, 1431, -2406, 1384, -2362, 1338, -2319, 1294, -2277, 1254, -2238, 1214, -2200, 1176, -2163, 1139, -2128, 1104, -2095, 1070, -2062, 1038, -2030, 1006, -2001, 976, -1971, 947, -1943, 919, -1915, 892, -1890, 866, -1864, 840, -1839, 815, -1815, 792, -1792, 769, -1770, 746, -1748, 724, -1727, 703, -1706, 682, -1687, 662, -1667, 643, -1648, 624, -1630, 606, -1612, 588, -1594, 571, -1578, 554, -1561, 537, -1545, 521, -1529, 505, -1514, 490, -1499, 475, -1484, 461, -1470, 446, -1456, 432, -1442, 419, -1429, 405, -1416, 391, -1403, 378, -1391, 366, -1378, 354, -1366, 342, -1355, 330, -1343, 319, -1332, 308, -1321, 296, -1310, 286, -1299, 275, -1289, 265, -1278, 254, -1269, 244, -1259, 234, -1248, 225, -1239, 215, -1230, 206, -1220, 196, -1211, 188, -1202, 178, -1193, 170, -1185, 161, -1176, 153, -1168, 144, -1160, 136, -1152, 128, -1144, 120, -1136, 112, -1128, 104, -1121, 96, -1113, 89, -1105, 82, -1099, 74, -1091, 67, -1084, 60, -1077, 53, -1070, 46, -1063, 39, -1057, 33, -1050, 26, -1043, 19, -1037, 13, -1030, 6, -1024, 0, -1018, -6, -1012, -12, -1006, -19, -1000, -25, -994, -31, -988, -36, -982, -43, -976, -48, -970, -54, -965, -59, -959, -65, -954, -70, -948, -76, -943, -81, -938, -87, -932, -92, -927, -97, -922, -102, -917, -107, -912, -113, -906, -118, -901, -123, -897, -128, -892, -132, -887, -138, -882, -142, -878, -147, -873, -151, -868, -156, -864, -161, -859, -165, -854, -170, -850, -175, -845, -179, -841, -183, -836, -188, -832, -192, -827, -197, -823, -201, -819, -205, -815, -209, -811, -213, -806, -218, -802, -222, -798, -226, -794, -230, -790, -234, -786, -238, -782, -243, -778, -246, -774, -251, -770, -254, -766, -258, -762, -262, -758, -266, -754, -270, -750, -274, -747, -278, -743, -281, -739, -285, -735, -289, -731, -293, -728, -296, -724, -300, -720, -304, -717, -307, -713, -311, -709, -315, -706, -318, -702, -322, -699, -325, -695, -329, -692, -332, -688, -336, -685, -339, -681, -343, -678, -346, -674, -350, -671, -353, -667, -357, -664, -360, -660, -364, -657, -367, -654, -370, -650, -374, -647, -377, -643, -381, -640, -384, -637, -387, -634, -390, -630, -394, -627, -397, -624, -400, -621, -404, -617, -407, -614, -410, -611, -414, -607, -417, -604, -420, -601, -423, -598, -427, -594, -430, -591, -433, -588, -436, -585, -439, -582, -442, -578, -445, -575, -449, -572, -452, -569, -455, -566, -458, -562, -462, -559, -465, -556, -468, -553, -471, -550, -474, -547, -478, -543, -481, -540, -484, -537, -487, -534, -490, -531, -493, -528, -496, -525, -499, -521, -503, -518, -506, -515, -509, -512, -512, -509, -515, -506, -518, -503, -521, -499, -525, -496, -528, -493, -531, -490, -534, -487, -537, -484, -540, -481, -543, -478, -547, -474, -550, -471, -553, -468, -556, -465, -559, -462, -562, -458, -566, -455, -569, -452, -572, -449, -575, -445, -578, -442, -582, -439, -585, -436, -588, -433, -591, -430, -594, -427, -598, -423, -601, -420, -604, -417, -607, -414, -611, -410, -614, -407, -617, -404, -621, -400, -624, -397, -627, -394, -630, -390, -634, -387, -637, -384, -640, -381, -643, -377, -647, -374, -650, -370, -654, -367, -657, -364, -660, -360, -664, -357, -667, -353, -671, -350, -674, -346, -678, -343, -681, -339, -685, -336, -688, -332, -692, -329, -695, -325, -699, -322, -702, -318, -706, -315, -709, -311, -713, -307, -717, -304, -720, -300, -724, -296, -728, -293, -731, -289, -735, -285, -739, -281, -743, -278, -747, -274, -750, -270, -754, -266, -758, -262, -762, -258, -766, -254, -770, -251, -774, -246, -778, -243, -782, -238, -786, -234, -790, -230, -794, -226, -798, -222, -802, -218, -806, -213, -811, -209, -815, -205, -819, -201, -823, -197, -827, -192, -832, -188, -836, -183, -841, -179, -845, -175, -850, -170, -854, -165, -859, -161, -864, -156, -868, -151, -873, -147, -878, -142, -882, -138, -887, -132, -892, -128, -897, -123, -901, -118, -906, -113, -912, -107, -917, -102, -922, -97, -927, -92, -932, -87, -938, -81, -943, -76, -948, -70, -954, -65, -959, -59, -965, -54, -970, -48, -976, -43, -982, -36, -988, -31, -994, -25, -1000, -19, -1006, -12, -1012, -6, -1018, 0, -1024, 6, -1030, 13, -1037, 19, -1043, 26, -1050, 33, -1057, 39, -1063, 46, -1070, 53, -1077, 60, -1084, 67, -1091, 74, -1099, 82, -1105, 89, -1113, 96, -1121, 104, -1128, 112, -1136, 120, -1144, 128, -1152, 136, -1160, 144, -1168, 153, -1176, 161, -1185, 170, -1193, 178, -1202, 188, -1211, 196, -1220, 206, -1230, 215, -1239, 225, -1248, 234, -1259, 244, -1269, 254, -1278, 265, -1289, 275, -1299, 286, -1310, 296, -1321, 308, -1332, 319, -1343, 330, -1355, 342, -1366, 354, -1378, 366, -1391, 378, -1403, 391, -1416, 405, -1429, 419, -1442, 432, -1456, 446, -1470, 461, -1484, 475, -1499, 490, -1514, 505, -1529, 521, -1545, 537, -1561, 554, -1578, 571, -1594, 588, -1612, 606, -1630, 624, -1648, 643, -1667, 662, -1687, 682, -1706, 703, -1727, 724, -1748, 746, -1770, 769, -1792, 792, -1815, 815, -1839, 840, -1864, 866, -1890, 892, -1915, 919, -1943, 947, -1971, 976, -2001, 1006, -2030, 1038, -2062, 1070, -2095, 1104, -2128, 1139, -2163, 1176, -2200, 1214, -2238, 1254, -2277, 1294, -2319, 1338, -2362, 1384, -2406, 1431, -2454, 1481, -2503, 1533, -2556, 1587, -2610, 1644, -2668, 1704, -2729, 1769, -2792, 1835, -2859, 1907, -2931, 1983, -3007, 2062, -3086, 2148, -3172, 2238, -3262, 2335, -3358, 2438, -3463, 2551, -3575, 2670, -3694, 2799, -3824, 2939, -3964, 3090, -4117, 3257, -4283, 3438, -4463, 3638, -4663, 3861, -4883, 4105, -5128, 4379, -5402, 4686, -5711, 5036, -6058, 5433, -6457, 5894, -6917, 6429, -7453, 7063, -8084, 7823, -8844, 8752, -9773, 9909, -10934, 11398, -12426, 13386, -14414, 16168, -17196, 20345, -21369, 27297, -28325, 41204, -42230, 82931, -83954
};

static const int real_fft_test_output_data_1024[] = {
    523776, 0, -511, 166887, -514, 83437, -515, 55627, -512, 41720, -516, 33372, -515, 27809, -515, 23835, -512, 20855, -510, 18539, -509, 16683, -509, 15164, -513, 13901, -511, 12831, -512, 11912, -511, 11117, -512, 10422, -511, 9808, -512, 9261, -511, 8774, -513, 8333, -514, 7934, -514, 7575, -514, 7243, -513, 6941, -513, 6662, -512, 6405, -512, 6168, -513, 5945, -512, 5739, -512, 5547, -512, 5369, -512, 5198, -512, 5042, -512, 4891, -512, 4749, -511, 4617, -513, 4491, -512, 4372, -512, 4259, -511, 4151, -511, 4048, -511, 3952, -511, 3857, -511, 3769, -513, 3685, -512, 3603, -512, 3526, -512, 3452, -512, 3380, -512, 3311, -513, 3246, -512, 3182, -512, 3120, -513, 3063, -512, 3006, -512, 2950, -512, 2897, -512, 2847, -512, 2797, -512, 2750, -511, 2704, -512, 2659, -512, 2616, -512, 2574, -512, 2533, -512, 2494, -512, 2456, -512, 2419, -512, 2382, -512, 2348, -512, 2313, -512, 2280, -512, 2248, -511, 2216, -511, 2186, -512, 2156, -512, 2127, -512, 2098, -512, 2071, -512, 2044, -512, 2017, -512, 1993, -512, 1967, -512, 1942, -513, 1918, -513, 1895, -513, 1872, -512, 1850, -512, 1828, -512, 1807, -512, 1786, -513, 1765, -512, 1745, -512, 1726, -512, 1708, -512, 1688, -512, 1670, -512, 1652, -512, 1633, -512, 1616, -512, 1599, -512, 1582, -511, 1566, -512, 1550, -511, 1534, -512, 1518, -511, 1503, -512, 1489, -512, 1474, -512, 1459, -512, 1445, -512, 1431, -512, 1417, -512, 1404, -512, 1390, -512, 1377, -512, 1365, -513, 1351, -512, 1340, -512, 1327, -511, 1315, -512, 1303, -512, 1292, -512, 1281, -512, 1269, -512, 1258, -512, 1247, -512, 1236, -512, 1225, -512, 1215, -512, 1205, -512, 1195, -512, 1184, -512, 1174, -512, 1164, -512, 1155, -512, 1146, -511, 1136, -512, 1127, -512, 1118, -512, 1109, -512, 1100, -512, 1091, -512, 1083, -512, 1074, -512, 1066, -512, 1058, -512, 1049, -512, 1040, -512, 1033, -512, 1025, -512, 1017, -512, 1009, -512, 1002, -512, 994, -512, 987, -512, 979, -512, 973, -512, 965, -512, 958, -512, 951, -512, 944, -512, 937, -512, 930, -512, 923, -512, 917, -512, 911, -512, 904, -512, 897, -512, 891, -512, 885, -512, 878, -512, 872, -512, 866, -512, 860, -512, 854, -512, 848, -512, 842, -512, 837, -512, 831, -512, 825, -512, 819, -512, 814, -512, 808, -512, 803, -512, 798, -512, 792, -512, 787, -512, 782, -512, 776, -512, 771, -512, 766, -512, 761, -512, 756, -512, 751, -512, 746, -512, 741, -512, 737, -512, 732, -512, 727, -512, 722, -512, 718, -512, 713, -512, 708, -512, 704, -512, 699, -512, 695, -512, 690, -512, 686, -512, 681, -512, 677, -512, 673, -512, 668, -512, 664, -512, 660, -512, 656, -512, 652, -512, 648, -512, 644, -512, 640, -512, 636, -512, 632, -512, 628, -512, 624, -512, 620, -512, 616, -512, 612, -512, 608, -512, 605, -512, 601, -512, 597, -512, 594, -512, 590, -512, 586, -512, 583, -512, 579, -512, 575, -512, 572, -512, 568, -512, 565, -512, 561, -512, 558, -512, 555, -512, 551, -512, 548, -512, 544, -512, 541, -512, 538, -512, 534, -512, 531, -512, 528, -512, 525, -512, 522, -512, 518, -512, 515, -512, 512, -512, 509, -512, 506, -512, 503, -512, 500, -512, 496, -512, 493, -512, 490, -512, 487, -512, 485, -512, 482, -512, 479, -512, 476, -512, 473, -512, 470, -512, 467, -512, 464, -512, 461, -512, 458, -512, 456, -512, 453, -512, 450, -512, 447, -512, 444, -512, 442, -512, 439, -512, 436, -512, 434, -512, 431, -512, 428, -512, 426, -512, 423, -512, 420, -512, 418, -512, 415, -512, 412, -512, 410, -512, 407, -512, 405, -512, 402, -512, 400, -512, 397, -512, 395, -512, 392, -512, 390, -512, 387, -512, 385, -512, 382, -512, 380, -512, 377, -512, 375, -512, 372, -512, 370, -512, 368, -512, 365, -512, 363, -512, 361, -512, 358, -512, 356, -512, 354, -512, 351, -512, 349, -512, 347, -512, 344, -512, 342, -512, 340, -512, 338, -512, 335, -512, 333, -512, 331, -512, 329, -512, 326, -512, 324, -512, 322, -512, 320, -512, 318, -512, 316, -512, 313, -512, 311, -512, 309, -512, 307, -512, 305, -512, 303, -512, 301, -512, 298, -512, 296, -512, 294, -512, 292, -512, 290, -512, 288, -512, 286, -512, 284, -512, 282, -512, 280, -512, 278, -512, 276, -512, 274, -512, 272, -512, 270, -512, 268, -512, 266, -512, 264, -512, 262, -512, 260, -512, 258, -512, 256, -512, 254, -512, 252, -512, 250, -512, 248, -512, 246, -512, 244, -512, 242, -512, 240, -512, 238, -512, 236, -512, 235, -512, 233, -512, 230, -512, 229, -512, 227, -512, 225, -512, 223, -512, 221, -512, 220, -512, 218, -512, 216, -512, 214, -512, 212, -512, 210, -512, 208, -512, 207, -512, 205, -512, 203, -512, 201, -512, 199, -512, 198, -512, 196, -512, 194, -512, 192, -512, 190, -512, 189, -512, 187, -512, 185, -512, 183, -512, 182, -512, 180, -512, 178, -512, 176, -512, 174, -512, 173, -512, 171, -512, 169, -512, 167, -512, 166, -512, 164, -512, 162, -512, 160, -512, 159, -512, 157, -512, 155, -512, 154, -512, 152, -512, 150, -512, 149, -512, 147, -512, 145, -512, 143, -512, 142, -512, 140, -512, 139, -512, 137, -512, 135, -512, 133, -512, 132, -512, 130, -512, 128, -512, 127, -512, 125, -512, 123, -512, 122, -512, 120, -512, 118, -512, 117, -512, 115, -512, 113, -512, 112, -512, 110, -512, 108, -512, 107, -512, 105, -512, 104, -512, 102, -512, 100, -512, 99, -512, 97, -512, 95, -512, 94, -512, 92, -512, 90, -512, 89, -512, 87, -512, 86, -512, 84, -512, 82, -512, 81, -512, 79, -512, 78, -512, 76, -512, 74, -512, 73, -512, 71, -512, 70, -512, 68, -512, 66, -512, 65, -512, 63, -512, 62, -512, 60, -512, 58, -512, 57, -512, 55, -512, 54, -512, 52, -512, 50, -512, 49, -512, 47, -512, 46, -512, 44, -512, 43, -512, 41, -512, 39, -512, 38, -512, 36, -512, 35, -512, 33, -512, 32, -512, 30, -512, 28, -512, 27, -512, 25, -512, 24, -512, 22, -512, 20, -512, 19, -512, 17, -512, 16, -512, 14, -512, 13, -512, 11, -512, 9, -512, 8, -512, 6, -512, 5, -512, 3, -512, 2, -512, 0
};

SEC_USED(.fft_data) static int test_datain[4096];
SEC_USED(.fft_data) static int test_dataout[4096];
SEC_USED(.fft_data) static fft_config_struct test_fft_config;

___interrupt
static void fft_isr(void)
{
    if (JL_FFT->CON & BIT(7)) {

    }
}

static void hw_fft_test(void)
{
#if 0
    REAL_FFT_FUN(datain, 7, datain);    //输入数据为128个实数，放在datain,输出数据是65个复数，排布为实部，虚部，实部，虚部。。。
    puts("REAL_FFT_FUN RES:\n");
    for (int i = 0; i < sizeof(datain) / sizeof(datain[0]); i++) {
        printf("%d, ", datain[i]);
    }

    REAL_IFFT_FUN(datain, 7, dataout);   //反变换，得到128个实数
    puts("REAL_IFFT_FUN RES:\n");
    for (int i = 0; i < sizeof(dataout) / sizeof(dataout[0]); i++) {
        printf("%d, ", dataout[i]);
    }

    COMPLEX_FFT_FUN(datain, 6, datain); //输入数据为64个复数，放在datain,输出数据是64个复数，排布为实部，虚部，实部，虚部。。。
    puts("COMPLEX_FFT_FUN RES:\n");
    for (int i = 0; i < sizeof(datain) / sizeof(datain[0]); i++) {
        printf("%d, ", datain[i]);
    }

    COMPLEX_IFFT_FUN(datain, 6, dataout); //反变换，得到64个复数
    puts("COMPLEX_IFFT_FUN RES:\n");
    for (int i = 0; i < sizeof(dataout) / sizeof(dataout[0]); i++) {
        printf("%d, ", dataout[i]);
    }
#endif

    /* request_irq(IRQ_FFT_IDX, 0, fft_isr, 0); */

    static u8 fft_test_cnt = 0;
    int *out_ptr = NULL;

    if (fft_test_cnt++ % 2) {
        out_ptr = test_datain;
    } else {
        out_ptr = test_dataout;
    }

    if (fft_test_cnt == 64) {
        fft_test_cnt = 0;
    }

    hwfft_fill_fft_ctx((hwfft_ctx_t *)&test_fft_config, REAL_FFT512_CONFIG, (long *)test_datain, (long *)out_ptr);

    for (int i = 0; i < ARRAY_SIZE(real_fft_test_input_data_512); i++) {
        test_datain[i] = real_fft_test_input_data_512[i];
    }

    hwfft_fft_i32((hwfft_ctx_t *)&test_fft_config);

    for (int i = 0; i < ARRAY_SIZE(real_fft_test_input_data_512) + 2; i++) {
        /* printf("%d, ", out_ptr[i]); */
        if (i % 2 == 0 && out_ptr[i] != real_fft_test_output_data_real_512[i / 2]) {
            ___trig;
            printf("fft test real fail !!!\n");
            put_buf((u8 *)out_ptr, sizeof(real_fft_test_input_data_512) + 8);
            while (1);
        } else if (i % 2 && out_ptr[i] != real_fft_test_output_data_img_512[i / 2]) {
            ___trig;
            printf("fft test img fail !!!\n");
            put_buf((u8 *)out_ptr, sizeof(real_fft_test_input_data_512) + 8);
            while (1);
        }
    }

    if (out_ptr != test_datain) {
        memset(test_datain, 0, sizeof(test_datain));
    }

    hwfft_fill_fft_ctx((hwfft_ctx_t *)&test_fft_config, REAL_IFFT512_CONFIG, (long *)out_ptr, (long *)test_datain);

    hwfft_ifft_i32((hwfft_ctx_t *)&test_fft_config);

    for (int i = 0; i < ARRAY_SIZE(real_fft_test_input_data_512); i++) {
        /* printf("%d, ", test_datain[i]); */
        if (test_datain[i] != real_fft_test_input_data_512[i]) {
            ___trig;
            printf("ifft test real fail !!!\n");
            put_buf((u8 *)test_datain, sizeof(real_fft_test_input_data_512));
            while (1);
        }
    }

    hwfft_fill_fft_ctx((hwfft_ctx_t *)&test_fft_config, COMPLEX_FFT256_CONFIG, (long *)test_datain, (long *)out_ptr);

    for (int i = 0; i < ARRAY_SIZE(complex_fft_test_output_data_256); i++) {
        test_datain[i] = i;
    }

    hwfft_fft_i32((hwfft_ctx_t *)&test_fft_config);

    for (int i = 0; i < ARRAY_SIZE(complex_fft_test_output_data_256); i++) {
        /* printf("%d, ", out_ptr[i]); */
        if (out_ptr[i] != complex_fft_test_output_data_256[i]) {
            ___trig;
            printf("fft test complex fail !!!\n");
            put_buf((u8 *)out_ptr, sizeof(complex_fft_test_output_data_256));
            while (1);
        }
    }

    if (out_ptr != test_datain) {
        memset(test_datain, 0, sizeof(test_datain));
    }

    hwfft_fill_fft_ctx((hwfft_ctx_t *)&test_fft_config, COMPLEX_IFFT256_CONFIG, (long *)out_ptr, (long *)test_datain);

    hwfft_ifft_i32((hwfft_ctx_t *)&test_fft_config);

    for (int i = 0; i < ARRAY_SIZE(complex_fft_test_output_data_256); i++) {
        /* printf("%d, ", test_datain[i]); */
        if (test_datain[i] != i) {
            ___trig;
            printf("ifft test complex fail !!!\n");
            put_buf((u8 *)test_datain, sizeof(complex_fft_test_output_data_256));
            while (1);
        }
    }

    hwfft_fill_fft_ctx((hwfft_ctx_t *)&test_fft_config, COMPLEX_FFT512_CONFIG, (long *)test_datain, (long *)out_ptr);

    for (int i = 0; i < ARRAY_SIZE(complex_fft_test_output_data_512); i++) {
        test_datain[i] = i;
    }

    hwfft_fft_i32((hwfft_ctx_t *)&test_fft_config);

    for (int i = 0; i < ARRAY_SIZE(complex_fft_test_output_data_512); i++) {
        /* printf("%d, ", out_ptr[i]); */
        if (out_ptr[i] != complex_fft_test_output_data_512[i]) {
            ___trig;
            printf("fft test complex fail !!!\n");
            put_buf((u8 *)out_ptr, sizeof(complex_fft_test_output_data_512));
            while (1);
        }
    }

    if (out_ptr != test_datain) {
        memset(test_datain, 0, sizeof(test_datain));
    }

    hwfft_fill_fft_ctx((hwfft_ctx_t *)&test_fft_config, COMPLEX_IFFT512_CONFIG, (long *)out_ptr, (long *)test_datain);

    hwfft_ifft_i32((hwfft_ctx_t *)&test_fft_config);

    for (int i = 0; i < ARRAY_SIZE(complex_fft_test_output_data_512); i++) {
        /* printf("%d, ", test_datain[i]); */
        if (test_datain[i] != i) {
            ___trig;
            printf("ifft test complex fail !!!\n");
            put_buf((u8 *)test_datain, sizeof(complex_fft_test_output_data_512));
            while (1);
        }
    }

    hwfft_fill_fft_ctx((hwfft_ctx_t *)&test_fft_config, REAL_FFT1024_CONFIG, (long *)test_datain, (long *)out_ptr);

    for (int i = 0; i < ARRAY_SIZE(real_fft_test_output_data_1024) - 2; i++) {
        test_datain[i] = i;
    }

    hwfft_fft_i32((hwfft_ctx_t *)&test_fft_config);

    for (int i = 0; i < ARRAY_SIZE(real_fft_test_output_data_1024); i++) {
        /* printf("%d, ", out_ptr[i]); */
        if (out_ptr[i] != real_fft_test_output_data_1024[i]) {
            ___trig;
            printf("fft test complex fail !!!\n");
            put_buf((u8 *)out_ptr, sizeof(real_fft_test_output_data_1024));
            while (1);
        }
    }

    if (out_ptr != test_datain) {
        memset(test_datain, 0, sizeof(test_datain));
    }

    hwfft_fill_fft_ctx((hwfft_ctx_t *)&test_fft_config, REAL_IFFT1024_CONFIG, (long *)out_ptr, (long *)test_datain);

    hwfft_ifft_i32((hwfft_ctx_t *)&test_fft_config);

    for (int i = 0; i < ARRAY_SIZE(real_fft_test_output_data_1024) - 2; i++) {
        /* printf("%d, ", test_datain[i]); */
        if (test_datain[i] != i) {
            ___trig;
            printf("ifft test complex fail !!!\n");
            put_buf((u8 *)test_datain, sizeof(real_fft_test_output_data_1024) - 2);
            while (1);
        }
    }

    //循环测试
    for (int n = 0; n < ARRAY_SIZE(real_tab); n += 2) {
        hwfft_fill_fft_ctx((hwfft_ctx_t *)&test_fft_config, real_tab[n].config, (long *)test_datain, (long *)out_ptr);

        for (int i = 0; i < real_tab[n].block; i++) {
            test_datain[i] = i * fft_test_cnt;
        }

        hwfft_fft_i32((hwfft_ctx_t *)&test_fft_config);

        if (out_ptr != test_datain) {
            memset(test_datain, 0, sizeof(test_datain));
        }

        hwfft_fill_fft_ctx((hwfft_ctx_t *)&test_fft_config, real_tab[n + 1].config, (long *)out_ptr, (long *)test_datain);

        hwfft_ifft_i32((hwfft_ctx_t *)&test_fft_config);

        for (int i = 0; i < real_tab[n + 1].block; i++) {
            /* printf("%d, ", test_datain[i]); */
            if (test_datain[i] != i * fft_test_cnt && test_datain[i] != i * fft_test_cnt + 1 && test_datain[i] != i * fft_test_cnt - 1) {
                ___trig;
                printf("real test fail n : %d, i : %d, cnt : %d !!!\n", n, i, fft_test_cnt);
                for (int i = 0; i < real_tab[n + 1].block; i++) {
                    printf("%d, ", test_datain[i]);
                }
                /* put_buf((u8 *)test_datain, real_tab[n + 1].block * 4); */
                while (1);
            }
        }
    }

    for (int n = 0; n < ARRAY_SIZE(complex_tab); n += 2) {
        hwfft_fill_fft_ctx((hwfft_ctx_t *)&test_fft_config, complex_tab[n].config, (long *)test_datain, (long *)out_ptr);

        for (int i = 0; i < complex_tab[n].block * 2; i++) {
            test_datain[i] = i;
        }

        hwfft_fft_i32((hwfft_ctx_t *)&test_fft_config);

        if (out_ptr != test_datain) {
            memset(test_datain, 0, sizeof(test_datain));
        }

        hwfft_fill_fft_ctx((hwfft_ctx_t *)&test_fft_config, complex_tab[n + 1].config, (long *)out_ptr, (long *)test_datain);

        hwfft_ifft_i32((hwfft_ctx_t *)&test_fft_config);

        for (int i = 0; i < complex_tab[n + 1].block * 2; i++) {
            /* printf("%d, ", test_datain[i]); */
            if (test_datain[i] != i) {
                ___trig;
                printf("complex test fail n : %d, i : %d, cnt : %d !!!\n", n, i, 2);
                for (int i = 0; i < real_tab[n + 1].block * 2; i++) {
                    printf("%d, ", test_datain[i]);
                }
                /* put_buf((u8 *)test_datain, complex_tab[n + 1].block * 2 * 4); */
                while (1);
            }
        }
    }
}

static void fft_test_task(void *priv)
{
    while (1) {
        hw_fft_test();
        os_time_dly(1);
    }
}

static int fft_test_init(void)
{
    return thread_fork("fft_test", 0, 256, 0, 0, fft_test_task, NULL);
}
late_initcall(fft_test_init);

#endif

