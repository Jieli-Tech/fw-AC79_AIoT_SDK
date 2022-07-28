#ifndef __SOCK_API_H__
#define __SOCK_API_H__

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "os/os_api.h"
#include "list.h"
#include "string.h"


/*! \addtogroup SOCKET_API
 *  @ingroup NETWORK_LIB
 *  @brief	Network socket api
 *  @{
 */



enum sock_api_msg_type {
    SOCK_SEND_TO,	/*!< The message type used in the cb_func which is called after socket send data timeout */
    SOCK_RECV_TO,	/*!< The message type used in the cb_func which is called after socket recv data timeout */
    SOCK_EVENT_ALWAYS,	/*!< The message type used in the cb_func which is called by operating socket */
    SOCK_CONNECT_SUCC,	/*!< The message type used in the cb_func which is called after socket connect unsucessfully */
    SOCK_CONNECT_FAIL,	/*!< The message type used in the cb_func which is called after socket connect sucessfully */
    SOCK_UNREG,		/*!< The message type used in the cb_func which is called by sock_unreg */
};



/**
 * @brief The handle of socket
 */
struct sock_hdl {
    int sock;	/*!< Socket descriptor */
    int (*cb_func)(enum sock_api_msg_type type, void *priv);	/*!< Callback function */
    void *priv;		/*!< The pointer to the private data of the callback function */
    int connect_to;	/*!< The timeout of the socket connect (s) */
    int send_to;	/*!< The timeout of the socket send (ms) */
    int send_to_flag; 	/*!< The flag of the socket send timeout */
    int recv_to;	/*!< The timeout of the socket recv (ms) */
    int recv_to_flag;	/*!< The flag of the socket recv timeout */
    OS_MUTEX send_mtx;	/*!< The mutex of the socket send */
    OS_MUTEX recv_mtx;	/*!< The mutex of the socket recv */
    // OS_MUTEX close_mtx;
    int quit;		/*!< The quit flag of the socket */
    unsigned int magic;	/*!< The magic of the sock_hdl */
};



/*----------------------------------------------------------------------------*/
/**@brief  Set quit flag of the socket handle
   @param  sock_hdl: The pointer to the socket handle
   @note
*/
/*----------------------------------------------------------------------------*/
void sock_set_quit(void *sock_hdl);

/*----------------------------------------------------------------------------*/
/**@brief  Get quit flag of the socket handle
   @param  sock_hdl: The pointer to the socket handle
   @return 0: not quit
   @return 1: quit
   @note
*/
/*----------------------------------------------------------------------------*/
int sock_get_quit(void *sock_hdl);

/*----------------------------------------------------------------------------*/
/**@brief  Clear quit flag of the socket handle
   @param  sock_hdl: The pointer to the socket handle
   @note
*/
/*----------------------------------------------------------------------------*/
void sock_clr_quit(void *sock_hdl);

/*----------------------------------------------------------------------------*/
/**@brief  Get socket descriptor of the socket handle
   @param  sock_hdl: The pointer to the socket handle
   @return socket descriptor
   @note
*/
/*----------------------------------------------------------------------------*/
int sock_get_socket(void *sock_hdl);

/*----------------------------------------------------------------------------*/
/**@brief  Allocate memory for sock_hdl and obtain the socket descriptor
   @param  domain: Protocol families, only support AF_INET and AF_INET6
   @param  type: Socket protocol types, only support SOCK_STREAM(TCP), SOCK_DGRAM(UDP), SOCK_RAW(RAW)
   @param  protocol: The IP protocol, only support IPPROTO_IP, IPPROTO_IPV6, IPPROTO_ICMP, IPPROTO_ICMPV6, IPPROTO_UDPLITE, IPPROTO_RAW
   @param  cb_func: Callback function
   @param  priv: The pointer to the private data of the callback function
   @return The pointer to the socket handle
   @return NULL: register fail
   @note
*/
/*----------------------------------------------------------------------------*/
void *sock_reg(int domain, int type, int protocol, int (*cb_func)(enum sock_api_msg_type type, void *priv), void *priv);

/*----------------------------------------------------------------------------*/
/**@brief  Close socket and free the sock_hdl
   @param  sock_hdl: The pointer to the socket handle
   @note
*/
/*----------------------------------------------------------------------------*/
void sock_unreg(void *sock_hdl);

/*----------------------------------------------------------------------------*/
/**@brief  Set the recv timeout value of the socket
   @param  sock_hdl: The pointer to the socket handle
   @param  millsec: Recv timeout (ms)
   @note
*/
/*----------------------------------------------------------------------------*/
void sock_set_recv_timeout(void *sock_hdl, unsigned int millsec);

/*----------------------------------------------------------------------------*/
/**@brief  Get the recv_to_flag of the socket handle
   @param  sock_hdl: The pointer to the socket handle
   @return The recv_to_flag of the socket handle
   @note
*/
/*----------------------------------------------------------------------------*/
int sock_recv_timeout(void *sock_hdl);

/*----------------------------------------------------------------------------*/
/**@brief  Check the socket if would block
   @param  sock_hdl: The pointer to the socket handle
   @return 0: NO-BLOCK
   @return 1: BLOCK
   @note
*/
/*----------------------------------------------------------------------------*/
int sock_would_block(void *sock_hdl);

