#ifndef  __WR_IO_H__
#define  __WR_IO_H__

#include "websrv_http.h"

#ifdef __cplusplus
extern "C" {
#endif

int wr_sock_nwrite(void *sockhdl, const char *buf, size_t n);

int wr_read_head(void *sockhdl, char *buf, size_t bufsize);
int wr_load_body(WrHttp *pHttp);
int sendFileStream(const WrHttp *pHttp, const char *filePath);
int websrv_virfile_reg(const char *fname, const char *contents, unsigned int len);

#ifdef __cplusplus
}
#endif

#endif
