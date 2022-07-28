#ifndef _PTHREAD_H
#define _PTHREAD_H
#include "os/os_api.h"
#include <time.h>

typedef void *pthread_condattr_t;
typedef void *pthread_t;
typedef void *mqd_t;
typedef void *pthread_barrierattr_t;

typedef struct pthread_attr {
    uint32_t ulpthreadAttrStorage;
} PthreadAttrType_t;
typedef PthreadAttrType_t        pthread_attr_t;

typedef struct {
    StaticSemaphore_t xSemaphore; /**< FreeRTOS semaphore. */
    int value;                    /**< POSIX semaphore count. */
} sem_internal_t;
typedef sem_internal_t             PosixSemType_t;
typedef PosixSemType_t sem_t;

typedef struct pthread_mutexattr {
    uint32_t ulpthreadMutexAttrStorage;
} PthreadMutexAttrType_t;
typedef PthreadMutexAttrType_t   pthread_mutexattr_t;

typedef struct pthread_cond_internal {
    BaseType_t xIsInitialized;            /**< Set to pdTRUE if this condition variable is initialized, pdFALSE otherwise. */
    StaticSemaphore_t xCondWaitSemaphore; /**< Threads block on this semaphore in pthread_cond_wait. */
    unsigned iWaitingThreads;             /**< The number of threads currently waiting on this condition variable. */
} pthread_cond_internal_t;
typedef  pthread_cond_internal_t    PthreadCondType_t;
typedef  PthreadCondType_t       pthread_cond_t;

struct sched_param {
    int sched_priority;
};

typedef struct pthread_barrier_internal {
    unsigned uThreadCount;                   /**< Current number of threads that have entered barrier. */
    unsigned uThreshold;                     /**< The count argument of pthread_barrier_init. */
    StaticSemaphore_t xThreadCountSemaphore; /**< Prevents more than uThreshold threads from exiting pthread_barrier_wait at once. */
    StaticEventGroup_t xBarrierEventGroup;   /**< FreeRTOS event group that blocks to wait on threads entering barrier. */
} pthread_barrier_t;

struct mq_attr {
    long mq_flags;   /**< Message queue flags. */
    long mq_maxmsg;  /**< Maximum number of messages. */
    long mq_msgsize; /**< Maximum message size. */
    long mq_curmsgs; /**< Number of messages currently queued. */
};

typedef struct pthread_mutexattr_internal {
    int iType; /**< Mutex type. */
} pthread_mutexattr_internal_t;
typedef struct pthread_mutex_internal {
    BaseType_t xIsInitialized;          /**< Set to pdTRUE if this mutex is initialized, pdFALSE otherwise. */
    StaticSemaphore_t xMutex;           /**< FreeRTOS mutex. */
    TaskHandle_t xTaskOwner;            /**< Owner; used for deadlock detection and permission checks. */
    pthread_mutexattr_internal_t xAttr; /**< Mutex attributes. */
} pthread_mutex_internal_t;
typedef pthread_mutex_internal_t   PthreadMutexType_t;
typedef PthreadMutexType_t       pthread_mutex_t;

#define O_CLOEXEC      0x0001 /**< Close the file descriptor upon exec(). */
#define O_CREAT        0x0002 /**< Create file if it does not exist. */
#define O_DIRECTORY    0x0004 /**< Fail if file is a non-directory file. */
#define O_EXCL         0x0008 /**< Exclusive use flag. */
#define O_NOCTTY       0x0010 /**< Do not assign controlling terminal. */
#define O_NOFOLLOW     0x0020 /**< Do not follow symbolic links. */
#define O_TRUNC        0x0040 /**< Truncate flag. */
#define O_TTY_INIT     0x0080 /**< termios structure provides conforming behavior. */
#define O_SEARCH       0x4000  /**< Open directory for search only. */
#define O_EXEC         0x1000  /**< Open for execute only (non-directory files). */

#ifdef O_RDWR
#undef O_RDWR
#endif
#define O_RDWR         0xA000  /**< Open for reading and writing. */

#ifdef O_RDONLY
#undef O_RDONLY
#endif
#define O_RDONLY       0x2000  /**< Open for reading only. */

#ifdef O_WRONLY
#undef O_WRONLY
#endif
#define O_WRONLY       0x8000  /**< Open for writing only. */

