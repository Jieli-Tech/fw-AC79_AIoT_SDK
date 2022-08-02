
/*
 * airkiss package format
 * |-------------------------Header-----------------------|
 * |   4 bytes    | 4 bytes |     4 bytes    |   4 bytes  |
 * | magic number | version | package length | command id |
 * |-------------------------Body-------------------------|
 */
#include "generic/typedef.h"
#include "os/os_api.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "json_c/json.h"

static struct {
    char *uuid;
    char *device_type;
    char *device_id_prefix;
} turing_airkiss_para = {0};

#define __this (&turing_airkiss_para)

static const int AIRKISS_DEFAULT_LAN_PORT = 12476;
static const unsigned char AIRKISS_MAGIC_NUMBER[4] = { 0xfd, 0x01, 0xfe, 0xfc};
static const unsigned char AIRKISS_VERSION[4] = { 0x00, 0x20, 0x00, 0x02};
static const unsigned char AIRKISS_SEND_VERSION[4] = { 0x00, 0x10, 0x00, 0x01};

static volatile u8 s_stop_send = false;
static int turing_bind_task_pid;

typedef enum {
    /* error message */
    TURING_AIRKISS_ERR = -1,

    /* correct message, but do not need to handle */
    TURING_AIRKISS_CONTINUE = 0,

    /* received discovery request */
    TURING_AIRKISS_SSDP_REQ = 1,

    /* message package ready */
    TURING_AIRKISS_PAKE_READY = 2
} turing_airkiss_ret_t;

typedef enum {
    TURING_AIRKISS_SSDP_REQ_CMD = 0x1,
    TURING_AIRKISS_SSDP_RESP_CMD = 0x1001,
    TURING_AIRKISS_SSDP_NOTIFY_CMD = 0x1002
} airkiss_lan_cmdid_t;

static int turing_get_socket_errno(int fd)
{
    int sock_errno = 0;
    uint32_t optlen = sizeof(sock_errno);

    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &sock_errno, &optlen) < 0) {
        log_e("getsockopt failed");
    }

    return sock_errno;
}

static turing_airkiss_ret_t turing_airkiss_lan_recv(const unsigned char *msg, uint16_t len)
{
    // msg must contain at least 16 bytes
    if (msg == NULL || len < 16) {
        log_i("turing_airkiss_lan_recv error msg");
        return TURING_AIRKISS_ERR;
    }

    if (memcmp(msg, AIRKISS_MAGIC_NUMBER, sizeof(AIRKISS_MAGIC_NUMBER)) != 0) {
        log_i("not airkiss packet");
        return TURING_AIRKISS_ERR;
    }
    msg += sizeof(AIRKISS_MAGIC_NUMBER);

    // skip version code
    log_d("version code:0x%x%x%x%x", msg[0], msg[1], msg[2], msg[3]);
    msg += 4;

    // skip msg length
    msg += 4;

    uint32_t cmd = msg[0] << 24 | msg[1] << 16 | msg[2] << 8 | msg[3];
    log_d("airkiss cmd: 0x%x", cmd);
    if (cmd == TURING_AIRKISS_SSDP_REQ_CMD) {
        return TURING_AIRKISS_SSDP_REQ;
    }

    return TURING_AIRKISS_CONTINUE;
}

