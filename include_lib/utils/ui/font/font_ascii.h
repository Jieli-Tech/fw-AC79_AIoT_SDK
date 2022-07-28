#ifndef __FONT_ASCII_H__
#define __FONT_ASCII_H__

#include "typedef.h"
extern bool InitFont_ASCII(struct font_info *info);
extern u8 GetASCIICharacterData(struct font_info *info, u16 asc);
extern u8 GetASCIICharacterWidth(struct font_info *info, u16 asc);

#endif
