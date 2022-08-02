#ifndef __FONT_OTHERLANGUAGE_H__
#define __FONT_OTHERLANGUAGE_H__

#include "typedef.h"

extern bool InitFont_OtherLanguage(struct font_info *info);
extern u16 TextOut_OtherLanguage(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);
extern u16 TextOutW_OtherLanguage(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);
extern u16 ConvertUTF16toOtherLanguage(struct font_info *info, u16 utf);
extern u16 TextOutW_AllLanguage(struct font_info *info, u8 *str, u16 len, u16 x, u16 y);

#endif
