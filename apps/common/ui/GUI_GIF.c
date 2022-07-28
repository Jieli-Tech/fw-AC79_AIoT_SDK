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
File        : GUI_GIF.c
Purpose     : Implementation of rendering GIF images
---------------------------END-OF-HEADER------------------------------
*/

#include <stdlib.h>
#include <string.h>

#ifdef CONFIG_UI_ENABLE

#include "GUI_GIF_Private.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
/* Input buffer configuration */
#ifndef   GUI_GIF_INPUT_BUFFER_SIZE
#define GUI_GIF_INPUT_BUFFER_SIZE 0
#endif

/* Constants for reading GIFs */
#define GIF_INTRO_TERMINATOR ';'
#define GIF_INTRO_EXTENSION  '!'
#define GIF_INTRO_IMAGE      ','

#define GIF_COMMENT     0xFE
#define GIF_APPLICATION 0xFF
#define GIF_PLAINTEXT   0x01
#define GIF_GRAPHICCTL  0xF9

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/
#if GUI_GIF_INPUT_BUFFER_SIZE
static u8 _aInputBuffer[GUI_GIF_INPUT_BUFFER_SIZE];
#endif

static const int _aMaskTbl[16] = {
    0x0000, 0x0001, 0x0003, 0x0007,
    0x000f, 0x001f, 0x003f, 0x007f,
    0x00ff, 0x01ff, 0x03ff, 0x07ff,
    0x0fff, 0x1fff, 0x3fff, 0x7fff,
};


/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static u8   _aBuffer[256];

/*********************************************************************
*
*       Private data
*
**********************************************************************
*/
const int GUI_GIF__aInterlaceOffset[4] = {  8, 8, 4, 2 };
const int GUI_GIF__aInterlaceYPos[4]   = {  0, 4, 2, 1 };

GUI_GIF_CONTEXT pContext;

/*********************************************************************
*
*       Static code
*
**********************************************************************
*//*********************************************************************
*
*       GUI__Read16
*/
static u16 GUI__Read16(const u8 **ppData)
{
    const u8 *pData;
    u16  Value;
    pData = *ppData;
    Value = *pData;
    Value |= (u16)(*(pData + 1) << 8);
    pData += 2;
    *ppData = pData;
    return Value;
}

/*********************************************************************
*
*       GUI__Read32
*/
static u32 GUI__Read32(const u8 **ppData)
{
    const u8 *pData;
    u32  Value;
    pData = *ppData;
    Value = *pData;
    Value |= ((u32) * (pData + 1) << 8);
    Value |= ((u32) * (pData + 2) << 16);
    Value |= ((u32) * (pData + 3) << 24);
    pData += 4;
    *ppData = pData;
    return Value;
}

/*********************************************************************
*
*       _ReadBytes
*
* Purpose:
*   Reads a string from the given pointer if possible and increments the pointer
*/
static void _ReadBytes(GUI_GIF_CONTEXT *pContext, u8 *pBuffer, int Len)
{
    const u8 *pData;
    if (GUI_GIF__ReadData(pContext, Len, &pData, 0)) {
        return; /* Error */
    }
    memcpy(pBuffer, pData, Len);
}

/*********************************************************************
*
*       _GetDataBlock
*
* Purpose:
*   Reads a LZW data block. The first byte contains the length of the block,
*   so the maximum length is 256 byte
*
* Return value:
*   Length of the data block
*/
static int _GetDataBlock(GUI_GIF_CONTEXT *pContext, u8 *pBuffer)
{
    u8 Count;
    const u8 *pData;
    if (GUI_GIF__ReadData(pContext, 1, &pData, 0)) {
        return 0; /* Error */
    }
    Count = *pData; /* Read the length of the data block */
    if (Count) {
        if (pBuffer) {
            _ReadBytes(pContext, pBuffer, Count);
        } else {
            if (GUI_GIF__ReadData(pContext, Count, &pData, 0)) {
                return 0; /* Error */
            }
        }
    }
    return ((int)Count);
}

/*********************************************************************
*
*       _GetNextCode
*
* Purpose:
*   Returns the next LZW code from the LZW stack. One LZW code contains up to 12 bits.
*
* Return value:
*   >= 0 if succeed
*   <  0 if not succeed
*/
static int _GetNextCode(GUI_GIF_CONTEXT *pContext)
{
    int i, j, End;
    long Result;
    if (pContext->ReturnClear) {
        /* The first code should be a clear code. */
        pContext->ReturnClear = 0;
        return pContext->ClearCode;
    }
    End = pContext->CurBit + pContext->CodeSize;
    if (End >= pContext->LastBit) {
        int Count;
        if (pContext->GetDone) {
            return -1; /* Error */
        }
        pContext->aBuffer[0] = pContext->aBuffer[pContext->LastByte - 2];
        pContext->aBuffer[1] = pContext->aBuffer[pContext->LastByte - 1];
        if ((Count = _GetDataBlock(pContext, &pContext->aBuffer[2])) == 0) {
            pContext->GetDone = 1;
        }
        if (Count < 0) {
            return -1; /* Error */
        }
        pContext->LastByte = 2 + Count;
        pContext->CurBit   = (pContext->CurBit - pContext->LastBit) + 16;
        pContext->LastBit  = (2 + Count) * 8 ;
        End                  = pContext->CurBit + pContext->CodeSize;
    }
    j = End >> 3;
    i = pContext->CurBit >> 3;
    if (i == j) {
        Result = (long)pContext->aBuffer[i];
    } else if (i + 1 == j) {
        Result = (long)pContext->aBuffer[i] | ((long)pContext->aBuffer[i + 1] << 8);
    } else {
        Result = (long)pContext->aBuffer[i] | ((long)pContext->aBuffer[i + 1] << 8) | ((long)pContext->aBuffer[i + 2] << 16);
    }
    Result = (Result >> (pContext->CurBit & 0x7)) & _aMaskTbl[pContext->CodeSize];
    pContext->CurBit += pContext->CodeSize;
    return (int)Result;
}

