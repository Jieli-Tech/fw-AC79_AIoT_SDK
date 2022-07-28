#include "sock_api.h"
#include "os/os_api.h"
#include "sys/time.h"

#define DEFAULT_SEND_TO 100 //ms
#define DEFAULT_RECV_TO 100 //ms
#define DEFAULT_SELECT_TO 5000 //ms
#define DEFAULT_CONNECT_TO 15 //s
#define SOCK_MAGIC 0x1a2b3c4d

extern int gettimeofday(struct timeval *__restrict __tv, void *__tz);

inline static int sock_hdl_check(struct sock_hdl *hdl, const char *func)
{
    if (hdl == NULL ||	hdl->magic != SOCK_MAGIC) {
        printf("%s hdl err\n", func);
        return -1;
    }
    return 0;
}

void sock_set_quit(void *sock_hdl)
{
    if (sock_hdl_check(sock_hdl, __FUNCTION__)) {
        return;
    }

    struct sock_hdl *hdl = (struct sock_hdl *)sock_hdl;

    hdl->quit = 1;
}

void sock_clr_quit(void *sock_hdl)
{
    if (sock_hdl_check(sock_hdl, __FUNCTION__)) {
        return;
    }

    struct sock_hdl *hdl = (struct sock_hdl *)sock_hdl;

    hdl->quit = 0;
}

int sock_get_quit(void *sock_hdl)
{
    if (sock_hdl_check(sock_hdl, __FUNCTION__)) {
        return -1;
    }

    struct sock_hdl *hdl = (struct sock_hdl *)sock_hdl;

    return hdl->quit;
}

int sock_get_socket(void *sock_hdl)
{
    if (sock_hdl_check(sock_hdl, __FUNCTION__)) {
        return -1;
    }

    struct sock_hdl *hdl = (struct sock_hdl *)sock_hdl;

    return hdl->sock;
}

void *sock_reg(int domain, int type, int protocol, int (*cb_func)(enum sock_api_msg_type type, void *priv), void *priv)
{
    int sock = socket(domain, type, protocol);
    int send_to_millsec = DEFAULT_SEND_TO;
    int recv_to_millsec = DEFAULT_RECV_TO;

    if (sock == -1) {
        return NULL;
    }

    struct sock_hdl *hdl = (struct sock_hdl *)calloc(1, sizeof(struct sock_hdl));
    if (hdl == NULL) {
        return NULL;
    }

    hdl->sock = sock;
    hdl->cb_func = cb_func;
    hdl->priv = priv;

    setsockopt(hdl->sock, SOL_SOCKET, SO_RCVTIMEO, (const void *)&recv_to_millsec, sizeof(recv_to_millsec));
    setsockopt(hdl->sock, SOL_SOCKET, SO_SNDTIMEO, (const void *)&send_to_millsec, sizeof(send_to_millsec));

    os_mutex_create(&hdl->send_mtx);
    os_mutex_create(&hdl->recv_mtx);
    //os_mutex_create(&hdl->close_mtx);
    hdl->magic = SOCK_MAGIC;

    return hdl;
}

void sock_unreg(void *sock_hdl)
{
    if (sock_hdl_check(sock_hdl, __FUNCTION__)) {
        return;
    }

    int ret;
    struct sock_hdl *hdl = (struct sock_hdl *)sock_hdl;

    hdl->magic = (u32_t) - 1;
    // if (os_mutex_del(&hdl->close_mtx, 1)) {
    //     return;
    // }

    hdl->quit = 1;

    if (hdl->cb_func) {
        hdl->cb_func(SOCK_UNREG, hdl->priv);
    }
    os_mutex_pend(&hdl->send_mtx, 0);
    os_mutex_del(&hdl->send_mtx, 1);
    os_mutex_pend(&hdl->recv_mtx, 0);
    os_mutex_del(&hdl->recv_mtx, 1);
    ret =  closesocket(hdl->sock);
    if (ret) {
        printf("\r\n\r\n  sock_unreg err = %d \r\n", sock_get_error(hdl));
    }
    free(hdl);
}

