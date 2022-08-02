
#include "typedef.h"

static void avg8x8_aligned(u8 *block, const u8 *s0, const u8 *s1,
                           int dest_size, int line_size, int h)
{
    __asm__ volatile(
        " %3 = %3-4\n\t"
        " %4 = %4-4\n\t"
        " 1: \n\t"
        "  %5 -= 1 # r4 = [%1++=4]\n\t"
        "  r6 = [%2++=4]\n\t"
        "  r5 = [%1++=%4]\n\t"
        "  r7 = [%2++=%4]\n\t"
        "  r4 = r4 +|+|+|+ r6 (uavg,rnd) \n\t"
        "  r5 = r5 +|+|+|+ r7 (uavg,rnd) \n\t"
        "  [%0++=4] = r4\n\t"
        "  [%0++=%3] = r5\n\t"
        "if(%5!=0) goto 1b \n\t"
        :"=&r"(block), "=&r"(s0), "=&r"(s1), "=&r"(dest_size), "=&r"(line_size), "=&r"(h)
        :"0"(block), "1"(s0), "2"(s1), "3"(dest_size), "4"(line_size), "5"(h)
        :"r5", "r4", "r7", "r6"
    );
}

#define AVG8x8_UNALIGNED_S0(S0_N,S0_A)                              \
static void avg8x8_unaligned_##S0_N##_0 (u8 *block, const u8 *s0, const u8 *s1,     \
                 int dest_size, int line_size, int h)               \
{                                                                   \
    __asm__ volatile(                                               \
        " %3 = %3-4\n\t"                                            \
        " %4 = %4-4\n\t"                                            \
        " r9 = %4-4\n\t"                                            \
        " r10 = %2-"#S0_N"\n\t"                                           \
        " 1: \n\t"                                                  \
        "  %5 -= 1 # r4 = [%1++=4]\n\t"                             \
        "  r5 = [%1++=%4]\n\t"                                      \
        "  r6 = [r10++=4]\n\t"                                      \
        "  r7 = [r10++=4]\n\t"                                      \
        "  r8 = [r10++=r9]\n\t"                                     \
        "  r6 = "#S0_A"(r7,r6)\n\t"                                 \
        "  r7 = "#S0_A"(r8,r7)\n\t"                                 \
        "  r4 = r4 +|+|+|+ r6 (uavg,rnd) \n\t"                      \
        "  r5 = r5 +|+|+|+ r7 (uavg,rnd) \n\t"                      \
        "  [%0++=4] = r4\n\t"                                       \
        "  [%0++=%3] = r5\n\t"                                      \
        "if(%5!=0) goto 1b \n\t"                                    \
        :"=&r"(block),"=&r"(s1), "=&r"(s0), "=&r"(dest_size), "=&r"(line_size), "=&r"(h)    \
        :"0"(block),"1"(s1), "2"(s0), "3"(dest_size), "4"(line_size), "5"(h)                \
        :"r5", "r4", "r7", "r6", "r9","r8","r10"                                            \
        );                                                                                  \
}                                                                                           \


#define AVG8x8_UNALIGNED_S1(S0_N,S0_A)                              \
static void avg8x8_unaligned_0_##S0_N  (u8 *block, const u8 *s0, const u8 *s1,     \
                 int dest_size, int line_size, int h)               \
{                                                                   \
    __asm__ volatile(                                               \
        " %3 = %3-4\n\t"                                            \
        " %4 = %4-4\n\t"                                            \
        " r9 = %4-4\n\t"                                            \
        " r10 = %2-"#S0_N"\n\t"                                           \
        " 1: \n\t"                                                  \
        "  %5 -= 1 # r4 = [%1++=4]\n\t"                             \
        "  r5 = [%1++=%4]\n\t"                                      \
        "  r6 = [r10++=4]\n\t"                                      \
        "  r7 = [r10++=4]\n\t"                                      \
        "  r8 = [r10++=r9]\n\t"                                     \
        "  r6 = "#S0_A"(r7,r6)\n\t"                                 \
        "  r7 = "#S0_A"(r8,r7)\n\t"                                 \
        "  r4 = r4 +|+|+|+ r6 (uavg,rnd) \n\t"                      \
        "  r5 = r5 +|+|+|+ r7 (uavg,rnd) \n\t"                      \
        "  [%0++=4] = r4\n\t"                                       \
        "  [%0++=%3] = r5\n\t"                                      \
        "if(%5!=0) goto 1b \n\t"                                    \
        :"=&r"(block),"=&r"(s0), "=&r"(s1), "=&r"(dest_size), "=&r"(line_size), "=&r"(h)    \
        :"0"(block),"1"(s0), "2"(s1), "3"(dest_size), "4"(line_size), "5"(h)                \
        :"r5", "r4", "r7", "r6", "r9","r8","r10"                                            \
        );                                                                                  \
}                                                                                           \


