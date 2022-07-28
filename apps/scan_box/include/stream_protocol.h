#ifndef __STRM_PTL_H_
#define __STRM_PTL_H_


int stream_protocol_task_create(char *path, char *mode);
int stream_protocol_task_kill(void);
int stream_packet_cb(u8 type, u8 *buf, u32 size);

#endif