/*----------------------------------------------------------------------------*/
/**@brief  Set the send timeout value of the socket
   @param  sock_hdl: The pointer to the socket handle
   @param  millsec: Send timeout (ms)
   @note
*/
/*----------------------------------------------------------------------------*/
void sock_set_send_timeout(void *sock_hdl, unsigned int millsec);

/*----------------------------------------------------------------------------*/
/**@brief  Get the send_to_flag of the socket handle
   @param  sock_hdl: The pointer to the socket handle
   @return The send_to_flag of the socket handle
   @note
*/
/*----------------------------------------------------------------------------*/
int sock_send_timeout(void *sock_hdl);

/*----------------------------------------------------------------------------*/
/**@brief  Receive data by socket(TCP)
   @param  sock_hdl: The pointer to the socket handle
   @param  buf: The pointer to the receive buffer
   @param  len: The size of the receive buffer
   @param  flag: The way of receive, see socket.h
   @return Number of bytes receive if OK, –1 on error
   @return 0: socket is disconnected
   @note
*/
/*----------------------------------------------------------------------------*/
int sock_recv(void *sock_hdl, void *buf, unsigned int len, int flag);

/*----------------------------------------------------------------------------*/
/**@brief  Send data by socket(TCP)
   @param  sock_hdl: The pointer to the socket handle
   @param  buf: The pointer to the send buffer
   @param  len: The size of the send buffer
   @param  flag: The way of send, see socket.h
   @return Number of bytes send if OK, –1 on error
   @return 0: socket is disconnected
   @note
*/
/*----------------------------------------------------------------------------*/
int sock_send(void *sock_hdl, const void *buf, unsigned int len, int flag);

/**
 * @brief Receive data from specific address
 *
 * @param sock_hdl: The pointer to the socket handle
 * @param buf: The pointer to the receive buffer
 * @param len: The size of the receive buffer
 * @param flags: The way of receive, see socket.h
 * @param from: The information of the remote address
 * @param fromlen: The pointer to the size of the struct sockaddr
 *
 * @return Number of bytes receive if OK, –1 on error
 */
int sock_recvfrom(void *sock_hdl, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);

/**
 * @brief Send data from specific address
 *
 * @param sock_hdl: The pointer to the socket handle
 * @param data: The pointer to the send buffer
 * @param size: Yhe size of the send buffer
 * @param flags: The way of send, see socket.h
 * @param to: The information of the remote address
 * @param tolen: The pointer to the size of the struct sockaddr
 *
 * @return Number of bytes send if OK, –1 on error
 */
int sock_sendto(void *sock_hdl, const void *data, size_t size, int flags, const struct sockaddr *to, socklen_t tolen);

/*----------------------------------------------------------------------------*/
/**@brief  Set the connect timeout value of the socket
   @param  sock_hdl: The pointer to the socket handle
   @param  sec: Connect timeout (s)
*/
/*----------------------------------------------------------------------------*/
void sock_set_connect_to(void *sock_hdl, int sec);

/**
 * @brief The connect function is used by a TCP client to establish a connection with a TCP server.
 * @param sock_hdl: The pointer to the socket handle
 * @param name: The socket address structure contain the IP address and port number of the server
 * @param namelen: The sizeof the struct sockaddr
 *
 * @return 0: Connect successful
 * @return -1: Connect failed
 * @return
 */
int sock_connect(void *sock_hdl, const struct sockaddr *name, socklen_t namelen);

/**
 * @brief This function allows the process to instruct the kernel to wait for any one of multiple events to occur and to wake up the process only when one or more of these events occurs or when a specified amount of time has passed
 *
 * @param sock_hdl: The pointer to the socket handle
 * @param readset: The file descriptor of read
 * @param writeset: The file descriptor of write
 * @param exceptset: The file descriptor of exception
 * @param tv: A timeval structure specifies the number of seconds and microseconds
 *
 * @return Positive count of ready descriptors, 0 on timeout, –1 on error
 * @note  tv is NULL:wait forever  tv time is 0:don't wait at all
 */
int sock_select(void *sock_hdl, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *tv);

/**
 * @brief The function is called by a TCP server to return the next completed connection from the front of the completed connection queue
 *
 * @param sock_hdl: The pointer to the socket handle
 * @param addr: Save the protocol address of the connected peer process (the client).
 * @param addrlen: The pointer to the size of the struct sockaddr
 * @param cb_func: The accept callback function
 * @param priv: The pointer to the private data of the callback function
 *
 * @return The pointer to the socket handle of a completed connection
 * @return NULL: Accept failed
 */
void *sock_accept(void *sock_hdl, struct sockaddr *addr, socklen_t *addrlen, int (*cb_func)(enum sock_api_msg_type type, void *priv), void *priv);

/**
 * @brief The bind function assigns a local protocol address to a socket.
 *
 * @param sock_hdl: The pointer to the socket handle
 * @param name: The socket address structure contain the IP address and port number of the local address
 * @param namelen: The sizeof the struct sockaddr
 *
 * @return 0: Bind successful
 * @return -1: Bind failed
 */
