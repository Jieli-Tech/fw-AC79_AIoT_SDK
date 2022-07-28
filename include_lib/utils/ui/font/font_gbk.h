#ifndef __FONT_GBK_H__
#define __FONT_GBK_H__

#include "typedef.h"

extern bool InitFont_GBK(struct font_info *info);
extern u16 TextOut_GBK(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);
extern u16 TextOutW_GBK(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);
extern u16 ConvertUTF16toGB2312(struct font_info *info, u16 utf);
extern u16 ConvertUTF16toGBK(struct font_info *info, u16 utf);

#endif
