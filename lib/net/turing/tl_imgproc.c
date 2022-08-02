#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include "printf.h"

typedef struct _IplImageTuling {
    int nSize;             /**< sizeof(IplImage) */
    int nChannels;         /**< Most of OpenCV functions support 1,2,3 or 4 channels */
    int depth;             /**< Pixel depth in bits: IPL_DEPTH_8U, IPL_DEPTH_8S, IPL_DEPTH_16S,
                               IPL_DEPTH_32S, IPL_DEPTH_32F and IPL_DEPTH_64F are supported.  */
    int width;             /**< Image width in pixels.                           */
    int height;            /**< Image height in pixels.                          */
    int imageSize;         /**< Image data size in bytes
                               (==image->height*image->widthStep
                               in case of interleaved data)*/
    unsigned char *imageData;        /**< Pointer to aligned image data.         */
    int widthStep;         /**< Size of aligned image row in bytes.    */
} IplImageTL;

enum {
    TL_CV_GRAY = 0,
    TL_CV_RGB = 1,
    TL_CV_BGR = 2,
    //420
    TL_CV_YUV_NV21 = 10,
    TL_CV_YUV_NV12 = 11,
    TL_CV_YUV_YV12 = 12,
    TL_CV_YUV_IYUV = 13,
    TL_CV_YUV_I420 = 14,
    // 422
    TL_CV_YUV_UYVY = 20,
    TL_CV_YUV_YUY2 = 21,
    TL_CV_YUV_Y422 = 22,
    TL_CV_YUV_UYNV = 23,
    TL_CV_YUV_YVYU = 24,
    TL_CV_YUV_YUYV = 25,
    TL_CV_YUV_YUNV = 26
};

typedef enum _TL_IMAGE_TYPE_ {
    TL_CV_GRAY2GRAY = 0,
//    TL_CV_BGR2RGB = 4,
    TL_CV_BGR2GRAY = 6,
    TL_CV_RGB2GRAY = 7,
//    TL_CV_RGB2BGR = TL_CV_BGR2RGB,
    TL_CV_YUV2GRAY_420 = 106,
    TL_CV_YUV2GRAY_NV21 = TL_CV_YUV2GRAY_420,
    TL_CV_YUV2GRAY_NV12 = TL_CV_YUV2GRAY_420,
    TL_CV_YUV2GRAY_YV12 = TL_CV_YUV2GRAY_420,
    TL_CV_YUV2GRAY_IYUV = TL_CV_YUV2GRAY_420,
    TL_CV_YUV2GRAY_I420 = TL_CV_YUV2GRAY_420,
    TL_CV_YUV2GRAY_Y422P = TL_CV_YUV2GRAY_I420,

    TL_CV_YUV2GRAY_UYVY = 123,
    TL_CV_YUV2GRAY_YUY2 = 124,
    TL_CV_YUV2GRAY_Y422 = TL_CV_YUV2GRAY_UYVY,
    TL_CV_YUV2GRAY_UYNV = TL_CV_YUV2GRAY_UYVY,
    TL_CV_YUV2GRAY_YVYU = TL_CV_YUV2GRAY_YUY2,
    TL_CV_YUV2GRAY_YUYV = TL_CV_YUV2GRAY_YUY2,
    TL_CV_YUV2GRAY_YUNV = TL_CV_YUV2GRAY_YUY2

} TL_IMAGE_TYPE;

enum {
    TL_CV_INTER_NN = 0,
    TL_CV_INTER_LINEAR = 1,
    TL_CV_INTER_CUBIC = 2,
    TL_CV_INTER_AREA = 3,
    TL_CV_INTER_LANCZOS4 = 4
};

/** Threshold types */
enum {
    TL_CV_THRESH_BINARY = 0,  /**< value = value > threshold ? max_value : 0       */
    TL_CV_THRESH_BINARY_INV = 1,  /**< value = value > threshold ? 0 : max_value       */
    TL_CV_THRESH_TRUNC = 2,  /**< value = value > threshold ? threshold : value   */
    TL_CV_THRESH_TOZERO = 3,  /**< value = value > threshold ? value : 0           */
    TL_CV_THRESH_TOZERO_INV = 4,  /**< value = value > threshold ? 0 : value           */
    TL_CV_THRESH_MASK = 7,
    TL_CV_THRESH_OTSU = 8, /**< use Otsu algorithm to choose the optimal threshold value;
                                 combine the flag with one of the above CV_THRESH_* values */
    TL_CV_THRESH_TRIANGLE = 16  /**< use Triangle algorithm to choose the optimal threshold value;
                                 combine the flag with one of the above CV_THRESH_* values, but not
                                 with CV_THRESH_OTSU */
};


static int cvFloorTL(double value)
{
    int i = (int) value;
    return i - (i > value);
}

static short saturate_cast_short(float v)
{
    return (short)((unsigned long int)((long long int) v - SHRT_MIN) <= (unsigned long int) USHRT_MAX ? v : v > 0
                   ? SHRT_MAX
                   : SHRT_MIN);
}

