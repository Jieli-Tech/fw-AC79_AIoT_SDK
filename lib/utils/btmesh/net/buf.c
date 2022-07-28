/* buf.c - Buffer management */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adaptation.h"
#include "net/buf.h"
#include "adv.h"

#define LOG_TAG             "[MESH-buf]"
#define LOG_INFO_ENABLE
/* #define LOG_DEBUG_ENABLE */
#define LOG_WARN_ENABLE
#define LOG_ERROR_ENABLE
#define LOG_DUMP_ENABLE
#include "mesh_log.h"

#define NET_BUF_DBG             BT_DBG
#define NET_BUF_ERR             BT_ERR
#define NET_BUF_WARN            BT_WARN
#define NET_BUF_INFO            BT_INFO
#define NET_BUF_ASSERT          ASSERT

#if MESH_RAM_AND_CODE_MAP_DETAIL
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ble_mesh_buf_bss")
#pragma data_seg(".ble_mesh_buf_data")
#pragma const_seg(".ble_mesh_buf_const")
#pragma code_seg(".ble_mesh_buf_code")
#endif
#else /* MESH_RAM_AND_CODE_MAP_DETAIL */
#pragma bss_seg(".ble_mesh_bss")
#pragma data_seg(".ble_mesh_data")
#pragma const_seg(".ble_mesh_const")
#pragma code_seg(".ble_mesh_code")
#endif /* MESH_RAM_AND_CODE_MAP_DETAIL */

#if CONFIG_NET_BUF_WARN_ALLOC_INTERVAL > 0
#define WARN_ALLOC_INTERVAL K_SECONDS(CONFIG_NET_BUF_WARN_ALLOC_INTERVAL)
#else
#define WARN_ALLOC_INTERVAL K_FOREVER
#endif

/* Linker-defined symbol bound to the static pool structs */
extern struct net_buf_pool _net_buf_pool_list[];

struct net_buf_pool *net_buf_pool_get(int id)
{
    return &_net_buf_pool_list[id];
}

static int pool_id(struct net_buf_pool *pool)
{
    return pool - _net_buf_pool_list;
}

int net_buf_id(struct net_buf *buf)
{
    struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);

    return buf - pool->__bufs;
}

static inline struct net_buf *pool_get_uninit(struct net_buf_pool *pool,
        u16_t uninit_count)
{
    struct net_buf *buf;

    buf = &pool->__bufs[pool->buf_count - uninit_count];

    buf->pool_id = pool_id(pool);

    return buf;
}

void net_buffer_reset(struct net_buf *buf)
{
    NET_BUF_ASSERT(buf->flags == 0);
    NET_BUF_ASSERT(buf->frags == NULL);

    net_buf_simple_reset(&buf->b);
}

static u8_t *fixed_data_alloc(struct net_buf *buf, size_t *size, s32_t timeout)
{
    struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);
    const struct net_buf_pool_fixed *fixed = pool->alloc->alloc_data;

    *size = min(fixed->data_size, *size);

#if NET_BUF_USE_MALLOC
    return (u8 *)(*((u32 *)fixed->data_pool)) + fixed->data_size * net_buf_id(buf);
#else
    return fixed->data_pool + fixed->data_size * net_buf_id(buf);
#endif /* NET_BUF_USE_MALLOC */
}

static void fixed_data_unref(struct net_buf *buf, u8_t *data)
{
    /* Nothing needed for fixed-size data pools */
}

const struct net_buf_data_cb net_buf_fixed_cb = {
    .alloc = fixed_data_alloc,
    .unref = fixed_data_unref,
};

static u8_t *data_alloc(struct net_buf *buf, size_t *size, s32_t timeout)
{
    struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);

    return pool->alloc->cb->alloc(buf, size, timeout);
}

static u8_t *data_ref(struct net_buf *buf, u8_t *data)
{
    struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);

    return pool->alloc->cb->ref(buf, data);
}

static void data_unref(struct net_buf *buf, u8_t *data)
{
    struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);

    if (buf->flags & NET_BUF_EXTERNAL_DATA) {
        return;
    }

    pool->alloc->cb->unref(buf, data);
}

