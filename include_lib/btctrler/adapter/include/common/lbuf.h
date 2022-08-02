#ifndef ADAPTER_LBUF_H
#define ADAPTER_LBUF_H

// #include "generic/lbuf.h"
//
//
#include "generic/typedef.h"
#include "generic/list.h"

// #define LBUF_SANITY_CHECK

struct lbuff_head_btctrler {
    struct list_head head;
    struct list_head free;
    /*u16 index;*/
};

struct hentry {
#ifdef LBUF_SANITY_CHECK
    u32 magic_a;    // : 4
#endif
    struct list_head entry; // : 8
    struct lbuff_head_btctrler *head; // : 16
    u16 len;    // : 2
    u8 state;   // : 1
    char ref;   // : 1
#ifdef LBUF_SANITY_CHECK
    u32 magic_b;    // : 4
#endif
};

struct hfree {
#ifdef LBUF_SANITY_CHECK
    u32 magic_a;
#endif
    struct list_head entry;
    u16 len;
#ifdef LBUF_SANITY_CHECK
    u32 magic_b;
#endif
};

#define LBUF_INIT_SIZE      (sizeof(struct lbuff_head_btctrler) + sizeof(struct hfree) + 3)

int lbuf_get_chunk_num_btctrler(u32 total_size, u32 chunk_size);

struct lbuff_head_btctrler *lbuf_init_btctrler(void *buf, u32 buf_len);

void *lbuf_alloc_btctrler(struct lbuff_head_btctrler *head, u32 len);

void *lbuf_realloc_btctrler(void *lbuf, int size);

void lbuf_push_btctrler(void *lbuf);

void *lbuf_pop_btctrler(struct lbuff_head_btctrler *head);

void *lbuf_query_used_btctrler(struct lbuff_head_btctrler *head);

void lbuf_free_btctrler(void *lbuf);

int lbuf_empty_btctrler(struct lbuff_head_btctrler *head);

void lbuf_clear_btctrler(struct lbuff_head_btctrler *head);

bool lbuf_more_data_btctrler(struct lbuff_head_btctrler *head);

u32 lbuf_remain_len_btctrler(struct lbuff_head_btctrler *head, u32 len);

/* void lbuf_debug_btctrler(struct lbuff_head *head);*/
void lbuf_debug_btctrler(struct lbuff_head_btctrler *head, u8 flag);

void lbuf_info_btctrler(struct lbuff_head_btctrler *head);	//获取lbuf的alloc使用情况

void *lbuf_peek_btctrler(struct lbuff_head_btctrler *head);

void lbuf_push_queue_btctrler(struct list_head *head, void *lbuf);

void *lbuf_pop_queue_btctrler(struct list_head *head);

void *lbuf_peek_queue_btctrler(struct list_head *head);

void lbuf_clear_queue_btctrler(struct list_head *head);

#endif

