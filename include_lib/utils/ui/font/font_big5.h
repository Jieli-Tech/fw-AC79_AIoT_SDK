#ifndef __FONT_BIG5_H__
#define __FONT_BIG5_H__

#include "typedef.h"

extern bool InitFont_BIG5(struct font_info *info);
extern u16 TextOut_BIG5(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);
extern u16 TextOutW_BIG5(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);
extern u16 ConvertUTF16toBIG5(struct font_info *info, u16 utf);

#endif