int sock_bind(void *sock_hdl, const struct sockaddr *name, socklen_t namelen);

/**
 * @brief The listen function converts an unconnected socket into a passive socket, indicating that the kernel should accept incoming connection requests directed to this socket.
 *
 * @param sock_hdl: The pointer to the socket handle
 * @param backlog: Specifies the maximum number of connections the kernel should queue for this socket
 *
 * @return 0: Listen successful
 * @return -1: Listen failed
 * @note This function is normally called after both the socket reg and bind functions and must be called before calling the accept function.
 * @note This function is called only by a TCP server
 */
int sock_listen(void *sock_hdl, int backlog);

/**
 * @brief Get the error of the socket operation
 *
 * @param sock_hdl: The pointer to the socket handle
 *
 * @return The error of the socket operation
 */
int sock_get_error(void *sock_hdl);

/**
 * @brief  Socket select before receive data
 *
 * @param sock_hdl: The pointer to the socket handle
 *
 * @return 0: Successful
 * @return -1: Failed
 */
int sock_select_rdset(void *sock_hdl);

/**
 * @brief Set keepalive of the tcp socket
 *
 * @param sock_hdl: The pointer to the socket handle
 * @param keep_idle: The time continuously no receive data up to the value of keep_idle, start to send keepalive packet
 * @param keep_intv: The interval of the keepalive packet send if no receive keepalive packet
 * @param keep_cnt: Disconnect socket if no receive keepalive packet continuously, the count upto keep_cnt
 *
 * @return 0: Set successful
 * @return -1: Set failed
 */
int socket_set_keepalive(void *sock_hdl, int keep_idle, int keep_intv, int keep_cnt);

/**
 * @brief Set the socket to allow local address reuse
 *
 * @param sock_hdl: The pointer to the socket handle
 *
 * @return 0: Set successful
 * @return -1: Set failed
 */
int sock_set_reuseaddr(void *sock_hdl);

/**
 * @brief This function performs various descriptor control operations
 *
 * @param sock_hdl: The pointer to the socket handle
 * @param cmd: Control aommand
 * @param val: Socket file status flag
 *
 * @return -1: Control failed
 * @return 0: Set successful
 * @return 1: NONBLOCK
 * @note  Currently only the commands F_GETFL and F_SETFL are implemented.
 * @note  Only the flag O_NONBLOCK is implemented.
 * @note  Nonblocking I/O— We can set the O_NONBLOCK file status flag using the F_SETFL command to set a socket as nonblocking
 */
int sock_fcntl(void *sock_hdl, int cmd, int val);

/**
 * @brief sock_getsockopt
 *
 * @param sock_hdl: The pointer to the socket handle
 * @param level: Specifies the code in the system that interprets the option(the general socket code or some protocol-specific code)
 * @param optname: The type of the option
 * @param optval: Pointer to which the current value of the option is stored by getsockopt
 * @param optlen: Pointer to the size of the optval
 *
 * @return 0: Successful
 * @return -1: Failed
 */
int sock_getsockopt(void *sock_hdl, int level, int optname, void *optval, socklen_t *optlen);

/**
 * @brief sock_setsockopt
 *
 * @param sock_hdl: The pointer to the socket handle
 * @param level: Specifies the code in the system that interprets the option(the general socket code or some protocol-specific code)
 * @param optname: The type of the option
 * @param optval: Pointer to a variable from which the new value of the option is fetched by setsockopt
 * @param optlen: The size of the optval
 *
 * @return 0: Successful
 * @return -1: Failed
 */
int sock_setsockopt(void *sock_hdl, int level, int optname, const void *optval, socklen_t optlen);

/**
 * @brief This return the local protocol address associated with a socket
 *
 * @param sock_hdl: The pointer to the socket handle
 * @param name: The information of the local address
 * @param namelen: The pointer to the size of the struct sockaddr
 *
 * @return 0: sucessful
 * @return -1: failed
 * @note 1: After connect successfully returns in a TCP client that does not call bind, getsockname returns the local IP address and local port number assigned to the connection by the kernel
 * @note 2: After calling bind with a port number of 0 (telling the kernel to choose the local port number), getsockname returns the local port number that was assigned
 * @note 3: Getsockname can be called to obtain the address family of a socket
 * @note 4: In a TCP server that binds the wildcard IP address, once a connection is established with a client (accept returns successfully), the server can call getsockname to obtain the local IP address assigned to the connection
*/
int sock_getsockname(void *sock_hdl, struct sockaddr *name, socklen_t *namelen);

/**
 * @brief This return the foregin protocol address associated with a socket
 *
 * @param sock_hdl: The pointer to the socket handle
 * @param name: The information of the remote address
 * @param namelen: The pointer to the size of the struct sockaddr
 *
 * @return 0: sucessful
 * @return -1: failed
 * @note When a server is execed by the process that calls accept, the only way the server can obtain the identity of the client is to call getpeername
 */
int sock_getpeername(void *sock_hdl, struct sockaddr *name, socklen_t *namelen);


/*! @}*/

#endif

