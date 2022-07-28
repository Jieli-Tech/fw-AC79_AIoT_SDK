/*
*********************************************************************************************************
*                                               uC/LIB
*                                       Custom Library Modules
*
*                    Copyright 2004-2020 Silicon Laboratories Inc. www.silabs.com
*
*                                 SPDX-License-Identifier: APACHE-2.0
*
*               This software is subject to an open source license and is distributed by
*                Silicon Laboratories Inc. pursuant to the terms of the Apache License,
*                    Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                     STANDARD MEMORY OPERATIONS
*
* Filename : lib_mem.c
* Version  : V1.39.00
*********************************************************************************************************
* Note(s)  : (1) NO compiler-supplied standard library functions are used in library or product software.
*
*                (a) ALL standard library functions are implemented in the custom library modules :
*
*                    (1) \<Custom Library Directory>\lib_*.*
*
*                    (2) \<Custom Library Directory>\Ports\<cpu>\<compiler>\lib*_a.*
*
*                          where
*                                  <Custom Library Directory>      directory path for custom library software
*                                  <cpu>                           directory name for specific processor (CPU)
*                                  <compiler>                      directory name for specific compiler
*
*                (b) Product-specific library functions are implemented in individual products.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    MICRIUM_SOURCE
#define    LIB_MEM_MODULE
#include  "lib_mem.h"
#include  "lib_math.h"
#include  "lib_str.h"



/*
*********************************************************************************************************
*                                              Mem_Clr()
*
* Description : Clears data buffer (see Note #2).
*
* Argument(s) : pmem        Pointer to memory buffer to clear.
*
*               size        Number of data buffer octets to clear (see Note #1).
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Null clears allowed (i.e. zero-length clears).
*
*                   See also 'Mem_Set()  Note #1'.
*
*               (2) Clear data by setting each data octet to 0.
*********************************************************************************************************
*/

void  Mem_Clr(void        *pmem,
              CPU_SIZE_T   size)
{
    Mem_Set(pmem,
            0u,                                                 /* See Note #2.                                         */
            size);
}


/*
*********************************************************************************************************
*                                              Mem_Set()
*
* Description : Fills data buffer with specified data octet.
*
* Argument(s) : pmem        Pointer to memory buffer to fill with specified data octet.
*
*               data_val    Data fill octet value.
*
*               size        Number of data buffer octets to fill (see Note #1).
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Null sets allowed (i.e. zero-length sets).
*
*               (2) For best CPU performance, optimized to fill data buffer using 'CPU_ALIGN'-sized data
*                   words. Since many word-aligned processors REQUIRE that multi-octet words be accessed on
*                   word-aligned addresses, 'CPU_ALIGN'-sized words MUST be accessed on 'CPU_ALIGN'd
*                   addresses.
*
*               (3) Modulo arithmetic is used to determine whether a memory buffer starts on a 'CPU_ALIGN'
*                   address boundary.
*
*                   Modulo arithmetic in ANSI-C REQUIREs operations performed on integer values.  Thus
*                   address values MUST be cast to an appropriately-sized integer value PRIOR to any
*                  'mem_align_mod' arithmetic operation.
*********************************************************************************************************
*/

void  Mem_Set(void        *pmem,
              CPU_INT08U   data_val,
              CPU_SIZE_T   size)
{
    CPU_SIZE_T   size_rem;
    CPU_ALIGN    data_align;
    CPU_ALIGN   *pmem_align;
    CPU_INT08U  *pmem_08;
    CPU_DATA     mem_align_mod;
    CPU_DATA     i;


#if (LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (size < 1) {                                             /* See Note #1.                                         */
        return;
    }
    if (pmem == (void *)0) {
        return;
    }
