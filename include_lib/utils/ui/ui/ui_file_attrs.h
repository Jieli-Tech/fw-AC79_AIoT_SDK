#ifndef UI_FILE_ATTRS_H
#define UI_FILE_ATTRS_H



#include "ui/ui_core.h"
#include "ui/control.h"



#define FATTRS_CHILD_NUM 	 (FATTRS_CHILD_END - FATTRS_CHILD_BEGIN)



struct ui_fattrs {
    struct element elm;
    char fsize_str[16];
    struct ui_file_attrs attrs;
    const struct ui_fattrs_info *info;
    const struct element_event_handler *handler;
};




int ui_file_attrs_show(struct element *elm, struct ui_file_attrs *attrs);


int ui_file_attrs_get(struct element *elm, struct ui_file_attrs *attrs);

int ui_file_attrs_set(struct element *elm, struct ui_file_attrs *attrs);
































#endif
