#include "printf.h"
#include <stdlib.h>
//#include <stdio.h>
#include <math.h>

#include "pixel_flow.h"

#define __INLINE inline
#define __ASM asm

#define USE_SIMD 0 //使用原始M3汇编
#define USE_BFIN 1 //使用blackfin汇编
extern u32 sad8x8(const u8 *s0, const u8 *s1, int dest_size, int line_size, int h);
extern void avg8x8(u8 *block, const u8 *s0, const u8 *s1, int dest_size, int line_size, int h);

/*两个方法来得到最后的位移:
     0：所有位移技术的算术平均值；
     1：取计数最大的位移附近的几个计数做加权平均；*/
#define USE_HIST_FILTER  1

#define FIXED_PRECISE  16  //定点精度

#define BOTTOM_FLOW_SEARCH_WINDOW_SIZE  4

// Threshold
#define PARA_FEATURE_THRESHOLD		120
#define PARA_FULLPIXEL_THRESHOLD	5000


#define SEARCH_SIZE	BOTTOM_FLOW_SEARCH_WINDOW_SIZE // maximum offset to search: 4 + 1/2 pixels
#define TILE_SIZE	8               						// x & y tile size
#define NUM_BLOCKS	8 //3 // x & y number of tiles to check

#define FLOW_DEBUG //printf

/**
 * @brief Compute the average pixel gradient of all horizontal and vertical steps
 *
 * TODO compute_diff is not appropriate for low-light mode images
 *
 * @param image ...
 * @param offX x coordinate of upper left corner of 8x8 pattern in image
 * @param offY y coordinate of upper left corner of 8x8 pattern in image
 */
static inline u32 compute_diff(u8 *image, u16 offX, u16 offY, u16 row_size)
{
    /* calculate position in image buffer */
    u16 off = (offY + 2) * row_size + (offX + 2); // we calc only the 4x4 pattern
    u32 acc = 0;

#if USE_SIMD

    /* calc row diff */
    acc = __USAD8(*((u32 *) &image[off + 0 + 0 * row_size]), *((u32 *) &image[off + 0 + 1 * row_size]));
    acc = __USADA8(*((u32 *) &image[off + 0 + 1 * row_size]), *((u32 *) &image[off + 0 + 2 * row_size]), acc);
    acc = __USADA8(*((u32 *) &image[off + 0 + 2 * row_size]), *((u32 *) &image[off + 0 + 3 * row_size]), acc);

    /* we need to get columns */
    u32 col1 = (image[off + 0 + 0 * row_size] << 24) | image[off + 1 + 0 * row_size] << 16 | image[off + 2 + 0 * row_size] << 8 | image[off + 3 + 0 * row_size];
    u32 col2 = (image[off + 0 + 1 * row_size] << 24) | image[off + 1 + 1 * row_size] << 16 | image[off + 2 + 1 * row_size] << 8 | image[off + 3 + 1 * row_size];
    u32 col3 = (image[off + 0 + 2 * row_size] << 24) | image[off + 1 + 2 * row_size] << 16 | image[off + 2 + 2 * row_size] << 8 | image[off + 3 + 2 * row_size];
    u32 col4 = (image[off + 0 + 3 * row_size] << 24) | image[off + 1 + 3 * row_size] << 16 | image[off + 2 + 3 * row_size] << 8 | image[off + 3 + 3 * row_size];

    /* calc column diff */
    acc = __USADA8(col1, col2, acc);
    acc = __USADA8(col2, col3, acc);
    acc = __USADA8(col3, col4, acc);
#else

#if USE_BFIN


    acc = sad8x8(&image[off], &image[off +       1], row_size, row_size, 4);
    acc += sad8x8(&image[off], &image[off + row_size], row_size, row_size, 4);
#else
    int i;
    acc = 0;
    for (i = 0; i < 4; i++) {
        acc += abs(image[off + 0 + (i + 1) * row_size] - image[off + 0 + (i + 0) * row_size]);
        acc += abs(image[off + 1 + (i + 1) * row_size] - image[off + 1 + (i + 0) * row_size]);
        acc += abs(image[off + 2 + (i + 1) * row_size] - image[off + 2 + (i + 0) * row_size]);
        acc += abs(image[off + 3 + (i + 1) * row_size] - image[off + 3 + (i + 0) * row_size]);
        acc += abs(image[off + 4 + (i + 1) * row_size] - image[off + 4 + (i + 0) * row_size]);
        acc += abs(image[off + 5 + (i + 1) * row_size] - image[off + 5 + (i + 0) * row_size]);
        acc += abs(image[off + 6 + (i + 1) * row_size] - image[off + 6 + (i + 0) * row_size]);
        acc += abs(image[off + 7 + (i + 1) * row_size] - image[off + 7 + (i + 0) * row_size]);
    }
    for (i = 0; i < 4; i++) {
        acc += abs(image[off + 1 + (i + 0) * row_size] - image[off + 0 + (i + 0) * row_size]);
        acc += abs(image[off + 2 + (i + 0) * row_size] - image[off + 1 + (i + 0) * row_size]);
        acc += abs(image[off + 3 + (i + 0) * row_size] - image[off + 2 + (i + 0) * row_size]);
        acc += abs(image[off + 4 + (i + 0) * row_size] - image[off + 3 + (i + 0) * row_size]);
        acc += abs(image[off + 5 + (i + 0) * row_size] - image[off + 4 + (i + 0) * row_size]);
        acc += abs(image[off + 6 + (i + 0) * row_size] - image[off + 5 + (i + 0) * row_size]);
        acc += abs(image[off + 7 + (i + 0) * row_size] - image[off + 6 + (i + 0) * row_size]);
        acc += abs(image[off + 8 + (i + 0) * row_size] - image[off + 7 + (i + 0) * row_size]);
    }
#endif
#endif

    return acc;

}

#if (USE_SIMD==1)
/**
 * @brief Compute SAD distances of subpixel shift of two 8x8 pixel patterns.
 *
 * @param image1 ...
 * @param image2 ...
 * @param off1X x coordinate of upper left corner of pattern in image1
 * @param off1Y y coordinate of upper left corner of pattern in image1
 * @param off2X x coordinate of upper left corner of pattern in image2
 * @param off2Y y coordinate of upper left corner of pattern in image2
 * @param acc array to store SAD distances for shift in every direction
 */
