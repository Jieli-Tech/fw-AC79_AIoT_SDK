#ifndef __FONT_SJIS_H__
#define __FONT_SJIS_H__

#include "typedef.h"

extern bool InitFont_SJIS(struct font_info *info);
extern u16 TextOut_SJIS(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);
extern u16 TextOutW_SJIS(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);
extern u16 ConvertUTF16toSJIS(struct font_info *info, u16 utf);

#endif
