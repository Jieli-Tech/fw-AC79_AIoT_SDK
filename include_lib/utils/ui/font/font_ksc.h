#ifndef __FONT_KSC_H__
#define __FONT_KSC_H__

#include "typedef.h"

extern bool InitFont_KSC(struct font_info *info);
extern u16 TextOut_KSC(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);
extern u16 TextOutW_KSC(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);
extern u16 ConvertUTF16toKSC(struct font_info *info, u16 utf);

#endif
