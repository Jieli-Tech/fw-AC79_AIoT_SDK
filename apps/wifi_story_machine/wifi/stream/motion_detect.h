#ifndef TURINGIMAGE_MOTION_DETECT_C_H
#define TURINGIMAGE_MOTION_DETECT_C_H


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
    void *frame1;

    // int isMove;
    // int outputImgHeight;
    // int outputImgWidth;
    // int outputImgChannels;
    // unsigned char * outputImgData;
} MotionDetection;

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


void initMotionDetection(MotionDetection *md, int min_thr, int max_thr, int minThrWaitTime, int dstHeight, int cropRate,
                         int serverWaitTime, int timer, long long nowTime);
unsigned char *getMotionResult(MotionDetection *md, const unsigned char *image, int width, int height, int channels,
                               TL_IMAGE_TYPE imageType, long long nowTime, int ocrFlag, int fingerFlag);
void updateState(MotionDetection *md);
int getNumOfChanges(MotionDetection *md);
void cvFreeImageTL(void *ptr);

#endif //TURINGIMAGE_MOTION_DETECT_C_H

