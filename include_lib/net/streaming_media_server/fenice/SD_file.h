#ifndef __SD_FILE_H__
#define __SD_FILE_H__

//#include <string.h>

extern void *sd_file_open(const char *name, char *mode);
extern void sd_file_close(void *sd_fd);
extern unsigned char *sd_file_gets(unsigned char *buf, unsigned int buf_len, void *_sd_fd);
extern int sd_file_name_modify(const char *ori_name,  char *dst_name);
#endif

