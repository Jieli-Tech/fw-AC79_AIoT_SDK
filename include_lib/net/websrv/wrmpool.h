#ifndef __WR_MPOOL_H__
#define __WR_MPOOL_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *begin;/*start pos*/
    size_t len;/*capacity*/
    int index;/*curIndex*/
    int cflag;/*clear flag ,when capacity isn't enough and call malloc ,you must set it to 1*/
} wr_mpool;

void wr_mpool_init(wr_mpool *pmp, char *begin, size_t len);
void *wr_mpool_malloc(wr_mpool *pmp, size_t mlen);
void wr_mpool_free(wr_mpool *pmp, void *p);
void wr_mpool_clear(wr_mpool *pmp);

#ifdef __cplusplus
}
#endif
#endif
