//#include "AISP_TSL_complex.h"
//#include "AISP_TSL_common.h"

#include "generic/typedef.h"
#include "app_config.h"
#include "nn_function_vec.h"

#ifdef CONFIG_CPU_WL80
#define ASM_ENABLE 2    //WL80无硬件矩阵运算加速
#else
#define ASM_ENABLE 2
#endif

#ifdef CONFIG_ASR_ALGORITHM

#define SHR(a,shift)    ((a) >> (shift))

#define PSHR(a,shift)   ((a)>=0?(SHR((a)+(1<<(shift-1)),shift)):\
                            (SHR((a)+(1<<(shift-1))-1,shift)))

#if 0
__attribute__((always_inline))
void MATRIX_COMPLEX_DotMultiAddCf(aisp_cpx_s32_t *pfcomRslt, aisp_cpx_s32_t *pstComA, aisp_cpx_s32_t *pstComB, aisp_cpx_s32_t *pstComC, int usNum)
{
    int  ulOff;
#if ASM_ENABLE==1
    for (ulOff = 0; ulOff < usNum; ulOff++) {
        {
            pfcomRslt[ulOff].real = pstComC[ulOff].real + pstComA[ulOff].real * pstComB[ulOff].real - pstComA[ulOff].imag * pstComB[ulOff].imag;
            pfcomRslt[ulOff].imag = pstComC[ulOff].imag + pstComA[ulOff].real * pstComB[ulOff].imag + pstComA[ulOff].imag * pstComB[ulOff].real;
        }
    }
#elif ASM_ENABLE==2
    {
        S64 res64, res640;
        int tmp;
        int oneval = 1;
        int reptime = usNum;
        int *a_ptr = &pstComA[0].real;
        int *a1_ptr = &pstComA[0].imag;
        int *b_ptr = &pstComB[0].real;
        int *b1_ptr = &pstComB[0].imag;
        int *c_ptr = &pstComC[0].real;
        int *zptr0 = &pfcomRslt[0].real;

        __asm__ volatile(
            " 1 : \n\t"
            " rep %[reptime] { \n\t"
            "%[res64]=[%[c_ptr]++=4]*%[oneval](s) \n\t"
            "%[res64]+=[%[a_ptr]++=0]*[%[b_ptr]++=0](s) \n\t"
            "%[res64]-=[%[a1_ptr]++=0]*[%[b1_ptr]++=0](s) \n\t"
            "[%[zptr0]++=4]=%[res64].l \n\t"
            "%[res64]=[%[c_ptr]++=4]*%[oneval](s) \n\t"
            "%[res64]+=[%[a_ptr]++=8]*[%[b1_ptr]++=8](s) \n\t"
            "%[res64]+=[%[b_ptr]++=8]*[%[a1_ptr]++=8](s) \n\t"
            "[%[zptr0]++=4]=%[res64].l \n\t"
            " }\n\t"
            " if(%[reptime]!=0 )goto 1b \n\t"
            : [res64] "=&r"(res64),
            [a_ptr] "=&r"(a_ptr),
            [a1_ptr]"=&r"(a1_ptr),
            [b_ptr] "=&r"(b_ptr),
            [b1_ptr]"=&r"(b1_ptr),
            [c_ptr]"=&r"(c_ptr),
            [zptr0]"=&r"(zptr0),
            [oneval]"=&r"(oneval),
            [reptime]"=&r"(reptime)
            : "[res64]"(res64),
            "[a_ptr]"(a_ptr),
            "[a1_ptr]"(a1_ptr),
            "[b_ptr]"(b_ptr),
            "[b1_ptr]"(b1_ptr),
            "[c_ptr]"(c_ptr),
            "[zptr0]"(zptr0),
            "[oneval]"(oneval),
            "[reptime]"(reptime)
            : "cc", "memory"
        );
    }
#elif ASM_ENABLE==3
#endif
}
#endif