#define AVG8x8_UNALIGNED_BOTH(S0_N,S1_N,S0_A,S1_A)                           \
    static void avg8x8_unaligned_##S0_N##_##S1_N (u8 *block, const u8 *s0, const u8 *s1,   \
                     int dest_size, int line_size, int h)               \
    {                                                                   \
        __asm__ volatile(                                               \
            " %3 = %3-4\n\t"                                            \
            " %4 = %4-8\n\t"                                            \
            " r9 = %1-"#S0_N"\n\t"                                       \
            " r10 = %2-"#S1_N"\n\t"                                      \
            " 1: \n\t"                                                  \
            "  %5 -= 1 \n\t"                                            \
            "  r4 = [r9++=4]\n\t"                                       \
            "  r5 = [r9++=4]\n\t"                                      \
            "  r8 = [r9++=%4]\n\t"                                      \
            "  r4 = "#S0_A"(r5,r4)\n\t"                                 \
            "  r5 = "#S0_A"(r8,r5)\n\t"                                 \
            "  r6 = [r10++=4]\n\t"                                      \
            "  r7 = [r10++=4]\n\t"                                      \
            "  r8 = [r10++=%4]\n\t"                                     \
            "  r6 = "#S1_A"(r7,r6)\n\t"                                 \
            "  r7 = "#S1_A"(r8,r7)\n\t"                                 \
            "  r4 = r4 +|+|+|+ r6 (uavg,rnd) \n\t"                      \
            "  r5 = r5 +|+|+|+ r7 (uavg,rnd) \n\t"                      \
            "  [%0++=4] = r4\n\t"                                       \
            "  [%0++=%3] = r5\n\t"                                      \
            "if(%5!=0) goto 1b \n\t"                                    \
            :"=&r"(block),"=&r"(s0), "=&r"(s1), "=&r"(dest_size), "=&r"(line_size), "=&r"(h)     \
            :"0"(block),"1"(s0), "2"(s1), "3"(dest_size), "4"(line_size), "5"(h)                 \
            :"r5", "r4", "r7", "r6", "r9","r8","r10"                                             \
            );                                                                                   \
    }                                                                                            \



AVG8x8_UNALIGNED_S0(1, align8)
AVG8x8_UNALIGNED_S0(2, align16)
AVG8x8_UNALIGNED_S0(3, align24)
AVG8x8_UNALIGNED_S1(1, align8)
AVG8x8_UNALIGNED_S1(2, align16)
AVG8x8_UNALIGNED_S1(3, align24)
AVG8x8_UNALIGNED_BOTH(1, 1, align8, align8)
AVG8x8_UNALIGNED_BOTH(1, 2, align8, align16)
AVG8x8_UNALIGNED_BOTH(1, 3, align8, align24)
AVG8x8_UNALIGNED_BOTH(2, 1, align16, align8)
AVG8x8_UNALIGNED_BOTH(2, 2, align16, align16)
AVG8x8_UNALIGNED_BOTH(2, 3, align16, align24)
AVG8x8_UNALIGNED_BOTH(3, 1, align24, align8)
AVG8x8_UNALIGNED_BOTH(3, 2, align24, align16)
AVG8x8_UNALIGNED_BOTH(3, 3, align24, align24)

void avg8x8(u8 *block, const u8 *s0, const u8 *s1,
            int dest_size, int line_size, int h)
{
    int s0_lower2bit = (int)s0 & 0x3;
    int s1_lower2bit = (int)s1 & 0x3;

    static void (*avg8x8_tab[4][4])(u8 * block, const u8 * s0, const u8 * s1,
                                    int dest_size, int line_size, int h) = {
        {avg8x8_aligned, avg8x8_unaligned_0_1, avg8x8_unaligned_0_2, avg8x8_unaligned_0_3},
        {avg8x8_unaligned_1_0, avg8x8_unaligned_1_1, avg8x8_unaligned_1_2, avg8x8_unaligned_1_3},
        {avg8x8_unaligned_2_0, avg8x8_unaligned_2_1, avg8x8_unaligned_2_2, avg8x8_unaligned_2_3},
        {avg8x8_unaligned_3_0, avg8x8_unaligned_3_1, avg8x8_unaligned_3_2, avg8x8_unaligned_3_3},
    };


    avg8x8_tab[s0_lower2bit][s1_lower2bit](block, s0, s1, dest_size, line_size, h);
}