/*********************************************************************
*
*       _ReadExtension
*
* Purpose:
*   Reads an extension block. One extension block can consist of several data blocks.
*   If an unknown extension block occures, the routine failes.
*/
static int _ReadExtension(GUI_GIF_CONTEXT *pContext, int *pTransIndex, GUI_GIF_IMAGE_INFO *pInfo, u8 *pDisposal)
{
    u8 Label;
    const u8 *pData;
    if (GUI_GIF__ReadData(pContext, 1, &pData, 0)) {
        return 1; /* Error */
    }
    Label = *pData;
    switch (Label) {
    case GIF_PLAINTEXT:
    case GIF_APPLICATION:
    case GIF_COMMENT:
        while (_GetDataBlock(pContext, _aBuffer) > 0);
        return 0;
    case GIF_GRAPHICCTL:
        if (_GetDataBlock(pContext, _aBuffer) != 4) { /* Length of a graphic control block must be 4 */
            return 1;
        }
        if (pInfo) {
            pInfo->Delay    = (_aBuffer[2] << 8) | _aBuffer[1];
        }
        if (pDisposal) {
            *pDisposal = (_aBuffer[0] >> 2) & 0x7;
        }
        if (pTransIndex) {
            *pTransIndex = -1;
            if ((_aBuffer[0] & 0x1) != 0) {
                *pTransIndex = _aBuffer[3];
            }
        }
        /* Skip block terminator, must be 0 */
        if (GUI_GIF__ReadData(pContext, 1, &pData, 0)) {
            return 1; /* Error */
        }
        if (*pData) {
            return 1; /* Error */
        }
        return 0;
    }
    return 1; /* Error */
}

/*********************************************************************
*
*       _ReadComment
*
* Purpose:
*   Reads a comment from the extension block if available and returns the number
*   of comment bytes.
*/
static int _ReadComment(GUI_GIF_CONTEXT *pContext, u8 *pBuffer, int MaxSize, int *pSize)
{
    u8 Label;
    int Size;
    const u8 *pData;
    if (GUI_GIF__ReadData(pContext, 1, &pData, 0)) {
        return 1; /* Error */
    }
    Label = *pData;
    switch (Label) {
    case GIF_PLAINTEXT:
    case GIF_APPLICATION:
        while (_GetDataBlock(pContext, _aBuffer) > 0);
        return 0;
    case GIF_COMMENT:
        Size = _GetDataBlock(pContext, _aBuffer);
        if (Size > MaxSize) {
            Size = MaxSize;
        }
        if (pBuffer) {
            *pSize = Size;
            memcpy(pBuffer, _aBuffer, Size);
        }
        return 0;
    case GIF_GRAPHICCTL:
        if (_GetDataBlock(pContext, _aBuffer) != 4) { /* Length of a graphic control block must be 4 */
            return 1;
        }
        /* Skip block terminator, must be 0 */
        if (GUI_GIF__ReadData(pContext, 1, &pData, 0)) {
            return 1; /* Error */
        }
        if (*pData) {
            return 1; /* Error */
        }
        return 0;
    }
    return 1; /* Error */
}

/*********************************************************************
*
*       _ReadColorMap
*/
static int _ReadColorMap(GUI_GIF_CONTEXT *pContext, int NumColors)
{
    int i;
    for (i = 0; i < NumColors; i++) {
        u8 r, g, b;
        const u8 *pData;
        if (GUI_GIF__ReadData(pContext, 3, &pData, 0)) {
            return 1; /* Error */
        }
        r = *(pData + 0);
        g = *(pData + 1);
        b = *(pData + 2);
        pContext->aColorTable[i] = r | ((u16)g << 8) | ((u32)b << 16);
    }
    return 0;
}

/*********************************************************************
*
*       _InitGIFDecoding
*
* Purpose:
*   The routine initializes the static SOURCE structure and checks
*   if the file is a legal GIF file.
*
* Return value:
*   0 on success, 1 on error
*/
static int _InitGIFDecoding(GUI_GIF_CONTEXT *pContext)
{
    const u8 *pData;
    if (GUI_GIF__ReadData(pContext, 6, &pData, 1)) {
        return 1; /* Error */
    }
    /* Check if the file is a legal GIF file by checking the 6 byte file header */
    if ((*(pData + 0) != 'G') ||
        (*(pData + 1) != 'I') ||
        (*(pData + 2) != 'F') ||
        (*(pData + 3) != '8') ||
        ((*(pData + 4) != '7') && (*(pData + 4) != '9')) ||
        (*(pData + 5) != 'a')) {
        return 1;
    }
    return 0;
}

