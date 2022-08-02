
#ifndef __FS_PRIV_H__
#define __FS_PRIV_H__

//#include <string.h>

void *fs_priv_fopen(char *path, const char *mode);
int fs_priv_fclose(void *stream);
unsigned int fs_priv_fread(void *buffer, size_t size, size_t count, void *stream);
unsigned int fs_priv_fwrite(const void *buffer, size_t size, size_t count, void *stream);
int fs_priv_fseek(void *stream, long offset, int fromwhere);
int fs_priv_stat(const char *file_name,  struct stat *buf);

#endif

