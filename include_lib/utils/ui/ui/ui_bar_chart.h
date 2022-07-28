#ifndef __UI_BAT_CHART_H__
#define __UI_BAT_CHART_H__

#include "typedef.h"
#include "ui/ui_core.h"
#include "rect.h"


struct barchart_text_info {
    u8 move;
    int min_value;
    int max_value;
    int text_color;
};


struct ui_barchart {
    s16  left;
    s16  top;
    s16  width;
    s16  height;
    u8 *format;
    u8 *str;
};






typedef struct c_barchart_ctrl {
    u16    backcolor;
    u16    activecolor;
    u16    unactivecolor;
    short  left;
    short  top;
    short  width;
    short  height;
    short  interval;
    short  child_width;
    short  child_height;
    struct ui_barchart *child_barchart;
    s16     max_scale;
    u8     column;
    u8   *table;
    u8    offset;
    struct draw_context *dc;
    struct rect draw;
} c_bar_chart_ctrl;


extern struct c_barchart_ctrl *barchart_create(struct rect *layout, u16 width, u16 height, u16 column, u8 margin);

extern void barchart_release(struct c_barchart_ctrl *ctrl);

extern int barchart_redraw(struct c_barchart_ctrl *ctrl, struct draw_context *dc, int x_offset, int y_offset);

extern int barchart_add_char(struct c_barchart_ctrl *ctrl, u8 data);

extern int barchart_add_int_table(struct c_barchart_ctrl *ctrl, int *data, u16 count);

extern int barchart_add_char_table(struct c_barchart_ctrl *ctrl, u8 *data, u16 count);

extern void set_barchart_color(struct c_barchart_ctrl *ctrl, u8 index, u16 activecolor, u16 unactivecolor);

extern void set_barchart_backcolor(struct c_barchart_ctrl *ctrl, u16 color);

#endif