/* 0 < shift < 32 */
__attribute__((always_inline))
void AISP_TSL_cmac_vec_32x32(int *piSrcA, int *piSrcB, int *piDst, int len, int shift)
{
#if ASM_ENABLE==1
    int a0, b0, c0, d0;
    int iBinIdx = 0;
    for (iBinIdx = 0; iBinIdx < len; iBinIdx++) {
        a0 = *piSrcA++;/* A->real */
        b0 = *piSrcA++;/* A->imag */
        c0 = *piSrcB++;/* B->real */
        d0 = *piSrcB++;/* B->imag */

        *piDst++ += PSHR(((S64)a0 * c0) - ((S64)b0 * d0), shift);
        *piDst++ += PSHR(((S64)a0 * d0) + ((S64)b0 * c0), shift);
    }
    "%[zptr0]++=4 \n\t"
#elif ASM_ENABLE==2
    {
        long long int res64, res640;
        int tmp, tmp1;
        int reptime = len;
        int *a_ptr = piSrcA;
        int *c_ptr = piSrcB;
        int *b_ptr = piSrcA + 1;
        int *d_ptr = piSrcB + 1;
        int *zptr0 = piDst;

        __asm__ volatile(
            " 1 : \n\t"
            " rep %[reptime] { \n\t"
            "%[res64]=[%[a_ptr]++=8]*[%[c_ptr]++=8](s) \n\t"
            "%[res64]-=[%[b_ptr]++=8]*[%[d_ptr]++=8](s) \n\t"
            "%[tmp]=%[res64]>>>%[shift](round) \n\t"
            "[%[zptr0]+0]+=%[tmp] \n\t"
            "%[zptr0]+=8 \n\t"
            " }\n\t"
            " if(%[reptime]!=0 )goto 1b \n\t"
            : [res64] "=&r"(res64),
            [a_ptr] "=&r"(a_ptr),
            [c_ptr]"=&r"(c_ptr),
            [b_ptr] "=&r"(b_ptr),
            [d_ptr]"=&r"(d_ptr),
            [zptr0]"=&r"(zptr0),
            [tmp]"=&r"(tmp),
            [reptime]"=&r"(reptime),
            [shift]"=&r"(shift)
            : "[res64]"(res64),
            "[a_ptr]"(a_ptr),
            "[c_ptr]"(c_ptr),
            "[b_ptr]"(b_ptr),
            "[d_ptr]"(d_ptr),
            "[zptr0]"(zptr0),
            "[reptime]"(reptime),
            "[shift]"(shift)
            : "cc", "memory"
        );

        reptime = len;
        a_ptr = piSrcA;
        c_ptr = piSrcB;
        b_ptr = piSrcA + 1;
        d_ptr = piSrcB + 1;
        zptr0 = piDst + 1;

        __asm__ volatile(
            " 1 : \n\t"
            " rep %[reptime] { \n\t"
            "%[res64]=[%[a_ptr]++=8]*[%[d_ptr]++=8](s) \n\t"
            "%[res64]+=[%[b_ptr]++=8]*[%[c_ptr]++=8](s) \n\t"
            "%[tmp]=%[res64]>>>%[shift](round) \n\t"
            "[%[zptr0]+0]+=%[tmp] \n\t"
            "%[zptr0]+=8 \n\t"
            " }\n\t"
            " if(%[reptime]!=0 )goto 1b \n\t"
            : [res64] "=&r"(res64),
            [a_ptr] "=&r"(a_ptr),
            [c_ptr]"=&r"(c_ptr),
            [b_ptr] "=&r"(b_ptr),
            [d_ptr]"=&r"(d_ptr),
            [zptr0]"=&r"(zptr0),
            [tmp]"=&r"(tmp),
            [reptime]"=&r"(reptime),
            [shift]"=&r"(shift)
            : "[res64]"(res64),
            "[a_ptr]"(a_ptr),
            "[c_ptr]"(c_ptr),
            "[b_ptr]"(b_ptr),
            "[d_ptr]"(d_ptr),
            "[zptr0]"(zptr0),
            "[reptime]"(reptime),
            "[shift]"(shift)
            : "cc", "memory"
        );
    }
#elif ASM_ENABLE==3
    int len_ = len / 4 * 4;
    vector_complex_zs32_ys32_mac_xs32(piDst, piSrcA, piSrcB, len_, shift); //长度2对齐
    int *srcA, *srcB, *dst;
    srcA = piSrcA + len_ * 2;
    srcB = piSrcB + len_ * 2;
    dst = piDst + len_ * 2;
    len_ = len - len_;
    int a0, b0, c0, d0;
    int iBinIdx = 0;

    for (iBinIdx = 0; iBinIdx < len_; iBinIdx++) {
        a0 = *srcA++;/* A->real */
        b0 = *srcA++;/* A->imag */
        c0 = *srcB++;/* B->real */
        d0 = *srcB++;/* B->imag */

        *dst++ += PSHR(((s64)a0 * c0) - ((s64)b0 * d0), shift);
        *dst++ += PSHR(((s64)a0 * d0) + ((s64)b0 * c0), shift);
    }
#endif
}

/* 0 < iShiftFst < 32, 0 < iShiftSnd < 32 */
__attribute__((always_inline))
void AISP_TSL_cmacWithGain_vec_32x32(int *piSrcA, int *piSrcB, int *piDst, int iLen, int iShiftFst, int iShiftSnd, int iGain)
{
    int iReal, iImag;
    int a0, b0, c0, d0;
    int iBinIdx = 0;

#if ASM_ENABLE==1
    for (iBinIdx = 0; iBinIdx < iLen; iBinIdx++) {
        a0 = *piSrcA++;/* A->real */
        b0 = *piSrcA++;/* A->imag */
        c0 = *piSrcB++;/* B->real */
        d0 = *piSrcB++;/* B->imag */

        iReal = PSHR((S64)iGain * a0, iShiftFst);
        iImag = PSHR((S64)iGain * b0, iShiftFst);
        /* conj */
        *piDst++ += PSHR(((S64)iReal * c0) + ((S64)iImag * d0), iShiftSnd);
        *piDst++ += PSHR(((S64)iImag * c0) - ((S64)iReal * d0), iShiftSnd);
    }
#elif ASM_ENABLE==2
    {
        long long int res64, res640;
        int tmp, tmp1;
        int reptime = iLen;
        int *a_ptr = piSrcA;
        int *c_ptr = piSrcB;
        int *d_ptr = piSrcB + 1;
        int *zptr0 = piDst;

        __asm__ volatile(
            " 1 : \n\t"
            "%[res64]=[%[a_ptr]++=4]*%[iGain](s) \n\t"
            "%[iReal]= %[res64]>>>%[iShiftFst](round) \n\t"
            "%[res64]=[%[a_ptr]++=4]*%[iGain](s) \n\t"
            "%[iImag]= %[res64]>>>%[iShiftFst](round) \n\t"
            "%[res64]=[%[c_ptr]++=0]*%[iReal](s) \n\t"
            "%[res64]+=[%[d_ptr]++=0]*%[iImag](s) \n\t"
            "%[tmp]=%[res64]>>>%[iShiftSnd](round) \n\t"
            "[%[zptr0]+0]+=%[tmp] \n\t"
            "%[zptr0]+=4 \n\t"
            "%[res64]=[%[c_ptr]++=8]*%[iImag](s) \n\t"
            "%[res64]-=[%[d_ptr]++=8]*%[iReal](s) \n\t"
            "%[tmp]=%[res64]>>>%[iShiftSnd](round) \n\t"
            "[%[zptr0]+0]+=%[tmp] \n\t"
            "%[zptr0]+=4 \n\t"
            " if(--%[reptime]!=0) goto 1b \n\t"
            : [res64] "=&r"(res64),
            [a_ptr] "=&r"(a_ptr),
            [c_ptr]"=&r"(c_ptr),
            [d_ptr]"=&r"(d_ptr),
            [zptr0]"=&r"(zptr0),
            [tmp]"=&r"(tmp),
            [iReal]"=&r"(iReal),
            [iImag]"=&r"(iImag),
            [reptime]"=&r"(reptime),
            [iGain]"=&r"(iGain),
            [iShiftFst]"=&r"(iShiftFst),
            [iShiftSnd]"=&r"(iShiftSnd)
            : "[res64]"(res64),
            "[a_ptr]"(a_ptr),
            "[c_ptr]"(c_ptr),
            "[d_ptr]"(d_ptr),
            "[zptr0]"(zptr0),
            "[reptime]"(reptime),
            "[iGain]"(iGain),
            "[iShiftFst]"(iShiftFst),
            "[iShiftSnd]"(iShiftSnd)
            : "cc", "memory"
        );
    }
#elif ASM_ENABLE==3
    vector_real_zs32_x32_mul_const(piSrcA, piSrcA, 2 * iLen, iShiftFst, iGain); //长度4对齐
    vector_complex_zs32_ys32_mac_conj_xs32(piDst, piSrcA, piSrcB, iLen, iShiftSnd); //长度2对齐
    int len_ = iLen / 4 * 4;
    int *srcA, *srcB, *dst;
    srcA = piSrcA + len_ * 2;
    srcB = piSrcB + len_ * 2;
    dst = piDst + len_ * 2;
    len_ = iLen - len_;

    for (iBinIdx = 0; iBinIdx < iLen; iBinIdx++) {
        a0 = *srcA++;/* A->real */
        b0 = *srcA++;/* A->imag */
        c0 = *srcB++;/* B->real */
        d0 = *srcB++;/* B->imag */

        iReal = PSHR((s64)iGain * a0, iShiftFst);
        iImag = PSHR((s64)iGain * b0, iShiftFst);
        /* conj */
        *dst++ += PSHR(((s64)iReal * c0) + ((s64)iImag * d0), iShiftSnd);
        *dst++ += PSHR(((s64)iImag * c0) - ((s64)iReal * d0), iShiftSnd);
    }
#endif
}