static inline u32 compute_subpixel(u8 *image1, u8 *image2, u16 off1X, u16 off1Y, u16 off2X, u16 off2Y, u32 *acc, u16 row_size)
{
    /* calculate position in image buffer */
    u16 off1 = off1Y * row_size + off1X; // image1
    u16 off2 = off2Y * row_size + off2X; // image2

    u32 s0, s1, s2, s3, s4, s5, s6, s7, t1, t3, t5, t7;

    for (u16 i = 0; i < 8; i++) {
        acc[i] = 0;
    }


    /*
     * calculate for each pixel in the 8x8 field with upper left corner (off1X / off1Y)
     * every iteration is one line of the 8x8 field.
     *
     *  + - + - + - + - + - + - + - + - +
     *  |   |   |   |   |   |   |   |   |
     *  + - + - + - + - + - + - + - + - +
     *
     *
     */

    for (u16 i = 0; i < 8; i++) {
        /*
         * first column of 4 pixels:
         *
         *  + - + - + - + - + - + - + - + - +
         *  | x | x | x | x |   |   |   |   |
         *  + - + - + - + - + - + - + - + - +
         *
         * the 8 s values are from following positions for each pixel (X):
         *  + - + - + - +
         *  +   5   7   +
         *  + - + 6 + - +
         *  +   4 X 0   +
         *  + - + 2 + - +
         *  +   3   1   +
         *  + - + - + - +
         *
         *  variables (s1, ...) contains all 4 results (32bit -> 4 * 8bit values)
         *
         */

        /* compute average of two pixel values */
        s0 = (__UHADD8(*((u32 *) &image2[off2 +  0 + (i + 0) * row_size]), *((u32 *) &image2[off2 + 1 + (i + 0) * row_size])));
        s1 = (__UHADD8(*((u32 *) &image2[off2 +  0 + (i + 1) * row_size]), *((u32 *) &image2[off2 + 1 + (i + 1) * row_size])));
        s2 = (__UHADD8(*((u32 *) &image2[off2 +  0 + (i + 0) * row_size]), *((u32 *) &image2[off2 + 0 + (i + 1) * row_size])));
        s3 = (__UHADD8(*((u32 *) &image2[off2 +  0 + (i + 1) * row_size]), *((u32 *) &image2[off2 - 1 + (i + 1) * row_size])));
        s4 = (__UHADD8(*((u32 *) &image2[off2 +  0 + (i + 0) * row_size]), *((u32 *) &image2[off2 - 1 + (i + 0) * row_size])));
        s5 = (__UHADD8(*((u32 *) &image2[off2 +  0 + (i - 1) * row_size]), *((u32 *) &image2[off2 - 1 + (i - 1) * row_size])));
        s6 = (__UHADD8(*((u32 *) &image2[off2 +  0 + (i + 0) * row_size]), *((u32 *) &image2[off2 + 0 + (i - 1) * row_size])));
        s7 = (__UHADD8(*((u32 *) &image2[off2 +  0 + (i - 1) * row_size]), *((u32 *) &image2[off2 + 1 + (i - 1) * row_size])));

        /* these 4 t values are from the corners around the center pixel */
        t1 = (__UHADD8(s0, s1));
        t3 = (__UHADD8(s3, s4));
        t5 = (__UHADD8(s4, s5));
        t7 = (__UHADD8(s7, s0));

        /*
         * finally we got all 8 subpixels (s0, t1, s2, t3, s4, t5, s6, t7):
         *  + - + - + - +
         *  |   |   |   |
         *  + - 5 6 7 - +
         *  |   4 X 0   |
         *  + - 3 2 1 - +
         *  |   |   |   |
         *  + - + - + - +
         */

        /* fill accumulation vector */
        acc[0] = __USADA8((*((u32 *) &image1[off1 + 0 + i * row_size])), s0, acc[0]);
        acc[1] = __USADA8((*((u32 *) &image1[off1 + 0 + i * row_size])), t1, acc[1]);
        acc[2] = __USADA8((*((u32 *) &image1[off1 + 0 + i * row_size])), s2, acc[2]);
        acc[3] = __USADA8((*((u32 *) &image1[off1 + 0 + i * row_size])), t3, acc[3]);
        acc[4] = __USADA8((*((u32 *) &image1[off1 + 0 + i * row_size])), s4, acc[4]);
        acc[5] = __USADA8((*((u32 *) &image1[off1 + 0 + i * row_size])), t5, acc[5]);
        acc[6] = __USADA8((*((u32 *) &image1[off1 + 0 + i * row_size])), s6, acc[6]);
        acc[7] = __USADA8((*((u32 *) &image1[off1 + 0 + i * row_size])), t7, acc[7]);

        /*
         * same for second column of 4 pixels:
         *
         *  + - + - + - + - + - + - + - + - +
         *  |   |   |   |   | x | x | x | x |
         *  + - + - + - + - + - + - + - + - +
         *
         */

        s0 = (__UHADD8(*((u32 *) &image2[off2 + 4 + (i + 0) * row_size]), *((u32 *) &image2[off2 + 5 + (i + 0) * row_size])));
        s1 = (__UHADD8(*((u32 *) &image2[off2 + 4 + (i + 1) * row_size]), *((u32 *) &image2[off2 + 5 + (i + 1) * row_size])));
        s2 = (__UHADD8(*((u32 *) &image2[off2 + 4 + (i + 0) * row_size]), *((u32 *) &image2[off2 + 4 + (i + 1) * row_size])));
        s3 = (__UHADD8(*((u32 *) &image2[off2 + 4 + (i + 1) * row_size]), *((u32 *) &image2[off2 + 3 + (i + 1) * row_size])));
        s4 = (__UHADD8(*((u32 *) &image2[off2 + 4 + (i + 0) * row_size]), *((u32 *) &image2[off2 + 3 + (i + 0) * row_size])));
        s5 = (__UHADD8(*((u32 *) &image2[off2 + 4 + (i - 1) * row_size]), *((u32 *) &image2[off2 + 3 + (i - 1) * row_size])));
        s6 = (__UHADD8(*((u32 *) &image2[off2 + 4 + (i + 0) * row_size]), *((u32 *) &image2[off2 + 4 + (i - 1) * row_size])));
        s7 = (__UHADD8(*((u32 *) &image2[off2 + 4 + (i - 1) * row_size]), *((u32 *) &image2[off2 + 5 + (i - 1) * row_size])));

        t1 = (__UHADD8(s0, s1));
        t3 = (__UHADD8(s3, s4));
        t5 = (__UHADD8(s4, s5));
        t7 = (__UHADD8(s7, s0));

        acc[0] = __USADA8((*((u32 *) &image1[off1 + 4 + i * row_size])), s0, acc[0]);
        acc[1] = __USADA8((*((u32 *) &image1[off1 + 4 + i * row_size])), t1, acc[1]);
        acc[2] = __USADA8((*((u32 *) &image1[off1 + 4 + i * row_size])), s2, acc[2]);
        acc[3] = __USADA8((*((u32 *) &image1[off1 + 4 + i * row_size])), t3, acc[3]);
        acc[4] = __USADA8((*((u32 *) &image1[off1 + 4 + i * row_size])), s4, acc[4]);
        acc[5] = __USADA8((*((u32 *) &image1[off1 + 4 + i * row_size])), t5, acc[5]);
        acc[6] = __USADA8((*((u32 *) &image1[off1 + 4 + i * row_size])), s6, acc[6]);
        acc[7] = __USADA8((*((u32 *) &image1[off1 + 4 + i * row_size])), t7, acc[7]);
    }

    return 0;
}
#endif


