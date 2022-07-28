#ifndef _LED_UI_SERVER_H_
#define _LED_UI_SERVER_H_


#include "server/server_core.h"
#include "ui/includes.h"


enum {
    LED_UI_REQ_SHOW,
    LED_UI_REQ_HIDE,
    LED_UI_REQ_MSG,
    LED_UI_REQ_SHOW_SYNC,
};

struct led_ui_param {
    u32 id;
};


struct led_ui_msg {
    const char *receiver;
    const char *msg;
    void *exdata;
};


union led_uireq {
    struct led_ui_param 	show;
    struct led_ui_param 	hide;
    struct led_ui_msg       msg;
};


struct led_ui_args {
    u32 style_ver;
    u32 res_ver;
    u32 str_ver;
};

int led_ui_register_msg_handler(const char *receiver, const struct uimsg_handl *handler);
int led_ui_unregister_msg_handler(const char *receiver);

#endif