/** pthread api*/
/**
 * @brief Destroy the thread attributes object.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_attr_destroy.html
 *
 * @retval 0 - Upon successful completion.
 */
int pthread_attr_destroy(pthread_attr_t *attr);

/**
 * @brief Get detachstate attribute.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_attr_getdetachstate.html
 *
 * @retval 0 - Upon successful completion.
 */
int pthread_attr_getdetachstate(const pthread_attr_t *attr,
                                int *detachstate);

/**
 * @brief Get schedparam attribute.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_attr_getschedparam.html
 *
 * @retval 0 - Upon successful completion.
 */
int pthread_attr_getschedparam(const pthread_attr_t *attr,
                               struct sched_param *param);

/**
 * @brief Get stacksize attribute.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_attr_getstacksize.html
 *
 * @retval 0 - Upon successful completion.
 */
int pthread_attr_getstacksize(const pthread_attr_t *attr,
                              size_t *stacksize);

/**
 * @brief Initialize the thread attributes object.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_attr_init.html
 *
 * @retval 0 - Upon successful completion.
 *
 * @note Currently, only stack size, sched_param, and detach state attributes
 * are supported. Also see pthread_attr_get*() and pthread_attr_set*().
 */
int pthread_attr_init(pthread_attr_t *attr);

/**
 * @brief Set detachstate attribute.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_attr_setdetachstate.html
 *
 * @retval 0 - Upon successful completion
 * @retval EINVAL - The value of detachstate is not valid. Currently, supported detach states are --
 *                  PTHREAD_CREATE_DETACHED and PTHREAD_CREATE_JOINABLE.
 */
int pthread_attr_setdetachstate(pthread_attr_t *attr,
                                int detachstate);

/**
 * @brief Set schedparam attribute.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_attr_setschedparam.html
 *
 * @retval 0 - Upon successful completion.
 * @retval EINVAL - The value of param is not valid.
 * @retval ENOTSUP - An attempt was made to set the attribute to an unsupported value.
 */
int pthread_attr_setschedparam(pthread_attr_t *attr,
                               const struct sched_param *param);

/**
 * @brief Set the schedpolicy attribute.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_attr_setschedpolicy.html
 *
 * @retval 0 - Upon successful completion.
 *
 * @warning This function is a stub and always returns 0.
 */
int pthread_attr_setschedpolicy(pthread_attr_t *attr,
                                int policy);

/**
 * @brief Set stacksize attribute.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_attr_setstacksize.html
 *
 * @retval 0 - Upon successful completion.
 * @retval EINVAL - The value of stacksize is less than {PTHREAD_STACK_MIN}.
 */
int pthread_attr_setstacksize(pthread_attr_t *attr,
                              size_t stacksize);

/**
 * @brief Destroy a barrier object.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_barrier_destroy.html
 *
 * @retval 0 - Upon successful completion.
 *
 * @note This function does not validate whether there is any thread blocking on the barrier before destroying.
 */
int pthread_barrier_destroy(pthread_barrier_t *barrier);

/**
 * @brief Initialize a barrier object.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_barrier_init.html
 *
 * @retval 0 - Upon successful completion.
 * @retval EINVAL - The value specified by count is equal to zero.
 * @retval ENOMEM - count cannot fit into FreeRTOS event group type OR insufficient memory exists to initialize the barrier.
 *
 * @note attr is ignored.
 *
 * @note pthread_barrier_init() is implemented with FreeRTOS event group.
 * To ensure count fits in event group, count may be at most 8 when configUSE_16_BIT_TICKS is 1;
 * it may be at most 24 otherwise. configUSE_16_BIT_TICKS is configured in application FreeRTOSConfig.h
 * file, which defines how many bits tick count type has. See further details and limitation about event
 * group and configUSE_16_BIT_TICKS in FreeRTOS site.
 */
int pthread_barrier_init(pthread_barrier_t *barrier,
                         const pthread_barrierattr_t *attr,
                         unsigned count);

/**
 * @brief Synchronize at a barrier.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_barrier_wait.html
 *
 * @retval PTHREAD_BARRIER_SERIAL_THREAD - Upon successful completion, the first thread.
 * @retval 0 - Upon successful completion, other thread(s).
 */
int pthread_barrier_wait(pthread_barrier_t *barrier);

/**
 * @brief Thread creation.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_create.html
 *
 * @retval 0 - Upon successful completion.
 * @retval EAGAIN - Insufficient memory for either thread structure or task creation.
 */