static int turing_airkiss_pack_device_info(uint32_t cmdid, unsigned char **outbuf, uint16_t *len)
{
    struct json_object *msg = NULL;
    struct json_object *dev_info = NULL;
    unsigned char *buf = NULL;
    const char *body = NULL;
    char *device_id = NULL;
    int id_len = 0;
    int body_len = 0;
    int ret = -1;

    if (outbuf == NULL || *outbuf != NULL || len == NULL) {
        log_e("invalid arguments");
        return -1;
    }

    do {
        msg = json_object_new_object();
        if (msg == NULL) {
            log_e("create msg failed");
            break;
        }

        dev_info = json_object_new_object();
        if (dev_info == NULL) {
            log_e("create dev_info failed");
            break;
        }
        json_object_object_add(msg, "deviceInfo", dev_info);

        if (__this->uuid == NULL || __this->uuid[0] == 0 ||
            __this->device_id_prefix == NULL || __this->device_id_prefix[0] == 0 ||
            __this->device_type == NULL || __this->device_type[0] == 0) {
            log_e("invalid device_type or device_id_prefix or uuid");
            break;
        }

        id_len = strlen(__this->device_id_prefix) + strlen(__this->uuid) + 1 + 1;
        device_id = (char *)malloc(id_len);
        if (device_id == NULL) {
            log_e("no free memory");
            break;
        }
        snprintf(device_id, id_len, "%s_%s", __this->device_id_prefix, __this->uuid);

        json_object_object_add(dev_info, "deviceType", json_object_new_string(__this->device_type));
        json_object_object_add(dev_info, "deviceId", json_object_new_string(device_id));
        free(device_id);
        device_id = NULL;

        body = json_object_to_json_string(msg);
        if (body == NULL) {
            log_e("get string of msg failed");
            break;
        }
        log_d("body = %s", body);

        body_len = strlen(body);

        // the length of header is 16 bytes
        *len = body_len + 16;
        buf = (unsigned char *)malloc(*len);
        if (buf == NULL) {
            log_e("No free memory");
            break;
        }
        *outbuf = buf;

        // magic number
        memcpy(buf, AIRKISS_MAGIC_NUMBER, sizeof(AIRKISS_MAGIC_NUMBER));
        buf += 4;

        // version code
        memcpy(buf, AIRKISS_SEND_VERSION, sizeof(AIRKISS_SEND_VERSION));
        buf += 4;

        // the length of message
        buf[0] = buf[1] = 0;
        buf[2] = (unsigned char)((*len) >> 8);
        buf[3] = (unsigned char)(*len);
        buf += 4;

        // command id
        buf[0] = (unsigned char)(cmdid >> 24);
        buf[1] = (unsigned char)(cmdid >> 16);
        buf[2] = (unsigned char)(cmdid >> 8);
        buf[3] = (unsigned char)cmdid;
        buf += 4;

        memcpy(buf, body, body_len);
        ret = 0;
    } while (0);

    if (msg) {
        json_object_put(msg);
    }

    return ret;
}

