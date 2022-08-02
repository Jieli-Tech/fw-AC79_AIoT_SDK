#undef _typedef_h_

#if 0
#include "stddef.h"
#include "vfs.h"

void *coap_fopen(const char *path, const char *mode)
{
    if (*mode == 'r') {
        return mutex_f_Open(path, 0x04);
    } else i
        f(*mode == 'w') {
        mute_f_delete(path);
        return mutex_f_Open(path, 0x01);
    }
}

int coap_fclose(void *stream)
{
    if (stream != NULL) {
        mutex_f_Close(stream);
    }

    return 0;
}

inline unsigned int coap_fread(void *buffer, size_t size, size_t count, void *stream)
{
    return mutex_f_Read(stream, buffer, size * count);
}

inline unsigned int coap_fwrite(const void *buffer, size_t size, size_t count, void *stream)
{
    return mutex_f_Write(stream, buffer, size * count);
}


inline int coap_get_file_len(const char *file_name)
{
    return mutex_f_size(file_name);

}
#endif
