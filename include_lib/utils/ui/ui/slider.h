#ifndef SLIDER_H
#define SLIDER_H




enum {
    UNIT_TIME,
    UINT_PERSENT,
};

struct slider_info {
    //struct text txt;
    const char *format;
    u32 val;
};


struct slider {
    //struct text *curr_txt;
    //struct text *total_txt;
    u32 curr_val;
    u32 total_val;
    int (*on_change)(void *, int);
};






#define REGISTER_SLIDER_ON_CHANGE(id, on_change) \
	REGISTER_VIEW_ON_CHANGE(id, on_change)
















#endif


