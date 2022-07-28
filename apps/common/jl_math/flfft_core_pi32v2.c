// #ifdef OnChip
// #pragma code_seg(".fft_code")
// #pragma data_seg(".fft_data")
// #pragma data_seg(".fft_bss")
// #pragma const_seg(".fft_const")
// #endif

#ifndef NOINLINE
#define NOINLINE
#endif
#ifndef AT_RAM_CODE
#define AT_RAM_CODE
#endif

typedef struct {
    int N; // real fft len
    int log2_N;// log2(N)
    int Skip_index; // MAX_FFT_SUPPORT/N
    int fDivN; // Divide N after real ifft or not
    int *tab;
    int TabSize;
} fifft_config;

extern void jl_fft_N(int *in, int *out, int fft_N);
extern void jl_ifft_N(int *in, int *out, int fft_N);

#define dest_exp 15
static inline int find_max_exp(const float *in, int N)
{
    int max_exp = 0;
    asm volatile(
        " 1: \n\t"
        " rep %1 {\n\t"
        " 	%1 = [%2++=4] \n\t"
        " 	%1 = fabs(%1) \n\t"
        " 	iff(%1 > %0) { \n\t"
        " 		%0 = %1 \n\t"
        " 	} \n\t"
        " }\n\t"
        " if(%1>0) goto 1b \n\t"
        " if(%0!=%1) { \n\t"
        " 	%0 = uextra(%0,p:23,l:8) \n\t"
        " 	%0 += -127 \n\t"
        " } \n\t"
        : "=&r"(max_exp),
        "=&r"(N),
        "=&r"(in)
        : "0"(max_exp),
        "1"(N),
        "2"(in)
        :);
    return max_exp;
}

static inline void i2f(const int *in, float *out, int N, float exp_offset)
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
        "=&r"(exp_offset)
        : "0"(tmp32_1),
        "1"(N),
        "2"(in),
        "3"(out),
        "4"(exp_offset)
        :);
}

static inline void f2i(const float *in, int *out, int N, float exp_offset)
{
    int tmp32_1;
    asm volatile(
        " 1: \n\t"
        " rep %1 { \n\t"
        " 	%0 = [%2++=4] \n\t"
        " 	%0 = %0 * %4 (f) \n\t"
        " 	%0 = ftoi(%0)(trunc) \n\t"
        " 	[%3++=4] = %0 \n\t"
        " } \n\t"
        " if(%1>0) goto 1b \n\t"
        : "=&r"(tmp32_1),
        "=&r"(N),
        "=&r"(in),
        "=&r"(out),
        "=&r"(exp_offset)
        : "0"(tmp32_1),
        "1"(N),
        "2"(in),
        "3"(out),
        "4"(exp_offset)
        :);
}

static inline float gen_pow_2(int power)
{
    float tmp32_1;
    int exp_tmp;
    asm volatile(
        " %0 = 0 \n\t"
        " %1 = 127 \n\t"
        " %1 += %4 \n\t"
        " if(%1 < 0){ \n\t"
        " 	%1 = 0 \n\t"
        " } \n\t"
        " if(%1 > 254){ \n\t"
        " 	%1 = 254 \n\t"
        " } \n\t"
        " %0 <= insert(%1,p:23,l:8) \n\t"
        : "=&r"(tmp32_1),
        "=&r"(exp_tmp)
        : "0"(tmp32_1),
        "1"(exp_tmp),
        "r"(power)
        :);
    return tmp32_1;
}

AT_RAM_CODE
__attribute__((noinline)) void flrfft(fifft_config *fc, const float *in, float *out)
{
    //init pi32v2 fft hardware
    int exp_offset = dest_exp - find_max_exp(in, fc->N);
    if (exp_offset < 0) {
        exp_offset = 0;
    }
    float up_offset = gen_pow_2(exp_offset);
    float down_offset = gen_pow_2(-exp_offset);
    f2i(in, (int *)out, fc->N, up_offset);

    jl_fft_N((int *)out, (int *)out, fc->N);

    i2f((int *)out, out, fc->N + 2, down_offset);
}

AT_RAM_CODE
__attribute__((noinline)) void flrifft(fifft_config *fc, const float *in, float *out)
{
    //init pi32v2 fft hardware
    int exp_offset = dest_exp - find_max_exp(in, fc->N);
    if (exp_offset < 0) {
        exp_offset = 0;
    }
    float up_offset = gen_pow_2(exp_offset);
    float down_offset = gen_pow_2(-exp_offset);
    f2i(in, (int *)in, fc->N + 2, up_offset);

    jl_ifft_N((int *)in, (int *)out, fc->N);

    i2f((int *)out, out, fc->N, down_offset);
    if (in != out) {
        i2f((int *)in, (float *)in, fc->N + 2, down_offset);
    }
}

