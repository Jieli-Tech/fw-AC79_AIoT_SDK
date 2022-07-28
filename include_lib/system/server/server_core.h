#ifndef SERVER_H
#define SERVER_H

#include "generic/typedef.h"
#include "system/task.h"
#include "spinlock.h"
#include "list.h"


/// \cond DO_NOT_DOCUMENT
#define REQ_COMPLETE_CALLBACK 			0x01000000
#define REQ_WAIT_COMPLETE 				0x02000000
#define REQ_HI_PRIORITY 				0x04000000
#define REQ_TYPE_MASK 					0x00ffffff


struct server_req {
    int type;
    int err;
    void *server_priv;
    struct list_head entry;
    struct server *server;
    void *user;
    const char *owner;
    OS_SEM sem;
    union {
        int state;
        void (*func)(void *, void *, int);
    } complete;
    u32 arg[0];
};

struct server_info {
    const char *name;
    u16 reqlen;
    u8  reqnum;
    void *(*open)(void *, void *);
    void (*close)(void *);
};

#define REQ_BUF_LEN  	512

struct server {
    bool avaliable;
    spinlock_t lock;
    void *server;
    OS_SEM sem;
    OS_MUTEX mutex;
    struct list_head *req_buf;
    struct list_head free;
    struct list_head pending;
    const struct server_info *info;
    const char *owner;
    void *handler_priv;
    void (*event_handler)(void *,  int argc, int *argv);
};


#define SERVER_REGISTER(info) \
	const struct server_info info SEC_USED(.server_info)

#define server_load(server) \
	load_module(server)

/// \endcond

/**
 *  @brief 打开服务
 *  @param name:	   服务名称
 *  @param arg: 	   服务的私有参数
 *  @return server:	   创建成功返回服务句柄 NULL:打开失败
 */
struct server *server_open(const char *name, void *arg);

/**
 *  @brief 注册服务事件回调
 *  @param server:		server_open返回的服务句柄
 *  @param priv:		服务事件回调的私有指针
 *  @param handler:		服务事件回调函数
 *  @note  执行该函数的当前任务必须带有队列消息，并且需要接收队列消息才能执行事件回调
 */
void server_register_event_handler(struct server *server, void *priv,
                                   void (*handler)(void *, int argc, int *argv));

/**
 *  @brief 注册服务事件回调，并指定执行服务事件回调函数的任务
 *  @param server:		server_open返回的服务句柄
 *  @param priv:		服务事件回调的私有指针
 *  @param handler:		服务事件回调函数
 *  @param task_name: 	指定执行服务事件回调函数的任务名称
 *  @note  指定的任务必须带有队列消息，并且需要接收队列消息才能执行事件回调
 */
void server_register_event_handler_to_task(struct server *server, void *priv,
        void (*handler)(void *, int argc, int *argv), const char *task_name);

/**
 *  @brief 关闭服务
 *  @param server:		server_open返回的服务句柄
 */
void server_close(struct server *server);

/**
 *  @brief 同步请求服务，待服务请求成功才退出函数
 *  @param server:		server_open返回的服务句柄
 *  @param req_type:	服务请求的类型
 *  @param arg:			对应服务请求的参数
 *  @return 0:请求成功 非0:请求失败
 */
int server_request(struct server *server, int req_type, void *arg);

/**
 *  @brief 异步请求服务，该函数退出并不代表服务已经请求成功
 *  @param server:		server_open返回的服务句柄
 *  @param req_type:	服务请求的类型
 *  @param arg:			对应服务请求的参数
 *  @return 0:请求成功 非0:请求失败
 */
int server_request_async(struct server *server, int req_type, void *arg, ...);

/**
 *  @brief 服务请求响应完成
 *  @param req:			服务请求信息
 *  @return 0:成功 非0:失败
 */
int server_req_complete(struct server_req *req);

/**
 *  @brief 服务有事件通知上层应用处理
 *  @param server:		server_open返回的服务句柄
 *  @param argc:		事件的参数个数
 *  @param argv:		事件参数数组
 *  @return 0:成功 非0:失败
 */
int server_event_handler(void *server, int argc, int *argv);

/**
 *  @brief 清除任务消息回调还没执行的通过server_event_handler的事件通知, 参数必须与server_event_handler指定事件相同
 *  @param server:		server_open返回的服务句柄
 *  @param argc:		通知时对应的事件的参数个数
 *  @param argv:		通知时对应的事件参数数组
 *  @return 0:成功 非0:失败
 */
int server_event_handler_del(void *_server, int argc, int *argv);

#endif