static inline u32 __USADA8_C(u8 *I1, u8 *I2, u32 A)
{
    return A + abs((int) * I1++ - *I2++) + abs((int) * I1++ - *I2++) + abs((int) * I1++ - *I2++) + abs((int) * I1++ - *I2++);
}
static inline u32 __USADA8_C2(u8 *I1, u32 I2, u32 A)
{
    return A + abs((int) * I1++ - ((I2 >> 0) & 0xff)) + abs((int) * I1++ - ((I2 >> 8) & 0xff)) + abs((int) * I1++ - ((I2 >> 16) & 0xff)) + abs((int) * I1++ - ((I2 >> 24) & 0xff));
}
static inline u32 __UHADD8_C(u8 *I1, u8 *I2)
{
    return
        (((*I1++ + *I2++ + 1) / 2) << 0) | \
        (((*I1++ + *I2++ + 1) / 2) << 8) | \
        (((*I1++ + *I2++ + 1) / 2) << 16) | \
        (((*I1++ + *I2++ + 1) / 2) << 24) ;
}
static inline u32 __UHADD8_C2(u32 I1, u32 I2)
{
    return
        (((((I1 >> 0) & 0xff) + ((I2 >> 0) & 0xff) + 1) / 2) << 0) | \
        (((((I1 >> 8) & 0xff) + ((I2 >> 8) & 0xff) + 1) / 2) << 8) | \
        (((((I1 >> 16) & 0xff) + ((I2 >> 16) & 0xff) + 1) / 2) << 16) | \
        (((((I1 >> 24) & 0xff) + ((I2 >> 24) & 0xff) + 1) / 2) << 24) ;
}

/**
 * @brief Compute SAD distances of subpixel shift of two 8x8 pixel patterns.
 *
 * @param image1 ...
 * @param image2 ...
 * @param off1X x coordinate of upper left corner of pattern in image1
 * @param off1Y y coordinate of upper left corner of pattern in image1
 * @param off2X x coordinate of upper left corner of pattern in image2
 * @param off2Y y coordinate of upper left corner of pattern in image2
 * @param acc array to store SAD distances for shift in every direction
 */
