
#ifndef _264_STREAM_IFACE_H_
#define _264_STREAM_IFACE_H_

void stream_start(void);
void stream_stop_clr(void);
unsigned int stream_read(unsigned char *buf, unsigned int len);
void stream_dec_frame(void);


#endif