// cvCopy
static void cvCopyImageTL(struct _IplImageTuling *src, struct _IplImageTuling *dst)
{
    int i = 0, j = 0;
    unsigned char *srcPixel;
    unsigned char *dstPixel;
    if (src->widthStep != dst->widthStep || src->height != dst->height || src->nChannels != dst->nChannels) {
        printf("error copyImage shape is different\n");
        return;
    }
    for (i = 0; i < src->height; i++) {
        srcPixel = src->imageData + i * src->widthStep;
        dstPixel = dst->imageData + i * dst->widthStep;
        for (j = 0; j < src->widthStep; j++) {
            dstPixel[j] = srcPixel[j];
        }
    }
}

static unsigned char rgbToGray(const unsigned char R, const unsigned char G, const unsigned char B)
{
    unsigned char re;
    re = (unsigned char)(((int) R * 19595 + (int) G * 38469 + (int) B * 7472) >> 16);
    return re;
}

static void cvBgrToGray(const unsigned char *srcData, unsigned char *dstData, int srcWidthStep, int srcHeight, int srcChannels, int dstWidthStep)
{
    int i, j, k;
    unsigned char *srcPixel;
    unsigned char *dstPixel;
    for (i = 0; i < srcHeight; i++) {
        srcPixel = (unsigned char *)(srcData + i * srcWidthStep);
        dstPixel = (dstData + i * dstWidthStep);
        for (j = 0, k = 0; j < srcWidthStep; j += srcChannels, k++) {
//            dstPixel[k] = rgbToGray(srcPixel[j + 2], srcPixel[j + 1], srcPixel[j]);
            dstPixel[k] = (unsigned char)(
                              ((int) srcPixel[j + 2] * 19595 + (int) srcPixel[j + 1] * 38469 + (int) srcPixel[j] * 7472) >> 16);
        }
    }
}

static void cvRgbToGray(const unsigned char *srcData, unsigned char *dstData, int srcWidthStep, int srcHeight, int srcChannels, int dstWidthStep)
{
    int i, j, k;
    unsigned char *srcPixel;
    unsigned char *dstPixel;
    for (i = 0; i < srcHeight; i++) {
        srcPixel = (unsigned char *)(srcData + i * srcWidthStep);
        dstPixel = (dstData + i * dstWidthStep);
        for (j = 0, k = 0; j < srcWidthStep; j += srcChannels, k++) {
//            dstPixel[k] = rgbToGray(srcPixel[j], srcPixel[j + 1], srcPixel[j + 2]);
            dstPixel[k] = (unsigned char)(
                              ((int) srcPixel[j] * 19595 + (int) srcPixel[j + 1] * 38469 + (int) srcPixel[j + 2] * 7472) >> 16);
        }
    }
}

static void cvGrayToGray(const unsigned char *srcData, unsigned char *dstData, int srcWidthStep, int srcHeight)
{
    memcpy(dstData, srcData, srcWidthStep * srcHeight);
}

static void cvYuvToGray_420(const unsigned char *srcData, unsigned char *dstData, int srcWidth, int srcHeight)
{
    memcpy(dstData, srcData, srcWidth * srcHeight);
}

static void cvYuvToGray_UYVY(const unsigned char *srcData, unsigned char *dstData, int srcHeight, int srcWidth, int dstWidthStep)
{
    int i, j, k, y;
    unsigned char *srcPixel;
    unsigned char *dstPixel;
    y = srcWidth * 2;
    for (i = 0; i < srcHeight; i++) {
        srcPixel = (unsigned char *)(srcData + i * srcWidth * 2);
        dstPixel = (dstData + i * dstWidthStep);
        for (j = 0, k = 0; j < y; j += 2, k++) {
            dstPixel[k] = srcPixel[j + 1];
        }
    }
}

static void cvYuvToGray_YUYV(const unsigned char *srcData, unsigned char *dstData, int srcHeight, int srcWidth, int dstWidthStep)
{
    int i, j, k, y;
    unsigned char *srcPixel;
    unsigned char *dstPixel;
    y = srcWidth * 2;
    for (i = 0; i < srcHeight; i++) {
        srcPixel = (unsigned char *)(srcData + i * srcWidth * 2);
        dstPixel = (dstData + i * dstWidthStep);
        for (j = 0, k = 0; j < y; j += 2, k++) {
            dstPixel[k] = srcPixel[j];
        }
    }
}