__attribute__((always_inline))
void AISP_TSL_addWithGain_vec_32x32(int *piSrcA, int *piDst, int iLen, int iShift, int iGain)
{
#if ASM_ENABLE==1
    U32 blkCnt;                               /* Loop counter */
    S32 a, b, c, d;                           /* Temporary variables */

    blkCnt = len;

    while (blkCnt > 0U) {
        /* C[2 * i    ] = A[2 * i] * B[2 * i    ] - A[2 * i + 1] * B[2 * i + 1]. */
        /* C[2 * i + 1] = A[2 * i] * B[2 * i + 1] + A[2 * i + 1] * B[2 * i    ]. */
        a = *piSrcA++;
        b = *piSrcA++;
#if 0
        c = *piSrcB++;
        d = *piSrcB++;
        /* store result in 3.x format in destination buffer. */
        *piDst++ = PSHR(((S64) a * c)  - ((S64) b * d), iShift);
        *piDst++ = PSHR(((S64) a * d)  + ((S64) b * c), iShift);
#endif
        *piDst++ += PSHR((S64) a * iGain, iShift);
        *piDst++ += PSHR((S64) b * iGain, iShift);
        blkCnt--;
    }
#elif ASM_ENABLE==2
    {
        long long int res64;
        int tmp;
        int reptime = iLen;
        int *a_ptr = piSrcA;
        int *zptr0 = piDst;

        __asm__ volatile(
            " 1 : \n\t"
            " rep %[reptime] { \n\t"
            "%[res64]=[%[a_ptr]++=4]*%[iGain](s) \n\t"
            "%[tmp]=%[res64]>>>%[iShift](round) \n\t"
            "[%[zptr0]+0]+=%[tmp] \n\t"
            "%[zptr0]+=4 \n\t"
            " }\n\t"
            " if(%[reptime]!=0 )goto 1b \n\t"
            : [res64] "=&r"(res64),
            [a_ptr] "=&r"(a_ptr),
            [iGain] "=&r"(iGain),
            [zptr0]"=&r"(zptr0),
            [tmp]"=&r"(tmp),
            [reptime]"=&r"(reptime),
            [iShift]"=&r"(iShift)
            : "[res64]"(res64),
            "[a_ptr]"(a_ptr),
            "[iGain]"(iGain),
            "[zptr0]"(zptr0),
            "[reptime]"(reptime),
            "[iShift]"(iShift)
            : "cc", "memory"
        );
    }
#elif ASM_ENABLE==3
    int len_ = 0;
    if (iLen > 1024) {
        len_ = iLen - 1024;
    }
    if (len_ > 0) {
        vector_real_zs32_ys32_mac_xs32(piDst, piSrcA, 1024, iShift, iGain);//长度4对齐
        vector_real_zs32_ys32_mac_xs32(piDst + 1024, piSrcA + 1024, len_, iShift, iGain);
    } else {
        vector_real_zs32_ys32_mac_xs32(piDst, piSrcA, iLen, iShift, iGain);
    }
#endif
}

__attribute__((always_inline))
void AISP_TSL_cmul_vec_32x32(int *piSrcA, int *piSrcB, int *piDst, int len, int shift)
{
    unsigned int blkCnt;                               /* Loop counter */
    int a, b, c, d;                           /* Temporary variables */

    blkCnt = len;

#if ASM_ENABLE==1
    while (blkCnt > 0U) {
        /* C[2 * i    ] = A[2 * i] * B[2 * i    ] - A[2 * i + 1] * B[2 * i + 1]. */
        /* C[2 * i + 1] = A[2 * i] * B[2 * i + 1] + A[2 * i + 1] * B[2 * i    ]. */
        a = *piSrcA++;
        b = *piSrcA++;
        c = *piSrcB++;
        d = *piSrcB++;
        /* store result in 3.x format in destination buffer. */
        *piDst++ = PSHR(((S64) a * c)  - ((S64) b * d), shift);
        *piDst++ = PSHR(((S64) a * d)  + ((S64) b * c), shift);
        blkCnt--;
    }
#elif ASM_ENABLE==2
    {
        long long int res64, res640;
        int tmp;
        int reptime = blkCnt;
        int *a_ptr = piSrcA;
        int *c_ptr = piSrcB;
        int *b_ptr = piSrcA + 1;
        int *d_ptr = piSrcB + 1;
        int *zptr0 = piDst;

        __asm__ volatile(
            " 1 : \n\t"
            " rep %[reptime] { \n\t"
            "%[res64]=[%[a_ptr]++=0]*[%[c_ptr]++=0](s) \n\t"
            "%[res64]-=[%[b_ptr]++=0]*[%[d_ptr]++=0](s) \n\t"
            "%[tmp]=%[res64]>>>%[shift](round) \n\t"
            "[%[zptr0]++=4]=%[tmp] \n\t"
            "%[res64]=[%[a_ptr]++=8]*[%[d_ptr]++=8](s) \n\t"
            "%[res64]+=[%[b_ptr]++=8]*[%[c_ptr]++=8](s) \n\t"
            "%[tmp]=%[res64]>>>%[shift](round) \n\t"
            "[%[zptr0]++=4]=%[tmp] \n\t"
            " }\n\t"
            " if(%[reptime]!=0 )goto 1b \n\t"
            : [res64] "=&r"(res64),
            [a_ptr] "=&r"(a_ptr),
            [c_ptr]"=&r"(c_ptr),
            [b_ptr] "=&r"(b_ptr),
            [d_ptr]"=&r"(d_ptr),
            [zptr0]"=&r"(zptr0),
            [tmp]"=&r"(tmp),
            [reptime]"=&r"(reptime),
            [shift]"=&r"(shift)
            : "[res64]"(res64),
            "[a_ptr]"(a_ptr),
            "[c_ptr]"(c_ptr),
            "[b_ptr]"(b_ptr),
            "[d_ptr]"(d_ptr),
            "[zptr0]"(zptr0),
            "[reptime]"(reptime),
            "[shift]"(shift)
            : "cc", "memory"
        );
    }
#elif ASM_ENABLE==3
    int len_ = len / 4 * 4;
    vector_complex_zs32_ys32_mul_xs32(piDst, piSrcA, piSrcB, len_, shift);//长度4对齐
    int *srcA, *srcB, *dst;
    srcA = piSrcA + len_ * 2;
    srcB = piSrcB + len_ * 2;
    dst = piDst + len_ * 2;
    len_ = len - len_;

    while (len_ > 0U) {
        /* C[2 * i    ] = A[2 * i] * B[2 * i    ] - A[2 * i + 1] * B[2 * i + 1]. */
        /* C[2 * i + 1] = A[2 * i] * B[2 * i + 1] + A[2 * i + 1] * B[2 * i    ]. */
        a = *srcA++;
        b = *srcA++;
        c = *srcB++;
        d = *srcB++;
        /* store result in 3.x format in destination buffer. */
        *dst++ = PSHR(((s64) a * c)  - ((s64) b * d), shift);
        *dst++ = PSHR(((s64) a * d)  + ((s64) b * c), shift);
        len_--;
    }
#endif
}

