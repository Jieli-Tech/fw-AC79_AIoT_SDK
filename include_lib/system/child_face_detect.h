#ifndef __CHILD_FACE_DETECT_H_
#define __CHILD_FACE_DETECT_H_
#include <stdint.h>
#include <math.h>
#include <string.h>




typedef int8_t q7_t;
typedef int16_t q15_t;
typedef int32_t q31_t;
typedef float f32;
typedef unsigned short u16;
typedef struct {
    q15_t w;
    q15_t h;
    q15_t c;
    q7_t *pixel;
} frame;

typedef struct {
    q15_t x1;
    q15_t y1;
    q15_t x2;
    q15_t y2;
    q15_t offset_x1;
    q15_t offset_y1;
    q15_t offset_x2;
    q15_t offset_y2;
} BBox;

typedef struct {
    q15_t x;
    q15_t y;
} Point_t;

typedef struct {
    int fast_mode;
    int num_r;
    int num_o;
    int i_w[2];
    int i_h[2];
    int scale[2];
    int thresh[3];
    int smile_thresh;
    q15_t *confidence_;
    BBox *bounding_box;
    Point_t *alignment_temp;

    frame *img;
    frame *img_temp;
    frame *score;
    frame *location;
    frame *keypoint;

    BBox *bound_temp;
    q15_t *conf_temp;
    q7_t *smile_idx;

} face_detect_f;


int face_detect_process(frame *image, face_detect_f *e, q15_t *confidence_, BBox *bounding_box, Point_t *alignment_temp);

int face_detect_free(face_detect_f *e);

void  face_detect_init(int fast_m, int num_r, int num_o, int *thresh, int smile_thresh, face_detect_f *e);

void smile_face_process(int global_count, frame *image, face_detect_f *e, q15_t *confidence_, BBox *bounding_box, Point_t *alignment_temp, q7_t *idx_smile);

#endif