static void cvRsizeNear(struct _IplImageTuling *src, struct _IplImageTuling *dst)
{
    int i, j, k, stepSrc, stepDst;
    int sx, sy;
    unsigned char *dataDst;
    unsigned char *dataSrc;
    int n = src->width / dst->width;

    stepSrc = src->widthStep;
    stepDst = dst->widthStep;
    dataSrc = (unsigned char *) src->imageData;
    dataDst = (unsigned char *) dst->imageData;

    for (i = 0; i < dst->height; i++) {
//        sx = cvFloorTL(i * n);
        sx = i * n;
        sx = sx > (src->height - 1) ? (src->height - 1) : sx;

        for (j = 0; j < dst->width; j++) {
//            sy = cvFloorTL(j * n);
            sy = j * n;
            sy = sy > (src->width - 1) ? (src->width - 1) : sy;
            for (k = 0; k < src->nChannels; ++k) {
                *(dataDst + i * stepDst + src->nChannels * j + k) = (*(dataSrc + sx * stepSrc + sy * src->nChannels + k));
            }
        }
    }
}

static void cvGrayRsizeNear(const unsigned char *dataSrc, int srcWidth, int srcHeight, struct _IplImageTuling *dst)
{
    int i, j, k, stepDst, stepSrc = srcWidth, nChannels = 1;
    int sx, sy;
    unsigned char *dataDst;
    int n = srcWidth / dst->width;
    stepDst = dst->widthStep;
    dataDst = (unsigned char *) dst->imageData;

    for (i = 0; i < dst->height; i++) {
        sx = i * n;
        sx = sx > (srcHeight - 1) ? (srcHeight - 1) : sx;

        for (j = 0; j < dst->width; j++) {
            sy = j * n;
            sy = sy > (srcWidth - 1) ? (srcWidth - 1) : sy;
            for (k = 0; k < nChannels; ++k) {
                *(dataDst + i * stepDst + nChannels * j + k) = (*(dataSrc + sx * stepSrc + sy * nChannels + k));
            }
        }
    }
}

static void cvHalfRGB(unsigned char *dataSrc, unsigned char *dataDst, int srcHeight, int srcWidth, int srcChannels)
{
    //support RGB,GBR,GRAY format
    int i, j, k, sx, sy, stepSrc = srcChannels * srcWidth, stepDst = srcChannels * srcWidth / 2;
    for (i = 0; i < srcHeight / 2; i++) {
        sx = i * 2;
        sx = sx > (srcHeight - 1) ? (srcHeight - 1) : sx;

        for (j = 0; j < srcWidth / 2; j++) {
            sy = j * 2;
            sy = sy > (srcWidth - 1) ? (srcWidth - 1) : sy;
            for (k = 0; k < srcChannels; ++k) {
                *(dataDst + i * stepDst + srcChannels * j + k) = (*(dataSrc + sx * stepSrc + sy * srcChannels + k));
            }
        }
    }
}

static void cvHalfYUV420(unsigned char *dataSrc, unsigned char *dataDst, int srcHeight, int srcWidth)
{
    //support YUV420
    int i = 0;
    for (int y = 0; y < srcHeight; y += 2) {
        for (int x = 0; x < srcWidth; x += 2) {
            dataDst[i] = dataSrc[y * srcWidth + x];
            i++;
        }
    }
    for (int y = 0; y < srcHeight / 2; y += 2) {
        for (int x = 0; x < srcWidth; x += 4) {
            dataDst[i] = dataSrc[(srcWidth * srcHeight) + (y * srcWidth) + x];
            i++;
            dataDst[i] = dataSrc[(srcWidth * srcHeight) + (y * srcWidth) + (x + 1)];
            i++;
        }
    }
}

static void cvHalfYUV422(unsigned char *dataSrc, unsigned char *dataDst, int srcHeight, int srcWidth)
{
    //support YUV422
    int i = 0;
    for (int y = 0; y < srcHeight; y += 2) {
        for (int x = 0; x < srcWidth; x += 4) {
            dataDst[i++] = dataSrc[(y * srcWidth * 2) + x * 2];
            dataDst[i++] = dataSrc[(y * srcWidth * 2) + (x * 2 + 1)];
            dataDst[i++] = dataSrc[(y * srcWidth * 2) + (x * 2 + 2)];
            dataDst[i++] = dataSrc[(y * srcWidth * 2) + (x * 2 + 3)];
        }
    }
}