__attribute__((always_inline))
void AISP_TSL_cmul_real_32x32(int *piSrcA, int *piSrcB, int *piDst, int len, int shift)
{
    unsigned int blkCnt;                /* Loop counter */
    int a, c;                           /* Temporary variables */

    blkCnt = len;

#if ASM_ENABLE==1
    while (blkCnt > 0U) {
        /* C[2 * i    ] = A[2 * i] * B[2 * i    ] - A[2 * i + 1] * B[2 * i + 1]. */
        /* C[2 * i + 1] = A[2 * i] * B[2 * i + 1] + A[2 * i + 1] * B[2 * i    ]. */
        a = *piSrcA++;
        c = *piSrcB++;
        /* store result in 3.x format in destination buffer. */
        *piDst++ = PSHR(((S64)a * c), shift);
        blkCnt--;
    }
#elif ASM_ENABLE==2
    {
        long long int res64, res640;
        int tmp;
        int reptime = blkCnt;
        int *a_ptr = piSrcA;
        int *c_ptr = piSrcB;
        int *zptr0 = piDst;

        __asm__ volatile(
            " 1 : \n\t"
            " rep %[reptime] { \n\t"
            "%[res64]=[%[a_ptr]++=4]*[%[c_ptr]++=4](s) \n\t"
            "%[tmp]=%[res64]>>>%[shift](round) \n\t"
            "[%[zptr0]++=4]=%[tmp] \n\t"
            " }\n\t"
            " if(%[reptime]!=0 )goto 1b \n\t"
            : [res64] "=&r"(res64),
            [a_ptr] "=&r"(a_ptr),
            [c_ptr]"=&r"(c_ptr),
            [zptr0]"=&r"(zptr0),
            [tmp]"=&r"(tmp),
            [reptime]"=&r"(reptime),
            [shift]"=&r"(shift)
            : "[res64]"(res64),
            "[a_ptr]"(a_ptr),
            "[c_ptr]"(c_ptr),
            "[zptr0]"(zptr0),
            "[reptime]"(reptime),
            "[shift]"(shift)
            : "cc", "memory"
        );
    }
#elif ASM_ENABLE==3
    vector_real_zs32_ys32_mul_xs32(piDst, piSrcA, piSrcB, len, shift);//长度4对齐
#endif
}

__attribute__((always_inline))
void AISP_TSL_cmul_real_32x16(int *piSrcA, short *piSrcB, int *piDst, int len, int shift)
{
    unsigned int blkCnt;               /* Loop counter */
    int a;
    short c;                           /* Temporary variables */

    blkCnt = len;

#if ASM_ENABLE==1
    while (blkCnt > 0U) {
        /* C[2 * i    ] = A[2 * i] * B[2 * i    ] - A[2 * i + 1] * B[2 * i + 1]. */
        /* C[2 * i + 1] = A[2 * i] * B[2 * i + 1] + A[2 * i + 1] * B[2 * i    ]. */
        a = *piSrcA++;
        c = *piSrcB++;
        /* store result in 3.x format in destination buffer. */
        *piDst++ = PSHR(((S64)a * c), shift);
        blkCnt--;
    }
#elif ASM_ENABLE==2
    {
        long long int res64;
        int  tmp;
        int  reptime = blkCnt;
        int *a_ptr = piSrcA;
        short *c_ptr = piSrcB;
        int *zptr0 = piDst;

        //r1=r7_r6>>>r0(round)
        //r7_r6=[r4++=4]
        //r8 = [r5++=4]
        //[r1++=4]=r3
        __asm__ volatile(
            " 1 : \n\t"
            " rep %[reptime] { \n\t"
            "%[res64]=  h[%[c_ptr]++=2] * [%[a_ptr]++=4]  (s)\n\t"
            "%[tmp]=%[res64]>>>%[shift](round)\n\t"
            "[%[zptr0]++=4]=%[tmp]\n\t"
            " }\n\t"
            " if(%[reptime]!=0 )goto 1b \n\t"
            : [res64] "=&r"(res64),
            [a_ptr] "=&r"(a_ptr),
            [c_ptr]"=&r"(c_ptr),
            [zptr0]"=&r"(zptr0),
            [tmp]"=&r"(tmp),
            [reptime]"=&r"(reptime),
            [shift]"=&r"(shift)
            : "[res64]"(res64),
            "[a_ptr]"(a_ptr),
            "[c_ptr]"(c_ptr),
            "[zptr0]"(zptr0),
            "[reptime]"(reptime),
            "[shift]"(shift)
            : "cc", "memory", "r8", "r9"
        );
    }
#elif ASM_ENABLE==3
    vector_real_zs32_ys32_mul_xs16(piDst, piSrcA, piSrcB, len, shift);//长度4对齐
#endif
}

