#include "http.h"

#ifndef  __ANALYSIS_H__
#define  __ANALYSIS_H__

#ifdef __cplusplus
extern "C" {
#endif

int checkmethod(WrHttp *pHttp);
int parseURL(WrHttp *pHttp);
int parseHeader(WrHttp *pHttp);
int checkpath(WrHttp *pHttp);
int cookieCheck(const WrHttp *pHttp);
int cacheCheck(const WrHttp *pHttp);
int decodeParam(WrHttp *pHttp);

#ifdef __cplusplus
}
#endif
#endif