int pthread_create(pthread_t *thread,
                   const pthread_attr_t *attr,
                   void *(*startroutine)(void *),
                   void *arg);

/**
 * @brief Broadcast a condition.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_cond_broadcast.html
 *
 * @retval 0 - Upon successful completion.
 */
int pthread_cond_broadcast(pthread_cond_t *cond);

/**
 * @brief Destroy condition variables.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_cond_destroy.html
 *
 * @retval 0 - Upon successful completion.
 */
int pthread_cond_destroy(pthread_cond_t *cond);

/**
 * @brief Initialize condition variables.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_cond_init.html
 *
 * @retval 0 - Upon successful completion.
 * @retval ENOMEM - Insufficient memory exists to initialize the condition variable.
 *
 * @note attr is ignored and treated as NULL. Default setting is always used.
 */
int pthread_cond_init(pthread_cond_t *cond,
                      const pthread_condattr_t *attr);

/**
 * @brief Signal a condition.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_cond_signal.html
 *
 * @retval 0 - Upon successful completion.
 */
int pthread_cond_signal(pthread_cond_t *cond);

/**
 * @brief Wait on a condition with a timeout.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_cond_timedwait.html
 *
 * @retval 0 - Upon successful completion.
 * @retval EINVAL - The abstime argument passed in does not refer to an initialized structure OR
 *                  the abstime parameter specified a nanoseconds field value less than zero or
 *                  greater than or equal to 1000 million.
 * @retval ETIMEDOUT - The time specified by abstime to pthread_cond_timedwait() has passed.
 */

int pthread_cond_timedwait(pthread_cond_t *cond,
                           pthread_mutex_t *mutex,
                           const struct timespec *abstime);
/**
 * @brief Wait on a condition.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_cond_wait.html
 *
 * @retval 0 - Upon successful completion.
 */
int pthread_cond_wait(pthread_cond_t *cond,
                      pthread_mutex_t *mutex);

/**
 * @brief Compare thread IDs.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_equal.html
 *
 * @retval 0 - t1 and t2 are both not NULL && equal.
 * @retval non-zero - otherwise.
 */
int pthread_equal(pthread_t t1,
                  pthread_t t2);

/**
 * @brief Thread termination.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_exit.html
 *
 * @retval void - this function cannot return to its caller.
 */
void pthread_exit(void *value_ptr);

/**
 * @brief Dynamic thread scheduling parameters access.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_getschedparam.html
 *
 * @retval 0 - Upon successful completion.
 *
 * @note policy is always set to SCHED_OTHER by this function.
 */
int pthread_getschedparam(pthread_t thread,
                          int *policy,
                          struct sched_param *param);

/**
 * @brief Wait for thread termination.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_join.html
 *
 * @retval 0 - Upon successful completion.
 * @retval EDEADLK - The value specified by the thread argument to pthread_join() does not refer
 *                   to a joinable thread OR multiple simultaneous calls to pthread_join()
 *                   specifying the same target thread OR the value specified by the thread argument
 *                   to pthread_join() refers to the calling thread.
 */
int pthread_join(pthread_t thread,
                 void **retval);

/**
 * @brief Destroy a mutex.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_mutex_destroy.html
 *
 * @retval 0 - Upon successful completion.
 *
 * @note If there exists a thread holding this mutex, this function returns 0 with mutex not being destroyed.
 */
int pthread_mutex_destroy(pthread_mutex_t *mutex);

/**
 * @brief Initialize a mutex.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_mutex_init.html
 *
 * @retval 0 - Upon successful completion.
 * @retval ENOMEM - Insufficient memory exists to initialize the mutex structure.
 * @retval EAGAIN - Unable to initialize the mutex structure member(s).
 */
int pthread_mutex_init(pthread_mutex_t *mutex,
                       const pthread_mutexattr_t *attr);

/**
 * @brief Lock a mutex.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_mutex_lock.html
 *
 * @retval 0 - Upon successful completion.
 * @retval EINVAL - the abstime parameter specified a nanoseconds field value less than zero
 *                  or greater than or equal to 1000 million.
 * @retval EDEADLK - The mutex type is PTHREAD_MUTEX_ERRORCHECK and the current thread already
 *                   owns the mutex.
 */
