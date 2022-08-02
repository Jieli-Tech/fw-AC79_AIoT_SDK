#ifndef _CHARACTER_CODING_H
#define _CHARACTER_CODING_H

#include "generic/typedef.h"

int url_encode(const char *str, const int strSize, char *result, const int resultSize);

int url_decode(const char *input, const u32 inlen, char *output, const u32 outbuf_len);

int utf8_2_unicode_one(const unsigned char *in, u16 *unicode);

int utf8_2_unicode(const u8 *utf8, int utf8_len, u8 *unicode, int unic_len);

int unicode_2_utf8_one(u16 unic, unsigned char *pOutput);

int unicode_2_utf8(u8 *utf8, int utf8_len, const u8 *unicode, int unic_len);

u32 utf8_to_utf16(u8 bigendian, const u8 *utf8_buf, int utf8_len, u16 *utf16_buf);

u32 utf16_to_utf8(u8 bigendian, const u8 *utf16, int utf16_len, u8 *utf8);

u32 utf16_to_gb2312(u8 bigendian, const u8 *utf16, u16 *gb2312, u32 len, FILE *fp);

u32 utf16_to_gbk(u8 bigendian, const u8 *utf16, u16 *gbk, u32 len, FILE *fp);

u32 utf8_to_gb2312(u8 bigendian, const u8 *utf8, u16 *gb2312, u32 len, const char *file_path);

u32 utf8_to_gbk(u8 bigendian, const u8 *utf8, u16 *gb2312, u32 len, const char *file_path);

//暂时仅适用于纯ascii码的长文件名和目录，不适用于中文，中文名请自行手动拼接
int long_file_name_encode(const char *input, u8 *output, u32 output_buf_len);	//返回拼接后的长度

#endif