static void cvResizeTL(struct _IplImageTuling *src, struct _IplImageTuling *dst, int interpolation)
{
    float fy;
    int sy;
    short cbufy[2];
    short cbufx[2];
    float fx;
    int i, j, k, sx, stepDst, stepSrc, iWidthSrc, iHeightSrc;
    unsigned char *dataDst;
    unsigned char *dataSrc;
    double xRatio;
    double yRatio;

    xRatio = (double) src->width / dst->width;
    yRatio = (double) src->height / dst->height;
    dataDst = (unsigned char *) dst->imageData;
    dataSrc = (unsigned char *) src->imageData;
    stepDst = dst->widthStep;
    stepSrc = src->widthStep;
    iWidthSrc = src->width;
    iHeightSrc = src->height;
    i = 0;
    j = 0;
    k = 0;

    for (i = 0; i < dst->height; ++i) {
//        float fy = (float) ((1000*i + 500) * yRatio - 500)/1000;
        fy = (float)((i + 0.5) * yRatio - 0.5);
        sy = (int) fy - ((int) fy > fy);
        fy -= sy;
        sy = sy < (iHeightSrc - 2) ? sy : (iHeightSrc - 2);
        sy = 0 > sy ? 0 : sy;

        cbufy[0] = saturate_cast_short(((1.f - fy) * 2048));
        cbufy[1] = (short)(2048 - cbufy[0]);

        for (j = 0; j < dst->width; ++j) {
            fx = (float)((j + 0.5) * xRatio - 0.5);
            sx = (int) fx - ((int) fx > fx);
            fx -= sx;

            if (sx < 0) {
                fx = 0, sx = 0;
            }

            if (sx >= iWidthSrc - 1) {
                fx = 0, sx = iWidthSrc - 2;
            }

            cbufx[0] = saturate_cast_short((1.f - fx) * 2048);
            cbufx[1] = (short)(2048 - cbufx[0]);

            for (k = 0; k < src->nChannels; ++k) {
                *(dataDst + i * stepDst + src->nChannels * j + k) =
                    (unsigned char)(
                        ((short)(*(dataSrc + sy * stepSrc + src->nChannels * sx + k)) * cbufx[0] * cbufy[0] +
                         (short)(*(dataSrc + (sy + 1) * stepSrc + src->nChannels * sx + k)) * cbufx[0] * cbufy[1] +
                         (short)(*(dataSrc + sy * stepSrc + src->nChannels * (sx + 1) + k)) * cbufx[1] * cbufy[0] +
                         (short)(*(dataSrc + (sy + 1) * stepSrc + src->nChannels * (sx + 1) + k)) * cbufx[1] * cbufy[1]) >> 22);
            }
        }
    }
}

static void cvAbsDiffTL(struct _IplImageTuling *src1, struct _IplImageTuling *src2, struct _IplImageTuling *dst)
{
    int i, j, k;
    unsigned char *src1Pixel;
    unsigned char *src2Pixel;
    unsigned char *dstPixel;

    if (src1 == NULL || src2 == NULL || dst == NULL) {
        printf("tl_error cvAbsDiffTL src1 or src2 or dst is NULL \n");
        return;
    }
    if (src1->widthStep != src2->widthStep || src1->height != src2->height || src1->nChannels != src2->nChannels) {
        printf("tl_error cvAbsDiffTL src1 src2 shape is different\n");
        return;
    }

    if (src1->widthStep != dst->widthStep || src1->height != dst->height || src1->nChannels != dst->nChannels) {
        printf("tl_error cvAbsDiffTL src1 dst shape is different\n");
        return;
    }

    for (i = 0; i < src1->height; i++) {
        src1Pixel = (unsigned char *)(src1->imageData + i * src1->widthStep);
        src2Pixel = (unsigned char *)(src2->imageData + i * src2->widthStep);
        dstPixel = (unsigned char *) dst->imageData + i * dst->widthStep;
        for (j = 0; j < src1->widthStep; j += src1->nChannels) {
            for (k = 0; k < src1->nChannels; k++) {
                dstPixel[j + k] = (unsigned char)(abs((int) src1Pixel[j + k] - (int) src2Pixel[j + k]));
            }
        }
    }
}

static void cvThresholdTL(struct _IplImageTuling *src, struct _IplImageTuling *dst, double threshold, double max_value,
                          int threshold_type)
{
    int i;

    if (src == NULL || dst == NULL) {
        printf("tl_error cvThresholdTL src or dst is NULL \n");
        return;
    }

    for (i = 0; i < src->height * src->width * src->nChannels; i++) {
        if (TL_CV_THRESH_BINARY == threshold_type) {
            if ((int) src->imageData[i] > (int) threshold) {
                dst->imageData[i] = (unsigned char)(255);
            } else {
                dst->imageData[i] = (unsigned char)(0);
            }
        }
    }
}

static void cvErodeTL(struct _IplImageTuling *src, struct _IplImageTuling *dst, int m, int iterations)
{
    int x, y, w, h;
    unsigned char *srcData;
    unsigned char *dstData;
    unsigned int t;
    int x2, y2, x3, y3;
    dstData = (unsigned char *) dst->imageData;
    srcData = (unsigned char *) src->imageData;
    w = src->width;
    h = src->height;

    while (iterations-- > 0) {
        for (y = 0; y < h; y++) {
            for (x = 0; x < w; x++) {
                y2 = y - 1;
                if (y2 < 0) {
                    y2 = h - 1;
                }
                y3 = y + 1;
                if (y3 >= h) {
                    y3 = 0;
                }

                x2 = x - 1;
                if (x2 < 0) {
                    x2 = w - 1;
                }
                x3 = x + 1;
                if (x3 >= w) {
                    x3 = 0;
                }

                t = (unsigned int) srcData[y * w + x];
                if ((unsigned int) srcData[y2 * w + x] < t) {
                    t = (unsigned int) srcData[y2 * w + x];
                }
                if ((unsigned int) srcData[y3 * w + x] < t) {
                    t = (unsigned int) srcData[y3 * w + x];
                }
                if ((unsigned int) srcData[y * w + x2] < t) {
                    t = (unsigned int) srcData[y * w + x2];
                }
                if ((unsigned int) srcData[y * w + x3] < t) {
                    t = (unsigned int) srcData[y * w + x3];
                }
                dstData[y * w + x] = (unsigned char) t;
            }
        }
    }
}