int sock_setsockopt(void *sock_hdl, int level, int optname, const void *optval, socklen_t optlen)
{
    return setsockopt(((struct sock_hdl *)sock_hdl)->sock, level, optname, optval, optlen);
}

int sock_getsockopt(void *sock_hdl, int level, int optname, void *optval, socklen_t *optlen)
{
    return getsockopt(((struct sock_hdl *)sock_hdl)->sock, level, optname, optval, optlen);
}

int sock_set_reuseaddr(void *sock_hdl)
{
    int opt = 1;
    return setsockopt(((struct sock_hdl *)sock_hdl)->sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
}

void sock_set_recv_timeout(void *sock_hdl, u32 millsec)
{
    if (millsec < DEFAULT_RECV_TO) {
        setsockopt(((struct sock_hdl *)sock_hdl)->sock, SOL_SOCKET, SO_RCVTIMEO, (const void *)&millsec, sizeof(millsec));
    }

    ((struct sock_hdl *)sock_hdl)->recv_to = millsec;
}

int sock_recv_timeout(void *sock_hdl)
{
    return ((struct sock_hdl *)sock_hdl)->recv_to_flag;
}

int sock_would_block(void *sock_hdl)
{
    if (sock_hdl_check(sock_hdl, __FUNCTION__)) {
        return -1;
    }

    int sock_err;
    int err_len = sizeof(sock_err);

    getsockopt(((struct sock_hdl *)sock_hdl)->sock, SOL_SOCKET, SO_ERROR, (void *)&sock_err, (socklen_t *)&err_len);

    if (sock_err != EWOULDBLOCK && sock_err != 0 && sock_err != ENOTCONN  && sock_err != EINPROGRESS) {
        /* printf("\r\n\r\n sock_would_block SO_ERROR = %d \r\n", sock_err); */
    }

    return (sock_err == EWOULDBLOCK || sock_err == 0 || sock_err == EINPROGRESS);
}

void sock_set_send_timeout(void *sock_hdl, u32 millsec)
{
    if (millsec < DEFAULT_SEND_TO) {
        setsockopt(((struct sock_hdl *)sock_hdl)->sock, SOL_SOCKET, SO_SNDTIMEO, (const void *)&millsec, sizeof(millsec));
    }

    ((struct sock_hdl *)sock_hdl)->send_to = millsec;
}

int sock_send_timeout(void *sock_hdl)
{
    return ((struct sock_hdl *)sock_hdl)->send_to_flag;
}

int sock_getsockname(void *sock_hdl, struct sockaddr *name, socklen_t *namelen)
{
    return getsockname(((struct sock_hdl *)sock_hdl)->sock, name, namelen);
}

int sock_getpeername(void *sock_hdl, struct sockaddr *name, socklen_t *namelen)
{
    return getpeername(((struct sock_hdl *)sock_hdl)->sock, name, namelen);
}

int sock_recvfrom(void *sock_hdl, void *mem, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen)
{
    if (sock_hdl_check(sock_hdl, __FUNCTION__)) {
        return -1;
    }

    struct sock_hdl *hdl = (struct sock_hdl *)sock_hdl;
    int ret, already_read;
    int recv_to_timing;
    struct timeval tv;

    if (flags == MSG_DONTWAIT) {
        if (os_mutex_pend(&hdl->recv_mtx, 0)) {
            puts("sock_recvfrom sock_unreged!\n");
            return -1;
        }
        ret = recvfrom(hdl->sock, mem, len, flags, from, fromlen);
        os_mutex_post(&hdl->recv_mtx);

        return ret;
    } else if (flags == MSG_WAITALL) {
        already_read = 0;
        if (os_mutex_pend(&hdl->recv_mtx, 0)) {
            puts("sock_recvfrom sock_unreged!\n");
            return -1;
        }
        if (hdl->recv_to) {
            hdl->recv_to_flag = 0;
            gettimeofday(&tv, NULL);
            recv_to_timing = tv.tv_sec * 1000 + tv.tv_usec / 1000 + hdl->recv_to;
        }
        do {
            if (hdl->quit) {
                os_mutex_post(&hdl->recv_mtx);
                return -1;
            }

            ret = recvfrom(hdl->sock, mem + already_read, len - already_read, 0, from, fromlen);
            if (hdl->cb_func && hdl->cb_func(SOCK_EVENT_ALWAYS, hdl->priv) < 0) {
                os_mutex_post(&hdl->recv_mtx);
                return -1;
            }

            if (ret <= 0) {
                if (sock_would_block(hdl)) {
                    if (hdl->recv_to) {
                        gettimeofday(&tv, NULL);
                        if (tv.tv_sec * 1000 + tv.tv_usec / 1000  > recv_to_timing) {
                            hdl->recv_to_flag = 1;
                            os_mutex_post(&hdl->recv_mtx);
                            return -1;
                        }
                    }

                    if (hdl->cb_func && hdl->cb_func(SOCK_RECV_TO, hdl->priv) < 0) {
                        os_mutex_post(&hdl->recv_mtx);
                        return -1;
                    }

                    continue;
                } else {
                    os_mutex_post(&hdl->recv_mtx);
                    return ret;
                }
            }
            already_read += ret;
        } while (already_read < len);
        os_mutex_post(&hdl->recv_mtx);
        return len;
    } else {
        if (hdl->recv_to) {
            hdl->recv_to_flag = 0;
            gettimeofday(&tv, NULL);
            recv_to_timing = tv.tv_sec * 1000 + tv.tv_usec / 1000 + hdl->recv_to;
        }
        while (1) {
            if (hdl->quit) {
                return -1;
            }

            if (os_mutex_pend(&hdl->recv_mtx, 0)) {
                puts("sock_recvfrom sock_unreged!\n");
                return -1;
            }
            ret = recvfrom(hdl->sock, mem, len, flags, from, fromlen);
            os_mutex_post(&hdl->recv_mtx);
            if (hdl->cb_func && hdl->cb_func(SOCK_EVENT_ALWAYS, hdl->priv) < 0) {
                return -1;
            }
            if (ret <= 0) {
                if (sock_would_block(hdl)) {
                    if (hdl->recv_to) {
                        gettimeofday(&tv, NULL);
                        if (tv.tv_sec * 1000 + tv.tv_usec / 1000 > recv_to_timing) {
                            hdl->recv_to_flag = 1;
                            return -1;
                        }
                    }

                    if (hdl->cb_func && hdl->cb_func(SOCK_RECV_TO, hdl->priv) < 0) {
                        return -1;
                    }
                    continue;
                } else {
                    return ret;
                }
            }
            break;
        }
        return ret;
    }
}

int sock_recv(void *sock_hdl, void *buf, u32 len, int flag)
{
    return sock_recvfrom(sock_hdl, buf, len, flag, NULL, NULL);
}

int sock_sendto(void *sock_hdl, const void *data, size_t size, int flags, const struct sockaddr *to, socklen_t tolen)
{
    if (sock_hdl_check(sock_hdl, __FUNCTION__)) {
        return -1;
    }

    int ret;
    struct sock_hdl *hdl = (struct sock_hdl *)sock_hdl;

    if (os_mutex_pend(&hdl->send_mtx, 0)) {
        puts("sock_sendto sock_unreged!\n");
        return -1;
    }
    ret = sendto(hdl->sock, data, size, flags, to, tolen);
    os_mutex_post(&hdl->send_mtx);

    return ret;
}

int sock_send(void *sock_hdl, const void *buf, u32 len, int flag)
{
    if (sock_hdl_check(sock_hdl, __FUNCTION__)) {
        return -1;
    }

    struct sock_hdl *hdl = (struct sock_hdl *)sock_hdl;
    int ret, already_wr;
    int send_to_timing;
    struct timeval tv;

    if (flag == MSG_DONTWAIT) {
        if (os_mutex_pend(&hdl->send_mtx, 0)) {
            puts("sock_send sock_unreged!\n");
            return -1;
        }
        ret = send(hdl->sock, buf, len, flag);
        os_mutex_post(&hdl->send_mtx);

        return ret;
    } else {
        already_wr = 0;
        if (os_mutex_pend(&hdl->send_mtx, 0)) {
            puts("sock_send sock_unreged!\n");
            return -1;
        }

        if (hdl->send_to) {
            hdl->send_to_flag = 0;
            gettimeofday(&tv, NULL);
            send_to_timing = tv.tv_sec * 1000 + tv.tv_usec / 1000 + hdl->send_to;
        }
        while (1) {
            if (hdl->quit) {
                os_mutex_post(&hdl->send_mtx);
                return -1;
            }

            ret = send(hdl->sock, (char *)buf + already_wr, len - already_wr, flag);
            if (hdl->cb_func && hdl->cb_func(SOCK_EVENT_ALWAYS, hdl->priv) < 0) {
                os_mutex_post(&hdl->send_mtx);
                return -1;
            }

            if (ret > 0) {
                if (ret == len - already_wr) {
                    os_mutex_post(&hdl->send_mtx);
                    return len;
                }

                already_wr += ret;
                continue;
            } else {
                if (sock_would_block(hdl)) {
                    if (hdl->send_to) {
                        gettimeofday(&tv, NULL);
                        if (tv.tv_sec * 1000 + tv.tv_usec / 1000  > send_to_timing) {
                            hdl->send_to_flag = 1;
                            os_mutex_post(&hdl->send_mtx);
                            return -1;
                        }
                    }

                    if (hdl->cb_func && hdl->cb_func(SOCK_SEND_TO, hdl->priv) < 0) {
                        os_mutex_post(&hdl->send_mtx);
                        return -1;
                    }
                    continue;
                }
                os_mutex_post(&hdl->send_mtx);
                return ret;
            }
        }
    }
}

void *sock_accept(void *sock_hdl, struct sockaddr *addr, socklen_t *addrlen, int (*cb_func)(enum sock_api_msg_type type, void *priv), void *priv)
{
    if (sock_hdl_check(sock_hdl, __FUNCTION__)) {
        return NULL;
    }

    struct sock_hdl *hdl = (struct sock_hdl *)sock_hdl;
    int ret;
    int send_to_millsec = DEFAULT_SEND_TO;
    int recv_to_millsec = DEFAULT_RECV_TO;

    while (1) {
        if (hdl->quit) {
            return NULL;
        }

        ret = accept(hdl->sock, addr, addrlen);
        if (hdl->cb_func && hdl->cb_func(SOCK_EVENT_ALWAYS, hdl->priv) < 0) {
            return NULL;
        }
        if (ret < 0) {
            if (sock_would_block(hdl)) {
                if (hdl->cb_func && hdl->cb_func(SOCK_RECV_TO, hdl->priv) < 0) {
                    return NULL;
                }
            } else {
                if (hdl->cb_func && hdl->cb_func(SOCK_EVENT_ALWAYS, hdl->priv) < 0) {
                    return NULL;
                }
                printf("sock_accept SO_ERROR = %d \r\n",  sock_get_error(hdl));
            }
            continue;
        }

        struct sock_hdl *hdl2 = (struct sock_hdl *)calloc(1, sizeof(struct sock_hdl));
        if (hdl2 == NULL) {
            puts("sock_accept calloc fail\n");
            closesocket(ret);
            continue;
        }

        hdl2->sock = ret;
        hdl2->cb_func = cb_func;
        hdl2->priv = priv;

        hdl2->magic = SOCK_MAGIC;
        setsockopt(hdl2->sock, SOL_SOCKET, SO_RCVTIMEO, (const void *)&recv_to_millsec, sizeof(recv_to_millsec));
        setsockopt(hdl2->sock, SOL_SOCKET, SO_SNDTIMEO, (const void *)&send_to_millsec, sizeof(send_to_millsec));

        os_mutex_create(&hdl2->send_mtx);
        os_mutex_create(&hdl2->recv_mtx);
        // os_mutex_create(&hdl2->close_mtx);

        return hdl2;
    }
}

int sock_bind(void *sock_hdl, const struct sockaddr *name, socklen_t namelen)
{
    struct sock_hdl *hdl = (struct sock_hdl *)sock_hdl;
    return bind(hdl->sock, name, namelen);
}

int sock_listen(void *sock_hdl, int backlog)
{
    struct sock_hdl *hdl = (struct sock_hdl *)sock_hdl;
    return listen(hdl->sock, backlog);
}

void sock_set_connect_to(void *sock_hdl, int sec)
{
    struct sock_hdl *hdl = (struct sock_hdl *)sock_hdl;
    hdl->connect_to = sec;
}

int sock_get_error(void *sock_hdl)
{
    int sock_err;
    int err_len = sizeof(sock_err);

    getsockopt(((struct sock_hdl *)sock_hdl)->sock, SOL_SOCKET, SO_ERROR, (void *)&sock_err, (socklen_t *)&err_len);

    return sock_err;
}

int sock_fcntl(void *sock_hdl, int cmd, int val)
{
    return fcntl(((struct sock_hdl *)sock_hdl)->sock, cmd, val);
}

int sock_select_rdset(void *sock_hdl)
{
    if (sock_hdl_check(sock_hdl, __FUNCTION__)) {
        return -1;
    }

    struct sock_hdl *hdl = (struct sock_hdl *)sock_hdl;
    int ret;
    fd_set rdset, errset;
    struct timeval tv = {0, DEFAULT_RECV_TO * 1000};

    while (1) {
        if (hdl->quit) {
            return -1;
        }
        FD_ZERO(&rdset);
        FD_ZERO(&errset);
        FD_SET(hdl->sock, &rdset);
        FD_SET(hdl->sock, &errset);

        ret = select(hdl->sock + 1, &rdset, NULL, &errset, &tv);
        if (ret < 0 || FD_ISSET(hdl->sock, &errset)) {
            return -1;
        } else if (ret == 0) {
            if (hdl->cb_func && hdl->cb_func(SOCK_RECV_TO, hdl->priv) < 0) {
                return -1;
            }
            continue;
        } else if (FD_ISSET(hdl->sock, &rdset)) {
            return 0;
        }

        return -1;
    }
    return -1;
}

int sock_select(void *sock_hdl, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *tv)
{
    if (sock_hdl_check(sock_hdl, __FUNCTION__)) {
        return -1;
    }

    int ret;
    struct timeval timeout;
    unsigned int total_to_ms;
    struct sock_hdl *hdl = (struct sock_hdl *)sock_hdl;

    timeout.tv_sec  = DEFAULT_SELECT_TO / 1000;
    timeout.tv_usec = (DEFAULT_SELECT_TO % 1000) * 1000;

    if (tv) {
        total_to_ms = tv->tv_sec * 1000 + tv->tv_usec / 1000;
        if (total_to_ms < DEFAULT_SELECT_TO) {
            timeout.tv_sec  = tv->tv_sec;
            timeout.tv_usec = tv->tv_usec;
        }
    }

    while (1) {
        if (hdl->quit) {
            ret = -1;
            break;
        }
        if (readset) {
            FD_ZERO(readset);
            FD_SET(hdl->sock, readset);
        }
        if (writeset) {
            FD_ZERO(writeset);
            FD_SET(hdl->sock, writeset);
        }
        if (exceptset) {
            FD_ZERO(exceptset);
            FD_SET(hdl->sock, exceptset);
        }

        ret = select(hdl->sock + 1, readset, writeset, exceptset, &timeout);
        if (ret == 0) {
            if (tv) {
                if (total_to_ms <= DEFAULT_SELECT_TO) {
                    break;
                }
                total_to_ms -= DEFAULT_SELECT_TO;
            }
            continue;
        }
        break;
    }

    return ret;
}

int sock_connect(void *sock_hdl, const struct sockaddr *name, socklen_t namelen)
{
    if (sock_hdl_check(sock_hdl, __FUNCTION__)) {
        return -1;
    }

    struct sock_hdl *hdl = (struct sock_hdl *)sock_hdl;
    int ret, savefl, sock_err, timeout_cnt;
    int err_len = sizeof(sock_err);
    fd_set rdset, wrset, errset;
    struct timeval tv = {0, DEFAULT_RECV_TO * 1000};

    savefl = fcntl(hdl->sock, F_GETFL, 0);
    fcntl(hdl->sock, F_SETFL, savefl | O_NONBLOCK);

    ret = connect(hdl->sock, name, namelen);

    if (ret == 0) {
        goto EXIT;
    }

    ret = getsockopt(hdl->sock, SOL_SOCKET, SO_ERROR, (void *)&sock_err, (socklen_t *)&err_len);
    if (ret) {
        printf("sock_connect getsockopt err=%d\n", ret);
        ret = -1;
        goto EXIT;
    }
    if (sock_err != EINPROGRESS) {
        printf("sock_connect fault!0x%x\n", sock_err);
        ret = -1;
        goto EXIT;
    }

    hdl->connect_to = hdl->connect_to ? hdl->connect_to : DEFAULT_CONNECT_TO;
    timeout_cnt = hdl->connect_to * 1000 / DEFAULT_RECV_TO;

    while (1) {
        FD_ZERO(&rdset);
        FD_ZERO(&wrset);
        FD_ZERO(&errset);
        FD_SET(hdl->sock, &rdset);
        FD_SET(hdl->sock, &wrset);
        FD_SET(hdl->sock, &errset);

        ret = select(hdl->sock + 1, &rdset, &wrset, &errset, &tv);
        if (ret < 0 || FD_ISSET(hdl->sock, &errset) || hdl->quit) {
            ret = -1;
            goto EXIT;
        } else if (ret == 0) {
            if (--timeout_cnt == 0) {
                ret = -1;
                goto EXIT;
            }
            if (hdl->cb_func && hdl->cb_func(SOCK_RECV_TO, hdl->priv) < 0) {
                ret = -1;
                goto EXIT;
            }
        } else if (FD_ISSET(hdl->sock, &rdset) || FD_ISSET(hdl->sock, &wrset)) {
            ret = 0;
            goto EXIT;
        }
    }

EXIT:

    fcntl(hdl->sock, F_SETFL, savefl);

    if (hdl->cb_func) {
        hdl->cb_func(ret ? SOCK_CONNECT_FAIL : SOCK_CONNECT_SUCC, hdl->priv);
    }

    return ret ? -1 : 0;
}

int socket_set_keepalive(void *sock_hdl, int keep_idle, int keep_intv, int keep_cnt)
{
    struct sock_hdl *hdl = (struct sock_hdl *)sock_hdl;
    int alive = 1;

    if (keep_idle == 0) {
        keep_idle = 10;    /* 10秒钟无数据，触发保活机制，发送保活包 */
    }

    if (keep_intv == 0) {
        keep_intv = 3;    /* 如果没有收到回应，则3秒钟后重发保活包 */
    }

    if (keep_cnt == 0) {
        keep_cnt = 5;    /* 连续5次没收到保活包，视为连接失效 */
    }

    /* Set: use keepalive on fd */
    if (setsockopt(hdl->sock, SOL_SOCKET, SO_KEEPALIVE, &alive, sizeof alive) != 0) {
        printf("Set keepalive error: 0x%x.\n", (errno));
        return -1;
    }

    if (setsockopt(hdl->sock, IPPROTO_TCP, TCP_KEEPIDLE, &keep_idle, sizeof keep_idle) != 0) {
        printf("Set keepalive idle error: 0x%x.\n", (errno));
        return -1;
    }

    if (setsockopt(hdl->sock, IPPROTO_TCP, TCP_KEEPINTVL, &keep_intv, sizeof keep_intv) != 0) {
        printf("Set keepalive intv error: 0x%x.\n", (errno));
        return -1;
    }

    if (setsockopt(hdl->sock, IPPROTO_TCP, TCP_KEEPCNT, &keep_cnt, sizeof keep_cnt) != 0) {
        printf("Set keepalive cnt error: 0x%x.\n", (errno));
        return -1;
    }

    return 0;
}