static void turing_bind_device(void *priv)
{
    struct sockaddr_in local_addr;
    struct sockaddr_in remote_addr;
    socklen_t addr_len = sizeof(remote_addr);
    const uint16_t BUF_LEN = 200;
    uint16_t resp_len = 0;
    uint16_t recv_len = 0;
    unsigned char *buf = NULL;
    unsigned char *resp_buf = NULL;
    int ret = 0;
    int err = 0;
    int fd = -1;
    int optval = 1;
    volatile int lifecycle = (int)priv;

    buf = (unsigned char *)malloc(BUF_LEN);
    if (buf == NULL) {
        log_e("buf allocate fail");
        os_time_dly(100);	//此处要加延时是因为避免杀线程比创建线程跑得还要快，导致任务句柄释放了还在使用
        return;
    }

    do {
        fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); //IPPROTO_UDP
        if (fd < 0) {
            log_e("failed to create socket!");
            break;
        }

        ret = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (void *)&optval, sizeof(optval));
        if (ret < 0) {
            log_e("setsockopt failed");
            break;
        }

        memset(&local_addr, 0, sizeof(local_addr));
        local_addr.sin_family = AF_INET;
        local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        local_addr.sin_port = htons(AIRKISS_DEFAULT_LAN_PORT);

        ret = bind(fd, (const struct sockaddr *)&local_addr, sizeof(local_addr));
        if (ret < 0) {
            err = turing_get_socket_errno(fd);
            log_e("airkiss bind local port ERROR! errno %d", err);
            break;
        }

        ret = turing_airkiss_pack_device_info(TURING_AIRKISS_SSDP_RESP_CMD, &resp_buf, &resp_len);
        if (ret < 0) {
            log_e("Pack device info packet error!");
            break;
        }

        while (!s_stop_send && lifecycle) {
            struct timeval tv;
            fd_set rfds;

            FD_ZERO(&rfds);
            FD_SET(fd, &rfds);

            tv.tv_sec = 1;
            tv.tv_usec = 0;
            lifecycle--;

            ret = select(fd + 1, &rfds, NULL, NULL, &tv);
            if (ret > 0) {
                recv_len = recvfrom(fd, buf, BUF_LEN, 0, (struct sockaddr *)&remote_addr,
                                    (socklen_t *)&addr_len);
                if (recv_len <= 0) {
                    continue;
                }

                turing_airkiss_ret_t lan_ret = turing_airkiss_lan_recv(buf, recv_len);
                switch (lan_ret) {
                case TURING_AIRKISS_SSDP_REQ:
                    log_d("TURING_AIRKISS_SSDP_REQ");
                    remote_addr.sin_port = htons(AIRKISS_DEFAULT_LAN_PORT);
                    ret = sendto(fd, resp_buf, resp_len, 0,
                                 (struct sockaddr *)&remote_addr, addr_len);
                    if (ret < 0) {
                        err = turing_get_socket_errno(fd);
                        if (err != ENOMEM && err != EAGAIN) {
                            log_e("send notify msg ERROR! errno %d", err);
                            s_stop_send = true;
                        }
                    } else {
                        log_d("send notify msg OK!");
                    }
                    break;
                default:
                    log_v("Pack is not ssdp req!");
                    break;
                }
            } else {
                /* log_v("recv nothing!"); */
            }
        }
    } while (0);

    if (fd > -1) {
        close(fd);
        fd = -1;
    }

    if (buf) {
        free(buf);
        buf = NULL;
    }

    if (resp_buf) {
        free(resp_buf);
        resp_buf = NULL;
    }
}

int turing_start_bind_device(int lifecycle)
{
    if (lifecycle < 60) {
        log_e("the lifecycle is too short");
        return -1;
    }

    if (turing_bind_task_pid) {
        log_e("a task has been started!");
        return -1;
    }

    s_stop_send = false;

    return thread_fork("turing_bind_device_task", 20, 768, 0, &turing_bind_task_pid, turing_bind_device, (void *)lifecycle);
}

void turing_stop_bind_device_task(void)
{
    if (!turing_bind_task_pid) {
        return;
    }

    s_stop_send = true;
    thread_kill(&turing_bind_task_pid, KILL_WAIT);
    turing_bind_task_pid = 0;
}

int turing_set_airkiss_para(const char *device_type, const char *device_id_prefix, const char *uuid)
{
    if (!device_id_prefix || !device_type || !uuid) {
        return -1;
    }

    if (__this->device_type) {
        free(__this->device_type);
        __this->device_type = NULL;
    }
    if (__this->device_id_prefix) {
        free(__this->device_id_prefix);
        __this->device_id_prefix = NULL;
    }
    if (__this->uuid) {
        free(__this->uuid);
        __this->uuid = NULL;
    }

    __this->device_type = malloc(strlen(device_type) + 1);
    if (__this->device_type == NULL) {
        return -1;
    }
    strcpy(__this->device_type, device_type);
    __this->device_id_prefix = malloc(strlen(device_id_prefix) + 1);
    if (__this->device_id_prefix == NULL) {
        return -1;
    }
    strcpy(__this->device_id_prefix, device_id_prefix);
    __this->uuid = malloc(strlen(uuid) + 1);
    if (__this->uuid == NULL) {
        return -1;
    }
    strcpy(__this->uuid, uuid);

    return 0;
}