struct net_buf *net_buf_alloc_len(struct net_buf_pool *pool, size_t size,
                                  s32_t timeout)
{
    struct net_buf *buf;
    unsigned int key;
    u16_t uninit_count;

    NET_BUF_ASSERT(pool);

    NET_BUF_INFO("--func=%s, pool_id=%d", __FUNCTION__, pool_id(pool));

    key = irq_lock();

    if (pool->free_count) {

        pool->free_count--;

        do {
            uninit_count = pool->uninit_count--;

            if (0 == pool->uninit_count) {
                pool->uninit_count = pool->buf_count;
            }

            buf = pool_get_uninit(pool, uninit_count);

        } while (buf->flags | buf->ref);

        irq_unlock(key);

        goto success;
    }

    irq_unlock(key);

    NET_BUF_ERR("%s():%d: Failed to get free buffer", __FUNCTION__, __LINE__);

    return NULL;

success:
    NET_BUF_DBG("allocated buf 0x%x", buf);

    if (size) {
        buf->__buf = data_alloc(buf, &size, timeout);
        if (!buf->__buf) {
            NET_BUF_ERR("%s():%d: Failed to allocate data", __FUNCTION__, __LINE__);
            return NULL;
        }
    } else {
        buf->__buf = NULL;
    }

    NET_BUF_INFO("alloc free_count=%d, addr=0x%x", pool->free_count, buf);

    buf->ref   = 1;
    buf->flags = 0;
    buf->frags = NULL;
    buf->size  = size;

    net_buffer_reset(buf);

    return buf;
}

struct net_buf *net_buf_alloc_fixed(struct net_buf_pool *pool, s32_t timeout)
{
    const struct net_buf_pool_fixed *fixed = pool->alloc->alloc_data;

    return net_buf_alloc_len(pool, fixed->data_size, timeout);
}

static void net_buf_free(struct net_buf *buf)
{
    struct net_buf_pool *pool;

    while (buf) {
        struct net_buf *frags = buf->frags;

        buf->frags = NULL;

        pool = net_buf_pool_get(buf->pool_id);

        if (pool->destroy) {
            pool->destroy(buf);
        }

        pool = net_buf_pool_get(buf->pool_id);

        if (pool->free_count < pool->buf_count) {
            pool->free_count++;
            NET_BUF_INFO("free free_count=%d, pool_id=%d", pool->free_count, buf->pool_id);
        } else {
            NET_BUF_ERR("free free_count=%d, pool_id=%d", pool->free_count, buf->pool_id);
        }

        buf = frags;
    }
}

void net_buf_unref(struct net_buf *buf)
{
    NET_BUF_INFO("--func=%s, buf=0x%x, ref=%d", __FUNCTION__, buf, buf->ref);

    NET_BUF_ASSERT(buf);

    unsigned int key;

    key = irq_lock();

    if (--buf->ref) {
        irq_unlock(key);
        return;
    }

    net_buf_free(buf);

    irq_unlock(key);
}

struct net_buf *net_buf_ref(struct net_buf *buf)
{
    NET_BUF_INFO("--func=%s, buf=0x%x, ref=%d", __FUNCTION__, buf, buf->ref);

    NET_BUF_ASSERT(buf);

    buf->ref++;

    return buf;
}

struct net_buf *net_buf_get_next(struct net_buf *buf)
{
    unsigned int key;
    int next_id;

    key = irq_lock();

    struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);

    next_id = net_buf_id(buf) + 1;

    next_id = (next_id < pool->buf_count) ? next_id : 0;

    buf = &pool->__bufs[next_id];

    irq_unlock(key);

    return buf;
}

void net_buf_simple_reserve(struct net_buf_simple *buf, size_t reserve)
{
    NET_BUF_ASSERT(buf);
    NET_BUF_ASSERT(buf->len == 0);
    NET_BUF_DBG("buf 0x%x reserve %u", buf, reserve);

    buf->data = buf->__buf + reserve;
}

#if defined(CONFIG_NET_BUF_SIMPLE_LOG)
#define NET_BUF_SIMPLE_DBG(fmt, ...) NET_BUF_DBG(fmt, ##__VA_ARGS__)
#define NET_BUF_SIMPLE_ERR(fmt, ...) NET_BUF_ERR(fmt, ##__VA_ARGS__)
#define NET_BUF_SIMPLE_WARN(fmt, ...) NET_BUF_WARN(fmt, ##__VA_ARGS__)
#define NET_BUF_SIMPLE_INFO(fmt, ...) NET_BUF_INFO(fmt, ##__VA_ARGS__)
#define NET_BUF_SIMPLE_ASSERT(cond) NET_BUF_ASSERT(cond)
#else
#define NET_BUF_SIMPLE_DBG(fmt, ...)
#define NET_BUF_SIMPLE_ERR(fmt, ...)
#define NET_BUF_SIMPLE_WARN(fmt, ...)
#define NET_BUF_SIMPLE_INFO(fmt, ...)
#define NET_BUF_SIMPLE_ASSERT(cond)
#endif /* CONFIG_NET_BUF_SIMPLE_LOG */

