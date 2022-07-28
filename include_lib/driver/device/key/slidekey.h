#ifndef DEVICE_SLIDEKEY_H
#define DEVICE_SLIDEKEY_H

#include "generic/typedef.h"

struct slidekey_port {
    u8 io;
    u8 io_up_en;                //是否用外部上拉，1：用外部上拉， 0：用内部上拉10K
    u32 level;
    u32 ad_channel;
    int msg;
};

struct slidekey_platform_data {
    u8 enable;
    u8 num;
    const struct slidekey_port *port;
};

//SLIDEKEY API:
extern int slidekey_init(const struct slidekey_platform_data *slidekey_data);

#endif


