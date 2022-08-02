#ifndef __TVS_DATA_BUFFER_H__
#define __TVS_DATA_BUFFER_H__

#define TVS_ERROR_DATA_BUFFER_FULL            -100


typedef struct tvs_data_buffer_t tvs_data_buffer;

tvs_data_buffer *tvs_data_buffer_new(int max_size);

int tvs_data_buffer_read(tvs_data_buffer *buffer, char *data_buf, int data_buf_len);

int tvs_data_buffer_write(tvs_data_buffer *buffer, const char *data, int data_size);

bool tvs_data_buffer_consumer_wait(tvs_data_buffer *buffer, int wait_ms);

void tvs_data_buffer_notify_consumer(tvs_data_buffer *buffer);

bool tvs_data_buffer_producer_wait(tvs_data_buffer *buffer, int wait_ms);

void tvs_data_buffer_notify_producer(tvs_data_buffer *buffer);

void tvs_data_buffer_init_content(tvs_data_buffer *buffer);

void tvs_data_buffer_release_content(tvs_data_buffer *buffer);

bool tvs_data_buffer_has_space(tvs_data_buffer *buffer, int need_size);

bool tvs_data_buffer_has_enough_data(tvs_data_buffer *buffer, int need_size);

bool tvs_data_buffer_is_producer_alive(tvs_data_buffer *buffer);

void tvs_data_buffer_set_producer_alive(tvs_data_buffer *buffer, bool alive);

bool tvs_data_buffer_is_consumer_alive(tvs_data_buffer *buffer);

void tvs_data_buffer_set_consumer_alive(tvs_data_buffer *buffer, bool alive);

int tvs_data_buffer_get_total_size(tvs_data_buffer *buffer);

int tvs_data_buffer_get_data_size(tvs_data_buffer *buffer);

void tvs_data_buffer_set_max(tvs_data_buffer *buffer, int max_size);

char *tvs_data_buffer_read_ptr(tvs_data_buffer *buffer);

void tvs_data_buffer_remove_not_lock(tvs_data_buffer *buffer, int size);

void tvs_data_buffer_lock(tvs_data_buffer *buffer);

void tvs_data_buffer_unlock(tvs_data_buffer *buffer);

#endif
