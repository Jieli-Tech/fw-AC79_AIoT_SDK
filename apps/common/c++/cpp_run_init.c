#include "typedef.h"
void cpp_run_init(void)
{
    int i ;
    extern unsigned int ctors_begin, ctors_count ;
    unsigned int *dat = (unsigned int *)&ctors_begin ;
    unsigned int count = (unsigned int)&ctors_count ;
    void (*fun)() ;
    for (i = 0; i < (count) / 4 ; i++) {
        fun = (void (*)())dat[i];
        fun() ;
    }
}