/* 0 < iShiftFst < 32, 0 < iShiftSnd < 32 */
__attribute__((always_inline))
void AISP_TSL_cmacWithGain_vec_32x32_2(int *piSrcA, int *piSrcB, int *piDst, int iLen, int iShiftFst, int iShiftSnd, int iGain)
{
    int iReal, iImag;
    int a0, b0, c0, d0;
    int iBinIdx = 0;

#if ASM_ENABLE==1
    for (iBinIdx = 0; iBinIdx < iLen; iBinIdx++) {
        int tmp0, tmp1;
        a0 = *piSrcA++;/* A->real */
        b0 = *piSrcA++;/* A->imag */
        c0 = *piSrcB++;/* B->real */
        d0 = *piSrcB++;/* B->imag */

        tmp0 = PSHR(((S64)a0 * c0) + ((S64)b0 * d0), iShiftSnd);
        tmp1 = PSHR(((S64)b0 * c0) - ((S64)a0 * d0), iShiftSnd);

        iReal = PSHR((S64)iGain * tmp0, iShiftFst);
        iImag = PSHR((S64)iGain * tmp1, iShiftFst);
        /* conj */
        *piDst++ += iReal;
        *piDst++ += iImag;
    }
#elif ASM_ENABLE==2
    {
        long long int res64, res640;
        int tmp, tmp1;
        int reptime = iLen;
        int *a_ptr = piSrcA;
        int *c_ptr = piSrcB;
        int *b_ptr = piSrcA + 1;
        int *d_ptr = piSrcB + 1;
        int *zptr0 = piDst;

        __asm__ volatile(
            " 1 : \n\t"
            " rep %[reptime] { \n\t"
            "%[res64]=[%[a_ptr]++=8]*[%[c_ptr]++=8](s) \n\t"
            "%[res64]+=[%[b_ptr]++=8]*[%[d_ptr]++=8](s) \n\t"
            "%[tmp]=%[res64]>>>%[iShiftSnd](round) \n\t"
            "%[res64]=%[tmp]*%[iGain](s) \n\t"
            "%[tmp]=%[res64]>>>%[iShiftFst](round) \n\t"
            "[%[zptr0]+0]+=%[tmp] \n\t"
            "%[zptr0]+=8 \n\t"
            " }\n\t"
            " if(%[reptime]!=0 )goto 1b \n\t"
            : [res64] "=&r"(res64),
            [a_ptr] "=&r"(a_ptr),
            [c_ptr]"=&r"(c_ptr),
            [b_ptr] "=&r"(b_ptr),
            [d_ptr]"=&r"(d_ptr),
            [zptr0]"=&r"(zptr0),
            [tmp]"=&r"(tmp),
            [reptime]"=&r"(reptime),
            [iGain]"=&r"(iGain),
            [iShiftFst]"=&r"(iShiftFst),
            [iShiftSnd]"=&r"(iShiftSnd)
            : "[res64]"(res64),
            "[a_ptr]"(a_ptr),
            "[c_ptr]"(c_ptr),
            "[b_ptr]"(b_ptr),
            "[d_ptr]"(d_ptr),
            "[zptr0]"(zptr0),
            "[reptime]"(reptime),
            "[iGain]"(iGain),
            "[iShiftFst]"(iShiftFst),
            "[iShiftSnd]"(iShiftSnd)
            : "cc", "memory"
        );

        reptime = iLen;
        a_ptr = piSrcA;
        c_ptr = piSrcB;
        b_ptr = piSrcA + 1;
        d_ptr = piSrcB + 1;
        zptr0 = piDst + 1;

        __asm__ volatile(
            " 1 : \n\t"
            " rep %[reptime] { \n\t"
            "%[res64]=[%[b_ptr]++=8]*[%[c_ptr]++=8](s) \n\t"
            "%[res64]-=[%[a_ptr]++=8]*[%[d_ptr]++=8](s) \n\t"
            "%[tmp]=%[res64]>>>%[iShiftSnd](round) \n\t"
            "%[res64]=%[tmp]*%[iGain](s) \n\t"
            "%[tmp]=%[res64]>>>%[iShiftFst](round) \n\t"
            "[%[zptr0]+0]+=%[tmp] \n\t"
            "%[zptr0]+=8 \n\t"
            " }\n\t"
            " if(%[reptime]!=0 )goto 1b \n\t"
            : [res64] "=&r"(res64),
            [a_ptr] "=&r"(a_ptr),
            [c_ptr]"=&r"(c_ptr),
            [b_ptr] "=&r"(b_ptr),
            [d_ptr]"=&r"(d_ptr),
            [zptr0]"=&r"(zptr0),
            [tmp]"=&r"(tmp),
            [reptime]"=&r"(reptime),
            [iGain]"=&r"(iGain),
            [iShiftFst]"=&r"(iShiftFst),
            [iShiftSnd]"=&r"(iShiftSnd)
            : "[res64]"(res64),
            "[a_ptr]"(a_ptr),
            "[c_ptr]"(c_ptr),
            "[b_ptr]"(b_ptr),
            "[d_ptr]"(d_ptr),
            "[zptr0]"(zptr0),
            "[reptime]"(reptime),
            "[iGain]"(iGain),
            "[iShiftFst]"(iShiftFst),
            "[iShiftSnd]"(iShiftSnd)
            : "cc", "memory"
        );
    }
#elif ASM_ENABLE==3
    int len_ = iLen / 4 * 4;
    int temp_buf[1024];
    vector_complex_zs32_ys32_mul_conj_xs32(temp_buf, piSrcA, piSrcB, len_, iShiftSnd);//长度4对齐
    vector_real_zs32_ys32_mac_xs32(piDst, temp_buf, 2 * len_, iShiftFst, iGain); //长度4对齐
    int *srcA, *srcB, *dst;
    srcA = piSrcA + len_ * 2;
    srcB = piSrcB + len_ * 2;
    dst = piDst + len_ * 2;
    len_ = iLen - len_;

    for (iBinIdx = 0; iBinIdx < len_; iBinIdx++) {
        int tmp0, tmp1;
        a0 = *srcA++;
        b0 = *srcA++;
        c0 = *srcB++;
        d0 = *srcB++;

        tmp0 = PSHR(((s64)a0 * c0) + ((s64)b0 * d0), iShiftSnd);
        tmp1 = PSHR(((s64)b0 * c0) - ((s64)a0 * d0), iShiftSnd);

        iReal = PSHR((s64)iGain * tmp0, iShiftFst);
        iImag = PSHR((s64)iGain * tmp1, iShiftFst);

        *dst++ += iReal;
        *dst++ += iImag;
    }
#endif
}

