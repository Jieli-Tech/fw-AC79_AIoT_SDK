#ifndef __MG_COMPAT_H__
#define __MG_COMPAT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include "lwip/sockets.h"
#include "os/os_api.h"

#define	DIRSEP			'/'
#define	IS_DIRSEP_CHAR(c)	((c) == '/')
#define	ERRNO			errno
#define	INVALID_SOCKET		(-1)
typedef int SOCKET;
#define F_SETFD		2	/* set/clear close_on_exec */
#define FD_CLOEXEC	1	/* actually anything with low bit set goes */
#define  WR_SOCK_ERRNO errno
#define	 WR_EINTR EINTR


#if !defined(MIN)
#define MIN(a ,b) ((a)<(b)?(a):(b))
#endif

#if !defined(MAX)
#define MAX(a ,b) ((a)>(b)?(a):(b))
#endif

/*defined in mg_compat.c*/
int start_thread(int (*func)(void *), void *param);


extern const unsigned char FAVICON_ICO_FNAME[4286];
extern const unsigned char FAVICON_ICO[0x10000];

extern const unsigned char INDEX_HTML_FNAME[];
extern const unsigned char INDEX_HTML[2511];

extern const unsigned char JQUERY_MIN_JS_FNAME[];
extern const unsigned char JQUERY_MIN_JS[95702];

#ifdef __cplusplus
}
#endif

#endif
