#ifndef VIDEOBUF_H
#define VIDEOBUF_H


#include "video/video.h"


void videobuf_queue_init(struct videobuf_queue *q, int align, const char *name);

int videobuf_reqbufs(struct videobuf_queue *q, struct video_reqbufs *req);


int videobuf_dqbuf(struct videobuf_queue *q, struct video_buffer *b);

int videobuf_qbuf(struct videobuf_queue *q, struct video_buffer *b);

int videobuf_streamon(struct videobuf_queue *q, u8 *channel);

int videobuf_streamoff(struct videobuf_queue *q, u8 channel);

int videobuf_clear_stream(struct videobuf_queue *q, u8 channel);

struct videobuf_buffer *videobuf_stream_alloc(struct videobuf_queue *q, u32 size);

struct videobuf_buffer *videobuf_stream_realloc(struct videobuf_queue *q,
        struct videobuf_buffer *b, int size);

u32 videobuf_stream_free_space(struct videobuf_queue *q);

void videobuf_stream_free(struct videobuf_queue *q, struct videobuf_buffer *b);


int videobuf_stream_finish(struct videobuf_queue *q, struct videobuf_buffer *b);


int videobuf_query(struct videobuf_queue *q, struct videobuf_state *sta);

void videobuf_queue_release(struct videobuf_queue *q);











#endif

