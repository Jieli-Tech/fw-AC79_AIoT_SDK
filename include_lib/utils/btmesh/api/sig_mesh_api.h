#ifndef __SIG_MESH_API_H__
#define __SIG_MESH_API_H__

struct net_buf_simple;
struct bt_mesh_subnet;
struct bt_mesh_net_rx;

#include "api/basic_depend.h"
#include "api/mesh_config.h"
#include "kernel/atomic_h.h"
#include "api/access.h"
#include "api/mesh_main.h"
#include "api/proxy.h"
#include "api/cfg_cli.h"
#include "api/cfg_srv.h"
#include "api/health_cli.h"
#include "api/health_srv.h"
#include "api/cdb.h"

/*******************************************************************/
/*
 *-------------------   common
 */
#define __noinit

#define ADV_SCAN_UNIT(_ms) ((_ms) * 8 / 5)

#define BT_MESH_ADDR_IS_UNICAST(addr) ((addr) && (addr) < 0x8000)
#define BT_MESH_ADDR_IS_GROUP(addr) ((addr) >= 0xc000 && (addr) <= 0xff00)
#define BT_MESH_ADDR_IS_VIRTUAL(addr) ((addr) >= 0x8000 && (addr) < 0xc000)
#define BT_MESH_ADDR_IS_RFU(addr) ((addr) >= 0xff00 && (addr) <= 0xfffb)

// -- output terminal color
#define RedBold             "\033[31;1;7m" // 红色加粗
#define RedBoldBlink        "\033[31;1;5m" // 红色加粗、闪烁
#define BlueBold            "\033[34;1;7m" // 蓝色加粗
#define BlueBoldBlink       "\033[34;1;5m" // 蓝色加粗、闪烁
#define PurpleBold          "\033[35;1m"   // 紫色加粗
#define PurpleBoldBlink     "\033[35;1;5m" // 紫色加粗、闪烁
#define Reset               "\033[0;25m"   // 颜色复位

//< error type
#define ENONE           0  /* Err None */
#define	EPERM		    1	/* Operation not permitted */
#define	ENOENT		    2	/* No such file or directory */
#define	ESRCH		    3	/* No such process */
#define	EINTR		    4	/* Interrupted system call */
#define	EIO		        5	/* I/O error */
#define	ENXIO		    6	/* No such device or address */
#define	E2BIG		    7	/* Argument list too long */
#define	ENOEXEC		    8	/* Exec format error */
#define	EBADF		    9	/* Bad file number */
#define	ECHILD		    10	/* No child processes */
#define	EAGAIN		    11	/* Try again */
#define	ENOMEM		    12	/* Out of memory */
#define	EACCES		    13	/* Permission denied */
#define	EFAULT		    14	/* Bad address */
#define	ENOTBLK		    15	/* Block device required */
#define	EBUSY		    16	/* Device or resource busy */
#define	EEXIST		    17	/* File exists */
#define	EXDEV		    18	/* Cross-device link */
#define	ENODEV		    19	/* No such device */
#define	ENOTDIR		    20	/* Not a directory */
#define	EISDIR		    21	/* Is a directory */
#define	EINVAL		    22	/* Invalid argument */
#define	ENFILE		    23	/* File table overflow */
#define	EMFILE		    24	/* Too many open files */
#define	ENOTTY		    25	/* Not a typewriter */
#define	ETXTBSY		    26	/* Text file busy */
#define	EFBIG		    27	/* File too large */
#define	ENOSPC		    28	/* No space left on device */
#define	ESPIPE		    29	/* Illegal seek */
#define	EROFS		    30	/* Read-only file system */
#define	EMLINK		    31	/* Too many links */
#define	EPIPE		    32	/* Broken pipe */
#define	EDOM		    33	/* Math argument out of domain of func */
#define	ERANGE		    34	/* Math result not representable */
#define ENOTSUP         35      /* Unsupported value */
#define EMSGSIZE        36     /* Message size */
#define ENOBUFS         55      /* No buffer space available */
#define ENOTCONN        57     /* Socket is not connected */
#undef ETIMEDOUT
#define ETIMEDOUT       60    /* Connection timed out */
#define EALREADY        69    /* Operation already in progress */
#define ECANCELED       72 /* Operation canceled */
#define EBADMSG         77 /* Invalid STREAMS message */
#define SL_ERROR_BSD_EADDRNOTAVAIL          (-99L)  /* Cannot assign requested address */
#define EADDRNOTAVAIL                       SL_ERROR_BSD_EADDRNOTAVAIL

