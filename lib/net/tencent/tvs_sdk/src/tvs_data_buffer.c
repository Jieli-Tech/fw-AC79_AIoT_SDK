/**
* @file  tvs_data_buffer.c
* @brief 生产者、消费者和共享数据队列
* @date  2019-5-10
*
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "mongoose.h"
#include "tvs_data_buffer.h"
#include "os_wrapper.h"

#define TVS_LOG_DEBUG  0
#include "tvs_log.h"

struct tvs_data_buffer_t {
    void *producer_waiter;   /*!< 让生产者进入等待状态的信号量 */
    void *consumer_waiter;   /*!< 让消费者进入等待状态的信号量 */
    void *locker_mutex;      /*!< 用于线程安全的锁 */
    struct mbuf content_buffer;   /*!< 数据队列 */
    bool buf_init;
    int max_size;               /*!< 数据队列的最大值 */
    bool consumer_alive;       /*!< 消费者的存活状态 */
    bool producer_alive;       /*!< 生产者的存活状态 */
};

void tvs_data_buffer_lock(tvs_data_buffer *buffer)
{
    if (buffer == NULL || buffer->locker_mutex == NULL) {
        return;
    }

    os_wrapper_lock_mutex(buffer->locker_mutex, os_wrapper_get_forever_time());
}

void tvs_data_buffer_unlock(tvs_data_buffer *buffer)
{
    if (buffer == NULL || buffer->locker_mutex == NULL) {
        return;
    }

    os_wrapper_unlock_mutex(buffer->locker_mutex);
}

tvs_data_buffer *tvs_data_buffer_new(int max_size)
{
    tvs_data_buffer *buffer = TVS_MALLOC(sizeof(tvs_data_buffer));
    if (NULL == buffer) {
        return NULL;
    }

    memset(buffer, 0, sizeof(tvs_data_buffer));

    // 创建一个初值为0的二值信号量，用于让消费者进入等待状态
    buffer->consumer_waiter = os_wrapper_create_signal_mutex(0);

    // 创建一个初值为0的二值信号量，用于让生成进入等待状态
    buffer->producer_waiter = os_wrapper_create_signal_mutex(0);

    // 用于保护公共变量的锁
    buffer->locker_mutex = os_wrapper_create_locker_mutex();
    buffer->max_size = max_size;

    mbuf_init(&buffer->content_buffer, 0);

    return buffer;
}

// 消费者进行等待，等待生产者将数据放入队列
bool tvs_data_buffer_consumer_wait(tvs_data_buffer *buffer, int wait_ms)
{
    if (buffer == NULL || NULL == buffer->consumer_waiter) {
        return false;
    }
    bool obt = false;
    TVS_LOG_PRINTF("consumer is waiting %d ms\n", wait_ms);
    obt = os_wrapper_wait_signal(buffer->consumer_waiter, wait_ms);
    TVS_LOG_PRINTF("consumer wait end, obtained %d\n", obt);
    return obt;
}

// 生产者通知消费者退出等待状态
void tvs_data_buffer_notify_consumer(tvs_data_buffer *buffer)
{
    if (buffer == NULL || NULL == buffer->consumer_waiter) {
        return;
    }
    TVS_LOG_PRINTF("notify consumer\n");
    os_wrapper_post_signal(buffer->consumer_waiter);
}

// 生产者进入等待状态，等消费者消费队列中的数据
bool tvs_data_buffer_producer_wait(tvs_data_buffer *buffer, int wait_ms)
{
    if (buffer == NULL || NULL == buffer->producer_waiter) {
        return false;
    }

    bool obt = false;
    TVS_LOG_PRINTF("producer is waiting %d ms\n", wait_ms);
    obt = os_wrapper_wait_signal(buffer->producer_waiter, wait_ms);
    TVS_LOG_PRINTF("producer wait end, obtained %d\n", obt);
    return obt;
}

// 消费者通知生产者退出等待状态
void tvs_data_buffer_notify_producer(tvs_data_buffer *buffer)
{
    if (buffer == NULL || NULL == buffer->producer_waiter) {
        return;
    }
    TVS_LOG_PRINTF("notify producer\n");
    os_wrapper_post_signal(buffer->producer_waiter);
}

