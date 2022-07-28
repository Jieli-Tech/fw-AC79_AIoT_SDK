#ifndef BIGINT_IMPL_HEADER
#define BIGINT_IMPL_HEADER
#include "generic/typedef.h"

// #include "ssp_config.h"
#define CONFIG_BIGINT_BARRETT 1
//#define CONFIG_BIGINT_CRT 1
#define CONFIG_BIGINT_SQUARE 1
//#define CONFIG_INTEGER_16BIT 1
//#define CONFIG_BIGINT_SLIDING_WINDOW
//#define CONFIG_BIGINT_MONTGOMERY 1
//#define CONFIG_BIGINT_KARATSUBA
//#define SQU_KARATSUBA_THRESH 32
//#define MUL_KARATSUBA_THRESH 64

/* Maintain a number of precomputed variables when doing reduction */
#define BIGINT_M_OFFSET     0    /**< Normal modulo offset. */
#ifdef CONFIG_BIGINT_CRT
#define BIGINT_P_OFFSET     1    /**< p modulo offset. */
#define BIGINT_Q_OFFSET     2    /**< q module offset. */
#define BIGINT_NUM_MODS     3    /**< The number of modulus constants used. */
#else
#define BIGINT_NUM_MODS     2
#endif

/* Architecture specific functions for big ints */
#if defined(CONFIG_INTEGER_8BIT)
#define COMP_RADIX          256U       /**< Max component + 1 */
#define COMP_MAX            0xFFFFU/**< (Max dbl comp -1) */
#define COMP_BIT_SIZE       8   /**< Number of bits in a component. */
#define COMP_BYTE_SIZE      1   /**< Number of bytes in a component. */
#define COMP_NUM_NIBBLES    2   /**< Used For diagnostics only. */
typedef u8 comp;	        /**< A single precision component. */
typedef u16 long_comp;     /**< A double precision component. */
typedef int16_t slong_comp;     /**< A signed double precision component. */
#elif defined(CONFIG_INTEGER_16BIT)
#define COMP_RADIX          65536U       /**< Max component + 1 */
#define COMP_MAX            0xFFFFFFFFU/**< (Max dbl comp -1) */
#define COMP_BIT_SIZE       16  /**< Number of bits in a component. */
#define COMP_BYTE_SIZE      2   /**< Number of bytes in a component. */
#define COMP_NUM_NIBBLES    4   /**< Used For diagnostics only. */
typedef u16 comp;	        /**< A single precision component. */
typedef u32 long_comp;     /**< A double precision component. */
typedef s32 slong_comp;     /**< A signed double precision component. */
#else /* regular 32 bit */
#if defined(WIN32) && !defined(__GNUC__)
#define COMP_RADIX          4294967296i64
#define COMP_MAX            0xFFFFFFFFFFFFFFFFui64
#else
#define COMP_RADIX          4294967296ULL         /**< Max component + 1 */
#define COMP_MAX            0xFFFFFFFFFFFFFFFFULL/**< (Max dbl comp -1) */
#endif

#define COMP_BIT_SIZE       32  /**< Number of bits in a component. */
#define COMP_BYTE_SIZE      4   /**< Number of bytes in a component. */
#define COMP_NUM_NIBBLES    8   /**< Used For diagnostics only. */
typedef u32 comp;	        /**< A single precision component. */
typedef unsigned long long long_comp;     /**< A double precision component. */
typedef long long slong_comp;     /**< A signed double precision component. */
#endif

/**
 * @struct  _bigint
 * @brief A big integer basic object
 */
struct _bigint {
    struct _bigint *next;       /**< The next bigint in the cache. */
    short size;                 /**< The number of components in this bigint. */
    short max_comps;            /**< The heapsize allocated for this bigint */
    int refs;                   /**< An internal reference count. */
    comp *comps;                /**< A ptr to the actual component data */
};

typedef struct _bigint bigint;  /**< An alias for _bigint */

/**
 * Maintains the state of the cache, and a number of variables used in
 * reduction.
 */
typedef struct { /**< A big integer "session" context. */
    bigint *active_list;                    /**< Bigints currently used. */
    bigint *free_list;                      /**< Bigints not used. */
    bigint *bi_radix;                       /**< The radix used. */
    bigint *bi_mod[BIGINT_NUM_MODS];        /**< modulus */

#if defined(CONFIG_BIGINT_MONTGOMERY)
    bigint *bi_RR_mod_m[BIGINT_NUM_MODS];   /**< R^2 mod m */
    bigint *bi_R_mod_m[BIGINT_NUM_MODS];    /**< R mod m */
    comp N0_dash[BIGINT_NUM_MODS];
#elif defined(CONFIG_BIGINT_BARRETT)
    bigint *bi_mu[BIGINT_NUM_MODS];         /**< Storage for mu */
#endif
    bigint *bi_normalised_mod[BIGINT_NUM_MODS]; /**< Normalised mod storage. */
    bigint **g;                 /**< Used by sliding-window. */
    int window;                 /**< The size of the sliding window */
    int active_count;           /**< Number of active bigints. */
    int free_count;             /**< Number of free bigints. */

#ifdef CONFIG_BIGINT_MONTGOMERY
    u8 use_classical;      /**< Use classical reduction. */
#endif
    u8 mod_offset;         /**< The mod offset we are using */
    int np_bit;
    char *mem_pool;
    int tol_used;
} BI_CTX;


#define PERMANENT           0x7FFF55AA  /**< A magic number for permanents. */

#endif