#endif


    data_align = 0u;
    for (i = 0u; i < sizeof(CPU_ALIGN); i++) {                  /* Fill each data_align octet with data val.            */
        data_align <<=  DEF_OCTET_NBR_BITS;
        data_align  |= (CPU_ALIGN)data_val;
    }

    size_rem      =  size;
    mem_align_mod = (CPU_INT08U)((CPU_ADDR)pmem % sizeof(CPU_ALIGN));   /* See Note #3.                                 */

    pmem_08 = (CPU_INT08U *)pmem;
    if (mem_align_mod != 0u) {                                  /* If leading octets avail,                   ...       */
        i = mem_align_mod;
        while ((size_rem > 0) &&                                /* ... start mem buf fill with leading octets ...       */
               (i        < sizeof(CPU_ALIGN))) {                /* ... until next CPU_ALIGN word boundary.              */
            *pmem_08++ = data_val;
            size_rem -= sizeof(CPU_INT08U);
            i++;
        }
    }

    pmem_align = (CPU_ALIGN *)pmem_08;                          /* See Note #2.                                         */
    while (size_rem >= sizeof(CPU_ALIGN)) {                     /* While mem buf aligned on CPU_ALIGN word boundaries,  */
        *pmem_align++ = data_align;                              /* ... fill mem buf with    CPU_ALIGN-sized data.       */
        size_rem    -= sizeof(CPU_ALIGN);
    }

    pmem_08 = (CPU_INT08U *)pmem_align;
    while (size_rem > 0) {                                      /* Finish mem buf fill with trailing octets.            */
        *pmem_08++   = data_val;
        size_rem   -= sizeof(CPU_INT08U);
    }
}


/*
*********************************************************************************************************
*                                             Mem_Copy()
*
* Description : Copies data octets from one memory buffer to another memory buffer.
*
* Argument(s) : pdest       Pointer to destination memory buffer.
*
*               psrc        Pointer to source      memory buffer.
*
*               size        Number of octets to copy (see Note #1).
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Null copies allowed (i.e. zero-length copies).
*
*               (2) Memory buffers NOT checked for overlapping.
*
*                   (a) IEEE Std 1003.1, 2004 Edition, Section 'memcpy() : DESCRIPTION' states that "if
*                       copying takes place between objects that overlap, the behavior is undefined".
*
*                   (b) However, data octets from a source memory buffer at a higher address value SHOULD
*                       successfully copy to a destination memory buffer at a lower  address value even
*                       if any octets of the memory buffers overlap as long as no individual, atomic CPU
*                       word copy overlaps.
*
*                       Since Mem_Copy() performs the data octet copy via 'CPU_ALIGN'-sized words &/or
*                       octets; & since 'CPU_ALIGN'-sized words MUST be accessed on word-aligned addresses
*                       (see Note #3b), neither 'CPU_ALIGN'-sized words nor octets at unique addresses can
*                       ever overlap.
*
*                       Therefore, Mem_Copy() SHOULD be able to successfully copy overlapping memory
*                       buffers as long as the source memory buffer is at a higher address value than the
*                       destination memory buffer.
*
*               (3) For best CPU performance, optimized to copy data buffer using 'CPU_ALIGN'-sized data
*                   words. Since many word-aligned processors REQUIRE that multi-octet words be accessed on
*                   word-aligned addresses, 'CPU_ALIGN'-sized words MUST be accessed on 'CPU_ALIGN'd
*                   addresses.
*
*               (4) Modulo arithmetic is used to determine whether a memory buffer starts on a 'CPU_ALIGN'
*                   address boundary.
*
*                   Modulo arithmetic in ANSI-C REQUIREs operations performed on integer values.  Thus
*                   address values MUST be cast to an appropriately-sized integer value PRIOR to any
*                  'mem_align_mod' arithmetic operation.
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_OPTIMIZE_ASM_EN != DEF_ENABLED)
void  Mem_Copy(void        *pdest,
               const  void        *psrc,
               CPU_SIZE_T   size)
{
    CPU_SIZE_T    size_rem;
    CPU_SIZE_T    mem_gap_octets;
    CPU_ALIGN    *pmem_align_dest;
    const  CPU_ALIGN    *pmem_align_src;
    CPU_INT08U   *pmem_08_dest;
    const  CPU_INT08U   *pmem_08_src;
    CPU_DATA      i;
    CPU_DATA      mem_align_mod_dest;
    CPU_DATA      mem_align_mod_src;
    CPU_BOOLEAN   mem_aligned;


#if (LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (size < 1) {                                             /* See Note #1.                                         */
        return;
    }
    if (pdest == (void *)0) {
        return;
    }
    if (psrc  == (void *)0) {
        return;
    }