/*********************************************************************
*
*       _GetImageDimension
*
* Purpose:
*   Reads the image dimension from the logical screen descriptor
*
* Return value:
*   0 on success, 1 on error
*/
static int _GetImageDimension(GUI_GIF_CONTEXT *pContext, int *pxSize, int *pySize)
{
    int XSize, YSize;
    const u8 *pData;
    /* Initialize decoding */
    if (_InitGIFDecoding(pContext)) {
        return 1; /* Error */
    }
    /* Get data */
    if (GUI_GIF__ReadData(pContext, 4, &pData, 0)) {
        return 1; /* Error */
    }
    /* Read image size */
    XSize = GUI__Read16(&pData);
    YSize = GUI__Read16(&pData);
    if ((XSize > 2000) || (YSize > 2000)) {
        return 1; /* Error if image is too large */
    }
    if (pxSize) {
        *pxSize = XSize;
    }
    if (pySize) {
        *pySize = YSize;
    }
    return 0;
}

/*********************************************************************
*
*       _GetGlobalColorTable
*
* Purpose:
*   Reads the global color table if there is one. Returns the number of
*   available colors over the pointer pNumColors (can be NULL).
*
* Return value:
*   0 on success, 1 on error
*/
static int _GetGlobalColorTable(GUI_GIF_CONTEXT *pContext, int *pNumColors)
{
    u8 Flags;
    int NumColors;
    const u8 *pData;
    if (GUI_GIF__ReadData(pContext, 3, &pData, 0)) {
        return 1; /* Error */
    }
    /* Read flags from logical screen descriptor */
    Flags = *pData;
    NumColors = 2 << (Flags & 0x7);
    if (Flags & 0x80) {
        pContext->pBuffer = pData + 1;
        /* Read global color table */
        if (_ReadColorMap(pContext, NumColors)) {
            return 1; /* Error */
        }
        pContext->BkColorIndex = pContext->aColorTable[pContext->pBuffer[0]];
    }
    if (pNumColors) {
        *pNumColors = NumColors;
    }
    return 0;
}

/*********************************************************************
*
*       _GetSizeAndColorTable
*/
static int _GetSizeAndColorTable(GUI_GIF_CONTEXT *pContext, int *pxSize, int *pySize, int *pNumColors)
{
    /* Get image size */
    if (_GetImageDimension(pContext, pxSize, pySize)) {
        return 1; /* Error */
    }
    /* Get global color table (if available) */
    if (_GetGlobalColorTable(pContext, pNumColors)) {
        return 1; /* Error */
    }
    return 0;
}

/*********************************************************************
*
*       _SkipLocalColorTable
*/
static void _SkipLocalColorTable(GUI_GIF_CONTEXT *pContext)
{
    u8 Flags;
    const u8 *pData;
    if (GUI_GIF__ReadData(pContext, 9, &pData, 0)) {
        return; /* Error */
    }
    Flags = *(pData + 8);           /* Skip the first 8 bytes of the image descriptor, only 'Flags' are intresting */
    if (Flags & 0x80) {
        /* Skip local color table */
        int NumBytes, RemBytes, NumColors;
        NumColors = 2 << (Flags & 0x7);
        RemBytes = NumColors * 3 + 1; /* Skip colors (Number of colors * 3) and the codesize byte */
        while (RemBytes) {
            if (RemBytes > 256) {
                NumBytes = 256;
            } else {
                NumBytes = RemBytes;
            }
            if (GUI_GIF__ReadData(pContext, NumBytes, &pData, 0)) {
                return; /* Error */
            }
            RemBytes -= NumBytes;
        }
    } else {
        /* Skip codesize */
        if (GUI_GIF__ReadData(pContext, 1, &pData, 0)) {
            return; /* Error */
        }
    }
}


/*********************************************************************
*
*       _GetImageInfo
*/
static int _GetImageInfo(GUI_GIF_CONTEXT *pContext, GUI_GIF_IMAGE_INFO *pInfo, int Index)
{
    u8 Introducer;
    int NumColors, ImageCnt;
    const u8 *pData;
    /* Initialize decoding and get size and global color table */
    if (_GetSizeAndColorTable(pContext, NULL, NULL, &NumColors)) {
        return 1; /* Error */
    }
    ImageCnt = 0;
    /* Iterate over the blocks */
    do {
        if (GUI_GIF__ReadData(pContext, 1, &pData, 0)) {
            return 1; /* Error */
        }
        Introducer = *pData;
        switch (Introducer) {
        case GIF_INTRO_IMAGE:
            if (Index == ImageCnt) {
                if (GUI_GIF__ReadData(pContext, 8, &pData, 0)) {
                    return 1; /* Error */
                }
                pInfo->xPos  = GUI__Read16(&pData);
                pInfo->xPos  = GUI__Read16(&pData);
                pInfo->xSize = GUI__Read16(&pData);
                pInfo->ySize = GUI__Read16(&pData);
                return 0;
            }
            _SkipLocalColorTable(pContext);
            while (_GetDataBlock(pContext, 0) > 0); /* Skip data blocks */
            ImageCnt++;
            break;
        case GIF_INTRO_TERMINATOR:
            break;
        case GIF_INTRO_EXTENSION:
            if (_ReadExtension(pContext, NULL, (Index == ImageCnt) ? pInfo : NULL, NULL)) {
                return 1;
            }
            break;
        default:
            return 1;
        }
    } while (Introducer != GIF_INTRO_TERMINATOR); /* We do not support more than one image, so stop when the first terminator has been read */
    return 0;
}