int pthread_mutex_lock(pthread_mutex_t *mutex);

/**
 * @brief Lock a mutex with timeout.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_mutex_timedlock.html
 *
 * @retval 0 - Upon successful completion.
 * @retval EINVAL - The abstime argument passed in does not refer to an initialized structure OR
 *                  the abstime parameter specified a nanoseconds field value less than zero or
 *                  greater than or equal to 1000 million.
 * @retval EDEADLK - The mutex type is PTHREAD_MUTEX_ERRORCHECK and the current thread already owns the mutex.
 * @retval ETIMEDOUT - The mutex could not be locked before the specified timeout expired.
 */
int pthread_mutex_timedlock(pthread_mutex_t *mutex,
                            const struct timespec *abstime);

/**
 * @brief Attempt to lock a mutex. Fail immediately if mutex is already locked.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_mutex_trylock.html
 *
 * @retval 0 - Upon successful completion.
 * @retval EINVAL - the abstime parameter specified a nanoseconds field value less than zero
 *                  or greater than or equal to 1000 million.
 * @retval EDEADLK - The mutex type is PTHREAD_MUTEX_ERRORCHECK and the current thread already
 *                   owns the mutex.
 * @retval EBUSY - The mutex could not be acquired because it was already locked.
 */
int pthread_mutex_trylock(pthread_mutex_t *mutex);

/**
 * @brief Unlock a mutex.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_mutex_unlock.html
 *
 * @retval 0 - Upon successful completion.
 * @retval EPERM - The mutex type is PTHREAD_MUTEX_ERRORCHECK or PTHREAD_MUTEX_RECURSIVE, and
 *                 the current thread does not own the mutex.
 */
int pthread_mutex_unlock(pthread_mutex_t *mutex);

/**
 * @brief Destroy the mutex attributes object.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_mutexattr_destroy.html
 *
 * @retval 0 - Upon successful completion.
 */
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);

/**
 * @brief Get the mutex type attribute.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_mutexattr_gettype.html
 *
 * @retval 0 - Upon successful completion.
 */
int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr,
                              int *type);

/**
 * @brief Initialize the mutex attributes object.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_mutexattr_init.html
 *
 * @retval 0 - Upon successful completion.
 *
 * @note Currently, only the type attribute is supported. Also see pthread_mutexattr_settype()
 *       and pthread_mutexattr_gettype().
 */
int pthread_mutexattr_init(pthread_mutexattr_t *attr);

/**
 * @brief Set the mutex type attribute.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_mutexattr_settype.html
 *
 * @retval 0 - Upon successful completion.
 * @retval EINVAL - The value type is invalid.
 */
int pthread_mutexattr_settype(pthread_mutexattr_t *attr,
                              int type);

/**
 * @brief Get the calling thread ID.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_self.html
 *
 * @retval the thread ID of the calling thread.
 */
pthread_t pthread_self(void);

/**
 * @brief Dynamic thread scheduling parameters access.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_setschedparam.html
 *
 * @note policy is ignored; only priority (param.sched_priority) may be changed.
 *
 * @retval 0 - Upon successful completion.
 */
int pthread_setschedparam(pthread_t thread,
                          int policy,
                          const struct sched_param *param);


/** mqueue api*/
/**
 * @brief Close a message queue.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/mq_close.html
 *
 * @retval 0 - Upon successful completion
 * @retval -1 - A error occurred. errno is also set.
 *
 * @sideeffect Possible errno values
 * <br>
 * EBADF - The mqdes argument is not a valid message queue descriptor.
 */
int mq_close(mqd_t mqdes);

/**
 * @brief Get message queue attributes.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/mq_getattr.html
 *
 * @retval 0 - Upon successful completion
 * @retval -1 - A error occurred. errno is also set.
 *
 * @sideeffect Possible errno values
 * <br>
 * DBADF - The mqdes argument is not a valid message queue descriptor.
 */
int mq_getattr(mqd_t mqdes,
               struct mq_attr *mqstat);

