#ifndef __UTF8_2_UNICODE_H__
#define __UTF8_2_UNICODE_H__

#include "typedef.h"

int get_utf8_size(const unsigned char pInput);
int utf8_2_unicode_one(const unsigned char *pInput, u16 *Unic);
int utf8_2_unicode(u8 *utf8, u8 utf8_len, u8 *unicode, u8 unic_len);

#endif //__UTF8_2_UNICODE_H__