void tvs_data_buffer_init_content(tvs_data_buffer *buffer)
{
    if (buffer == NULL) {
        return;
    }

    tvs_data_buffer_lock(buffer);
    if (buffer->content_buffer.size != 0) {
        tvs_data_buffer_unlock(buffer);
        return;
    }
    mbuf_init(&buffer->content_buffer, 0);
    buffer->buf_init = 1;
    tvs_data_buffer_unlock(buffer);
}

void tvs_data_buffer_release_content(tvs_data_buffer *buffer)
{
    if (buffer == NULL) {
        return;
    }

    tvs_data_buffer_lock(buffer);
    mbuf_free(&buffer->content_buffer);
    buffer->buf_init = 0;
    tvs_data_buffer_unlock(buffer);
}

void tvs_data_buffer_set_max(tvs_data_buffer *buffer, int max_size)
{
    tvs_data_buffer_lock(buffer);

    buffer->max_size = max_size;

    tvs_data_buffer_unlock(buffer);
}

bool tvs_data_buffer_has_space(tvs_data_buffer *buffer, int need_size)
{
    bool ret = false;
    if (buffer == NULL) {
        return false;
    }

    tvs_data_buffer_lock(buffer);

    ret = (buffer->max_size - buffer->content_buffer.len >= need_size);

    tvs_data_buffer_unlock(buffer);

    return ret;
}


/*bool tvs_data_buffer_has_space_ex(tvs_data_buffer* buffer, int need_size, bool* consumer_alive) {
	bool ret = false;
	if (buffer == NULL) {
		return false;
	}

	tvs_data_buffer_lock(buffer);

	ret = (buffer->max_size - buffer->content_buffer.len >= need_size);

	*consumer_alive = buffer->consumer_alive

	tvs_data_buffer_unlock(buffer);

	return ret;
}*/


bool tvs_data_buffer_has_enough_data(tvs_data_buffer *buffer, int need_size)
{
    bool ret = false;
    if (buffer == NULL) {
        return false;
    }

    tvs_data_buffer_lock(buffer);

    ret = (buffer->content_buffer.len >= need_size);

    TVS_LOG_PRINTF("ask enough data %d - %d\n", need_size, buffer->content_buffer.len);

    tvs_data_buffer_unlock(buffer);

    return ret;
}

/*bool tvs_data_buffer_has_enough_data_ex(tvs_data_buffer* buffer, int need_size, bool* producer_alive) {
	bool ret = false;
	if (buffer == NULL) {
		return false;
	}

	tvs_data_buffer_lock(buffer);

	ret = (buffer->content_buffer.len >= need_size);

	TVS_LOG_PRINTF("ask enough data %d - %d\n", need_size, buffer->content_buffer.len);
	*producer_alive = buffer->producer_alive;

	tvs_data_buffer_unlock(buffer);

	return ret;
}*/


int tvs_data_buffer_get_total_size(tvs_data_buffer *buffer)
{
    if (buffer == NULL) {
        return 0;
    }

    int size = 0;
    tvs_data_buffer_lock(buffer);

    size = buffer->content_buffer.size;

    tvs_data_buffer_unlock(buffer);

    return size;
}

/*int tvs_data_buffer_wait_and_read(tvs_data_buffer* buffer, char* data_buffer, int buffer_size, int wait_time_ms) {
	if (buffer == NULL) {
		return TVS_ERROR_DATA_BUFFER_NULL;
	}

	tvs_data_buffer_lock(buffer);

	if (!buffer->buf_init) {
		tvs_data_buffer_unlock(buffer);
		TVS_LOG_PRINTF("read failed, not init\n");
		return TVS_ERROR_DATA_BUFFER_NOT_INIT;
	}

	int size = buffer->content_buffer.len;

	if (size < buffer_size) {
		if (buffer->producer_alive) {

			tvs_data_buffer_unlock(buffer);
			tvs_data_buffer_consumer_wait(buffer, wait_time_ms);
			return TVS_ERROR_DATA_BUFFER_READ_TOO_FAST;
		}
	}
	else {
		size = buffer_size;
	}

	if (size == 0) {
		tvs_data_buffer_unlock(buffer);
		TVS_LOG_PRINTF("read failed, empty buffer\n");
		return TVS_ERROR_DATA_BUFFER_EMPTY;
	}

	buffer_size = buffer_size > buffer->content_buffer.len ? buffer->content_buffer.len : buffer_size;

	memcpy(data_buffer, buffer->content_buffer.buf, buffer_size);

	mbuf_remove(&buffer->content_buffer, buffer_size);

	tvs_data_buffer_unlock(buffer);

	tvs_data_buffer_notify_producer(buffer);

	return buffer_size;
}*/

