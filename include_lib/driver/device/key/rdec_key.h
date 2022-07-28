#ifndef RDEC_KEY_H
#define RDEC_KEY_H

#include "generic/typedef.h"

#include "asm/rdec.h"

//RDECKEY API:
int rdec_key_init(const struct rdec_platform_data *rdec_key_data);

u8 rdec_get_key_value(void);

#endif

