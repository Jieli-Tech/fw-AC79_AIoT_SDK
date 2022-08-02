#ifndef __STREAM_MEDIA_MEM__
#define __STREAM_MEDIA_MEM__

#define STREAM_MEDIA_MEMHEAP_DEBUG	0	//打开检查内存泄漏，注意:要屏蔽mem_leak_test.h的malloc free等宏定义

#if STREAM_MEDIA_MEMHEAP_DEBUG
#include "mem_leak_test.h"
#define strm_alloca(sz) 		_vmalloc_dbg((sz), __FUNCTION__, __LINE__)
#define strm_alloc(sz) 			_vmalloc_dbg((sz), __FUNCTION__, __LINE__)
#define strm_calloca(n,sz) 		_vcalloc_dbg((n), (sz), __FUNCTION__, __LINE__)
#define strm_calloc(n,sz) 		_vcalloc_dbg((n), (sz), __FUNCTION__, __LINE__)
#define strm_realloc(pv,sz)		_vrealloc_dbg(pv, (sz), __FUNCTION__, __LINE__)
#define strm_malloc(sz) 		_vmalloc_dbg((sz), __FUNCTION__, __LINE__)
#define strm_free(pbuf) 		_vfree_dbg(pbuf)
#else
#include "malloc.h"
#define strm_alloca(sz) 		malloc(sz)
#define strm_alloc(sz) 			malloc(sz)
#define strm_calloca(n,sz) 		calloc(n,sz)
#define strm_calloc(n,sz) 		calloc(n,sz)
#define strm_realloc(pv,sz) 	realloc(pv,sz)
#define strm_malloc(sz) 		malloc(sz)
#define strm_free(pbuf) 		free(pbuf)
#endif

#endif