#endif


    size_rem           =  size;

    pmem_08_dest       = (CPU_INT08U *)pdest;
    pmem_08_src        = (const CPU_INT08U *)psrc;

    mem_gap_octets     = (CPU_SIZE_T)(pmem_08_src - pmem_08_dest);


    if (mem_gap_octets >= sizeof(CPU_ALIGN)) {                  /* Avoid bufs overlap.                                  */
        /* See Note #4.                                         */
        mem_align_mod_dest = (CPU_INT08U)((CPU_ADDR)pmem_08_dest % sizeof(CPU_ALIGN));
        mem_align_mod_src  = (CPU_INT08U)((CPU_ADDR)pmem_08_src  % sizeof(CPU_ALIGN));

        mem_aligned        = (mem_align_mod_dest == mem_align_mod_src) ? DEF_YES : DEF_NO;

        if (mem_aligned == DEF_YES) {                           /* If mem bufs' alignment offset equal, ...             */
            /* ... optimize copy for mem buf alignment.             */
            if (mem_align_mod_dest != 0u) {                     /* If leading octets avail,                   ...       */
                i = mem_align_mod_dest;
                while ((size_rem   >  0) &&                     /* ... start mem buf copy with leading octets ...       */
                       (i          <  sizeof(CPU_ALIGN))) {     /* ... until next CPU_ALIGN word boundary.              */
                    *pmem_08_dest++ = *pmem_08_src++;
                    size_rem      -=  sizeof(CPU_INT08U);
                    i++;
                }
            }

            pmem_align_dest = (CPU_ALIGN *)pmem_08_dest;        /* See Note #3.                                         */
            pmem_align_src  = (const CPU_ALIGN *)pmem_08_src;
            while (size_rem      >=  sizeof(CPU_ALIGN)) {       /* While mem bufs aligned on CPU_ALIGN word boundaries, */
                *pmem_align_dest++ = *pmem_align_src++;          /* ... copy psrc to pdest with CPU_ALIGN-sized words.   */
                size_rem         -=  sizeof(CPU_ALIGN);
            }

            pmem_08_dest = (CPU_INT08U *)pmem_align_dest;
            pmem_08_src  = (const CPU_INT08U *)pmem_align_src;
        }
    }

    while (size_rem > 0) {                                      /* For unaligned mem bufs or trailing octets, ...       */
        *pmem_08_dest++ = *pmem_08_src++;                        /* ... copy psrc to pdest by octets.                    */
        size_rem      -=  sizeof(CPU_INT08U);
    }
}
#endif


/*
*********************************************************************************************************
*                                             Mem_Move()
*
* Description : Moves data octets from one memory buffer to another memory buffer, or within the same
*               memory buffer. Overlapping is correctly handled for all move operations.
*
* Argument(s) : pdest       Pointer to destination memory buffer.
*
*               psrc        Pointer to source      memory buffer.
*
*               size        Number of octets to move (see Note #1).
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Null move operations allowed (i.e. zero-length).
*
*               (2) Memory buffers checked for overlapping.
*
*               (3) For best CPU performance, optimized to copy data buffer using 'CPU_ALIGN'-sized data
*                   words. Since many word-aligned processors REQUIRE that multi-octet words be accessed on
*                   word-aligned addresses, 'CPU_ALIGN'-sized words MUST be accessed on 'CPU_ALIGN'd
*                   addresses.
*
*               (4) Modulo arithmetic is used to determine whether a memory buffer starts on a 'CPU_ALIGN'
*                   address boundary.
*
*                   Modulo arithmetic in ANSI-C REQUIREs operations performed on integer values.  Thus
*                   address values MUST be cast to an appropriately-sized integer value PRIOR to any
*                  'mem_align_mod' arithmetic operation.
*********************************************************************************************************
*/

void  Mem_Move(void        *pdest,
               const  void        *psrc,
               CPU_SIZE_T   size)
{
    CPU_SIZE_T    size_rem;
    CPU_SIZE_T    mem_gap_octets;
    CPU_ALIGN    *pmem_align_dest;
    const  CPU_ALIGN    *pmem_align_src;
    CPU_INT08U   *pmem_08_dest;
    const  CPU_INT08U   *pmem_08_src;
    CPU_INT08S    i;
    CPU_DATA      mem_align_mod_dest;
    CPU_DATA      mem_align_mod_src;
    CPU_BOOLEAN   mem_aligned;


#if (LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (size < 1) {
        return;
    }
    if (pdest == (void *)0) {
        return;
    }
    if (psrc  == (void *)0) {
        return;
    }
