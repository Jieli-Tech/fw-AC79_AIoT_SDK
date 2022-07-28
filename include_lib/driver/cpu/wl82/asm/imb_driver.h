#ifndef IMB_DRIVER_H
#define IMB_DRIVER_H

#include "video/fb.h"
#include "typedef.h"
#include "asm/imd.h"


//IMAGE图层常用颜色定义
#define IMAGE_COLOR_WHITE   0xeb8080
#define IMAGE_COLOR_RED     0x515aef
#define IMAGE_COLOR_GREEN   0x903522
#define IMAGE_COLOR_BLUE    0x28ef6d
#define IMAGE_COLOR_YELLOW  0xd21092
#define IMAGE_COLOR_PURPLE  0x6acadd
#define IMAGE_COLOR_CYAN    0xa9a510
#define IMAGE_COLOR_BLACK   0x108080

enum LAYER_ERROR_CODE {
    ERROR_NONE,
    ERROR_FORMAT,  /*图层格式错误*/
    ERROR_OPENED,  /*图层已打开,图层句柄有效*/
    ERROR_NONSPEC, /*非指定图层*/
    ERROR_SPEC_OPENED,/*指定图层已打开*/
};

enum {
    LAYER_STA_CLOSED,
    LAYER_STA_READY,
    LAYER_STA_OPENED,  /*图层已打开*/
    LAYER_STA_PAUSE = 0x10,
};


// struct imb_layer_merge {
// u8 rotate_en;           //旋转使能
// u8 hori_mirror_en;      //水平镜像使能
// u8 vert_mirror_en;      //垂直镜像使能
// u8 packed_format;       //数据格式
// u32 dest_addr;          //存放地址
// u16 width;
// u16 height;
// u8 mode;
// u8 status;
// };

struct lcd_screen_info {
    u16 xoffset;
    u16 yoffset;
    u16 xres;
    u16 yres;
    u8 rotate;
};

struct fb_platform_data {
    u8 z_order[8];
};

#define RATIO(r) ((r==FB_COLOR_FORMAT_YUV420)?12:\
				 ((r==FB_COLOR_FORMAT_YUV422)?16:\
				 ((r==FB_COLOR_FORMAT_RGB888)?24:\
				 ((r==FB_COLOR_FORMAT_RGB565)?16:\
				 ((r==FB_COLOR_FORMAT_16K)?16:\
				 ((r==FB_COLOR_FORMAT_256)?8:\
				 ((r==FB_COLOR_FORMAT_2BIT)?2:\
				 ((r==FB_COLOR_FORMAT_1BIT)?1:0))))))))

// #define REGISTER_LAYER_BUF(layer,f,w,h,n) \
// u8 layer##_##f##_##w##x##h##_##n[(w*h*n*RATIO(f)/8+31)/32*32] ALIGNE(32);\
// static struct layer_buf layer##_##f##_##w##x##h##_##n##_t SEC_USED(.layer_buf_t) = {\
// .format = f,\
// .num = n,\
// .total_num = n,\
// .width = w,\
// .height = h,\
// .baddr = layer##_##f##_##w##x##h##_##n,\
// .size = (w*h*n*RATIO(f)/8+31)/32*32,\
// }\

// extern struct layer_buf layer_buf_begin[];
// extern struct layer_buf layer_buf_end[];

// #define list_for_each_layer_buf(p) \
// for (p=layer_buf_begin; p < layer_buf_end; p++)

// int imb_init();
// void imb_layer_set_baddr(struct imb_layer *layer, u8 index);

#endif