#if 0
__attribute__((always_inline))
void AISP_TSL_addwin(U16 *x1, U16 *x2, U16 *dst, S32 size, S32 iShift)
{

    int tmp;
    __asm volatile(
        "1: \n\t "
        "rep %3 { \n\t"
        "r4 = [%1++=4] \n\t"
        "r5 = [%2++=4] \n\t"
        "r9_r8 = r4.h,r4.l *|* r5.h,r5.l (ssat) \n\t"
        "%[tmp] = r9 >>> %0\n\t"
        "h[%4++=2]=r1  \n\t"
        "h[%4++=2]=r1\n\t"
        "} \n\t"
        "if(%3!=0) goto 1b \n\t"
        : "=&r"(iShift), "=&r"(x1), "=&r"(x2), "=&r"(size), "=&r"(dst), [tmp]"=&r"(tmp)
        : "0"(iShift), "1"(x1), "2"(x2), "3"(size), "4"(dst), "[tmp]"(tmp)
        : "r1", "r5", "r4", "r7", "r6", "r9", "r8"
    );
    return;
}
#endif

#if 0
__attribute__((always_inline))
void JL_addwin(U16 *x1, U16 *x2, U16 *dst, int size, int iShift)
{
    int i, res;
    res = 0;

    int temp0, temp1;
    S64 rst;
    int *in0 = (int *)x1;
    int *in1 = (int *)x2;


    for (i = 0; i < size / 2; i++) {

        __asm volatile(
            "%0 = %1.h,%1.l  *|* %2.h,%2.l (ssat) \n\t"
            : "=&r"(rst), "=&r"(in0[i]), "=&r"(in1[i])
            : "0"(rst), "1"(in0[i]), "2"(in1[i])
            :
        );
        temp0 = (int)(rst & ((1ll << 32) - 1));
        temp1 = (int)(rst >> 32);
        dst[i * 2] = PSHR(temp0, iShift);
        dst[i * 2 + 1] = PSHR(temp1, iShift);

    }
}
#endif

#if 0
__attribute__((always_inline))
void AISP_TSL_addwin(U16 *x1, U16 *x2, U16 *dst, S32 size, S32 iShift)
{
    S32  iAlgn = size >> 1;
    jl_addwin(x1, x2, dst, iAlgn, iShift);
    for (; iAlgn < size; iAlgn++) {
        dst[iAlgn] = PSHR((x1[iAlgn] * x2[iAlgn]), 15);
    }
    return;
}
#endif

__attribute__((always_inline))
int jl_vector_multadd(char *x1, char *x2, int size)
{
    int sum = 0;

#if ASM_ENABLE==3
    vector_real_zs32_ys8_dot_product_xs8(&sum, x1, x2, 4 * size, 0); //长度4对齐
#else
    __asm volatile(
        "r9_r8 = 0 \n\t "
        "1: \n\t "
        "rep %3 { \n\t"
        "r4 = [%1++=4] \n\t"
        "r6 = [%2++=4] \n\t"
        "r5_r4 = unpackb(r4) (s) \n\t"
        "r7_r6 = unpackb(r6) (s) \n\t"
        "r9_r8 += r4.h,r4.l  *|* r6.h,r6.l (ssat) \n\t"
        "r9_r8 += r5.h,r5.l  *|* r7.h,r7.l (ssat) \n\t"
        "} \n\t"
        "if(%3!=0) goto 1b \n\t"
        "%0 += r9   \n\t"
        "%0 += r8   \n\t"
        : "=&r"(sum), "=&r"(x1), "=&r"(x2), "=&r"(size)
        : "0"(sum), "1"(x1), "2"(x2), "3"(size)
        : "r3", "r5", "r4", "r7", "r6", "r9", "r8"
    );
#endif
    return sum;
}

#define S64 long long int
#define S32  int
#define S16  short

__attribute__((always_inline))
void jl_vector_mult(char *x1, char *x2, int size, int *vout)
{
#if ASM_ENABLE==3
    vector_real_zs32_ys8_mul_xs8(vout, x1, x2, 4 * size, 0); //长度4对齐
#else
    int *ptr = vout;
    __asm volatile(
        "r9_r8 = 0 \n\t "
        "1: \n\t "
        "r4 = [%1++=4] \n\t"
        "r6 = [%2++=4] \n\t"
        "r5_r4 = unpackb(r4) (s) \n\t"
        "r7_r6 = unpackb(r6) (s) \n\t"
        "r9_r8 = r4.h,r4.l  *|* r6.h,r6.l (ssat) \n\t"
        "[%0++=4] = r8 \n\t"
        "[%0++=4] = r9 \n\t"
        "r9_r8 = r5.h,r5.l  *|* r7.h,r7.l (ssat) \n\t"
        "[%0++=4] = r8 \n\t"
        "[%0++=4] = r9 \n\t"
        " if(--%3!=0) goto 1b \n\t"
        : "=&r"(ptr), "=&r"(x1), "=&r"(x2), "=&r"(size)
        : "0"(ptr), "1"(x1), "2"(x2), "3"(size)
        : "r5", "r4", "r7", "r6", "r9", "r8"
    );
#endif
}

typedef struct CFloat {
    float re;
    float im;
} CFloat;

