#ifndef __RLE_H__
#define __RLE_H__

#include "system/includes.h"

struct rle_header {
    u32 addr: 22; //max 4M
    u32 len: 10;  //max 1024 bytes
};

struct rle_line {
    u32 addr: 22;
    u32 num: 10;
    u16 len[0];
};

int Rle_Decode(u8 *inbuf, int inSize, u8 *outbuf, int onuBufSize, int offset, int len, int pixel_size);

#endif