/**
 * @brief Open a message queue.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/mq_open.html
 *
 * @note Supported name pattern: leading &lt;slash&gt; character in name is always required;
 * the maximum length (excluding null-terminator) of the name argument can be NAME_MAX.
 * The default value of NAME_MAX in FreeRTOS_POSIX_portable_default.h is 64, which can be
 * overwritten by user.
 * @note mode argument is not supported.
 * @note Supported oflags: O_RDWR, O_CREAT, O_EXCL, and O_NONBLOCK.
 *
 * @retval Message queue descriptor -- Upon successful completion
 * @retval (mqd_t) - 1 -- An error occurred. errno is also set.
 *
 * @sideeffect Possible errno values
 * <br>
 * EINVAL - name argument is invalid (not following name pattern),
 * OR if O_CREAT is specified in oflag with attr argument not NULL and either mq_maxmsg or mq_msgsize is equal to or less than zero,
 * OR either O_CREAT or O_EXCL is not set and a queue with the same name is unlinked but pending to be removed.
 * <br>
 * EEXIST - O_CREAT and O_EXCL are set and the named message queue already exists.
 * <br>
 * ENOSPC - There is insufficient space for the creation of the new message queue.
 * <br>
 * ENOENT - O_CREAT is not set and the named message queue does not exist.
 */
mqd_t mq_open(const char *name,
              int oflag,
              mode_t mode,
              struct mq_attr *attr);

/**
 * @brief Receive a message from a message queue.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/mq_receive.html
 *
 * @note msg_prio argument is not supported. Messages are not checked for corruption.
 *
 * @retval The length of the selected message in bytes - Upon successful completion.
 * The message is removed from the queue
 * @retval -1 - An error occurred. errno is also set.
 *
 * @sideeffect Possible errno values
 * <br>
 * EBADF - The mqdes argument is not a valid message queue descriptor open for reading.
 * <br>
 * EMSGSIZE - The specified message buffer size, msg_len, is less than the message size attribute of the message queue.
 * <br>
 * ETIMEDOUT - The O_NONBLOCK flag was not set when the message queue was opened,
 * but no message arrived on the queue before the specified timeout expired.
 * <br>
 * EAGAIN - O_NONBLOCK was set in the message description associated with mqdes, and the specified message queue is empty.
 */
ssize_t mq_receive(mqd_t mqdes,
                   char *msg_ptr,
                   size_t msg_len,
                   unsigned int *msg_prio);

/**
 * @brief Send a message to a message queue.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/mq_send.html
 *
 * @note msg_prio argument is not supported.
 *
 * @retval 0 - Upon successful completion.
 * @retval -1 - An error occurred. errno is also set.
 *
 * @sideeffect Possible errno values
 * <br>
 * EBADF - The mqdes argument is not a valid message queue descriptor open for writing.
 * <br>
 * EMSGSIZE - The specified message length, msg_len, exceeds the message size attribute of the message queue,
 * OR insufficient memory for the message to be sent.
 * <br>
 * ETIMEDOUT - The O_NONBLOCK flag was not set when the message queue was opened,
 * but the timeout expired before the message could be added to the queue.
 * <br>
 * EAGAIN - The O_NONBLOCK flag is set in the message queue description associated with mqdes,
 * and the specified message queue is full.
 */
int mq_send(mqd_t mqdes,
            const char *msg_ptr,
            size_t msg_len,
            unsigned msg_prio);

/**
 * @brief Receive a message from a message queue with timeout.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/mq_timedreceive.html
 *
 * @note msg_prio argument is not supported. Messages are not checked for corruption.
 *
 * @retval The length of the selected message in bytes - Upon successful completion.
 * The message is removed from the queue
 * @retval -1 - An error occurred. errno is also set.
 *
 * @sideeffect Possible errno values
 * <br>
 * EBADF - The mqdes argument is not a valid message queue descriptor open for reading.
 * <br>
 * EMSGSIZE - The specified message buffer size, msg_len, is less than the message size attribute of the message queue.
 * <br>
 * EINVAL - The process or thread would have blocked, and the abstime parameter specified a nanoseconds field value
 * less than zero or greater than or equal to 1000 million.
 * <br>
 * ETIMEDOUT - The O_NONBLOCK flag was not set when the message queue was opened,
 * but no message arrived on the queue before the specified timeout expired.
 * <br>
 * EAGAIN - O_NONBLOCK was set in the message description associated with mqdes, and the specified message queue is empty.
 */
ssize_t mq_timedreceive(mqd_t mqdes,
                        char *msg_ptr,
                        size_t msg_len,
                        unsigned *msg_prio,
                        const struct timespec *abstime);

