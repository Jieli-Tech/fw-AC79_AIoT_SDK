#ifndef __CIRCLE_H__
#define __CIRCLE_H__

#include "typedef.h"
#include "generic/rect.h"

#define false 0
#define true  1

#define COORD_MAX     (16383)    /*To avoid overflow don't let the max [-32,32k] range */
#define COORD_MIN     (-16384)
#define MATH_ABS(x)   ((x)>0?(x):(-(x)))

typedef struct {
    uint16_t color;
    uint16_t width;
} style_t;

typedef struct {
    s16 x;
    s16 y;
    s16 disp_x;
    s16 disp_y;
    uint16_t width;
    uint16_t height;
    uint16_t pitch;
    uint16_t len;
    uint8_t *buf;
    int color;
    int bitdepth;
    struct rect *rect;
    style_t *style;
} area_t;

struct circle_info {
    int x;
    int y;
    int disp_x;
    int disp_y;
    int center_x;
    int center_y;
    int radius_big;
    int radius_small;
    int angle_begin;
    int angle_end;
    int angle_curr;
    int color;
    int left;
    int top;
    int width;
    int height;
    int bitmap_width;
    int bitmap_height;
    int bitmap_pitch;
    int bitmap_depth;
    u8 *bitmap;
    int bitmap_size;
};

void draw_arc(int16_t center_x, int16_t center_y, uint16_t radius, uint16_t start_angle, uint16_t end_angle, const area_t *area);

void draw_circle_by_percent(struct circle_info *info, u8 percent);

#endif


