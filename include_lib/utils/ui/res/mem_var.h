#ifndef MEM_VAR_H
#define MEM_VAR_H

#include "typedef.h"
#include "generic/list.h"

struct mem_var_element {
    u16 crc;
    u16 checksum;
    u16 len;
    u8 buf[0];
};

struct mem_var {
    struct list_head head;
    struct mem_var_element var;
};

struct mem_var_head {
    struct list_head head;
    int total_mem_size;
    int items;
    int use_mem_size;
    int hits;
    u8 debug;
};

extern struct mem_var_head var_list;

void mem_var_init(u32 size, u8 debug);
int mem_var_add(u32 index, u32 type, u32 id, u32 page, u32 prj, u8 *buf, u16 len);
void mem_var_free();
int mem_var_del(struct mem_var *var);
void mem_var_get(struct mem_var *var, u8 *buf, u16 len);
struct mem_var *mem_var_search(u32 index, u32 type, u32 id, u32 page, u32 prj);
void mem_var_stat();

#endif