/*********************************************************************
*
*       _GetGIFComment
*
* Purpose:
*   Returns the given comment of the GIF image.
*
* Parameters:
*   pData            - Pointer to start of the GIF file
*   NumBytes         - Number of bytes in the file
*   pBuffer          - Pointer to buffer to be filled by the routine
*   MaxSize          - Number of bytes in buffer
*   Index            - Index of the comment to be returned
*
* Return value:
*   0 on success, 1 on error
*/
static int _GetGIFComment(GUI_GIF_CONTEXT *pContext, u8 *pBuffer, int MaxSize, int Index)
{
    u8 Introducer;
    int NumColors, CommentCnt, Size;
    const u8 *pData;
    /* Initialize decoding and skip size and global color table */
    if (_GetSizeAndColorTable(pContext, NULL, NULL, &NumColors)) {
        return 1; /* Error */
    }
    CommentCnt = Size = 0;
    /* Iterate over the blocks */
    do {
        if (GUI_GIF__ReadData(pContext, 1, &pData, 0)) {
            return 1; /* Error */
        }
        Introducer = *pData;
        switch (Introducer) {
        case GIF_INTRO_IMAGE:
            _SkipLocalColorTable(pContext);
            while (_GetDataBlock(pContext, 0) > 0); /* Skip data blocks */
            break;
        case GIF_INTRO_TERMINATOR:
            break;
        case GIF_INTRO_EXTENSION:
            _ReadComment(pContext, (Index == CommentCnt) ? pBuffer : NULL, MaxSize, &Size);
            if ((Size) && (Index == CommentCnt)) {
                return 0;
            }
            break;
        default:
            return 1;
        }
    } while (Introducer != GIF_INTRO_TERMINATOR); /* We do not support more than one image, so stop when the first terminator has been read */
    return 1;
}


/*********************************************************************
*
*       Private code
*
**********************************************************************
*/
/*********************************************************************
*
*       GUI_GIF__GetNextByte
*
* Purpose:
*   Reads the next LZW code from the LZW stack and returns the first byte from the LZW code.
*
* Return value:
*   >= 0 if succeed
*   -1   if not succeed
*   -2   if end code has been read
*/
static int GUI_GIF__GetNextByte(GUI_GIF_CONTEXT *pContext)
{
    int i, Code, Incode;
    while ((Code = _GetNextCode(pContext)) >= 0) {
        if (Code == pContext->ClearCode) {
            /* Corrupt GIFs can make this happen */
            if (pContext->ClearCode >= (1 << MAX_NUM_LWZ_BITS)) {
                return -1; /* Error */
            }
            /* Clear the tables */
            memset((u8 *)pContext->aCode, 0, sizeof(pContext->aCode));
            for (i = 0; i < pContext->ClearCode; ++i) {
                pContext->aPrefix[i] = i;
            }
            /* Calculate the 'special codes' in dependence of the initial code size
               and initialize the stack pointer */
            pContext->CodeSize    = pContext->SetCodeSize + 1;
            pContext->MaxCodeSize = pContext->ClearCode << 1;
            pContext->MaxCode     = pContext->ClearCode + 2;
            pContext->sp          = pContext->aDecompBuffer;
            /* Read the first code from the stack after clearing and initializing */
            do {
                pContext->FirstCode = _GetNextCode(pContext);
            } while (pContext->FirstCode == pContext->ClearCode);
            pContext->OldCode = pContext->FirstCode;
            return pContext->FirstCode;
        }
        if (Code == pContext->EndCode) {
            return -2; /* End code */
        }
        Incode = Code;
        if (Code >= pContext->MaxCode) {
            *(pContext->sp)++ = pContext->FirstCode;
            Code = pContext->OldCode;
        }
        while (Code >= pContext->ClearCode) {
            *(pContext->sp)++ = pContext->aPrefix[Code];
            if (Code == pContext->aCode[Code]) {
                return Code;
            }
            if ((pContext->sp - pContext->aDecompBuffer) >= sizeof(pContext->aDecompBuffer)) {
                return Code;
            }
            Code = pContext->aCode[Code];
        }
        *(pContext->sp)++ = pContext->FirstCode = pContext->aPrefix[Code];
        if ((Code = pContext->MaxCode) < (1 << MAX_NUM_LWZ_BITS)) {
            pContext->aCode  [Code] = pContext->OldCode;
            pContext->aPrefix[Code] = pContext->FirstCode;
            ++pContext->MaxCode;
            if ((pContext->MaxCode >= pContext->MaxCodeSize) && (pContext->MaxCodeSize < (1 << MAX_NUM_LWZ_BITS))) {
                pContext->MaxCodeSize <<= 1;
                ++pContext->CodeSize;
            }
        }
        pContext->OldCode = Incode;
        if (pContext->sp > pContext->aDecompBuffer) {
            return *--(pContext->sp);
        }
    }
    return Code;
}