static inline u32 compute_subpixel_c(u8 *image1, u8 *image2, u16 off1X, u16 off1Y, u16 off2X, u16 off2Y, u32 *acc, u16 row_size)
{
    /* calculate position in image buffer */
    u16 off1 = off1Y * row_size + off1X; // image1
    u16 off2 = off2Y * row_size + off2X; // image2
    u16 i;
    u32 s0, s1, s2, s3, s4, s5, s6, s7, t1, t3, t5, t7;

    for (i = 0; i < 8; i++) {
        acc[i] = 0;
    }


    /*
     * calculate for each pixel in the 8x8 field with upper left corner (off1X / off1Y)
     * every iteration is one line of the 8x8 field.
     *
     *  + - + - + - + - + - + - + - + - +
     *  |   |   |   |   |   |   |   |   |
     *  + - + - + - + - + - + - + - + - +
     *
     *
     */

    for (i = 0; i < 8; i++) {
        /*
         * first column of 4 pixels:
         *
         *  + - + - + - + - + - + - + - + - +
         *  | x | x | x | x |   |   |   |   |
         *  + - + - + - + - + - + - + - + - +
         *
         * the 8 s values are from following positions for each pixel (X):
         *  + - + - + - +
         *  +   5   7   +
         *  + - + 6 + - +
         *  +   4 X 0   +
         *  + - + 2 + - +
         *  +   3   1   +
         *  + - + - + - +
         *
         *  variables (s1, ...) contains all 4 results (32bit -> 4 * 8bit values)
         *
         */

        /* compute average of two pixel values */
        s0 = (__UHADD8_C((u8 *) &image2[off2 +  0 + (i + 0) * row_size], (u8 *) &image2[off2 + 1 + (i + 0) * row_size]));
        s1 = (__UHADD8_C((u8 *) &image2[off2 +  0 + (i + 1) * row_size], (u8 *) &image2[off2 + 1 + (i + 1) * row_size]));
        s2 = (__UHADD8_C((u8 *) &image2[off2 +  0 + (i + 0) * row_size], (u8 *) &image2[off2 + 0 + (i + 1) * row_size]));
        s3 = (__UHADD8_C((u8 *) &image2[off2 +  0 + (i + 1) * row_size], (u8 *) &image2[off2 - 1 + (i + 1) * row_size]));
        s4 = (__UHADD8_C((u8 *) &image2[off2 +  0 + (i + 0) * row_size], (u8 *) &image2[off2 - 1 + (i + 0) * row_size]));
        s5 = (__UHADD8_C((u8 *) &image2[off2 +  0 + (i - 1) * row_size], (u8 *) &image2[off2 - 1 + (i - 1) * row_size]));
        s6 = (__UHADD8_C((u8 *) &image2[off2 +  0 + (i + 0) * row_size], (u8 *) &image2[off2 + 0 + (i - 1) * row_size]));
        s7 = (__UHADD8_C((u8 *) &image2[off2 +  0 + (i - 1) * row_size], (u8 *) &image2[off2 + 1 + (i - 1) * row_size]));

        /* these 4 t values are from the corners around the center pixel */
        t1 = (__UHADD8_C2(s0, s1));
        t3 = (__UHADD8_C2(s3, s4));
        t5 = (__UHADD8_C2(s4, s5));
        t7 = (__UHADD8_C2(s7, s0));

        /*
         * finally we got all 8 subpixels (s0, t1, s2, t3, s4, t5, s6, t7):
         *  + - + - + - +
         *  |   |   |   |
         *  + - 5 6 7 - +
         *  |   4 X 0   |
         *  + - 3 2 1 - +
         *  |   |   |   |
         *  + - + - + - +
         */

        /* fill accumulation vector */
        acc[0] = __USADA8_C2((u8 *) &image1[off1 + 0 + i * row_size], s0, acc[0]);
        acc[1] = __USADA8_C2((u8 *) &image1[off1 + 0 + i * row_size], t1, acc[1]);
        acc[2] = __USADA8_C2((u8 *) &image1[off1 + 0 + i * row_size], s2, acc[2]);
        acc[3] = __USADA8_C2((u8 *) &image1[off1 + 0 + i * row_size], t3, acc[3]);
        acc[4] = __USADA8_C2((u8 *) &image1[off1 + 0 + i * row_size], s4, acc[4]);
        acc[5] = __USADA8_C2((u8 *) &image1[off1 + 0 + i * row_size], t5, acc[5]);
        acc[6] = __USADA8_C2((u8 *) &image1[off1 + 0 + i * row_size], s6, acc[6]);
        acc[7] = __USADA8_C2((u8 *) &image1[off1 + 0 + i * row_size], t7, acc[7]);

        /*
         * same for second column of 4 pixels:
         *
         *  + - + - + - + - + - + - + - + - +
         *  |   |   |   |   | x | x | x | x |
         *  + - + - + - + - + - + - + - + - +
         *
         */

        s0 = (__UHADD8_C((u8 *) &image2[off2 + 4 + (i + 0) * row_size], (u8 *) &image2[off2 + 5 + (i + 0) * row_size]));
        s1 = (__UHADD8_C((u8 *) &image2[off2 + 4 + (i + 1) * row_size], (u8 *) &image2[off2 + 5 + (i + 1) * row_size]));
        s2 = (__UHADD8_C((u8 *) &image2[off2 + 4 + (i + 0) * row_size], (u8 *) &image2[off2 + 4 + (i + 1) * row_size]));
        s3 = (__UHADD8_C((u8 *) &image2[off2 + 4 + (i + 1) * row_size], (u8 *) &image2[off2 + 3 + (i + 1) * row_size]));
        s4 = (__UHADD8_C((u8 *) &image2[off2 + 4 + (i + 0) * row_size], (u8 *) &image2[off2 + 3 + (i + 0) * row_size]));
        s5 = (__UHADD8_C((u8 *) &image2[off2 + 4 + (i - 1) * row_size], (u8 *) &image2[off2 + 3 + (i - 1) * row_size]));
        s6 = (__UHADD8_C((u8 *) &image2[off2 + 4 + (i + 0) * row_size], (u8 *) &image2[off2 + 4 + (i - 1) * row_size]));
        s7 = (__UHADD8_C((u8 *) &image2[off2 + 4 + (i - 1) * row_size], (u8 *) &image2[off2 + 5 + (i - 1) * row_size]));

        t1 = (__UHADD8_C2(s0, s1));
        t3 = (__UHADD8_C2(s3, s4));
        t5 = (__UHADD8_C2(s4, s5));
        t7 = (__UHADD8_C2(s7, s0));

        acc[0] = __USADA8_C2((u8 *) &image1[off1 + 4 + i * row_size], s0, acc[0]);
        acc[1] = __USADA8_C2((u8 *) &image1[off1 + 4 + i * row_size], t1, acc[1]);
        acc[2] = __USADA8_C2((u8 *) &image1[off1 + 4 + i * row_size], s2, acc[2]);
        acc[3] = __USADA8_C2((u8 *) &image1[off1 + 4 + i * row_size], t3, acc[3]);
        acc[4] = __USADA8_C2((u8 *) &image1[off1 + 4 + i * row_size], s4, acc[4]);
        acc[5] = __USADA8_C2((u8 *) &image1[off1 + 4 + i * row_size], t5, acc[5]);
        acc[6] = __USADA8_C2((u8 *) &image1[off1 + 4 + i * row_size], s6, acc[6]);
        acc[7] = __USADA8_C2((u8 *) &image1[off1 + 4 + i * row_size], t7, acc[7]);
    }

    return 0;
}


static void avg8x8_c(u8 *block, const u8 *s0, const u8 *s1,
                     int dest_size, int line_size, int h)
{
    int i, j;
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            block[i * dest_size + j] = (s0[i * line_size + j] + s1[i * line_size + j] + 1) / 2;
        }
    }

}