#endif

    pmem_08_src  = (const CPU_INT08U *)psrc;
    pmem_08_dest = (CPU_INT08U *)pdest;
    if (pmem_08_src > pmem_08_dest) {
        Mem_Copy(pdest, psrc, size);
        return;
    }

    size_rem           =  size;

    pmem_08_dest       = (CPU_INT08U *)pdest + size - 1;
    pmem_08_src        = (const CPU_INT08U *)psrc  + size - 1;

    mem_gap_octets     = (CPU_SIZE_T)(pmem_08_dest - pmem_08_src);


    if (mem_gap_octets >= sizeof(CPU_ALIGN)) {                  /* Avoid bufs overlap.                                  */

        /* See Note #4.                                         */
        mem_align_mod_dest = (CPU_INT08U)((CPU_ADDR)pmem_08_dest % sizeof(CPU_ALIGN));
        mem_align_mod_src  = (CPU_INT08U)((CPU_ADDR)pmem_08_src  % sizeof(CPU_ALIGN));

        mem_aligned        = (mem_align_mod_dest == mem_align_mod_src) ? DEF_YES : DEF_NO;

        if (mem_aligned == DEF_YES) {                           /* If mem bufs' alignment offset equal, ...             */
            /* ... optimize copy for mem buf alignment.             */
            if (mem_align_mod_dest != (sizeof(CPU_ALIGN) - 1)) {/* If leading octets avail,                   ...       */
                i = (CPU_INT08S)mem_align_mod_dest;
                while ((size_rem   >  0) &&                     /* ... start mem buf copy with leading octets ...       */
                       (i          >= 0)) {                     /* ... until next CPU_ALIGN word boundary.              */
                    *pmem_08_dest-- = *pmem_08_src--;
                    size_rem      -=  sizeof(CPU_INT08U);
                    i--;
                }
            }

            /* See Note #3.                                         */
            pmem_align_dest = (CPU_ALIGN *)(((CPU_INT08U *)pmem_08_dest - sizeof(CPU_ALIGN)) + 1);
            pmem_align_src  = (const CPU_ALIGN *)(((CPU_INT08U *)pmem_08_src  - sizeof(CPU_ALIGN)) + 1);
            while (size_rem      >=  sizeof(CPU_ALIGN)) {       /* While mem bufs aligned on CPU_ALIGN word boundaries, */
                *pmem_align_dest-- = *pmem_align_src--;          /* ... copy psrc to pdest with CPU_ALIGN-sized words.   */
                size_rem         -=  sizeof(CPU_ALIGN);
            }

            pmem_08_dest = (CPU_INT08U *)pmem_align_dest + sizeof(CPU_ALIGN) - 1;
            pmem_08_src  = (const CPU_INT08U *)pmem_align_src  + sizeof(CPU_ALIGN) - 1;

        }
    }

    while (size_rem > 0) {                                      /* For unaligned mem bufs or trailing octets, ...       */
        *pmem_08_dest-- = *pmem_08_src--;                        /* ... copy psrc to pdest by octets.                    */
        size_rem      -=  sizeof(CPU_INT08U);
    }
}


/*
*********************************************************************************************************
*                                              Mem_Cmp()
*
* Description : Verifies that ALL data octets in two memory buffers are identical in sequence.
*
* Argument(s) : p1_mem      Pointer to first  memory buffer.
*
*               p2_mem      Pointer to second memory buffer.
*
*               size        Number of data buffer octets to compare (see Note #1).
*
* Return(s)   : DEF_YES, if 'size' number of data octets are identical in both memory buffers.
*
*               DEF_NO,  otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Null compares allowed (i.e. zero-length compares); 'DEF_YES' returned to indicate
*                   identical null compare.
*
*               (2) Many memory buffer comparisons vary ONLY in the least significant octets -- e.g.
*                   network address buffers.  Consequently, memory buffer comparison is more efficient
*                   if the comparison starts from the end of the memory buffers which will abort sooner
*                   on dissimilar memory buffers that vary only in the least significant octets.
*
*               (3) For best CPU performance, optimized to compare data buffers using 'CPU_ALIGN'-sized
*                   data words. Since many word-aligned processors REQUIRE that multi-octet words be accessed on
*                   word-aligned addresses, 'CPU_ALIGN'-sized words MUST be accessed on 'CPU_ALIGN'd
*                   addresses.
*
*               (4) Modulo arithmetic is used to determine whether a memory buffer starts on a 'CPU_ALIGN'
*                   address boundary.
*
*                   Modulo arithmetic in ANSI-C REQUIREs operations performed on integer values.  Thus
*                   address values MUST be cast to an appropriately-sized integer value PRIOR to any
*                  'mem_align_mod' arithmetic operation.
*********************************************************************************************************
*/

