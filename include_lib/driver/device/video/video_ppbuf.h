#ifndef VDIEO_PPBUF_H
#define VDIEO_PPBUF_H

#include "video/video_ioctl.h"
#include "generic/atomic.h"

#define  INUSED		BIT(0)
#define  OUTPUT		BIT(1)
#define  REQUEST	BIT(2)
#define  REQ_ONECE	BIT(3)

struct video_ppbuf {
    u8 *buf;
    u32 len;
    u32 size;
    u32 state;
    atomic_t open;
    atomic_t finish;
    atomic_t req;
    int req_vbuf[2];
};

void *video_ppbuf_open(void);
void video_ppbuf_close(void *priv);
char *video_ppbuf_malloc(void *priv, int size);
void video_ppbuf_free(void *priv, char *buf);
int video_ppbuf_output(void *priv, char *buf);
int video_ppbuf_request(void *priv, struct video_buffer *b);
int video_ppbuf_free_size(void *priv);
char *video_ppbuf_realloc(void *priv, char *buf, int size);
int video_ppbuf_reqbufs(struct video_reqbufs *b);
int video_ppbuf_size(void *priv, char *buf);
int video_ppbuf_set_fps(void *priv, int fps);
int video_ppbuf_set_cyctime(void *priv, int time);
int video_ppbuf_cyc_save_file_complet(void *priv);

#endif


