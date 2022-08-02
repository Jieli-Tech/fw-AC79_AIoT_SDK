#ifndef PLAY_CONTROL_H
#define PLAY_CONTROL_H


#include "view.h"


struct play_ctrl {
    struct view_group *group;
    union {
        int (*on_click)(struct play_ctrl *, int);
        int (*on_touch)(struct play_ctrl *, struct touch_event *);
    } h;
    int (*on_hold)(struct play_ctrl *, int);
    int (*on_change)(struct play_ctrl *, int);
};






#define REGISTER_PLAY_CTRL_ON_CLICK(id, on_click) \
	REGISTER_VIEW_ON_CLICK(id, on_click)

#define REGISTER_PLAY_CTRL_ON_HOLD(id, on_hold) \
	REGISTER_VIEW_ON_HOLD(id, on_hold)





#endif



