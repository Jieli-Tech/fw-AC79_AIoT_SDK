/*********************************************************************
*                SEGGER Microcontroller GmbH & Co. KG                *
*        Solutions for real time microcontroller applications        *
**********************************************************************
*                                                                    *
*        (c) 1996 - 2010  SEGGER Microcontroller GmbH & Co. KG       *
*                                                                    *
*        Internet: www.segger.com    Support:  support@segger.com    *
*                                                                    *
**********************************************************************

** emWin V5.06 - Graphical user interface for embedded applications **
emWin is protected by international copyright laws.   Knowledge of the
source code may not be used to write a similar product.  This file may
only be used in accordance with a license and should not be re-
distributed in any way. We appreciate your understanding and fairness.
----------------------------------------------------------------------
File        : GUI_GIF_Private.h
Purpose     : Private header file for GUI_GIF... functions
---------------------------END-OF-HEADER------------------------------
*/

#ifndef GUI_GIF_PRIVATE_H
#define GUI_GIF_PRIVATE_H

#include "system/includes.h"
/*#include "GUI_Private.h"*/
/*#include "header.h"*/

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
#define MAX_NUM_LWZ_BITS 12



typedef int GUI_GET_DATA_FUNC(void *p, const u8 **ppData, unsigned NumBytes, u32 Offset);

/*********************************************************************
*
*       Types
*
**********************************************************************
*/
/* Context structure */
typedef struct {

    unsigned            NumBytesInBuffer;     /* 缓冲区中的剩余字节 */
    const u8           *pBuffer;              /* 指向缓冲区以读取数据的指针 */
    GUI_GET_DATA_FUNC *pfGetData;             /* 函数指针 */
    void               *pParam;               /* 传递给函数的参数指针*/
    u32                 Offset;                  /* 数据指针 */

    u8    aBuffer[258];                       /* 数据块的输入缓冲区 */
    short aCode  [(1 << MAX_NUM_LWZ_BITS)];   /* 这个数组存储压缩字符串的LZW代码*/
    u8    aPrefix[(1 << MAX_NUM_LWZ_BITS)];   /* LZW代码的前缀字符 */
    u8    aDecompBuffer[3000];                /* 解压缩缓冲区。压缩越高，缓冲区中需要的字节就越多. */
    u8   *sp;                                 /* 指向解压缓冲区的指针 */
    int   CurBit;
    int   LastBit;
    int   GetDone;
    int   LastByte;
    int   ReturnClear;
    int   CodeSize;
    int   SetCodeSize;
    int   MaxCode;
    int   MaxCodeSize;
    int   ClearCode;
    int   EndCode;
    int   FirstCode;
    int   OldCode;

    int BkColorIndex;
    /* 调色板缓冲区 */
    int aColorTable[256];
} GUI_GIF_CONTEXT;

typedef struct {
    int XPos;
    int YPos;
    int XSize;
    int YSize;
    int Flags;
    int NumColors;
} IMAGE_DESCRIPTOR;

/* 从内存中读取数据的默认参数结构 */
typedef struct {
    const u8 *pFileData;
    u32   FileSize;
} GUI_GIF_PARAM;



typedef struct {
    int xPos;
    int yPos;
    int xSize;
    int ySize;
    int Delay;
    int NumImages;
} GUI_GIF_IMAGE_INFO;

typedef struct {
    int xSize;
    int ySize;
    int NumImages;
} GUI_GIF_INFO;


enum {
    TYPE_RGB888 = 1,
    TYPE_RGB565
};
/*********************************************************************
*
*       Private data
*
**********************************************************************
*/
extern const int GUI_GIF__aInterlaceOffset[4];
extern const int GUI_GIF__aInterlaceYPos[4];

/*********************************************************************
*
*       Interface
*
**********************************************************************
*/
static int  GUI_GIF__ReadData(GUI_GIF_CONTEXT *pContext, unsigned NumBytes, const u8 **ppData, unsigned StartOfFile);
static int  GUI_GIF__GetData(void *p, const u8 **ppData, unsigned NumBytesReq, u32 Offset);
static void GUI_GIF__InitLZW(GUI_GIF_CONTEXT *pContext, int InputCodeSize);
static int  GUI_GIF__GetNextByte(GUI_GIF_CONTEXT *pContext);

int Gif_to_Picture(const void *pGIF, u32 NumBytes, GUI_GIF_IMAGE_INFO *Info, char *inbuf, int type, int Index);
char *Gif_to_Gif(const void *pGIF, u32 NumBytes, GUI_GIF_IMAGE_INFO *in_Info, GUI_GIF_IMAGE_INFO *out_Info, char *inbuf, int type, int Index);
int GUI_GIF_GetInfo(const void *pGIF, u32 NumBytes, GUI_GIF_IMAGE_INFO *pInfo);
#endif /* GUI_GIF_PRIVATE_H */