static void cvCutTL(struct _IplImageTuling *src, struct _IplImageTuling *dst, int targetHeight)
{
    int i, j, k, index;
    index = 0;
    for (i = targetHeight; i < src->height; i++) {
        for (j = 0; j < src->width; j++) {
            for (k = 0; k < src->nChannels; k++) {
                dst->imageData[index] = src->imageData[i * src->widthStep + src->nChannels * j + k];
                index ++;
            }
        }
    }
}

#if 0
void printfTL(IplImageTL *src, int type)
{
    int i, j;
    unsigned char *data;
    int step;
    int channels;
    int sum_all;
    int count;
    unsigned char *pixel;
    unsigned int x_p;
    char log_info[8];
    printf("printfTL_1\n");
    data = (unsigned char *) src->imageData;
    step = src->widthStep / sizeof(unsigned char);
    channels = src->nChannels;
//	uchar b_pixel, g_pixel, r_pixel;
    printf("printfTL_2\n");
    printf("printfTL_->height:%d\t", src->height);
    printf("printfTL_->width:%d\n", src->width);
    printf("printfTL_->widthStep:%d\n", src->widthStep);
    printf("printfTL_->nChannels:%d\n", src->nChannels);
    sum_all = 0;

    for (i = 0; i < src->height; i += 1) { // height
        count = 0;
        for (j = 0; j < src->widthStep; j++) { // width
            pixel = (unsigned char *)(src->imageData + i * src->widthStep + j);
            x_p = (unsigned int)(*pixel);
            if (x_p == 255) {
                count += 1;
//                printf("%d\t", j);
            }
            if (type == 1) {
                memset(log_info, 0, 8);
                sprintf(log_info, "%d", x_p);
//                if(j==1279){
//                    printf("xxx=%s\n",log_info);
//                    int le = strlen(log_info);
//                    printf("len=%d\n",le);
//                }
                write_log_file("ttt.log", log_info, (unsigned int) strlen(log_info));
                write_log_file("ttt.log", "\n", (unsigned int)strlen("\n"));
//                sprintf(log_info, "%d", x_p);
            }
            if (j >= src->widthStep - 5) {
                printf("\nj=%d\n", j);
            }
//            printf("i=%d-%dmo\t", j,x_p);
            printf("%d\t", x_p);
//            if (j >= 60) {
//                break;
//            }

        }
        printf("count=%d\t", count);
        sum_all += count;
//        break;
    }
    printf("printf_mat_sum_all : %d\n", sum_all);
}

static void write_log_file(const char *filename, const char *buffer, unsigned int buf_size)
{
    FILE *fp;
    if (filename != NULL && buffer != NULL) {
        {
            fp = fopen(filename, "w+");
            if (fp != NULL) {
                fwrite(fp, buffer, buf_size);
                fclose(fp);
                fp = NULL;
            }
        }
    }
}
#endif

static unsigned char *cvImageTLToUChar(IplImageTL *image, TL_IMAGE_TYPE imageType)
{
    int imageSize;
    if (image->nChannels == 1) {
        imageSize = image->width * image->height;
    } else if (imageType == TL_CV_YUV2GRAY_420) {
        imageSize = image->width * image->height * 3 / 2;
    } else if (imageType == TL_CV_YUV2GRAY_UYVY || imageType == TL_CV_YUV2GRAY_YUY2) {
        imageSize = image->width * image->height * 2;
    } else {
        imageSize = image->width * image->height * image->nChannels + image->width % 4 * image->height;
    }

    unsigned char *imgData = (unsigned char *)malloc(imageSize);
    if (imgData) {
        memcpy(imgData, image->imageData, imageSize);
    }
    // for (int i = 0; i < imageSize; i++) {
    //     imgData[i] = image->imageData[i];
    // }
    return imgData;
}

