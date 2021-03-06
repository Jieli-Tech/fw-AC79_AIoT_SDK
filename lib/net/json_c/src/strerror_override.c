#define STRERROR_OVERRIDE_IMPL 1
#include "strerror_override.h"

/*
 * Override strerror() to get consistent output across platforms.
 */

static const struct {
    int errno_value;
    const char *errno_str;
} errno_list[] = {
#define STRINGIFY(x) #x
#define ENTRY(x) {x, &STRINGIFY(undef_ ## x)[6]}
    ENTRY(EPERM),
    ENTRY(ENOENT),
    ENTRY(ESRCH),
    ENTRY(EINTR),
    ENTRY(EIO),
    ENTRY(ENXIO),
    ENTRY(E2BIG),
    ENTRY(ENOEXEC),
    ENTRY(EBADF),
    ENTRY(ECHILD),
    ENTRY(EDEADLK),
    ENTRY(ENOMEM),
    ENTRY(EACCES),
    ENTRY(EFAULT),
#ifdef ENOTBLK
    ENTRY(ENOTBLK),
#endif
    ENTRY(EBUSY),
    ENTRY(EEXIST),
    ENTRY(EXDEV),
    ENTRY(ENODEV),
    ENTRY(ENOTDIR),
    ENTRY(EISDIR),
    ENTRY(EINVAL),
    ENTRY(ENFILE),
    ENTRY(EMFILE),
    ENTRY(ENOTTY),
#ifdef ETXTBSY
    ENTRY(ETXTBSY),
#endif
    ENTRY(EFBIG),
    ENTRY(ENOSPC),
    ENTRY(ESPIPE),
    ENTRY(EROFS),
    ENTRY(EMLINK),
    ENTRY(EPIPE),
    ENTRY(EDOM),
    ENTRY(ERANGE),
    ENTRY(EAGAIN),
    { 0, (char *)0 }
};

#if CONFIG_JSONC_STRERROR_ENABLE
static const int _json_c_strerror_enable = 1;
#else
// Enabled during tests
static const int _json_c_strerror_enable = 0;
#endif

#define PREFIX "ERRNO="

#if CONFIG_JSONC_STRERROR_ENABLE
static char errno_buf[128] = PREFIX;
#endif

char *_json_c_strerror(int errno_in)
{
    int start_idx;
    char digbuf[20];
    int ii, jj;

    if (!_json_c_strerror_enable) {
        return strerror(errno_in);
    }

#if CONFIG_JSONC_STRERROR_ENABLE
    // Avoid standard functions, so we don't need to include any
    // headers, or guess at signatures.

    for (ii = 0; errno_list[ii].errno_str != (char *)0; ii++) {
        const char *errno_str = errno_list[ii].errno_str;
        if (errno_list[ii].errno_value != errno_in) {
            continue;
        }

        for (start_idx = sizeof(PREFIX) - 1, jj = 0; errno_str[jj] != '\0'; jj++, start_idx++) {
            errno_buf[start_idx] = errno_str[jj];
        }
        errno_buf[start_idx] = '\0';
        return errno_buf;
    }

    // It's not one of the known errno values, return the numeric value.
    for (ii = 0; errno_in > 10; errno_in /= 10, ii++) {
        digbuf[ii] = "0123456789"[(errno_in % 10)];
    }
    digbuf[ii] = "0123456789"[(errno_in % 10)];

    // Reverse the digits
    for (start_idx = sizeof(PREFIX) - 1 ; ii >= 0; ii--, start_idx++) {
        errno_buf[start_idx] = digbuf[ii];
    }
    return errno_buf;
#else
    return NULL;
#endif
}