/**
 * @brief Send a message to a message queue with timeout.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/mq_timedsend.html
 *
 * @note msg_prio argument is not supported.
 *
 * @retval 0 - Upon successful completion.
 * @retval -1 - An error occurred. errno is also set.
 *
 * @sideeffect Possible errno values
 * <br>
 * EBADF - The mqdes argument is not a valid message queue descriptor open for writing.
 * <br>
 * EMSGSIZE - The specified message length, msg_len, exceeds the message size attribute of the message queue,
 * OR insufficient memory for the message to be sent.
 * <br>
 * EINVAL - The process or thread would have blocked, and the abstime parameter specified a nanoseconds field
 * value less than zero or greater than or equal to 1000 million.
 * <br>
 * ETIMEDOUT - The O_NONBLOCK flag was not set when the message queue was opened,
 * but the timeout expired before the message could be added to the queue.
 * <br>
 * EAGAIN - The O_NONBLOCK flag is set in the message queue description associated with mqdes,
 * and the specified message queue is full.
 */
int mq_timedsend(mqd_t mqdes,
                 const char *msg_ptr,
                 size_t msg_len,
                 unsigned msg_prio,
                 const struct timespec *abstime);

/**
 * @brief Remove a message queue.
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/mq_unlink.html
 *
 * @retval 0 - Upon successful completion.
 * @retval -1 - An error occurred. errno is also set.
 *
 * @sideeffect Possible errno values
 * <br>
 * EINVAL - name argument is invalid. Refer to requirements on name argument in mq_open().
 * <br>
 * ENOENT - The named message queue does not exist.
 */
int mq_unlink(const char *name);

/** semaphore api*/
/**
 * @brief Destroy an unnamed semaphore.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/sem_destroy.html
 *
 * @retval 0 - upon successful completion
 *
 * @note Semaphore is destroyed regardless of whether there is any thread currently blocked on this semaphore.
 */
int sem_destroy(sem_t *sem);

/**
 * @brief Get the value of a semaphore.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/sem_getvalue.html
 *
 * @retval 0 - Upon successful completion
 *
 * @note If sem is locked, then the object to which sval points is set to zero.
 */
int sem_getvalue(sem_t *sem,
                 int *sval);

/**
 * @brief Initialize an unnamed semaphore.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/sem_init.html
 *
 * @note pshared is ignored. Semaphores will always be considered "shared".
 *
 * @retval 0 - upon successful completion
 * @retval -1 - otherwise. System error variable errno is also set in this case.
 *
 * @sideeffect Possible errno values
 * <br>
 * EINVAL -  The value argument exceeds {SEM_VALUE_MAX}.
 * <br>
 * ENOSPC - A resource required to initialize the semaphore has been exhausted.
 */
int sem_init(sem_t *sem,
             int pshared,
             unsigned value);

/**
 * @brief Unlock a semaphore.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/sem_post.html
 *
 * @retval 0 - upon successful completion
 */
int sem_post(sem_t *sem);

/**
 * @brief Lock a semaphore with timeout.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/sem_timedwait.html
 *
 * @retval 0 - upon successful completion
 * @retval -1 - otherwise. System error variable errno is also set in this case.
 *
 * @sideeffect Possible errno values
 * <br>
 * EINVAL - parameter specified a nanoseconds field value less than zero or greater
 * than or equal to 1000 million
 * <br>
 * ETIMEDOUT - The semaphore could not be locked before the specified timeout expired.
 *
 * @note Deadlock detection is not implemented.
 */
int sem_timedwait(sem_t *sem,
                  const struct timespec *abstime);

/**
 * @brief Lock a semaphore if available.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/sem_trywait.html
 *
 * @retval 0 - upon successful completion
 * @retval -1 - otherwise. System error variable errno is also set in this case.
 *
 * @sideeffect Possible errno values
 * <br>
 * EAGAIN - The semaphore was already locked, so it cannot be immediately locked by the sem_trywait() operation.
 */
int sem_trywait(sem_t *sem);

/**
 * @brief Lock a semaphore.
 *
 * @see http://pubs.opengroup.org/onlinepubs/9699919799/functions/sem_wait.html
 *
 * @retval 0 - upon successful completion
 * @retval -1 - otherwise. System error variable errno is also set in this case.
 *
 * @note Deadlock detection is not implemented.
 */
int sem_wait(sem_t *sem);

#endif	/** _PTHREAD_H*/