static IplImageTL *cvCreateImageTL(int width, int height, int nChannels, TL_IMAGE_TYPE imageType)
{
    IplImageTL *image;
    int imageSize;
    if (imageType == TL_CV_GRAY2GRAY) {
        imageSize = width * height;
    } else if (imageType == TL_CV_YUV2GRAY_420) {
        imageSize = width * height * 3 / 2;
    } else if (imageType == TL_CV_YUV2GRAY_UYVY || imageType == TL_CV_YUV2GRAY_YUY2) {
        imageSize = width * height * 2;
    } else {
        imageSize = width * height * 3 + width % 4 * height;
    }
    image = (IplImageTL *) malloc(sizeof(IplImageTL));
    if (!image) {
        return NULL;
    }
//    printf("imageSize:%d\n", imageSize);
//    printf("imageSize:%d\twidth:%d\theight:%d\tnChannels:%d\n", imageSize, width, height, nChannels);
    image->imageData = (unsigned char *)malloc(imageSize);
    memset(image->imageData, 0, imageSize);
    image->width = width;//cols
    image->height = height;//rows
    image->widthStep = width * nChannels;
    image->nChannels = nChannels;
    image->imageSize = imageSize;

    return image;
}

//unsigned char 转image
static IplImageTL *cvCreateImageTLByUChar(const unsigned char *src, int width, int height, int nChannels, TL_IMAGE_TYPE imageType)
{
    int i = 0;
    IplImageTL *image;
    image = cvCreateImageTL(width, height, nChannels, imageType);
//    int allSize = width * height * nChannels;
//    printf("allSize:%d\twidth:%d\theight:%d\tnChannels:%d\n", allSize, width, height, nChannels);
    // for (i = 0; i < image->imageSize; i++) {
    //     image->imageData[i] = src[i];
    // }
    if (image) {
        memcpy(image->imageData, src, image->imageSize);
    }

    return image;
}

static void cvtColorTL(struct _IplImageTuling *src, struct _IplImageTuling *dst, TL_IMAGE_TYPE code)
{
    unsigned char *srcData;
    unsigned char *dstData;
    int srcWidthStep = src->widthStep;
    int srcHeight = src->height;
    int srcChannel = src->nChannels;
    int srcWidth = src->width;
    int dstWidthStep = dst->widthStep;
    srcData = (unsigned char *) src->imageData;
    dstData = (unsigned char *) dst->imageData;

    if (code == TL_CV_RGB2GRAY) {
        cvRgbToGray(srcData, dstData, srcWidthStep, srcHeight, srcChannel, dstWidthStep);
    } else if (code == TL_CV_BGR2GRAY) {
        cvBgrToGray(srcData, dstData, srcWidthStep, srcHeight, srcChannel, dstWidthStep);
    } else if (TL_CV_GRAY2GRAY == code) {
        cvGrayToGray(srcData, dstData, srcWidthStep, srcHeight);
    } else if (code == TL_CV_YUV2GRAY_420) {
        memcpy(dstData, srcData, srcWidth * srcHeight);
//        cvYuvToGray_420(srcData, dstData, srcWidth, srcHeight);
    } else if (code == TL_CV_YUV2GRAY_UYVY) {
        cvYuvToGray_UYVY(srcData, dstData, srcHeight, srcWidth, dstWidthStep);
    } else if (code == TL_CV_YUV2GRAY_YUYV) {
        cvYuvToGray_YUYV(srcData, dstData, srcHeight, srcWidth, dstWidthStep);
    }
}

static IplImageTL *cvCreateImageGRAYTLByUChar(const unsigned char *srcData, int srcWidth, int srcHeight, int srcChannel, TL_IMAGE_TYPE code)
{
    int srcWidthStep = srcWidth * srcChannel;

    IplImageTL *imageGray = cvCreateImageTL(srcWidth, srcHeight, 1, code);
    if (!imageGray) {
        return NULL;
    }

    unsigned char *dstData = (unsigned char *)imageGray->imageData;
    int dstWidthStep = imageGray->widthStep;

    if (code == TL_CV_RGB2GRAY) {
        cvRgbToGray(srcData, dstData, srcWidthStep, srcHeight, srcChannel, dstWidthStep);
    } else if (code == TL_CV_BGR2GRAY) {
        cvBgrToGray(srcData, dstData, srcWidthStep, srcHeight, srcChannel, dstWidthStep);
    } else if (TL_CV_GRAY2GRAY == code) {
        cvGrayToGray(srcData, dstData, srcWidthStep, srcHeight);
    } else if (code == TL_CV_YUV2GRAY_420) {
        memcpy(dstData, srcData, srcWidth * srcHeight);
//        cvYuvToGray_420(srcData, dstData, srcWidth, srcHeight);
    } else if (code == TL_CV_YUV2GRAY_UYVY) {
        cvYuvToGray_UYVY(srcData, dstData, srcHeight, srcWidth, dstWidthStep);
    } else if (code == TL_CV_YUV2GRAY_YUYV) {
        cvYuvToGray_YUYV(srcData, dstData, srcHeight, srcWidth, dstWidthStep);
    }

    return imageGray;
}

void cvFreeImageTL(void *ptr)
{
    struct _IplImageTuling *image = (struct _IplImageTuling *)ptr;

    if (image != NULL) {
        free(image->imageData);
        free(image);
    }
}


