#ifndef UI_FIGURE_H
#define UI_FIGURE_H

struct _line {
    int x_begin;
    int y_begin;
    int x_end;
    int y_end;
    int length;
    int angle;
    int color;
};

void draw_line(void *_dc, struct _line *line_info);
void draw_line_by_angle(void *_dc, struct _line *line_info);
void draw_triangle(void *_dc, struct _line *line_info1, struct _line *line_info2, struct _line *line_info3);
void draw_rect(void *_dc, struct rect *rectangle, u16 color);
void draw_circle_or_arc(void *_dc, struct circle_info *info);

#endif