/*********************************************************************
*
*       GUI_GIF__InitLZW
*
* Purpose:
*   Initializes the given LZW with the input code size
*/
static void GUI_GIF__InitLZW(GUI_GIF_CONTEXT *pContext, int InputCodeSize)
{
    pContext->SetCodeSize  = InputCodeSize;
    pContext->CodeSize     = InputCodeSize + 1;
    pContext->ClearCode    = (1 << InputCodeSize);
    pContext->EndCode      = (1 << InputCodeSize) + 1;
    pContext->MaxCode      = (1 << InputCodeSize) + 2;
    pContext->MaxCodeSize  = (1 << InputCodeSize) << 1;
    pContext->ReturnClear  = 1;
    pContext->LastByte     = 2;
    pContext->sp           = pContext->aDecompBuffer;
}

/*********************************************************************
*
*       GUI_GIF__ReadData
*/
static int GUI_GIF__ReadData(GUI_GIF_CONTEXT *pContext, unsigned NumBytesReq, const u8 **ppData, unsigned StartOfFile)
{
#if GUI_GIF_INPUT_BUFFER_SIZE
    if (StartOfFile) {
        pContext->NumBytesInBuffer = 0;
    }
    /* Check if there are sufficient bytes in buffer  */
    if (pContext->NumBytesInBuffer < NumBytesReq) {
        /* Fill up to maximum size */
        unsigned NumBytesRead;
        NumBytesRead = GUI_GIF_INPUT_BUFFER_SIZE - pContext->NumBytesInBuffer;
        if (pContext->NumBytesInBuffer) {
            memmove(_aInputBuffer, pContext->pBuffer, pContext->NumBytesInBuffer); /* Shift the remaining few bytes from top to bottom */
        }
        pContext->pBuffer = _aInputBuffer + pContext->NumBytesInBuffer;
        if ((pContext->NumBytesInBuffer += pContext->pfGetData(pContext->pParam, NumBytesRead, ppData, StartOfFile)) < NumBytesReq) {
            return 1;
        }
        memcpy((void *)pContext->pBuffer, (const void *)*ppData, NumBytesRead);
        pContext->pBuffer = _aInputBuffer;
    }
    *ppData = pContext->pBuffer;
    pContext->pBuffer          += NumBytesReq;
    pContext->NumBytesInBuffer -= NumBytesReq;
    return 0;
#else
    if (StartOfFile) {
        pContext->Offset = 0;
    }
    if ((unsigned)pContext->pfGetData(pContext->pParam, ppData, NumBytesReq, pContext->Offset) != NumBytesReq) {
        return 1;
    }
    pContext->Offset += NumBytesReq;
    /*
    if ((unsigned)pContext->pfGetData(pConte Points to a IMAGE_DESCRIPTOR structure, which contains infos about size, colors and interlacing.
    *   x0, y0       - Obvious.xt->pParam, NumBytesReq, ppData, StartOfFile) != NumBytesReq) {
      return 1;
    }
    */
    return 0;
#endif
}

/*********************************************************************
*
*       GUI_GIF__GetData
*/
static int GUI_GIF__GetData(void *p, const u8 **ppData, unsigned NumBytesReq, u32 Offset)
{
    int RemBytes, NumBytes;
    GUI_GIF_PARAM *pParam;
    pParam = (GUI_GIF_PARAM *)p;
    RemBytes = pParam->FileSize - Offset;
    NumBytes = 0;
    if (RemBytes > 0) {
        NumBytes = ((unsigned)RemBytes > NumBytesReq) ? NumBytesReq : RemBytes;
        *ppData  = pParam->pFileData + Offset;
    }
    return NumBytes;
}
/*********************************************************************
*
*       Public code
*
**********************************************************************/



/*********************************************************************
*
*       _GetGIFInfo
*/
static int _GetGIFInfo(GUI_GIF_CONTEXT *pContext, GUI_GIF_IMAGE_INFO *pInfo)
{
    u8 Introducer;
    int NumColors, ImageCnt;
    const u8 *pData;
    /* Initialize decoding and get size and global color table */
    if (_GetSizeAndColorTable(pContext, &pInfo->xSize, &pInfo->ySize, &NumColors)) {
        return 1; /* Error */
    }
    ImageCnt = 0;
    /* Iterate over the blocks */
    do {
        if (GUI_GIF__ReadData(pContext, 1, &pData, 0)) {
            return 1; /* Error */
        }
        Introducer = *pData;
        switch (Introducer) {
        case GIF_INTRO_IMAGE:           // ,    2C
            _SkipLocalColorTable(pContext);
            while (_GetDataBlock(pContext, 0) > 0); /* Skip data blocks */
            ImageCnt++;
            pInfo->NumImages = ImageCnt;
            break;
        case GIF_INTRO_TERMINATOR:     // ;    3B
            break;
        case GIF_INTRO_EXTENSION:      // !    21
            if (_ReadExtension(pContext, NULL, (ImageCnt == 0) ?  pInfo : NULL, NULL)) { /* Skip image extension */
                return 1;
            }
            break;
        default:
            return 1;
        }
    } while (Introducer != GIF_INTRO_TERMINATOR); /* We do not support more than one image, so stop when the first terminator has been read */
    pInfo->NumImages = ImageCnt;
    return 0;
}


/*********************************************************************
*
*       GUI_GIF_GetInfo
*/
int GUI_GIF_GetInfo(const void *pGIF, u32 NumBytes, GUI_GIF_IMAGE_INFO *pInfo)
{
    int r = 1;
    GUI_GIF_PARAM param = {0};

    param.pFileData 	= (const u8 *)pGIF;
    param.FileSize	= NumBytes;

    memset(&pContext, 0, sizeof(pContext));
    pContext.pfGetData 	= GUI_GIF__GetData;
    pContext.pParam 	= &param;
    r = _GetGIFInfo(&pContext, pInfo);
    return r;
}