int tvs_data_buffer_get_data_size(tvs_data_buffer *buffer)
{
    if (buffer == NULL) {
        return 0;
    }

    int size = 0;
    tvs_data_buffer_lock(buffer);

    size = buffer->content_buffer.len;

    tvs_data_buffer_unlock(buffer);

    return size;

}

int tvs_data_buffer_write(tvs_data_buffer *buffer, const char *data, int data_size)
{
    if (buffer == NULL) {
        return -1;
    }

    if (data == NULL || data_size == 0) {
        return 0;
    }

    // write data to buffer
    tvs_data_buffer_lock(buffer);

    int size = 0;
    if (buffer == NULL || buffer->max_size < data_size || !buffer->buf_init) {
        tvs_data_buffer_unlock(buffer);
        TVS_LOG_PRINTF("write failed, buffer %p, mbuf len %d, write size %d, buf init %d\n",
                       buffer, buffer->content_buffer.len, data_size, buffer->buf_init);
        return -1;
    }

    if (buffer->content_buffer.size != buffer->max_size) {
        mbuf_resize(&buffer->content_buffer, buffer->max_size);
    }

    if (data_size + buffer->content_buffer.len > buffer->max_size) {
        // full
        tvs_data_buffer_unlock(buffer);
        return TVS_ERROR_DATA_BUFFER_FULL;
    }

    mbuf_append(&buffer->content_buffer, data, data_size);
    size = data_size;
    tvs_data_buffer_unlock(buffer);

    tvs_data_buffer_notify_consumer(buffer);

    return size;
}

char *tvs_data_buffer_read_ptr(tvs_data_buffer *buffer)
{
    if (buffer == NULL) {
        return NULL;
    }
    return buffer->content_buffer.buf;
}

void tvs_data_buffer_remove_not_lock(tvs_data_buffer *buffer, int size)
{
    if (buffer == NULL) {
        return;
    }

    mbuf_remove(&buffer->content_buffer, size);
}

int tvs_data_buffer_read(tvs_data_buffer *buffer, char *data_buf, int data_buf_len)
{

    tvs_data_buffer_lock(buffer);
    if (buffer == NULL || buffer->content_buffer.len <= 0 || !buffer->buf_init) {
        TVS_LOG_PRINTF("read failed, buffer %p, mbuf len %d, need size %d, buf init %d\n",
                       buffer, buffer->content_buffer.len, data_buf_len, buffer->buf_init);
        tvs_data_buffer_unlock(buffer);
        return -1;
    }

    data_buf_len = data_buf_len > buffer->content_buffer.len ? buffer->content_buffer.len : data_buf_len;

    memcpy(data_buf, buffer->content_buffer.buf, data_buf_len);

    mbuf_remove(&buffer->content_buffer, data_buf_len);

    tvs_data_buffer_unlock(buffer);

    tvs_data_buffer_notify_producer(buffer);

    return data_buf_len;
}

bool tvs_data_buffer_is_producer_alive(tvs_data_buffer *buffer)
{
    if (buffer == NULL) {
        return false;
    }

    bool alive = false;

    tvs_data_buffer_lock(buffer);
    alive = buffer->producer_alive;
    tvs_data_buffer_unlock(buffer);

    return alive;
}

void tvs_data_buffer_set_producer_alive(tvs_data_buffer *buffer, bool alive)
{
    if (buffer == NULL) {
        return;
    }

    tvs_data_buffer_lock(buffer);
    buffer->producer_alive = alive;
    tvs_data_buffer_unlock(buffer);
    tvs_data_buffer_notify_consumer(buffer);
}

bool tvs_data_buffer_is_consumer_alive(tvs_data_buffer *buffer)
{
    if (buffer == NULL) {
        return false;
    }

    bool alive = false;

    tvs_data_buffer_lock(buffer);
    alive = buffer->consumer_alive;
    tvs_data_buffer_unlock(buffer);

    return alive;
}

void tvs_data_buffer_set_consumer_alive(tvs_data_buffer *buffer, bool alive)
{
    if (buffer == NULL) {
        return;
    }

    tvs_data_buffer_lock(buffer);
    buffer->consumer_alive = alive;
    tvs_data_buffer_unlock(buffer);

    tvs_data_buffer_notify_producer(buffer);
}