void CVecMultConjSum(CFloat *pDestVec,  CFloat *pVec1, CFloat *pVec2, int Count)
{
    int i = 0;
#if ASM_ENABLE==1
#if 1
    for (i = 0; i < Count; i++) {
        pDestVec[i].re = (pVec1[i].re * pVec2[i].re) + (pVec1[i].im * pVec2[i].im);
        pDestVec[i].im = (pVec2[i].re * pVec1[i].im) - (pVec1[i].re * pVec2[i].im);
    }
#else
    for (int i = 0; i < Count; i = i + 2) {
        pDestVec[i] = (pVec1[i] * pVec2[i]) + (pVec1[i + 1] * pVec2[i + 1]);
        pDestVec[i + 1] = (pVec2[i] * pVec1[i + 1]) - (pVec1[i] * pVec2[i + 1]);
    }
#endif
#elif ASM_ENABLE==2
    __asm volatile(
        " %3 -= 1 \n\t "
        " r4 = [%0++=4]\n\t"
        " r6 = [%1++=4]\n\t"
        "1: \n\t "
        " rep %3 { \n\t"
        " r8 = r4*r6 (f) # r5 = [%0++=4] \n\t"
        " r9 = r6*r5 (f) # r7 = [%1++=4] \n\t"
        " r8 += r5*r7 (f) # r6 = [%1++=4] \n\t"
        " r9 -= r4*r7 (f) # r4 = [%0++=4]\n\t"
        " [%2++=4] = r8 \n\t"
        " [%2++=4] = r9 \n\t"
        " }\n\t"
        " if(%3!=0) goto 1b \n\t"
        " r8 = r4*r6 (f) # r5 = [%0++=4] \n\t"
        " r9 = r6*r5 (f) # r7 = [%1++=4] \n\t"
        " r8 += r5*r7 (f)  \n\t"
        " r9 -= r4*r7 (f)  \n\t"
        " [%2++=4] = r8 \n\t"
        " [%2++=4] = r9 \n\t"
        : "=&r"(pVec1), "=&r"(pVec2), "=&r"(pDestVec), "=&r"(Count)
        : "0"(pVec1), "1"(pVec2), "2"(pDestVec), "3"(Count)
        : "r4", "r5", "r6", "r7", "r8", "r9", "memory");
#elif ASM_ENABLE==3
    vector_complex_zs32_ys32_mul_conj_xs32(pDestVec, pVec1, pVec2, Count, 0);
#endif
}

void CVecSub(CFloat *pDestVec,  CFloat *pVec1,  CFloat *pVec2, int Count)
{
    int i;

#if ASM_ENABLE==1
#if 1
    for (i = 0; i < Count; i++) {
        pDestVec[i].re = pVec1[i].re - pVec2[i].re;
        pDestVec[i].im = pVec1[i].im - pVec2[i].im;
    }
#else
    for (int i = 0; i < Count; i = i + 2) {
        pDestVec[i] = pVec1[i] - pVec2[i];
        pDestVec[i + 1] = pVec1[i + 1] - pVec2[i + 1];
    }
#endif
#elif ASM_ENABLE==2
    __asm volatile(
        " %3 -= 1 \n\t "
        " r4 = [%0++=4]\n\t"
        "1: \n\t "
        " rep %3 { \n\t"
        " r6 = [%1++=4]\n\t"
        " r7 = [%1++=4]\n\t"
        " r8 = r4 - r6 (f) # r5 = [%0++=4] \n\t"
        " r9 = r5 - r7 (f) # r4 = [%0++=4] \n\t"
        " [%2++=4] = r8 \n\t"
        " [%2++=4] = r9 \n\t"
        " }\n\t"
        " if(%3!=0) goto 1b \n\t"
        " r6 = [%1++=4]\n\t"
        " r7 = [%1++=4]\n\t"
        " r8 = r4 - r6 (f) # r5 = [%0++=4] \n\t"
        " r9 = r5 - r7 (f)  \n\t"
        " [%2++=4] = r8 \n\t"
        " [%2++=4] = r9 \n\t"
        : "=&r"(pVec1), "=&r"(pVec2), "=&r"(pDestVec), "=&r"(Count)
        : "0"(pVec1), "1"(pVec2), "2"(pDestVec), "3"(Count)
        : "r4", "r5", "r6", "r7", "r8", "r9", "memory");
#elif ASM_ENABLE==3
    vector_complex_zs32_ys32_sub_xs32(pDestVec, pVec1, pVec2, Count);
#endif
}

void CVecAdd(CFloat *pDestVec,  CFloat *pVec1,  CFloat *pVec2, int Count)
{
    int i;

#if ASM_ENABLE==1
#if 1
    for (i = 0; i < Count; i++) {
        pDestVec[i].re = pVec1[i].re + pVec2[i].re;
        pDestVec[i].im = pVec1[i].im + pVec2[i].im;
    }
#else
    for (int i = 0; i < Count; i = i + 2) {
        pDestVec[i] = pVec1[i] + pVec2[i];
        pDestVec[i + 1] = pVec1[i + 1] + pVec2[i + 1];
    }
#endif
#elif ASM_ENABLE==2
    __asm volatile(
        " %3 -= 1 \n\t "
        " r4 = [%0++=4]\n\t"
        "1: \n\t "
        " rep %3 { \n\t"
        " r6 = [%1++=4]\n\t"
        " r7 = [%1++=4]\n\t"
        " r8 = r4 + r6 (f) # r5 = [%0++=4] \n\t"
        " r9 = r5 + r7 (f) # r4 = [%0++=4] \n\t"
        " [%2++=4] = r8 \n\t"
        " [%2++=4] = r9 \n\t"
        " }\n\t"
        " if(%3!=0) goto 1b \n\t"
        " r6 = [%1++=4]\n\t"
        " r7 = [%1++=4]\n\t"
        " r8 = r4 + r6 (f) # r5 = [%0++=4] \n\t"
        " r9 = r5 + r7 (f)  \n\t"
        " [%2++=4] = r8 \n\t"
        " [%2++=4] = r9 \n\t"
        : "=&r"(pVec1), "=&r"(pVec2), "=&r"(pDestVec), "=&r"(Count)
        : "0"(pVec1), "1"(pVec2), "2"(pDestVec), "3"(Count)
        : "r4", "r5", "r6", "r7", "r8", "r9", "memory");
#elif ASM_ENABLE==3
    vector_complex_zs32_ys32_add_xs32(pDestVec, pVec1, pVec2, Count);
#endif
}

