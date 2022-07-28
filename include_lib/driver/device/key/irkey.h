#ifndef DEVICE_IRKEY_H
#define DEVICE_IRKEY_H

#include "generic/typedef.h"

struct irkey_platform_data {
    u8 enable;
    u8 port;
};

//IRKEY API:
extern int irkey_init(const struct irkey_platform_data *irkey_data);

extern u8 ir_get_key_value(void);

#endif