static u32 sad8x8_aligned(const u8 *s0, const u8 *s1,
                          int dest_size, int line_size, int h)
{
    int out = 0;
    __asm__ volatile(
        " %3 = %3-4\n\t"
        " %4 = %4-4\n\t"
        " r8 = 0\n\t"
        " r9 = 0\n\t"
        " 1: \n\t"
        " rep %5 {\n\t"
        "  r4 = [%1++=4]\n\t"
        "  r6 = [%2++=4]\n\t"
        "  r5 = [%1++=%3]\n\t"
        "  r7 = [%2++=%4]\n\t"
        "  r9_r8 += abs(r4 -|-|-|- r6) (usat) \n\t"
        "  r9_r8 += abs(r5 -|-|-|- r7) (usat) \n\t"
        "}\n\t"
        "r8 = r8.h,r8.l +|+ r9.h,r9.l (usat)  \n\t"
        "%0.l = r8.h + r8.l \n\t"
        :"=&r"(out), "=&r"(s0), "=&r"(s1), "=&r"(dest_size), "=&r"(line_size), "=&r"(h)
        :"0"(out), "1"(s0), "2"(s1), "3"(dest_size), "4"(line_size), "5"(h)
        :"r5", "r4", "r7", "r6", "r9", "r8"
    );
    return out;
}

#define SAD8x8_UNALIGNED_S0(S0_N,S0_A)                              \
static u32 sad8x8_unaligned_##S0_N##_0 (const u8 *s0, const u8 *s1,                 \
                 int dest_size, int line_size, int h)               \
{                                                                   \
    int out = 0;                                                    \
    __asm__ volatile(                                               \
        " %3 = %3-4\n\t"                                            \
        " %4 = %4-8\n\t"                                            \
        " r8 = 0\n\t"                                               \
        " r9 = 0\n\t"                                               \
        " r10 = %2-"#S0_N"\n\t"                                           \
        " 1: \n\t"                                                  \
        "  %5 -= 1 #  r4 = [%1++=4]\n\t"                             \
        "  r5 = [%1++=%3]\n\t"                                      \
        "  r6 = [r10++=4]\n\t"                                      \
        "  r7 = [r10++=4]\n\t"                                      \
        "  r12 = [r10++=%4]\n\t"                                   \
        "  r6 = "#S0_A"(r7,r6)\n\t"                                  \
        "  r7 = "#S0_A"(r12,r7)\n\t"                                 \
        "  r9_r8 += abs(r4 -|-|-|- r6) (usat) \n\t"                 \
        "  r9_r8 += abs(r5 -|-|-|- r7) (usat) \n\t"                 \
        "if(%5!=0) goto 1b \n\t"                                    \
        "r8 = r8.h,r8.l +|+ r9.h,r9.l (usat)  \n\t"                 \
        "%0.l = r8.h + r8.l \n\t"                                   \
        :"=&r"(out),"=&r"(s1), "=&r"(s0), "=&r"(line_size), "=&r"(dest_size), "=&r"(h)  \
        :"0"(out),"1"(s1), "2"(s0), "3"(line_size), "4"(dest_size), "5"(h)              \
        :"r5", "r4", "r7", "r6", "r9","r8","r10", "r12"                           \
        );                                                          \
    return out;                                                     \
}

#define SAD8x8_UNALIGNED_S1(S0_N,S0_A)                              \
static u32 sad8x8_unaligned_0_##S0_N (const u8 *s0, const u8 *s1,                 \
                 int dest_size, int line_size, int h)               \
{                                                                   \
    int out = 0;                                                    \
    __asm__ volatile(                                               \
        " %3 = %3-4\n\t"                                            \
        " %4 = %4-8\n\t"                                            \
        " r8 = 0\n\t"                                               \
        " r9 = 0\n\t"                                               \
        " r10 = %2-"#S0_N"\n\t"                                           \
        " 1: \n\t"                                                  \
        "  %5 -= 1 # r4 = [%1++=4]\n\t"                             \
        "  r5 = [%1++=%3]\n\t"                                      \
        "  r6 = [r10++=4]\n\t"                                      \
        "  r7 = [r10++=4]\n\t"                                      \
        "  r12 = [r10++=%4]\n\t"                                   \
        "  r6 = "#S0_A"(r7,r6)\n\t"                                  \
        "  r7 = "#S0_A"(r12,r7)\n\t"                                 \
        "  r9_r8 += abs(r4 -|-|-|- r6) (usat) \n\t"                 \
        "  r9_r8 += abs(r5 -|-|-|- r7) (usat) \n\t"                 \
        "if(%5!=0) goto 1b \n\t"                                    \
        "r8 = r8.h,r8.l +|+ r9.h,r9.l (usat)  \n\t"                 \
        "%0.l = r8.h + r8.l \n\t"                                   \
        :"=&r"(out),"=&r"(s0), "=&r"(s1), "=&r"(dest_size), "=&r"(line_size), "=&r"(h)  \
        :"0"(out),"1"(s0), "2"(s1), "3"(dest_size), "4"(line_size), "5"(h)              \
        :"r5", "r4", "r7", "r6", "r9","r8","r10","r12"                           \
        );                                                          \
    return out;                                                     \
}