typedef struct _MotionDetection_ {
    int dstHeight;
    int min_thr;
    int max_thr;
    int minThrWaitTime;
    int fps;
    int number_of_changes;
    int state;                  //  0-before max thr  1-after max thr 2 after min thr
    int sever_wait_state;
    int send_state;
    int cropRate;
    int serverWaitTime;
    int lastSend;
    int lossFrame;
    int lossFrameThr;
    long long compareTimer;
    long long lastTimer;
    long long lastMdTimer;
    IplImageTL *frame1;

    // int isMove;
    // int outputImgHeight;
    // int outputImgWidth;
    // int outputImgChannels;
    // unsigned char * outputImgData;
} MotionDetection;

static void setUserMode(MotionDetection *md, int ocrFlag, int fingerFlag)
{
    if (fingerFlag) {
        md->cropRate = 0;
    }
    if (fingerFlag && ocrFlag) {
        md->max_thr = 15;
        md->min_thr = 5;
    } else if (fingerFlag && !ocrFlag) {
        md->max_thr = 45;
        md->min_thr = 20;
    } else if (!fingerFlag && ocrFlag) {
        md->max_thr = 60;
        md->min_thr = 5;
    } else if (!fingerFlag && !ocrFlag) {
        md->max_thr = 60;
        md->min_thr = 20;
    }
}

void initMotionDetection(MotionDetection *md, int min_thr, int max_thr, int minThrWaitTime, int dstHeight, int cropRate,
                         int serverWaitTime, int timer, long long nowTime)
{
//    int frameThr;
    md->min_thr = min_thr * (100 - cropRate) / 100;
    md->max_thr = max_thr * (100 - cropRate) / 100;
    md->minThrWaitTime = minThrWaitTime;
    md->fps = 5;
    md->dstHeight = dstHeight;
    md->cropRate = cropRate;

//    frameThr = md->minThrWaitTime * fps / 1000;
    // md->send_state = 2 + (1 < frameThr ? 1 : frameThr);
    md->send_state = 2;

    md->number_of_changes = 0;
    md->state = 0;
    md->sever_wait_state = 0;
    md->serverWaitTime = serverWaitTime;
    md->lastSend = 0;
    md->lossFrame = md->fps / md->fps;
    md->lossFrameThr = 1;
    md->compareTimer = timer;
    md->lastTimer = nowTime + md->compareTimer;
    md->lastMdTimer = nowTime + md->serverWaitTime;
    md->frame1 = NULL;
}

static void getDifference(MotionDetection *md, IplImageTL *motion)
{
    int i = 0, j = 0;
    unsigned char *pixel_x;
    int pixel;
    for (j = 0; j < motion->height; j++) { // height
        for (i = 0; i < motion->width; i++) { // width
            pixel_x = (unsigned char *)(motion->imageData + j * motion->widthStep + i);
            pixel = (*pixel_x) + 0;
            if (pixel == 255) {
                md->number_of_changes++;
            }
        }
    }
}

static void updateState(MotionDetection *md)
{
    if (md->state >= md->send_state && md->lastSend == 1) {
        md->state = 0;
    }
    if (md->state == 0 && md->number_of_changes > md->max_thr) {
        md->state = 1;
    } else if (md->state == 1 && md->number_of_changes < md->min_thr) {
        md->state = 2;
    } else if (md->state >= 2) {
        if (md->number_of_changes < md->min_thr) {
            md->state += 1;
        } else {
            md->state = 1;
        }
    }
}

static void imageCompare(MotionDetection *md, IplImageTL *frame2)
{
    IplImageTL *temp;
    IplImageTL *motion;
    motion = cvCreateImageTL(md->frame1->width, md->frame1->height, md->frame1->nChannels, TL_CV_RGB2GRAY);
    cvAbsDiffTL(md->frame1, frame2, motion);
    cvThresholdTL(motion, motion, 35, 255, TL_CV_THRESH_BINARY);
    temp = cvCreateImageTL(motion->width, motion->height, motion->nChannels, TL_CV_RGB2GRAY);
    cvCopyImageTL(motion, temp);
    memset(motion->imageData, 0, motion->width * motion->height);

    cvErodeTL(temp, motion, 0, 1);
    cvFreeImageTL(temp);

    getDifference(md, motion);
    cvFreeImageTL(motion);
}

static void detectMotion(MotionDetection *md, IplImageTL *frame, TL_IMAGE_TYPE imageType)
{
    int cutHeight;
    int flag = 0;

    cutHeight = frame->height * md->cropRate / 100;
    cvCutTL(frame, frame, cutHeight);
    frame->height = frame->height - cutHeight;
    md->number_of_changes = 0;

    if (md->frame1 == NULL) {
        // first
        cvFreeImageTL(md->frame1);
        md->frame1 = cvCreateImageTL(frame->width, frame->height, frame->nChannels, imageType);
        cvCopyImageTL(frame, md->frame1);
        flag = 1;
    } else if (md->frame1->width != frame->width || md->frame1->height != frame->height) {
        cvFreeImageTL(md->frame1);
        md->frame1 = cvCreateImageTL(frame->width, frame->height, frame->nChannels, imageType);
        cvCopyImageTL(frame, md->frame1);
        flag = 1;
    }

    if (frame->width != md->frame1->width || frame->height != md->frame1->height) {
        printf("error 101 frame1 frame2 shape is diff \n");
        cvFreeImageTL(frame);
        cvFreeImageTL(md->frame1);
        return;
    }

    imageCompare(md, frame);

    if (flag == 0 && md->frame1->imageData != NULL) {
        cvFreeImageTL(md->frame1);
        md->frame1 = cvCreateImageTL(frame->width, frame->height, frame->nChannels, imageType);
        cvCopyImageTL(frame, md->frame1);
    }

    updateState(md);
}

