#ifndef __FONT_Thai_H__
#define __FONT_Thai_H__

#include "typedef.h"

typedef struct {
    u16 unicode;
    u16 ansi;
    u8  *buf;
    u8  attr: 4;
    u8  bigendian: 4;
    u16 width;
    u16 height;
} THaiASCSTRUCT;


extern bool IsThaiOneWord_W(char *pstr, const int iSize, int *i, THaiASCSTRUCT *w2, THaiASCSTRUCT *w3, u8 unicode, u8 bigendian);
extern u8 GetThaiLanguageCharacterData(struct font_info *info, u16 asc, u8 **buf);
extern u8 ThaiLanguagecompose(struct font_info *info, u16 *width, THaiASCSTRUCT *w1, THaiASCSTRUCT *w2, THaiASCSTRUCT *w3);

extern u16 TextOut_Thai(struct font_info *info, u8 *str,  u16 len,  u16 x,  u16 y);
extern u16 TextOutW_Thai(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);
#endif