CPU_BOOLEAN  Mem_Cmp(const  void        *p1_mem,
                     const  void        *p2_mem,
                     CPU_SIZE_T   size)
{
    CPU_SIZE_T    size_rem;
    CPU_ALIGN    *p1_mem_align;
    CPU_ALIGN    *p2_mem_align;
    const  CPU_INT08U   *p1_mem_08;
    const  CPU_INT08U   *p2_mem_08;
    CPU_DATA      i;
    CPU_DATA      mem_align_mod_1;
    CPU_DATA      mem_align_mod_2;
    CPU_BOOLEAN   mem_aligned;
    CPU_BOOLEAN   mem_cmp;


    if (size < 1) {                                             /* See Note #1.                                         */
        return (DEF_YES);
    }
    if (p1_mem == (void *)0) {
        return (DEF_NO);
    }
    if (p2_mem == (void *)0) {
        return (DEF_NO);
    }


    mem_cmp         =  DEF_YES;                                 /* Assume mem bufs are identical until cmp fails.       */
    size_rem        =  size;
    /* Start @ end of mem bufs (see Note #2).               */
    p1_mem_08       = (const CPU_INT08U *)p1_mem + size;
    p2_mem_08       = (const CPU_INT08U *)p2_mem + size;
    /* See Note #4.                                         */
    mem_align_mod_1 = (CPU_INT08U)((CPU_ADDR)p1_mem_08 % sizeof(CPU_ALIGN));
    mem_align_mod_2 = (CPU_INT08U)((CPU_ADDR)p2_mem_08 % sizeof(CPU_ALIGN));

    mem_aligned     = (mem_align_mod_1 == mem_align_mod_2) ? DEF_YES : DEF_NO;

    if (mem_aligned == DEF_YES) {                               /* If mem bufs' alignment offset equal, ...             */
        /* ... optimize cmp for mem buf alignment.              */
        if (mem_align_mod_1 != 0u) {                            /* If trailing octets avail,                  ...       */
            i = mem_align_mod_1;
            while ((mem_cmp == DEF_YES) &&                      /* ... cmp mem bufs while identical &         ...       */
                   (size_rem > 0)       &&                      /* ... start mem buf cmp with trailing octets ...       */
                   (i        > 0)) {                            /* ... until next CPU_ALIGN word boundary.              */
                p1_mem_08--;
                p2_mem_08--;
                if (*p1_mem_08 != *p2_mem_08) {                 /* If ANY data octet(s) NOT identical, cmp fails.       */
                    mem_cmp = DEF_NO;
                }
                size_rem -= sizeof(CPU_INT08U);
                i--;
            }
        }

        if (mem_cmp == DEF_YES) {                               /* If cmp still identical, cmp aligned mem bufs.        */
            p1_mem_align = (CPU_ALIGN *)p1_mem_08;              /* See Note #3.                                         */
            p2_mem_align = (CPU_ALIGN *)p2_mem_08;

            while ((mem_cmp  == DEF_YES) &&                     /* Cmp mem bufs while identical & ...                   */
                   (size_rem >= sizeof(CPU_ALIGN))) {           /* ... mem bufs aligned on CPU_ALIGN word boundaries.   */
                p1_mem_align--;
                p2_mem_align--;
                if (*p1_mem_align != *p2_mem_align) {           /* If ANY data octet(s) NOT identical, cmp fails.       */
                    mem_cmp = DEF_NO;
                }
                size_rem -= sizeof(CPU_ALIGN);
            }

            p1_mem_08 = (CPU_INT08U *)p1_mem_align;
            p2_mem_08 = (CPU_INT08U *)p2_mem_align;
        }
    }

    while ((mem_cmp == DEF_YES) &&                              /* Cmp mem bufs while identical ...                     */
           (size_rem > 0)) {                                    /* ... for unaligned mem bufs or trailing octets.       */
        p1_mem_08--;
        p2_mem_08--;
        if (*p1_mem_08 != *p2_mem_08) {                         /* If ANY data octet(s) NOT identical, cmp fails.       */
            mem_cmp = DEF_NO;
        }
        size_rem -= sizeof(CPU_INT08U);
    }

    return (mem_cmp);
}