static u32 sad8x8_c(const u8 *s0, const u8 *s1,
                    int dest_size, int line_size, int h)
{
    int i, j;
    u32 acc = 0;
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            acc += abs((int)s0[i * dest_size + j] - s1[i * line_size + j]);
        }
    }
    return acc;

}
#if USE_BFIN
static inline u32 compute_subpixel_bfin(u8 *image1, u8 *image2, u16 off1X, u16 off1Y, u16 off2X, u16 off2Y, u32 *acc, u16 row_size, u8 *b8x8_0, u8 *b8x8_1, u32 b8x8_stride)
{
    /* calculate position in image buffer */
    u16 off1 = off1Y * row_size + off1X; // image1
    u16 off2 = off2Y * row_size + off2X; // image2
    u16 i;

    for (i = 0; i < 8; i++) {
        acc[i] = 0;
    }

    /*
     *  + - + - + - +
     *  +   F   H   +
     *  + - + G + - +
     *  +   E X A   +
     *  + - + C + - +
     *  +   D   B   +
     *  + - + - + - +

     *  + - + - + - +
     *  |   |   |   |
     *  + - 5 6 7 - +
     *  |   4 X 0   |
     *  + - 3 2 1 - +
     *  |   |   |   |
     *  + - + - + - +
     */
    i = 0 ;
    /*A*/    avg8x8(b8x8_0, (u8 *) &image2[off2 +  0 + (i + 0) * row_size], (u8 *) &image2[off2 + 1 + (i + 0) * row_size], b8x8_stride, row_size, 8);
    /*B*/    avg8x8(b8x8_1, (u8 *) &image2[off2 +  0 + (i + 1) * row_size], (u8 *) &image2[off2 + 1 + (i + 1) * row_size], b8x8_stride, row_size, 8);
    acc[0] = sad8x8((u8 *) &image1[off1 +  0 + (i + 0) * row_size], b8x8_0, row_size, b8x8_stride, 8);
    avg8x8(b8x8_1, b8x8_0, b8x8_1, b8x8_stride, b8x8_stride, 8);
    acc[1] = sad8x8((u8 *) &image1[off1 +  0 + (i + 0) * row_size], b8x8_1, row_size, b8x8_stride, 8);
    /*H*/    avg8x8(b8x8_1, (u8 *) &image2[off2 +  0 + (i - 1) * row_size], (u8 *) &image2[off2 + 1 + (i - 1) * row_size], b8x8_stride, row_size, 8);
    avg8x8(b8x8_1, b8x8_0, b8x8_1, b8x8_stride, b8x8_stride, 8);
    acc[7] = sad8x8((u8 *) &image1[off1 +  0 + (i + 0) * row_size], b8x8_1, row_size, b8x8_stride, 8);
    /*C*/    avg8x8(b8x8_0, (u8 *) &image2[off2 +  0 + (i + 0) * row_size], (u8 *) &image2[off2 + 0 + (i + 1) * row_size], b8x8_stride, row_size, 8);
    acc[2] = sad8x8((u8 *) &image1[off1 +  0 + (i + 0) * row_size], b8x8_0, row_size, b8x8_stride, 8);
    /*G*/    avg8x8(b8x8_0, (u8 *) &image2[off2 +  0 + (i + 0) * row_size], (u8 *) &image2[off2 + 0 + (i - 1) * row_size], b8x8_stride, row_size, 8);
    acc[6] = sad8x8((u8 *) &image1[off1 +  0 + (i + 0) * row_size], b8x8_0, row_size, b8x8_stride, 8);
    /*E*/    avg8x8(b8x8_0, (u8 *) &image2[off2 +  0 + (i + 0) * row_size], (u8 *) &image2[off2 - 1 + (i + 0) * row_size], b8x8_stride, row_size, 8);
    /*D*/    avg8x8(b8x8_1, (u8 *) &image2[off2 +  0 + (i + 1) * row_size], (u8 *) &image2[off2 - 1 + (i + 1) * row_size], b8x8_stride, row_size, 8);
    acc[4] = sad8x8((u8 *) &image1[off1 +  0 + (i + 0) * row_size], b8x8_0, row_size, b8x8_stride, 8);
    avg8x8(b8x8_1, b8x8_0, b8x8_1, b8x8_stride, b8x8_stride, 8);
    acc[3] = sad8x8((u8 *) &image1[off1 +  0 + (i + 0) * row_size], b8x8_1, row_size, b8x8_stride, 8);
    /*F*/    avg8x8(b8x8_1, (u8 *) &image2[off2 +  0 + (i - 1) * row_size], (u8 *) &image2[off2 - 1 + (i - 1) * row_size], b8x8_stride, row_size, 8);
    avg8x8(b8x8_1, b8x8_0, b8x8_1, b8x8_stride, b8x8_stride, 8);
    acc[5] = sad8x8((u8 *) &image1[off1 +  0 + (i + 0) * row_size], b8x8_1, row_size, b8x8_stride, 8);


    return 0;
}

#endif

/**
 * @brief Compute SAD of two 8x8 pixel windows.
 *
 * @param image1 ...
 * @param image2 ...
 * @param off1X x coordinate of upper left corner of pattern in image1
 * @param off1Y y coordinate of upper left corner of pattern in image1
 * @param off2X x coordinate of upper left corner of pattern in image2
 * @param off2Y y coordinate of upper left corner of pattern in image2
 */
