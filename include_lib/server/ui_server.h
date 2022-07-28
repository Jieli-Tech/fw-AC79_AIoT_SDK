#ifndef UI_SERVER_H
#define UI_SERVER_H


#include "server/server_core.h"
#include "ui/includes.h"


enum {
    UI_REQ_SHOW,
    UI_REQ_HIDE,
    UI_REQ_MSG,
    UI_REQ_SHOW_SYNC,
};



struct ui_param {
    u32 id;
};


struct ui_msg {
    u32 receiver;
    const char *msg;
    void *exdata;
};


struct ui_animation {
    u32 id;
    u16 time;
    u16 action;
};


union uireq {
    struct ui_param 	show;
    struct ui_param 	hide;
    struct ui_msg       msg;
};


struct ui_args {
    u32 style_ver;
    u32 res_ver;
    u32 str_ver;
};



extern int ui_server_version();



void import_ui_server();

int ui_menu_get_busy(void);
void ui_menu_set_busy(int busy);


void ui_server_show_completed();




#endif

