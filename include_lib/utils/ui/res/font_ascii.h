#ifndef FONT_ASCII_H
#define FONT_ASCII_H


#include "typedef.h"


void font_ascii_get_width_and_height(char code, int *height, int *width);
int font_ascii_get_pix(char code, u8 *pixbuf, int buflen, int *height, int *width);

int font_ascii_width_check(const char *str);











#endif

