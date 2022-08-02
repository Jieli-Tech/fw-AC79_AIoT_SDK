#include "nopoll_mbedtls.h"

unsigned long ERR_get_error(void)
{
    printf("ERR_get_error: Not supported yet!");
    return 0;
}

void ERR_error_string_n(unsigned long e, char *buf, unsigned long len)
{
    printf("ERR_error_string_n: Not supported yet!");
    strncpy(buf, "Not Support", len - 1);
    buf[len - 1] = '\0';
}

