#ifndef _MEM_HEAP_H_
#define _MEM_HEAP_H_

#ifdef __cplusplus
extern "C" {
#endif

extern void *malloc(size_t size);
extern void *zalloc(size_t size);
extern void *calloc(size_t count, size_t size);
extern void *realloc(void *rmem, size_t newsize);
extern void  free(void *mem);


extern void *kmalloc(size_t size, int flags);
extern void *vmalloc(size_t size);
extern void vfree(void *addr);
extern void *kzalloc(unsigned int len, int a);
extern void kfree(void *p);

extern void malloc_stats(void);

extern void malloc_dump();

void mem_heap_check(const char *const func, unsigned int line);

void memory_init(void);

void mem_stats(void);

size_t xPortGetFreeHeapSize(void);
size_t xPortGetMinimumEverFreeHeapSize(void);
size_t xPortGetPhysiceMemorySize(void);

/*
 *  --Physic Memory
 */
void *get_physic_address(u32 page);

/*
 *  --Virtual Memory
 */
void *vmem_get_phy_adr(void *vaddr);

#ifdef __cplusplus
}
#endif

#endif /* _MEM_HEAP_H_ */
