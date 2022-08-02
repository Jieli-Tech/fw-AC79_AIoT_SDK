#ifndef _SDIO_ERR_H_
#define _SDIO_ERR_H_

enum {
    EIO = 1	,	/* I/O error */
    EINVAL		,	/* Invalid argument */
    ERANGE		,	/* Math result not representable */
    ETIMEDOUT	,	/* Connection timed out */
    ETIME	,	/* Timer expired */
    EBUSY		,	/* Device or resource busy */
    EAGAIN		,	/* Try again */
    EOPNOTSUPP	,	/* Operation not supported on transport endpoint */
    EILSEQ		,	/* Illegal byte sequence */
    ENOENT		,	/* No such file or directory */
    ENOMEM		,	/* Out of Memory */
    ENOSYS		,	/* Function not implemented */
    EFAULT		,	/* Bad address */
};

#endif  //_SDIO_ERR_H_