void *net_buf_simple_add(struct net_buf_simple *buf, size_t len)
{
    u8_t *tail = net_buf_simple_tail(buf);

    NET_BUF_SIMPLE_DBG("buf 0x%x len %u", buf, len);

    NET_BUF_SIMPLE_ASSERT(net_buf_simple_tailroom(buf) >= len);

    buf->len += len;
    return tail;
}

void *net_buf_simple_add_mem(struct net_buf_simple *buf, const void *mem,
                             size_t len)
{
    NET_BUF_SIMPLE_DBG("buf 0x%x len %u", buf, len);

    return memcpy(net_buf_simple_add(buf, len), mem, len);
}

u8_t *net_buf_simple_add_u8(struct net_buf_simple *buf, u8_t val)
{
    u8_t *u8;

    NET_BUF_SIMPLE_DBG("buf 0x%x val 0x%02x", buf, val);

    u8 = net_buf_simple_add(buf, 1);
    *u8 = val;

    return u8;
}

void net_buf_simple_add_le16(struct net_buf_simple *buf, u16_t val)
{
    NET_BUF_SIMPLE_DBG("buf 0x%x val %u", buf, val);

    val = sys_cpu_to_le16(val);
    memcpy(net_buf_simple_add(buf, sizeof(val)), &val, sizeof(val));
}

void net_buf_simple_add_be16(struct net_buf_simple *buf, u16_t val)
{
    NET_BUF_SIMPLE_DBG("buf 0x%x val %u", buf, val);

    val = sys_cpu_to_be16(val);
    memcpy(net_buf_simple_add(buf, sizeof(val)), &val, sizeof(val));
}

void net_buf_simple_add_le32(struct net_buf_simple *buf, u32_t val)
{
    NET_BUF_SIMPLE_DBG("buf 0x%x val %u", buf, val);

    val = sys_cpu_to_le32(val);
    memcpy(net_buf_simple_add(buf, sizeof(val)), &val, sizeof(val));
}

void net_buf_simple_add_be32(struct net_buf_simple *buf, u32_t val)
{
    NET_BUF_SIMPLE_DBG("buf 0x%x val %u", buf, val);

    val = sys_cpu_to_be32(val);
    memcpy(net_buf_simple_add(buf, sizeof(val)), &val, sizeof(val));
}

void *net_buf_simple_push(struct net_buf_simple *buf, size_t len)
{
    NET_BUF_SIMPLE_DBG("buf 0x%x len %u", buf, len);

    NET_BUF_SIMPLE_ASSERT(net_buf_simple_headroom(buf) >= len);

    buf->data -= len;
    buf->len += len;
    return buf->data;
}

void net_buf_simple_push_le16(struct net_buf_simple *buf, u16_t val)
{
    NET_BUF_SIMPLE_DBG("buf 0x%x val %u", buf, val);

    val = sys_cpu_to_le16(val);
    memcpy(net_buf_simple_push(buf, sizeof(val)), &val, sizeof(val));
}

void net_buf_simple_push_be16(struct net_buf_simple *buf, u16_t val)
{
    NET_BUF_SIMPLE_DBG("buf 0x%x val %u", buf, val);

    val = sys_cpu_to_be16(val);
    memcpy(net_buf_simple_push(buf, sizeof(val)), &val, sizeof(val));
}

void net_buf_simple_push_u8(struct net_buf_simple *buf, u8_t val)
{
    u8_t *data = net_buf_simple_push(buf, 1);

    *data = val;
}

void *net_buf_simple_pull(struct net_buf_simple *buf, size_t len)
{
    NET_BUF_SIMPLE_DBG("buf 0x%x len %u", buf, len);

    NET_BUF_SIMPLE_ASSERT(buf->len >= len);

    buf->len -= len;
    return buf->data += len;
}

u8_t net_buf_simple_pull_u8(struct net_buf_simple *buf)
{
    u8_t val;

    val = buf->data[0];
    net_buf_simple_pull(buf, 1);

    return val;
}