static inline u32 compute_sad_8x8(u8 *image1, u8 *image2, u16 off1X, u16 off1Y, u16 off2X, u16 off2Y, u16 row_size)
{
    /* calculate position in image buffer */
    u16 off1 = off1Y * row_size + off1X; // image1
    u16 off2 = off2Y * row_size + off2X; // image2

    u32 acc = 0;
#if USE_SIMD

    acc = __USAD8(*((u32 *) &image1[off1 + 0 + 0 * row_size]), *((u32 *) &image2[off2 + 0 + 0 * row_size]));
    acc = __USADA8(*((u32 *) &image1[off1 + 4 + 0 * row_size]), *((u32 *) &image2[off2 + 4 + 0 * row_size]), acc);

    acc = __USADA8(*((u32 *) &image1[off1 + 0 + 1 * row_size]), *((u32 *) &image2[off2 + 0 + 1 * row_size]), acc);
    acc = __USADA8(*((u32 *) &image1[off1 + 4 + 1 * row_size]), *((u32 *) &image2[off2 + 4 + 1 * row_size]), acc);

    acc = __USADA8(*((u32 *) &image1[off1 + 0 + 2 * row_size]), *((u32 *) &image2[off2 + 0 + 2 * row_size]), acc);
    acc = __USADA8(*((u32 *) &image1[off1 + 4 + 2 * row_size]), *((u32 *) &image2[off2 + 4 + 2 * row_size]), acc);

    acc = __USADA8(*((u32 *) &image1[off1 + 0 + 3 * row_size]), *((u32 *) &image2[off2 + 0 + 3 * row_size]), acc);
    acc = __USADA8(*((u32 *) &image1[off1 + 4 + 3 * row_size]), *((u32 *) &image2[off2 + 4 + 3 * row_size]), acc);

    acc = __USADA8(*((u32 *) &image1[off1 + 0 + 4 * row_size]), *((u32 *) &image2[off2 + 0 + 4 * row_size]), acc);
    acc = __USADA8(*((u32 *) &image1[off1 + 4 + 4 * row_size]), *((u32 *) &image2[off2 + 4 + 4 * row_size]), acc);

    acc = __USADA8(*((u32 *) &image1[off1 + 0 + 5 * row_size]), *((u32 *) &image2[off2 + 0 + 5 * row_size]), acc);
    acc = __USADA8(*((u32 *) &image1[off1 + 4 + 5 * row_size]), *((u32 *) &image2[off2 + 4 + 5 * row_size]), acc);

    acc = __USADA8(*((u32 *) &image1[off1 + 0 + 6 * row_size]), *((u32 *) &image2[off2 + 0 + 6 * row_size]), acc);
    acc = __USADA8(*((u32 *) &image1[off1 + 4 + 6 * row_size]), *((u32 *) &image2[off2 + 4 + 6 * row_size]), acc);

    acc = __USADA8(*((u32 *) &image1[off1 + 0 + 7 * row_size]), *((u32 *) &image2[off2 + 0 + 7 * row_size]), acc);
    acc = __USADA8(*((u32 *) &image1[off1 + 4 + 7 * row_size]), *((u32 *) &image2[off2 + 4 + 7 * row_size]), acc);

#else
#if USE_BFIN
    acc = sad8x8(&image1[off1], &image2[off2], row_size, row_size, 8);
#else
    int i, j;
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            acc += abs(image1[off1 + j + i * row_size] - image2[off2 + j + i * row_size]);
        }
    }
#endif

#endif
    return acc;
}


static int less_feature_ctor = 0;
static int less_fullpixel_ctor = 0;

/**
 * @brief Computes pixel flow from image1 to image2
 *
 * Searches the corresponding position in the new image (image2) from the old image (image1)
 * and calculates the average offset of all.
 *
 * @param image1 previous image buffer
 * @param image2 current image buffer (new)

 *
 * @return quality of flow calculation
 */
