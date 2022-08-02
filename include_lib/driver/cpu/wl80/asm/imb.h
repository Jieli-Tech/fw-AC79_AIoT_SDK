#ifndef __IMB_H__
#define __IMB_H__

#include "typedef.h"

#define IMB_YUV420      0
#define IMB_YUV422      1

struct imb_image_yuv_data {
    u8  fmt;
    u8  *y;
    u8  *u;
    u8  *v;
};

struct imb_stick_data {
    u16 width;
    u16 height;
    struct imb_image_yuv_data base_image;
    struct imb_image_yuv_data sticker;
    struct imb_image_yuv_data image;
};

int imb_init(void);
int imb_stick_image(struct imb_stick_data *data);
int imb_stick_open(struct imb_stick_data *data);
int imb_stick_start(u8 *y, u8 *u, u8 *v, u16 height);
int imb_stick_close(void);
#endif