/*******************添加自定义函数******************************/



/***做动图***/
static char *Gif_DrawFromData(GUI_GIF_CONTEXT *pContext, IMAGE_DESCRIPTOR *pDescriptor, GUI_GIF_IMAGE_INFO *Info, int Transparency, int Disposal, char *inbuf, int type)
{
    int Codesize, Index, XPos, YPos, YCnt, XCnt, XEnd, YEnd, Pass = 0, Interlace, type_cnt;
    int XSize, YSize, Width, Height, NumColors, BkColorIndex, ColorIndex;
    const int *pTrans;
    const u8 *pData;

    Width 		= Info->xSize;
    Height		= Info->ySize;
    XPos        = pDescriptor->XPos;
    YPos        = pDescriptor->YPos;
    XSize       = pDescriptor->XSize;
    YSize       = pDescriptor->YSize;
    NumColors   = pDescriptor->NumColors;
    XEnd        = XSize + XPos;
    YEnd        = YSize + YPos ;

    char *buf = inbuf;
    if (type == 1) {
        type_cnt = 3;
    }

    else if (type == 2) {
        type_cnt = 2;
    } else {
        printf("type not match\r\n");
        return NULL;
    }
    char *out_buf = (char *)calloc(1, XSize * YSize * type_cnt + 1);
    if (!out_buf) {
        printf("out_buf malloc failed\r\n");
        return NULL;
    }
    char *tmp_buf  = out_buf;



    pTrans = pContext->aColorTable;     //颜色表起始地址

    BkColorIndex = pContext->BkColorIndex;

    if (GUI_GIF__ReadData(pContext, 1, &pData, 0)) {
        return NULL; /* Error */
    }
    Codesize  = *pData;                    /* Read the LZW codesize */
    GUI_GIF__InitLZW(pContext, Codesize);                    /* Initialize the LZW stack with the LZW codesize */
    Interlace = (pDescriptor->Flags & 0x40) ? 1 : 0; /* Evaluate if image is interlaced */
    for (YCnt = 0; YCnt < YEnd; YCnt++) {
        if (YCnt < YPos) {
            buf = inbuf + Width * YPos * type_cnt;
            YCnt = YPos;
        }

        if (XPos > 0) {
            buf += XPos * type_cnt;
        }

        for (XCnt = 0; XCnt < XSize; XCnt++) {
            if (pContext->sp > pContext->aDecompBuffer) {
                Index = *--(pContext->sp);
            } else {
                Index = GUI_GIF__GetNextByte(pContext);
            }

            if (Index == -2) {
                printf("Index == -2\n");
                return NULL; /* End code */
            }
            if ((Index < 0) || (Index >= NumColors)) {
                printf("Index <0 || > NumColors\n");
                return NULL; /* If Index out of legal range stop decompressing, error */
            }
            //将读到的索引引向颜色表
            if (Index != Transparency || Disposal == 2) {
                if (Index != Transparency) {
                    ColorIndex = *(pTrans + Index);
                } else {
                    ColorIndex = BkColorIndex;
                }
                if (type == 1) {
                    *(buf + 0) = (ColorIndex >> 0) & 0xff; //R

                    *(buf + 1) = (ColorIndex >> 8) & 0xff; //G

                    *(buf + 2) = (ColorIndex >> 16) & 0xff; //B
                } else if (type == 2) {
                    *(buf + 0) = ((ColorIndex >> 0) & 0xf8) | (((ColorIndex >> 8) & 0xe0) >> 5);
                    *(buf + 1) = (((ColorIndex >> 8) & 0x1c) << 3) | (((ColorIndex >> 16) & 0xf8) >> 3);
                }
            }

            buf += type_cnt;
        }//end X
        if (Width > (XSize + XPos)) {
            buf += (Width - XSize - XPos) * type_cnt;
        }

        if (Interlace) {
            buf += (GUI_GIF__aInterlaceOffset[Pass] - 1) * Width * type_cnt;
            if ((buf - inbuf) >= type_cnt * Width * YEnd) {
                ++Pass;
                buf = inbuf + (GUI_GIF__aInterlaceYPos[Pass]) * Width * type_cnt;
            }
        }
    }//end Y

    buf = inbuf;	//init buf address
    buf = inbuf + Width * YPos * type_cnt;
    buf += XPos * type_cnt;

    /* for(YCnt = YPos; YCnt < YEnd; YCnt++) */
    /* { */
    /* memcpy(out_buf,inbuf+Width*YCnt*type_cnt+XPos*type_cnt,XSize*type_cnt); */
    /* out_buf += XSize*type_cnt; */
    /* } */

    for (YCnt = YPos; YCnt < YEnd; YCnt++) {
        memcpy(tmp_buf, buf, XSize * type_cnt);
        buf += Width * type_cnt;
        tmp_buf += XSize * type_cnt;
    }

    return out_buf;
}