u8 pixel_compute_flow(u8 *image2, u32 image_width, s32 *pixel_flow_x, s32 *pixel_flow_y)
{

    /* constants */
    const s16 winmin = -SEARCH_SIZE;
    const s16 winmax = SEARCH_SIZE;
    const u16 hist_size = 2 * (winmax - winmin + 1) + 1;

    /* variables */
    u16 pixLo = SEARCH_SIZE + 1 + 8;
    u16 pixHi = image_width - (SEARCH_SIZE + 1 + 8);
    u16 pixStep = (pixHi - pixLo) / (NUM_BLOCKS - 1);
    u16 i, j;
    u8 qual;
    u32 acc[8]; // subpixels
    u16 histx[hist_size]; // counter for x shift
    u16 histy[hist_size]; // counter for y shift
    s8  *dirsx; // shift directions in x
    s8  *dirsy; // shift directions in y
    u8  *subdirs; // shift directions of best subpixels
    u16 meancount = 0;
    u8 b8x16[8 * 16];
    u32 b8x16_stride = 16;


    //float meanflowx = 0.0f;
    //float meanflowy = 0.0f;
    s32 histflowx = 0;
    s32 histflowy = 0;

    static u8 *image1;
    if (!image1) {
        image1 = malloc(image_width * image_width + image_width * 3);
        if (!image1) {
            *pixel_flow_x = 0;
            *pixel_flow_y = 0;
            return 0;
        }

        memcpy(image1, image2, image_width * image_width);
    }
    dirsx = image1 + image_width * image_width;
    dirsy = dirsx + image_width;
    subdirs = dirsy + image_width;
    memset(dirsx, 0, image_width * 3);

    int xxx = 0;

    /* initialize with 0 */
    for (j = 0; j < hist_size; j++) {
        histx[j] = 0;
        histy[j] = 0;
    }

    /* iterate over all patterns
     */
    for (j = pixLo; j < pixHi; j += pixStep) {
        for (i = pixLo; i < pixHi; i += pixStep) {
            xxx++;
            /* test pixel if it is suitable for flow tracking */
            u32 diff = compute_diff(image1, i, j, image_width);
            if (diff < PARA_FEATURE_THRESHOLD) {
                FLOW_DEBUG("feature too small %d\n", less_feature_ctor++);
                continue;
            }

            u32 dist = 0xFFFFFFFF; // set initial distance to "infinity"
            s8 sumx = 0;
            s8 sumy = 0;
            s8 ii, jj;

            u8 *base1 = image1 + j * image_width + i;

            for (jj = winmin; jj <= winmax; jj++) {
                u8 *base2 = image2 + (j + jj) * image_width + i;

                for (ii = winmin; ii <= winmax; ii++) {
                    u32 temp_dist = compute_sad_8x8(image1, image2, i, j, i + ii, j + jj, image_width);
//					u32 temp_dist = ABSDIFF(base1, base2 + ii);
                    if (temp_dist < dist) {
                        sumx = ii;
                        sumy = jj;
                        dist = temp_dist;
                    }
                }
            }
            FLOW_DEBUG("%s %d %d %d %d %d\n", __FUNCTION__, __LINE__, sumx, sumy, dist, diff);

            /* acceptance SAD distance threshhold */
            if (dist < PARA_FULLPIXEL_THRESHOLD) {
                //meanflowx += (float) sumx;
                //meanflowy += (float) sumy;
#if USE_SIMD
                compute_subpixel(image1, image2, i, j, i + sumx, j + sumy, acc, image_width);
#else
#if USE_BFIN
                compute_subpixel_bfin(image1, image2, i, j, i + sumx, j + sumy, acc, image_width, b8x16, b8x16 + 8, b8x16_stride);
#else
                compute_subpixel_c(image1, image2, i, j, i + sumx, j + sumy, acc, image_width);
#endif
#endif
//printf("acc = 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x \r\n", acc[0], acc[1], acc[2], acc[3], acc[4], acc[5], acc[6], acc[7]);

                u32 mindist = dist; // best SAD until now
                u8 mindir = 8; // direction 8 for no direction
                u8 k;

                for (k = 0; k < 8; k++) {
                    //printf("%d ",acc[k]);
                    if (acc[k] < mindist) {
                        // SAD becomes better in direction k
                        mindist = acc[k];
                        mindir = k;
                    }
                }
                //printf("\n\n");

                dirsx[meancount] = sumx;
                dirsy[meancount] = sumy;
                subdirs[meancount] = mindir;

                // feed histogram filter
                u8 hist_index_x = 2 * sumx + (winmax - winmin + 1);
                if (subdirs[meancount] == 0 || subdirs[meancount] == 1 || subdirs[meancount] == 7) {
                    hist_index_x += 1;
                }
                if (subdirs[meancount] == 3 || subdirs[meancount] == 4 || subdirs[meancount] == 5) {
                    hist_index_x += -1;
                }
                u8 hist_index_y = 2 * sumy + (winmax - winmin + 1);
                if (subdirs[meancount] == 5 || subdirs[meancount] == 6 || subdirs[meancount] == 7) {
                    hist_index_y += 1;
                }
                if (subdirs[meancount] == 1 || subdirs[meancount] == 2 || subdirs[meancount] == 3) {
                    hist_index_y += -1;
                }

                meancount++;
                /*
                meancount++;

                // feed histogram filter
                u8 hist_index_x = 2 * sumx + (winmax - winmin + 1);
                if (subdirs[i] == 0 || subdirs[i] == 1 || subdirs[i] == 7) {
                    hist_index_x += 1;
                }
                if (subdirs[i] == 3 || subdirs[i] == 4 || subdirs[i] == 5) {
                    hist_index_x += -1;
                }
                u8 hist_index_y = 2 * sumy + (winmax - winmin + 1);
                if (subdirs[i] == 5 || subdirs[i] == 6 || subdirs[i] == 7) {
                    hist_index_y += 1;
                }
                if (subdirs[i] == 1 || subdirs[i] == 2 || subdirs[i] == 3) {
                    hist_index_y += -1;
                }
                */
                histx[hist_index_x]++;
                histy[hist_index_y]++;

            }
        }
    }

    /* create flow image if needed (image1 is not needed anymore)
     * -> can be used for debugging purpose
     */
//	if (global_data.param[PARAM_USB_SEND_VIDEO] )//&& global_data.param[PARAM_VIDEO_USB_MODE] == FLOW_VIDEO)
//	{
//
//		for (j = pixLo; j < pixHi; j += pixStep)
//		{
//			for (i = pixLo; i < pixHi; i += pixStep)
//			{
//
//				u32 diff = compute_diff(image1, i, j, (u16) global_data.param[PARAM_IMAGE_WIDTH]);
//				if (diff > global_data.param[PARAM_BOTTOM_FLOW_FEATURE_THRESHOLD])
//				{
//					image1[j * ((u16) global_data.param[PARAM_IMAGE_WIDTH]) + i] = 255;
//				}
//
//			}
//		}
//	}

    FLOW_DEBUG("meancount %d\n", meancount);

    /* evaluate flow calculation */
    if (meancount > 10) {
        //meanflowx /= meancount;
        //meanflowy /= meancount;

        s16 maxpositionx = 0;
        s16 maxpositiony = 0;
        u16 maxvaluex = 0;
        u16 maxvaluey = 0;

        /* position of maximal histogram peek */
        for (j = 0; j < hist_size; j++) {
            if (histx[j] > maxvaluex) {
                maxvaluex = histx[j];
                maxpositionx = j;
            }
            if (histy[j] > maxvaluey) {
                maxvaluey = histy[j];
                maxpositiony = j;
            }
        }

        /* check if there is a peak value in histogram */
        if (1) { //(histx[maxpositionx] > meancount / 6 && histy[maxpositiony] > meancount / 6)
            if (USE_HIST_FILTER) {

                /* use histogram filter peek value */
                u16 hist_x_min = maxpositionx;
                u16 hist_x_max = maxpositionx;
                u16 hist_y_min = maxpositiony;
                u16 hist_y_max = maxpositiony;

                /* x direction */
                if (maxpositionx > 1 && maxpositionx < hist_size - 2) {
                    hist_x_min = maxpositionx - 2;
                    hist_x_max = maxpositionx + 2;
                } else if (maxpositionx == 0) {
                    hist_x_min = maxpositionx;
                    hist_x_max = maxpositionx + 2;
                } else  if (maxpositionx == hist_size - 1) {
                    hist_x_min = maxpositionx - 2;
                    hist_x_max = maxpositionx;
                } else if (maxpositionx == 1) {
                    hist_x_min = maxpositionx - 1;
                    hist_x_max = maxpositionx + 2;
                } else  if (maxpositionx == hist_size - 2) {
                    hist_x_min = maxpositionx - 2;
                    hist_x_max = maxpositionx + 1;
                }

                /* y direction */
                if (maxpositiony > 1 && maxpositiony < hist_size - 2) {
                    hist_y_min = maxpositiony - 2;
                    hist_y_max = maxpositiony + 2;
                } else if (maxpositiony == 0) {
                    hist_y_min = maxpositiony;
                    hist_y_max = maxpositiony + 2;
                } else if (maxpositiony == hist_size - 1) {
                    hist_y_min = maxpositiony - 2;
                    hist_y_max = maxpositiony;
                } else if (maxpositiony == 1) {
                    hist_y_min = maxpositiony - 1;
                    hist_y_max = maxpositiony + 2;
                } else if (maxpositiony == hist_size - 2) {
                    hist_y_min = maxpositiony - 2;
                    hist_y_max = maxpositiony + 1;
                }

                s32 hist_x_value = 0;
                s32 hist_x_weight = 0;

                s32 hist_y_value = 0;
                s32 hist_y_weight = 0;
                u8 i;
                for (i = hist_x_min; i < hist_x_max + 1; i++) {
                    hist_x_value += (i * histx[i]);
                    hist_x_weight +=  histx[i];
                }

                for (i = hist_y_min; i < hist_y_max + 1; i++) {
                    hist_y_value += (i * histy[i]);
                    hist_y_weight +=  histy[i];
                }

                histflowx = (hist_x_value * FIXED_PRECISE / hist_x_weight - (winmax - winmin + 1) * FIXED_PRECISE) / 2;
                histflowy = (hist_y_value * FIXED_PRECISE / hist_y_weight - (winmax - winmin + 1) * FIXED_PRECISE) / 2;

            } else {

                /* use average of accepted flow values */
                s32 meancount_x = 0;
                s32 meancount_y = 0;
                u8 i;
                for (i = 0; i < meancount; i++) {
                    int subdirx = 0;
                    if (subdirs[i] == 0 || subdirs[i] == 1 || subdirs[i] == 7) {
                        subdirx = FIXED_PRECISE / 2;
                    }

                    if (subdirs[i] == 3 || subdirs[i] == 4 || subdirs[i] == 5) {
                        subdirx = - FIXED_PRECISE / 2;
                    }
                    histflowx += dirsx[i] * FIXED_PRECISE + subdirx;
                    meancount_x++;

                    int subdiry = 0;
                    if (subdirs[i] == 5 || subdirs[i] == 6 || subdirs[i] == 7) {
                        subdiry =  FIXED_PRECISE / 2;
                    }
                    if (subdirs[i] == 1 || subdirs[i] == 2 || subdirs[i] == 3) {
                        subdiry = - FIXED_PRECISE / 2;
                    }
                    histflowy += dirsy[i] * FIXED_PRECISE + subdiry;
                    meancount_y++;
                }

                histflowx /= meancount_x;
                histflowy /= meancount_y;
            }

            /* write results */
            *pixel_flow_x = histflowx;
            *pixel_flow_y = histflowy;
        } else {
            *pixel_flow_x = 0;
            *pixel_flow_y = 0;
            qual = 0;
            goto exit;
        }
    } else {
        *pixel_flow_x = 0;
        *pixel_flow_y = 0;
        qual = 0;
        goto exit;
    }

    /* calc quality */
    qual = (u8)(meancount * 255 / (NUM_BLOCKS * NUM_BLOCKS));

exit:
    memcpy(image1, image2, image_width * image_width);
    return qual;

}


