#ifndef __UI_WAVE_H__
#define __UI_WAVE_H__

#include "typedef.h"
#include "ui/ui_core.h"
#include "rect.h"

#define WAVE_BUFFER_LEN	1024
#define WAVE_READ_CACHE_LEN	8
#define BUFFER_EMPTY	-1111
#define BUFFER_FULL		-2222;




#define GL_RGB(r, g, b) ((0xFF << 24) | (((unsigned int)(r)) << 16) | (((unsigned int)(g)) << 8) | ((unsigned int)(b)))
#define GL_RGB_R(rgb) ((((unsigned int)(rgb)) >> 16) & 0xFF)
#define GL_RGB_G(rgb) ((((unsigned int)(rgb)) >> 8) & 0xFF)
#define GL_RGB_B(rgb) (((unsigned int)(rgb)) & 0xFF)


typedef struct c_wave_buffer {
    short m_wave_buf[WAVE_BUFFER_LEN];
    short m_head;
    short m_tail;
    int m_min_old;
    int m_max_old;
    int m_min_older;
    int m_max_older;
    int m_last_data;
    short 	m_read_cache_min[WAVE_READ_CACHE_LEN];
    short 	m_read_cache_mid[WAVE_READ_CACHE_LEN];
    short 	m_read_cache_max[WAVE_READ_CACHE_LEN];
    short	m_read_cache_sum;
    unsigned int m_refresh_sequence;
    int(*write_wave_data)(struct c_wave_buffer *, short data);
    int(*read_wave_data_by_frame)(struct c_wave_buffer *, short *max, short *min, short frame_len, unsigned int sequence, short offset);
    void(*reset)(struct c_wave_buffer *);
    void(*clear_data)(struct c_wave_buffer *);
    int(*read_data)(struct c_wave_buffer *);
    short(*get_cnt)(struct c_wave_buffer *);

} c_wave_buffer;






typedef struct c_wave_ctrl {
    unsigned int m_wave_color;
    unsigned int m_back_color;
    short  m_wave_left;
    short  m_wave_right;
    short  m_wave_top;
    short  m_wave_bottom;

    short  m_wave_width;
    short  m_wave_height;

    short m_max_data;
    short m_min_data;

    struct c_wave_buffer	*m_wave;
    struct draw_context *dc;
    short  			m_wave_cursor;
    int 			m_wave_speed;		//pixels per refresh
    unsigned int	m_wave_data_rate;	//data sample rate
    unsigned int	m_wave_refresh_rate;//refresh cycle in millisecond
    unsigned char 	m_frame_len_map[64];
    unsigned char 	m_frame_len_map_index;
    int             m_frame_len;
    unsigned char   stream;

    short           m_focus;
    unsigned char    *m_cache_max;
    unsigned char    *m_cache_mid;
    unsigned char    *m_cache_min;


    struct rect draw;

    int(*write_wave_data)(struct c_wave_buffer *, short data);
    void(*set_wave_in_out_rate)(struct c_wave_ctrl *, unsigned int data_rate, unsigned int refresh_rate);
    void(*set_wave_speed)(struct c_wave_ctrl *, unsigned int speed);
    void(*set_max_min)(struct c_wave_ctrl *, short max_data, short min_data);
    void(*refresh_wave)(struct c_wave_ctrl *, unsigned char frame);
    void(*clear_wave)(struct c_wave_ctrl *);
    void(*set_wave_color)(struct c_wave_ctrl *, unsigned int color);

} c_wave_ctrl;




c_wave_ctrl *c_wave_ctrl_create(struct rect *rect);
void draw_smooth_vline_redraw(c_wave_ctrl *__this, struct draw_context *dc, u8 draw_backcolor);
void c_wave_ctrl_release(c_wave_ctrl **__this);
void wave_set_screen_rect(c_wave_ctrl *__this, struct rect *rect);

#endif
