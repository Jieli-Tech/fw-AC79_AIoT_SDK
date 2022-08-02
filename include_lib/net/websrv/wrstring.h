/*
 * Copyright (c) 2009 windyrobin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * $Id: wrstring.h 3/14/2009 windyrobin@Gmail.com nanjing china$
 */

#ifndef __WRSTRING_H__
#define __WRSTRING_H__

#ifdef __cplusplus
extern "C" {
#endif

#define SKIP(ppBuf ,token) do{\
	char *cp = *(ppBuf);\
	while (*cp != (token)){\
		++cp;\
	}\
	*cp = '\0';\
	*(ppBuf) = ++cp;\
}while(0)

#define  TRIM_HEAD(ppbuf) do{\
	char *cp = *(ppbuf);\
	while(*cp <= 0x20){\
		++cp;\
	}\
}while(0)

/*trim left-brother's tail*/
#define  TRIMI_LB_TAIL(pBuf) do{\
		char *cp= (pBuf);\
		while(*(--cp) <=0x20){\
			*cp = '\0';\
		}\
}while(0)

/*trim left-brother's end pos*/
static char *find_lb_end(char *pBuf)
{
    while (*(--pBuf) <= 0x20);
    return ++pBuf;
}



int wr_strcasecmp(const char *lStr, const char *rStr);
int endscmp(const char *src, const char *substr);
char *wr_trim(char *pBuf);

/*we have define macro SKIP to replace it
void skip(char **ppBuf ,char token);
*/

#ifdef __cplusplus
}
#endif
#endif