/****做图片***/
static int Picture_DrawFromData(GUI_GIF_CONTEXT *pContext, IMAGE_DESCRIPTOR *pDescriptor, GUI_GIF_IMAGE_INFO *Info, int Transparency, int Disposal, char *inbuf, int type)
{
    int Codesize, Index, XPos, YPos, YCnt, XCnt, XEnd, YEnd, Pass = 0, Interlace, type_cnt;
    int XSize, YSize, Width, Height, NumColors, BkColorIndex, ColorIndex;
    const int *pTrans;
    const u8 *pData;

    Width 		= Info->xSize;
    Height		= Info->ySize;
    XPos        = pDescriptor->XPos;
    YPos        = pDescriptor->YPos;
    XSize       = pDescriptor->XSize;
    YSize       = pDescriptor->YSize;
    NumColors   = pDescriptor->NumColors;
    XEnd        = XSize + XPos;
    YEnd        = YSize + YPos ;

    char *buf = inbuf;
    if (type == 1) {
        type_cnt = 3;
    }

    else if (type == 2) {
        type_cnt = 2;
    } else {
        return -1;
    }



    pTrans = pContext->aColorTable;     //颜色表起始地址

    BkColorIndex = pContext->BkColorIndex;

    if (GUI_GIF__ReadData(pContext, 1, &pData, 0)) {
        return -1; /* Error */
    }
    Codesize  = *pData;                    /* Read the LZW codesize */
    GUI_GIF__InitLZW(pContext, Codesize);                    /* Initialize the LZW stack with the LZW codesize */
    Interlace = (pDescriptor->Flags & 0x40) ? 1 : 0; /* Evaluate if image is interlaced */
    for (YCnt = 0; YCnt < YEnd; YCnt++) {
        if (YCnt < YPos) {
            buf = inbuf + Width * YPos * type_cnt;
            YCnt = YPos;
        }

        if (XPos > 0) {
            buf += XPos * type_cnt;
        }

        for (XCnt = 0; XCnt < XSize; XCnt++) {
            if (pContext->sp > pContext->aDecompBuffer) {
                Index = *--(pContext->sp);
            } else {
                Index = GUI_GIF__GetNextByte(pContext);
            }

            if (Index == -2) {
                printf("Index == -2\n");
                return -2; /* End code */
            }
            if ((Index < 0) || (Index >= NumColors)) {
                printf("Index <0 || > NumColors\n");
                return -3; /* If Index out of legal range stop decompressing, error */
            }

            if (XCnt == 31 && YCnt == 60) {
                XCnt = XCnt;
            }

            //将读到的索引引向颜色表
            if (Index != Transparency || Disposal == 2) {
                if (Index != Transparency) {
                    ColorIndex = *(pTrans + Index);
                } else {
                    ColorIndex = BkColorIndex;
                }
                if (type == 1) {
                    *(buf + 0) = (ColorIndex >> 0) & 0xff; //R

                    *(buf + 1) = (ColorIndex >> 8) & 0xff; //G

                    *(buf + 2) = (ColorIndex >> 16) & 0xff; //B
                } else if (type == 2) {
                    *(buf + 0) = ((ColorIndex >> 0) & 0xf8) | (((ColorIndex >> 8) & 0xe0) >> 5);
                    *(buf + 1) = (((ColorIndex >> 8) & 0x1c) << 3) | (((ColorIndex >> 16) & 0xf8) >> 3);
                }
            }

            buf += type_cnt;
        }//end X
        if (Width > (XSize + XPos)) {
            buf += (Width - XSize - XPos) * type_cnt;
        }


        if (Interlace) {
            buf += (GUI_GIF__aInterlaceOffset[Pass] - 1) * Width * type_cnt;
            if ((buf - inbuf) >= type_cnt * Width * YEnd) {
                ++Pass;
                buf = inbuf + (GUI_GIF__aInterlaceYPos[Pass] + YPos) * Width * type_cnt;
            }
        }




    }//end Y

    /* free(pContext); */

    return 0;
}

/**这个做gif动图***/
static char *Gif_Maker(GUI_GIF_CONTEXT *pContext, GUI_GIF_IMAGE_INFO *Info1, GUI_GIF_IMAGE_INFO *Info2, char *inbuf, int type, int Index)
{
    u8 Disposal = 0;
    int XSize, YSize, TransIndex = 0, ImageCnt = 0 ;
    IMAGE_DESCRIPTOR Descriptor = {0};
    u8 Introducer;
    const u8 *pData;

    /* Initialize decoding and get size and global color table */
    if (_GetSizeAndColorTable(pContext, &XSize, &YSize, &Descriptor.NumColors)) {
        return NULL; /* Error */
    }
    /* Iterate over the blocks */
    do {
        if (GUI_GIF__ReadData(pContext, 1, &pData, 0)) {
            return NULL; /* Error */
        }
        Introducer = *pData;
        switch (Introducer) {
        case GIF_INTRO_IMAGE:   // ,    2C
            /* Read image descriptor */
            if (GUI_GIF__ReadData(pContext, 9, &pData, 0)) {
                return NULL; /* Error */
            }

            Info2->xPos 	= Descriptor.XPos  = GUI__Read16(&pData);
            Info2->yPos 	= Descriptor.YPos  = GUI__Read16(&pData);
            Info2->xSize 	= Descriptor.XSize = GUI__Read16(&pData);
            Info2->ySize	= Descriptor.YSize = GUI__Read16(&pData);

            Descriptor.Flags = *pData;
            if (Descriptor.Flags & 0x80) {
                /* Read local color table */
                Descriptor.NumColors = 2 << (Descriptor.Flags & 0x7);
                if (_ReadColorMap(pContext, Descriptor.NumColors)) {
                    return NULL; /* Error */
                }
            }

            if (Index == ImageCnt) {
                char *out_buf;
                out_buf = Gif_DrawFromData(pContext, &Descriptor, Info1, TransIndex, Disposal, inbuf, type);

                /* Skip block terminator, must be 0 */
                if (GUI_GIF__ReadData(pContext, 1, &pData, 0)) {
                    return NULL; /* Error */
                }
                return out_buf;

            } else {
                /* Skip codesize */
                if (GUI_GIF__ReadData(pContext, 1, &pData, 0)) {
                    return NULL; /* Error */
                }
                while (_GetDataBlock(pContext, 0) > 0); /* Skip data blocks */
            }
            ImageCnt++;
            break;
        case GIF_INTRO_TERMINATOR:  // ;    3B
            break;
        case GIF_INTRO_EXTENSION:    // !    21
            /* Read image extension */
            if (_ReadExtension(pContext, &TransIndex, (Index == ImageCnt) ? Info1 : NULL, (Index == ImageCnt) ? &Disposal : NULL)) {
                return NULL;
            }
            break;
        default:
            return NULL;
        }
    } while (Introducer != GIF_INTRO_TERMINATOR); /* We do not support more than one image, so stop when the first terminator has been read */
    return NULL;
}



