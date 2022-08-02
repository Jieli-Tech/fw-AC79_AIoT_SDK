#ifndef __LCD_CONFIG_H__
#define __LCD_CONFIG_H__

#include "app_config.h"

#if TCFG_LCD_ST7735S_ENABLE
#define LCD_W     128
#define LCD_H     128
#endif

#if TCFG_LCD_ST7789V_ENABLE
#define LCD_W     240
#define LCD_H     240
#endif

#if TCFG_LCD_ST7789S_ENABLE
#if HORIZONTAL_SCREEN
#define LCD_W     320
#define LCD_H     240
#else
#define LCD_W     240
#define LCD_H     320
#endif
#endif

#if TCFG_LCD_HX8357_ENABLE
#if HORIZONTAL_SCREEN
#define LCD_W     480
#define LCD_H     320
#else
#define LCD_W     320
#define LCD_H     480
#endif
#endif

#if TCFG_LCD_480x272_8BITS
#define LCD_W	480
#define LCD_H	272
#endif

#if TCFG_LCD_ILI9341_ENABLE
#if HORIZONTAL_SCREEN
#define LCD_W     320
#define LCD_H     240
#else
#define LCD_W     240
#define LCD_H     320
#endif
#endif

#if TCFG_LCD_ILI9488_ENABLE
#if HORIZONTAL_SCREEN
#define LCD_W     480
#define LCD_H     320
#else
#define LCD_W     320
#define LCD_H     480
#endif
#endif

#define LCD_RGB565_DATA_SIZE  LCD_W * LCD_H * 2
#define LCD_RGB888_DATA_SIZE  LCD_W * LCD_H * 3
#define LCD_YUV420_DATA_SIZE  LCD_W * LCD_H * 3 / 2


#endif