#define SAD8x8_UNALIGNED_BOTH(S0_N,S1_N,S0_A,S1_A)                  \
static u32 sad8x8_unaligned_##S0_N##_##S1_N (const u8 *s0, const u8 *s1,          \
                 int dest_size, int line_size, int h)               \
{                                                                   \
    int out = 0;                                                    \
    __asm__ volatile(                                               \
        " %3 = %3-8\n\t"                                            \
        " %4 = %4-8\n\t"                                            \
        " r8 = 0\n\t"                                               \
        " r9 = 0\n\t"                                               \
        " r11 = %1-"#S0_N"\n\t"                                           \
        " r10 = %2-"#S1_N"\n\t"                                           \
        " 1: \n\t"                                                  \
        "  %5 -= 1 \n\t" \
        "  r4 = [r11++=4]\n\t"                             \
        "  r5 = [r11++=4]\n\t"                                      \
        "  r12 = [r11++=%3]\n\t"                                   \
        "  r4 = "#S0_A"(r5,r4)\n\t"                                  \
        "  r5 = "#S0_A"(r12,r5)\n\t"                                 \
        "  r6 = [r10++=4]\n\t"                                      \
        "  r7 = [r10++=4]\n\t"                                      \
        "  r12 = [r10++=%4]\n\t"                                   \
        "  r6 = "#S1_A"(r7,r6)\n\t"                                  \
        "  r7 = "#S1_A"(r12,r7)\n\t"                                 \
        "  r9_r8 += abs(r4 -|-|-|- r6) (usat) \n\t"                 \
        "  r9_r8 += abs(r5 -|-|-|- r7) (usat) \n\t"                 \
        "if(%5!=0) goto 1b \n\t"                                    \
        "r8 = r8.h,r8.l +|+ r9.h,r9.l (usat)  \n\t"                 \
        "%0.l = r8.h + r8.l \n\t"                                   \
        :"=&r"(out),"=&r"(s0), "=&r"(s1), "=&r"(dest_size), "=&r"(line_size), "=&r"(h)  \
        :"0"(out),"1"(s0), "2"(s1), "3"(dest_size), "4"(line_size), "5"(h)              \
        :"r5", "r4", "r7", "r6", "r9","r8","r10","r11","r12"                           \
        );                                                          \
    return out;                                                     \
}

SAD8x8_UNALIGNED_S0(1, align8)
SAD8x8_UNALIGNED_S0(2, align16)
SAD8x8_UNALIGNED_S0(3, align24)
SAD8x8_UNALIGNED_S1(1, align8)
SAD8x8_UNALIGNED_S1(2, align16)
SAD8x8_UNALIGNED_S1(3, align24)
SAD8x8_UNALIGNED_BOTH(1, 1, align8, align8)
SAD8x8_UNALIGNED_BOTH(1, 2, align8, align16)
SAD8x8_UNALIGNED_BOTH(1, 3, align8, align24)
SAD8x8_UNALIGNED_BOTH(2, 1, align16, align8)
SAD8x8_UNALIGNED_BOTH(2, 2, align16, align16)
SAD8x8_UNALIGNED_BOTH(2, 3, align16, align24)
SAD8x8_UNALIGNED_BOTH(3, 1, align24, align8)
SAD8x8_UNALIGNED_BOTH(3, 2, align24, align16)
SAD8x8_UNALIGNED_BOTH(3, 3, align24, align24)

u32 sad8x8(const u8 *s0, const u8 *s1,
           int dest_size, int line_size, int h)
{
    int s0_lower2bit = (int)s0 & 0x3;
    int s1_lower2bit = (int)s1 & 0x3;

    static u32(*sad8x8_tab[4][4])(const u8 * s0, const u8 * s1,
                                  int dest_size, int line_size, int h) = {
        {sad8x8_aligned, sad8x8_unaligned_0_1, sad8x8_unaligned_0_2, sad8x8_unaligned_0_3},
        {sad8x8_unaligned_1_0, sad8x8_unaligned_1_1, sad8x8_unaligned_1_2, sad8x8_unaligned_1_3},
        {sad8x8_unaligned_2_0, sad8x8_unaligned_2_1, sad8x8_unaligned_2_2, sad8x8_unaligned_2_3},
        {sad8x8_unaligned_3_0, sad8x8_unaligned_3_1, sad8x8_unaligned_3_2, sad8x8_unaligned_3_3},
    };

    return sad8x8_tab[s0_lower2bit][s1_lower2bit](s0, s1, dest_size, line_size, h);
}


