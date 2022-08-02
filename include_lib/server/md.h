#ifndef MD_H
#define MD_H

#define IM_MIN(a,b) \
	( (a) < (b) ? (a) : (b) )

#define IM_MAX(a,b) \
	( (a) > (b) ? (a) : (b) )

#define IM_ABS(a) \
	( (a) > 0 ? (a) : (-(a)) )

#define OBJ_SIZE_MAX	 512
typedef enum {
    IM_FORMAT_NULL,
    IM_FORMAT_GRAY,
    IM_FORMAT_RGB,
    IM_FORMAT_YUV420SP,
    IM_FORMAT_HSV,
    IM_FORMAT_GRAD_MAG,
    IM_FORMAT_GRAD_ANGLE,
} image_format_t;

typedef struct image {
    int w;
    int h;
    image_format_t format;
    int stride;
    int bpp;
    int size;
    union {
        uint8_t *pixels;
        uint8_t *data;
        uint16_t *data16;
    };
} image_t;

typedef struct {
    int valid;
    int x1;
    int y1;
    int x2;
    int y2;
    int count;
} object_info_t;

void md_init(u8 weight_, int theshold_, int update_, int merge_th_, int mode_, u8 *ref_buf);
int md_process(u8 *cur_buf, object_info_t output_obj[16]);
void md_deinit();


#endif


