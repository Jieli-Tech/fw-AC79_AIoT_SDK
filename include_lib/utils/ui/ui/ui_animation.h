#ifndef UI_PIC_H
#define UI_PIC_H

#include "ui/ui_core.h"




struct ui_animation {
    struct element elm;
    u16 index;
    u16 loop;
    void *timer;
    const struct ui_animation_info *info;
    const struct element_event_handler *handler;
};

















#endif