/****这个做图片获取***/
static int Picture_Maker(GUI_GIF_CONTEXT *pContext, GUI_GIF_IMAGE_INFO *Info, char *inbuf, int type, int Index)
{
    u8 Disposal = 0;
    int XSize, YSize, TransIndex = 0, ImageCnt = 0, ret = 0;
    IMAGE_DESCRIPTOR Descriptor = {0};
    u8 Introducer;
    const u8 *pData;


    /* Initialize decoding and get size and global color table */
    if (_GetSizeAndColorTable(pContext, &XSize, &YSize, &Descriptor.NumColors)) {
        return -1; /* Error */
    }
    /* Iterate over the blocks */
    do {
        if (GUI_GIF__ReadData(pContext, 1, &pData, 0)) {
            return -1; /* Error */
        }
        Introducer = *pData;
        switch (Introducer) {
        case GIF_INTRO_IMAGE:   // ,    2C
            /* Read image descriptor */
            if (GUI_GIF__ReadData(pContext, 9, &pData, 0)) {
                return -1; /* Error */
            }

            Descriptor.XPos  = GUI__Read16(&pData);
            Descriptor.YPos  = GUI__Read16(&pData);
            Descriptor.XSize = GUI__Read16(&pData);
            Descriptor.YSize = GUI__Read16(&pData);
            Descriptor.Flags = *pData;
            if (Descriptor.Flags & 0x80) {
                /* Read local color table */
                Descriptor.NumColors = 2 << (Descriptor.Flags & 0x7);
                if (_ReadColorMap(pContext, Descriptor.NumColors)) {
                    return -1; /* Error */
                }
            }
            if (Index == ImageCnt) {
                if (Picture_DrawFromData(pContext, &Descriptor, Info, TransIndex, Disposal, inbuf, type)) {
                    return -1;
                }

                /* Skip block terminator, must be 0 */
                if (GUI_GIF__ReadData(pContext, 1, &pData, 0)) {
                    return -1; /* Error */
                }

                return 0;
            } else {
                /* Skip codesize */
                if (GUI_GIF__ReadData(pContext, 1, &pData, 0)) {
                    return -1; /* Error */
                }
                while (_GetDataBlock(pContext, 0) > 0); /* Skip data blocks */
            }
            ImageCnt++;
            break;
        case GIF_INTRO_TERMINATOR:  // ;    3B
            break;
        case GIF_INTRO_EXTENSION:    // !    21
            /* Read image extension */
            if (_ReadExtension(pContext, &TransIndex, NULL, NULL)) {
                return -1;
            }
            break;
        default:
            return -1;
        }
    } while (Introducer != GIF_INTRO_TERMINATOR); /* We do not support more than one image, so stop when the first terminator has been read */
    return 0;
}



char *Gif_to_Gif(const void *pGIF, u32 NumBytes, GUI_GIF_IMAGE_INFO *in_Info, GUI_GIF_IMAGE_INFO *out_Info, char *inbuf, int type, int Index)
{
    GUI_GIF_PARAM Param = {0};
    Param.pFileData = (const u8 *)pGIF;
    Param.FileSize  = NumBytes;

    memset(&pContext, 0, sizeof(pContext));
    pContext.pfGetData = GUI_GIF__GetData;
    pContext.pParam    = &Param;

    return Gif_Maker(&pContext, in_Info, out_Info, inbuf, type, Index);
}

int Gif_to_Picture(const void *pGIF, u32 NumBytes, GUI_GIF_IMAGE_INFO *Info, char *inbuf, int type, int Index)
{
    GUI_GIF_PARAM Param = {0};
    Param.pFileData = (const u8 *)pGIF;
    Param.FileSize  = NumBytes;

    memset(&pContext, 0, sizeof(pContext));
    pContext.pfGetData = GUI_GIF__GetData;
    pContext.pParam    = &Param;

    if (Picture_Maker(&pContext, Info, inbuf, type, Index)) {
        return -1;
    }
    return 0;
}

#endif

/*************************** End of file ****************************/


