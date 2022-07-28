#ifndef _YUV_SOFT_SCALING_
#define _YUV_SOFT_SCALING_

//dhx-jieli-2020-12-24

typedef struct _image {
    int w;
    int h;
    union {
        unsigned char *pixels;
        unsigned char *data;
        unsigned char *data16;
    };
} image_info;

//yuyv:YUYV源, yuv422p:YUV422P输出, width/height:源宽/源高
int YUYV422ToYUV422p(unsigned char *yuyv, unsigned char *yuv422p, int width, int height);

//YUV444P转YUV422p:dhx-jieli-2020-12-24
//yuv444p:YUV444P源, yuv422p:YUV422P输出, width/height:源宽/源高
int YUV444pToYUV422p(unsigned char *yuv444p, unsigned char *yuv422p, int width, int height);

//YUV444P转YUV420p:dhx-jieli-2020-12-24
//yuv444p:YUV444P源, yuv420p:YUV420P输出, width/height:源宽/源高
int YUV444pToYUV420p(unsigned char *yuv444p, unsigned char *yuv420p, int width, int height);

//yuv422p:YUV422P源, yuv420p:YUV420P输出, width/height:源宽/源高
int YUV422pToYUV420p(unsigned char *yuv422p, unsigned char *yuv420p, int width, int height);

//src:YUV源,out:YUV输出,src_w/src_h/out_w/out_h:源宽/源高/输出宽/输出高, angle:翻转角度(0/90/180/270)
int YUV420p_REVERSAL(char *src, char *out, int src_w, int src_h, int *out_w, int *out_h, int angle);

//YUV分辨率进行对齐，对齐只能比源尺寸要小(不会变大)
//src:YUV源,src_w/src_h/out_w/out_h/width_align/height_align:源宽/源高/输出宽/输出高/宽对齐像素点数/高对齐像素点数
int YUV420p_ALIGN(unsigned char *src, int src_w, int src_h, int *out_w, int *out_h, int width_align, int height_align);

//YUV缩小任意尺寸
//AC79(WL80) sys_clk=320M , 26ms:720P->VGA, 8ms:720P->320*240, 6ms:VGA->320*240;
//缩小可以不使用out作为缓存，out写NULL即可，直接在src覆盖
//src:YUV源,out:YUV输出,src_w/src_h/out_w/out_h:源宽/源高/输出宽/输出高
int YUV420p_Zoom_Out(unsigned char *src, unsigned char *out, int src_w, int src_h, int out_w, int out_h);

//YUV放大任意尺寸
//AC79(WL80) sys_clk=320M , 80ms:VGA->720P, 130ms:VGA->1080P, 260ms:720P->1080P;
//src:YUV源,out:YUV输出,src_w/src_h/out_w/out_h:源宽/源高/输出宽/输出高
int YUV420p_Zoom_In(unsigned char *src, unsigned char *out, int src_w, int src_h, int out_w, int out_h);

//YUV缩放
//AC79(WL80) sys_clk=320M , 26ms:720P->VGA, 8ms:720P->320*240, 6ms:VGA->320*240, 80ms:VGA->720P, 130ms:VGA->1080P, 260ms:720P->1080P;
//src:YUV源,out:YUV输出,src_w/src_h/out_w/out_h:源宽/源高/输出宽/输出高
int YUV420p_ZOOM(unsigned char *src, unsigned char *out, int src_w, int src_h, int out_w, int out_h);

//YUV缩放,和YUV420p_ZOOM一样
//AC79(WL80) sys_clk=320M , 26ms:720P->VGA, 8ms:720P->320*240, 6ms:VGA->320*240, 80ms:VGA->720P, 130ms:VGA->1080P, 260ms:720P->1080P;
//src:YUV源,out:YUV输出,src_w/src_h/out_w/out_h:源宽/源高/输出宽/输出高
int YUV420p_Soft_Scaling(unsigned char *src, unsigned char *out, int src_w, int src_h, int out_w, int out_h);


//YUV裁剪指定区域
//yuv420p:YUV源,src_w/src_h/源宽/源高/
//out:指定输出YUV缓存区，NULL则覆盖yuv420p，out_size指定输出缓冲区长度，out=NULL时out_size = 0即可
//w_start:裁剪起始宽(2对齐)，w_size:裁剪长度(2对齐)
//h_start:裁剪起始高(2对齐)，h_size:裁剪长度(2对齐)
//宽高起始(0,0)点为图片的左上角
int YUV420p_Cut(unsigned char *yuv420p, int src_w, int src_h, unsigned char *out, int out_size, int w_start, int w_size, int h_start, int h_size);

/**********rgb旋转函数*************/
//in_buf:RGB源,out_buf:RGB输出
void RGB_Soft_90(unsigned char mode, char *out_buf, char *in_buf, int width, int height);

#endif
