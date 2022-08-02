#ifndef __AEC_CONFIG_H__
#define __AEC_CONFIG_H__

int asp2_aec_get_heap_size(void);

void *asp2_aec_init(void *heap_buffer, int heap_size);

int asp2_aec_reset(void *m);

int asp2_aec_process(void *m, void *near_buf, void *far_buf, void *out, int *outsize);

int asp2_aec_free(void *m);

#endif