int getNumOfChanges(MotionDetection *md)
{
    return md->number_of_changes;
}

static unsigned char *getOutImgData(unsigned char *image, TL_IMAGE_TYPE imageType, int srcHeight, int srcWidth, int ocrFlag)
{
    unsigned char *outData = NULL;
    if (!ocrFlag) {
        if (imageType == TL_CV_YUV2GRAY_420) {
            //outData = (unsigned char*)malloc(srcHeight/2 * srcWidth/2 * 3/2);
#if 1
            cvHalfYUV420(image, image, srcHeight, srcWidth);
#else
            extern int YUV420p_Soft_Scaling(unsigned char *src, unsigned char *out, int src_w, int src_h, int out_w, int out_h);
            YUV420p_Soft_Scaling(image, image, srcWidth, srcHeight, srcWidth / 2, srcHeight / 2);
#endif
            outData = image;
        } else if (imageType == TL_CV_YUV2GRAY_UYVY || imageType == TL_CV_YUV2GRAY_YUY2) {
            cvHalfYUV422(image, image, srcHeight, srcWidth);
            outData = image;
        } else if (imageType == TL_CV_BGR2GRAY || imageType == TL_CV_RGB2GRAY) {
            //outData = (unsigned char*)malloc(srcHeight/2 * srcWidth/2 * 3);
            cvHalfRGB(image, image, srcHeight, srcWidth, 3);
            outData = image;
        } else if (imageType == TL_CV_GRAY2GRAY) {
            //outData = (unsigned char*)malloc(srcHeight/2 * srcWidth/2);
            cvHalfRGB(image, image, srcHeight, srcWidth, 1);
            outData = image;
        } else {
            outData = NULL;
        }
    } else {
        outData = image;
    }
    return outData;
}

static int checkCode(int width, int height, int channels, long long nowTime, int ocrFlag, int fingerFlag, int chekCode)
{
    int code = 0;
    int sumAll = channels + ocrFlag + fingerFlag;
    while (width != 0) {
        sumAll += width % 10;
        width /= 10;
    }
    while (height != 0) {
        sumAll += height % 10;
        height /= 10;
    }
    while (nowTime != 0) {
        sumAll += nowTime % 10;
        nowTime /= 10;
    }
    sumAll *= 23;
    code = sumAll % 256;

    if (code == chekCode) {
        return 1;
    } else {
        return 0;
    }
}

unsigned char *getMotionResult(MotionDetection *md, const unsigned char *image, int width, int height, int channels,
                               TL_IMAGE_TYPE imageType, long long nowTime, int ocrFlag, int fingerFlag)
{
    if (nowTime - md->lastTimer < md->compareTimer) {
        return NULL;
    }
    md->lastTimer = nowTime;
    IplImageTL *dstImageGRAY;
    unsigned char *outputImgData = NULL;

    setUserMode(md, ocrFlag, fingerFlag);
    //dstImageGRAY use for detection
    dstImageGRAY = cvCreateImageTL(md->dstHeight * width / height, md->dstHeight, 1, TL_CV_GRAY2GRAY);
    //if image is YUV420 format, get gray image easy
    if (imageType == TL_CV_YUV2GRAY_420) {
        cvGrayRsizeNear(image, width, height, dstImageGRAY);
    } else {
        //if image is not YUV420 format , we need malloc zone to get gray image
        IplImageTL *imageGRAY;
        imageGRAY = cvCreateImageGRAYTLByUChar(image, width, height, channels, imageType);
        cvRsizeNear(imageGRAY, dstImageGRAY);
        cvFreeImageTL(imageGRAY);
    }

    detectMotion(md, dstImageGRAY, TL_CV_GRAY2GRAY);
    //return image imageType is same as input image
    int isMove = md->state >= md->send_state;
    if (isMove > 0 && nowTime - md->lastMdTimer >= (long long) md->serverWaitTime) {
        md->lastMdTimer = nowTime;
        md->lastSend = 1;
        outputImgData = getOutImgData((unsigned char *)image, imageType, height, width, ocrFlag);
    } else {
        md->lastSend = 0;
    }
    cvFreeImageTL(dstImageGRAY);
    return outputImgData;
}