void CVecPowerSum(float *pDesVec,  CFloat *pVec, int Count)
{
    int i;
#if ASM_ENABLE==1
#if 1
    for (i = 0; i < Count; i++) {
        pDesVec[i] += (pVec[i].re * pVec[i].re) + (pVec[i].im * pVec[i].im);
    }
#else
    for (int i = 0; i < 2 * Count; i = i + 2) {
        pDesVec[i / 2] += (pVec[i] * pVec[i]) + (pVec[i + 1] * pVec[i + 1]);
    }
#endif
#elif ASM_ENABLE==2
    __asm volatile(
        " %2 -= 1 \n\t "
        " r4 = [%0++=4]\n\t"
        " r5 = [%0++=4]\n\t"
        "1: \n\t "
        " rep %2 { \n\t"
        " r6 = r4*r4 (f) #r7 = [%1]  \n\t"
        " r6 += r5*r5 (f) # r4 = [%0++=4] \n\t"
        " r6 = r6 + r7 (f) # r5 = [%0++=4] \n\t"
        " [%1++=4] = r6 \n\t"
        " }\n\t"
        " if(%2!=0) goto 1b \n\t"
        " r6 = r4*r4 (f) #r7 = [%1]  \n\t"
        " r6 += r5*r5 (f) \n\t"
        " r6 = r6 + r7 (f) \n\t"
        " [%1++=4] = r6 \n\t"
        : "=&r"(pVec), "=&r"(pDesVec), "=&r"(Count)
        : "0"(pVec), "1"(pDesVec), "2"(Count)
        : "r4", "r5", "r6", "r7", "memory");
#elif ASM_ENABLE==3
    vector_complex_zs32_qdt_xs32(pVec, pVec, Count, 0);

    //vector_complex_zs32_ys32_add_xs32(pDesVec, pDesVec, pVec, Count/8);
    vector_real_zs32_ys32_add_xs32(pDesVec, pDesVec, pVec, Count / 4);
#endif
}

void CVecAbsSum(float *pDest,  CFloat *pVec, int Count)
{
    int i;
    float fSum;

    fSum = *pDest;
#if ASM_ENABLE==1
    for (i = 0; i < Count; i++) {
        fSum += (fabs(pVec[i].re) + fabs(pVec[i].im));
    }
#else
    __asm volatile(
        " %2 -= 1 \n\t "
        " r4 = [%0++=4]\n\t"
        " r5 = [%0++=4]\n\t"
        "1: \n\t "
        " rep %2 { \n\t"
        " r6 = fabs(r4)   \n\t"
        " r4 = [%0++=4] \n\t"
        " r7 = fabs(r5)   \n\t"
        " r5 = [%0++=4] \n\t"
        " %1 = %1 + r6 (f) \n\t"
        " %1 = %1 + r7 (f) \n\t"
        " }\n\t"
        " if(%2!=0) goto 1b \n\t"
        " r6 = fabs(r4)   \n\t"
        " r7 = fabs(r5)   \n\t"
        " %1 = %1 + r6 (f) \n\t"
        " %1 = %1 + r7 (f) \n\t"
        : "=&r"(pVec), "=&r"(fSum), "=&r"(Count)
        : "0"(pVec), "1"(fSum), "2"(Count)
        : "r4", "r5", "r6", "r7", "memory");
#endif

    *pDest = fSum;
}

void complex_test()
{
    static CFloat pVec1[2] = {1.0f, 1.0f, 1.0f, 1.0f};
    static CFloat pVec2[2] = {3.0f, 3.0f, 3.0f, 3.0f};

    static CFloat pDestVec[2];

    static float pPow[2];
    static float sum = 0.0f;

    CVecMultConjSum(pDestVec,  pVec1, pVec2, 2);

    for (int i = 0; i < 2; i++) {

        printf("r:%d,i:%d\n", (int)(pDestVec[i].re * 1000), (int)(pDestVec[i].im * 1000));
    }

    printf("%s\n", "CVecSub");
    CVecSub(pDestVec,  pVec1, pVec2, 2);

    for (int i = 0; i < 2; i++) {

        printf("r:%d,i:%d\n", (int)(pDestVec[i].re * 1000), (int)(pDestVec[i].im * 1000));
    }

    printf("%s\n", "CVecPowerSum");
    CVecPowerSum(pPow,  pVec1, 2);

    for (int i = 0; i < 2; i++) {

        printf("r:%d\n", (int)(pPow[i] * 1000));
    }

    printf("%s\n", "CVecAbsSum");
    CVecAbsSum(&sum,  pVec1, 2);
    printf("%d\n", (int)(sum * 1000));



}

#if 0
void pi32v2_dot_prod_q15(
    const q15_t *pSrcA,
    const q15_t *pSrcB,
    uint32_t blockSize,
    q63_t *result)
{
    uint32_t blkCnt;                               /* Loop counter */
    volatile q63_t sum = 0;                                 /* Temporary return variable */

#if 1

    {
        blkCnt = blockSize >> 1;

        __asm volatile(
            " %3 -= 1\n\t " \
            " r6 = [%1++=4]\n\t" \
            " r7 = [%0++=4]\n\t" \
            " r15 = 0 \n\t" \
            "1: \n\t " \
            " rep %3 { \n\t" \
            " r7_r6 = r6.h,r6.l *|* r7.h,r7.l (ssat) \n\t" \
            " r6 = r6 + r7 (ssat) # r7 = [%0++=4]\n\t" \
            " %2.l = %2.l+r6  # r6 = [%1++=4]\n\t" \
            " %2.h = %2.h+r15+c  \n\t" \
            " }\n\t" \
            " if(%3!=0) goto 1b \n\t" \
            " r7_r6 = r6.h,r6.l *|* r7.h,r7.l (ssat) \n\t" \
            " r6 = r6 + r7 (ssat) \n\t" \
            " %2.l = %2.l+r6  \n\t" \
            " %2.h = %2.h+r15+c  \n\t" \
            : "=&w"(pSrcA), "=&w"(pSrcB), "=&r"(sum), "=&r"(blkCnt) \
            : "0"(pSrcA), "1"(pSrcB), "2"(sum), "3"(blkCnt) \
            : "r15", "r6", "r7");

    }
    blkCnt = blockSize & 0x01;

#else

    /* Initialize blkCnt with number of samples */
    blkCnt = blockSize;

#endif /* #if defined (PI32V2_MATH_LOOPUNROLL) */

    while (blkCnt > 0U) {
        /* C = A[0]* B[0] + A[1]* B[1] + A[2]* B[2] + .....+ A[blockSize-1]* B[blockSize-1] */
        /* Calculate dot product and store result in a temporary buffer. */
        sum += (q63_t)((q31_t) * pSrcA++ * *pSrcB++);


        /* Decrement loop counter */
        blkCnt--;
    }

    /* Store result in destination buffer in 34.30 format */
    *result = sum;
}

/**
  @} end of BasicDotProd group
 */

#endif

#endif