/**
 * @brief Check for macro definition in compiler-visible expressions
 *
 * This trick was pioneered in Linux as the config_enabled() macro.
 * The madness has the effect of taking a macro value that may be
 * defined to "1" (e.g. CONFIG_MYFEATURE), or may not be defined at
 * all and turning it into a literal expression that can be used at
 * "runtime".  That is, it works similarly to
 * "defined(CONFIG_MYFEATURE)" does except that it is an expansion
 * that can exist in a standard expression and be seen by the compiler
 * and optimizer.  Thus much ifdef usage can be replaced with cleaner
 * expressions like:
 *
 *     if (IS_ENABLED(CONFIG_MYFEATURE))
 *             myfeature_enable();
 *
 * INTERNAL
 * First pass just to expand any existing macros, we need the macro
 * value to be e.g. a literal "1" at expansion time in the next macro,
 * not "(1)", etc...  Standard recursive expansion does not work.
 */
#define IS_ENABLED(config_macro) _IS_ENABLED1(config_macro)

/* Now stick on a "_XXXX" prefix, it will now be "_XXXX1" if config_macro
 * is "1", or just "_XXXX" if it's undefined.
 *   ENABLED:   _IS_ENABLED2(_XXXX1)
 *   DISABLED   _IS_ENABLED2(_XXXX)
 */
#define _IS_ENABLED1(config_macro) _IS_ENABLED2(_XXXX##config_macro)

/* Here's the core trick, we map "_XXXX1" to "_YYYY," (i.e. a string
 * with a trailing comma), so it has the effect of making this a
 * two-argument tuple to the preprocessor only in the case where the
 * value is defined to "1"
 *   ENABLED:    _YYYY,    <--- note comma!
 *   DISABLED:   _XXXX
 *   DISABLED:   _XXXX0
 */
#define _XXXX1 _YYYY,
#define _XXXX0

/* Then we append an extra argument to fool the gcc preprocessor into
 * accepting it as a varargs macro.
 *                         arg1   arg2  arg3
 *   ENABLED:   _IS_ENABLED3(_YYYY,    1,    0)
 *   DISABLED   _IS_ENABLED3(_XXXX 1,  0)
 *   DISABLED   _IS_ENABLED3(_XXXX0 1,  0)
 */
#define _IS_ENABLED2(one_or_two_args) _IS_ENABLED3(one_or_two_args true, false)

/* And our second argument is thus now cooked to be 1 in the case
 * where the value is defined to 1, and 0 if not:
 */
#define _IS_ENABLED3(ignore_this, val, ...) val

/*******************************************************************/
/*
 *-------------------   models
 */

/** @def NET_BUF_SIMPLE_DEFINE
 *  @brief Define a net_buf_simple stack variable.
 *
 *  This is a helper macro which is used to define a net_buf_simple object
 *  on the stack.
 *
 *  @param _name Name of the net_buf_simple object.
 *  @param _size Maximum data storage for the buffer.
 */
#define NET_BUF_SIMPLE_DEFINE(_name, _size)     \
	u8_t net_buf_data_##_name[_size];       \
	struct net_buf_simple _name = {         \
		.data   = net_buf_data_##_name, \
		.len    = 0,                    \
		.size   = _size,                \
		.__buf  = net_buf_data_##_name, \
	}

/** @def NET_BUF_SIMPLE_DEFINE_STATIC
 *  @brief Define a static net_buf_simple variable.
 *
 *  This is a helper macro which is used to define a static net_buf_simple
 *  object.
 *
 *  @param _name Name of the net_buf_simple object.
 *  @param _size Maximum data storage for the buffer.
 */
#define NET_BUF_SIMPLE_DEFINE_STATIC(_name, _size)        \
	static __noinit u8_t net_buf_data_##_name[_size]; \
	static struct net_buf_simple _name = {            \
		.data   = net_buf_data_##_name,           \
		.len    = 0,                              \
		.size   = _size,                          \
		.__buf  = net_buf_data_##_name,           \
	}

