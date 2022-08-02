#ifndef  __WR_HASH_H__
#define  __WR_HASH_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "wrstring.h"
#include "wrmpool.h"

typedef struct _slist {
    void *key;
    void *value;
    struct _slist *next;
} wr_hashlist;

/*
   typedef struct{
   		void *key;
			void *value;
			}wr_pair;
			*/
typedef struct {
    wr_hashlist **buckets;
    size_t size;
    //	int used;
    //	int length;
    wr_mpool *mpool;
    int (*hashfun)(const void *key);
    int (*hashcmp)(const void *lkey, const void *rkey);
} wr_hashmap;

void wr_hash_init(wr_hashmap *hm, wr_mpool *mp, size_t _size);
void wr_hash_clear(wr_hashmap *hm);

int wr_hash_add(wr_hashmap *hm, const void *key, const void *value);
void *wr_hash_find(const wr_hashmap *hm, const void *key);

int wr_hashfun_elf(const char *s);
int wr_hashfun_str(const char *s);

static int wr_hashcmp_str(const char *s1, const char *s2)
{
    return strcmp(s1, s2) == 0 ;
}

static int wr_hashcasecmp_str(const char *s1, const char *s2)
{
    return wr_strcasecmp(s1, s2) == 0 ;
}

/*just return the first letter*/
static int wr_hashfun_uchar(const char *s)
{
    return (unsigned char)(*s);
}


#ifdef __cplusplus
}
#endif
#endif