#if 0
// ---------------------------- TEST ROUNTINE -----------------

#include <sys/time.h>
#include <unistd.h>

static int fps_test()
{
#define REPEAT_TIME 100
    static u32 pre_time = 0;
    static u32 frame_int_ctor = 1;
    if (((frame_int_ctor++) % REPEAT_TIME) == 0) {
        u32 cur_time = OSGetTime();
        printf("-------------------------------- %d frames FPS=(%d/10)\n", REPEAT_TIME, REPEAT_TIME * 1000 / (cur_time - pre_time));
        pre_time = cur_time;
        return 0;
    } else {
        return 1;
    }
}


void test_jflow()
{
    u8 *inputdata0 = (u8 *)malloc(BOTTOM_FLOW_IMAGE_WIDTH * BOTTOM_FLOW_IMAGE_WIDTH * 2);
    u8 *inputdata1 = inputdata0 + BOTTOM_FLOW_IMAGE_WIDTH * BOTTOM_FLOW_IMAGE_WIDTH;
    int i, j, m, n;
    s32 xFlow, yFlow, quality;


#if 0 //test bfin disaligned loaded
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            inputdata0[i * 16 + j] = 0;
            printf("%d ", inputdata0[i * 16 + j]);
        }
        inputdata0[i * 16 + 8] = 1;
        printf("%d ", inputdata0[i * 16 + 8]);
        printf("\n");
    }
    printf("\n");



    printf("sad 8x8 = %d\n", sad8x8(inputdata0, inputdata0 + 1, 16, 16, 8));
    return;
#endif


    for (i = 0; i < BOTTOM_FLOW_IMAGE_WIDTH * BOTTOM_FLOW_IMAGE_WIDTH; i++) {
        inputdata0[i] = i & 0xff;
    }

    do {
        for (m = -2; m < 2; m++) {
            for (n = -2; n < 2; n++) {
                memset(inputdata1, 0, BOTTOM_FLOW_IMAGE_WIDTH * BOTTOM_FLOW_IMAGE_WIDTH);
                for (i = 3; i < BOTTOM_FLOW_IMAGE_WIDTH - 3; i++) {
                    for (j = 3; j < BOTTOM_FLOW_IMAGE_WIDTH - 3; j++) {
                        inputdata1[i * BOTTOM_FLOW_IMAGE_WIDTH + j] = (inputdata0[(i + m) * BOTTOM_FLOW_IMAGE_WIDTH + j + n] + inputdata0[(i + m) * BOTTOM_FLOW_IMAGE_WIDTH + j + n + 1] + \
                                inputdata0[(i + m + 1) * BOTTOM_FLOW_IMAGE_WIDTH + j + n] + inputdata0[(i + m + 1) * BOTTOM_FLOW_IMAGE_WIDTH + j + n + 1]) / 4;
                        //inputdata1[i*BOTTOM_FLOW_IMAGE_WIDTH+j] = (inputdata0[(i+m)*BOTTOM_FLOW_IMAGE_WIDTH+j+n] + inputdata0[(i+m)*BOTTOM_FLOW_IMAGE_WIDTH+j+n+1])/2;
                    }
                }
                quality = pixel_compute_flow(inputdata0, inputdata1, &xFlow, &yFlow);
                printf(" ==== compute flow output : %d,%d,%d\n", quality, xFlow, yFlow);
            }
        }

    } while (0);


    //test fps
    while (0) {
        quality = pixel_compute_flow(inputdata0, inputdata1, &xFlow, &yFlow);
        fps_test();
    }


    free(inputdata0);
}
// ---------------------------- TEST ROUNTINE -----------------
#endif