/** @brief Simple network buffer representation.
 *
 *  This is a simpler variant of the net_buf object (in fact net_buf uses
 *  net_buf_simple internally). It doesn't provide any kind of reference
 *  counting, user data, dynamic allocation, or in general the ability to
 *  pass through kernel objects such as FIFOs.
 *
 *  The main use of this is for scenarios where the meta-data of the normal
 *  net_buf isn't needed and causes too much overhead. This could be e.g.
 *  when the buffer only needs to be allocated on the stack or when the
 *  access to and lifetime of the buffer is well controlled and constrained.
 *
 */
struct net_buf_simple {
    /** Pointer to the start of data in the buffer. */
    u8_t *data;

    /** Length of the data behind the data pointer. */
    u16_t len;

    /** Amount of data that this buffer can store. */
    u16_t size;

    /** Start of the data storage. Not to be accessed directly
     *  (the data pointer should be used instead).
     */
    u8_t *__buf;
};

/**
 * @brief Add a unsigned char variable at the tail of the buffer.
 *
 * @param buf Target buffer head address.
 *
 * @param val The variable will set.
 *
 * @return The tail address of the buffer.
 */
u8 *buffer_add_u8_at_tail(void *buf, u8 val);

/**
 * @brief Add a little-endian unsigned short variable at the tail of the buffer.
 *
 * @param buf Target buffer head address.
 *
 * @param val The variable will set.
 */
void buffer_add_le16_at_tail(void *buf, u16 val);

/**
 * @brief Add a big-endian unsigned short variable at the tail of the buffer.
 *
 * @param buf Target buffer head address.
 *
 * @param val The variable will set.
 */
void buffer_add_be16_at_tail(void *buf, u16 val);

/**
 * @brief Add a little-endian unsigned long variable at the tail of the buffer.
 *
 * @param buf Target buffer head address.
 *
 * @param val The variable will set.
 */
void buffer_add_le32_at_tail(void *buf, u32 val);

/**
 * @brief Add a big-endian unsigned long variable at the tail of the buffer.
 *
 * @param buf Target buffer head address.
 *
 * @param val The variable will set.
 */
void buffer_add_be32_at_tail(void *buf, u32 val);

/**
 * @brief Get the unsigned char variable from the buffer head address.
 *
 * @param buf Target buffer head address.
 *
 * @return Target variable.
 */
u8 buffer_pull_u8_from_head(void *buf);

/**
 * @brief Get the little-endian unsigned short variable from the buffer head address.
 *
 * @param buf Target buffer head address.
 *
 * @return Target variable.
 */
u16 buffer_pull_le16_from_head(void *buf);

/**
 * @brief Get the big-endian unsigned short variable from the buffer head address.
 *
 * @param buf Target buffer head address.
 *
 * @return Target variable.
 */
u16 buffer_pull_be16_from_head(void *buf);

/**
 * @brief Get the little-endian unsigned long variable from the buffer head address.
 *
 * @param buf Target buffer head address.
 *
 * @return Target variable.
 */
u32 buffer_pull_le32_from_head(void *buf);

/**
 * @brief Get the big-endian unsigned long variable from the buffer head address.
 *
 * @param buf Target buffer head address.
 *
 * @return Target variable.
 */
u32 buffer_pull_be32_from_head(void *buf);

/**
 * @brief Memcpy a array at the tail of the buffer.
 *
 * @param buf Target buffer head address.
 *
 * @param mem The source memory address.
 *
 * @param len The copy length.
 *
 * @return The result of the process : 0 is succ.
 */
void *buffer_memcpy(void *buf, const void *mem, u32 len);

/**
 * @brief Memset at the tail of the buffer.
 *
 * @param buf Target buffer head address.
 *
 * @param mem The set value.
 *
 * @param len The set length.
 *
 * @return The result of the process : 0 is succ.
 */
void *buffer_memset(struct net_buf_simple *buf, u8 val, u32 len);

/**
 * @brief Init the opcode at the head of the buffer.
 *
 * @param opcode Big-endian opcode.
 *
 * @return Little-endian opcode.
 */
u32 buffer_head_init(u32 opcode);

/**
 * @brief Loading the node info from storage (such as flash and so on).
 */
void settings_load(void);

/**
 * @brief
 *
 * @param buf Input & output buffer.
 * @param len Buffer length.
 *
 * @return 0.
 */
int bt_rand(void *buf, size_t len);

#endif /* __SIG_MESH_API_H__ */