u16_t net_buf_simple_pull_le16(struct net_buf_simple *buf)
{
    u16_t val;

    val = UNALIGNED_GET((u16_t *)buf->data);
    net_buf_simple_pull(buf, sizeof(val));

    return sys_le16_to_cpu(val);
}

u16_t net_buf_simple_pull_be16(struct net_buf_simple *buf)
{
    u16_t val;

    val = UNALIGNED_GET((u16_t *)buf->data);
    net_buf_simple_pull(buf, sizeof(val));

    return sys_be16_to_cpu(val);
}

u32_t net_buf_simple_pull_le32(struct net_buf_simple *buf)
{
    u32_t val;

    val = UNALIGNED_GET((u32_t *)buf->data);
    net_buf_simple_pull(buf, sizeof(val));

    return sys_le32_to_cpu(val);
}

u32_t net_buf_simple_pull_be32(struct net_buf_simple *buf)
{
    u32_t val;

    val = UNALIGNED_GET((u32_t *)buf->data);
    net_buf_simple_pull(buf, sizeof(val));

    return sys_be32_to_cpu(val);
}

size_t net_buf_simple_headroom(struct net_buf_simple *buf)
{
    return buf->data - buf->__buf;
}

size_t net_buf_simple_tailroom(struct net_buf_simple *buf)
{
    return buf->size - net_buf_simple_headroom(buf) - buf->len;
}

static void log_dump_slist(sys_slist_t *list)
{
    NET_BUF_INFO("list->head=0x%x", list->head);

    sys_snode_t *node = list->head;
    while (node) {
        NET_BUF_INFO("node next=0x%x", node);
        node = node->next;
    }

    NET_BUF_INFO("list->tail=0x%x", list->tail);
}

void net_buf_slist_put(sys_slist_t *list, struct net_buf *buf)
{
    struct net_buf *tail;
    unsigned int key;

    NET_BUF_INFO("--func=%s", __FUNCTION__);

    log_dump_slist(list);

    NET_BUF_ASSERT(list);
    NET_BUF_ASSERT(buf);

    for (tail = buf; tail->frags; tail = tail->frags) {
        tail->flags |= NET_BUF_FRAGS;
        NET_BUF_INFO("net_buf_slist_put NET_BUF_FRAGS");
    }

    key = irq_lock();
    sys_slist_append_list(list, &buf->node, &tail->node);
    irq_unlock(key);

    log_dump_slist(list);
}

void net_buf_slist_simple_put(sys_slist_t *head_list, sys_snode_t *dst_node)
{
    sys_snode_t *tail_node;
    unsigned int key;

    tail_node = dst_node;

    key = irq_lock();
    sys_slist_append_list(head_list, dst_node, tail_node);
    irq_unlock(key);
}

#define GET_STRUCT_MEMBER_OFFSET(type, member) \
    (u32)&(((struct type*)0)->member)

struct net_buf *net_buf_slist_simple_get(sys_slist_t *list)
{
    u8 *buf;
    unsigned int key;

    key = irq_lock();
    buf = (void *)sys_slist_get(list);
    if (buf) {
        buf -= GET_STRUCT_MEMBER_OFFSET(net_buf, entry_node);
    }
    irq_unlock(key);

    return (struct net_buf *)buf;
}

struct net_buf *net_buf_slist_get(sys_slist_t *list)
{
    struct net_buf *buf, *frag;
    unsigned int key;

    NET_BUF_INFO("--func=%s", __FUNCTION__);

    log_dump_slist(list);

    NET_BUF_ASSERT(list);

    key = irq_lock();
    buf = (void *)sys_slist_get(list);
    irq_unlock(key);

    if (!buf) {
        log_dump_slist(list);
        return NULL;
    }

    /* Get any fragments belonging to this buffer */
    for (frag = buf; (frag->flags & NET_BUF_FRAGS); frag = frag->frags) {
        NET_BUF_INFO("NET_BUF_FRAGS");
        key = irq_lock();
        frag->frags = (void *)sys_slist_get(list);
        irq_unlock(key);

        NET_BUF_ASSERT(frag->frags);

        /* The fragments flag is only for list-internal usage */
        frag->flags &= ~NET_BUF_FRAGS;
    }

    /* Mark the end of the fragment list */
    frag->frags = NULL;

    log_dump_slist(list);

    return buf;
}
