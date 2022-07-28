#include "app_config.h"

#ifdef USE_AI_SDK_DEMO

#include "server/server_core.h"
#include "system/includes.h"
#include "ai_test_server.h"

#define AI_TEST_MAX_MUN 5

#define list_for_each_ai_handler(ai) \
    for (ai = AI_test_server.ai; ai->ai_sdk && ai < AI_test_server.ai + AI_TEST_MAX_MUN; ai++)

struct ai_test_handler {
    u8 connect;
    u8 initialized;
    int id;
    const struct ai_test_sdk_api *ai_sdk;
};

struct ai_test_server {
    struct ai_test_handler ai[AI_TEST_MAX_MUN];
    void *private_data;
};

static struct ai_test_server AI_test_server;

int ai_test_server_event_notify(const struct ai_test_sdk_api *sdk, void *arg, int event)
{
    int msg[4];
    struct ai_test_handler *ai = NULL;

    list_for_each_ai_handler(ai) {
        if (ai->initialized && ai->ai_sdk == sdk) {
            goto __send_event;
        }
    }

    return -EINVAL;

__send_event:

    msg[0] = event;
    msg[1] = (int)arg;
    msg[2] = (int)sdk->name;

    return server_event_handler(AI_test_server.private_data, 3, msg);
}

int ai_test_server_event_url(const struct ai_test_sdk_api *sdk, const char *url, int event)
{
    int msg[4];
    struct ai_test_handler *ai = NULL;
    char *save_url = NULL;

    list_for_each_ai_handler(ai) {
        if (ai->initialized && ai->ai_sdk == sdk) {
            goto __send_event;
        }
    }

    return -EINVAL;

__send_event:

    if (url) {
        save_url = malloc(strlen(url) + 1);
        if (!save_url) {
            return -ENOMEM;
        }
        strcpy(save_url, url);
    }

    msg[0] = event;
    msg[1] = (int)save_url;
    msg[2] = (int)sdk->name;

    return server_event_handler(AI_test_server.private_data, 3, msg);
}

static void ai_test_check_connect_state(void *priv)
{
    int msg[2];
    struct ai_test_handler *ai = (struct ai_test_handler *)priv;

    if (!ai->initialized) {
        return;
    }

    int state = ai->ai_sdk->state_check();

    switch (state) {
    case AI_STAT_CONNECTED:
        if (!ai->connect) {
            ai->connect = 1;
            msg[0] = AI_SERVER_EVENT_CONNECTED;
            msg[1] = (int)ai->ai_sdk->name;
            server_event_handler(AI_test_server.private_data, 2, msg);
        }
        break;
    case AI_STAT_DISCONNECTED:
        if (ai->connect) {
            ai->connect = 0;
            msg[0] = AI_SERVER_EVENT_DISCONNECTED;
            msg[1] = (int)ai->ai_sdk->name;
            server_event_handler(AI_test_server.private_data, 2, msg);
        }
        break;
    }
}

static int ai_test_req_connect(struct ai_server *server, union ai_test_req *req)
{
    int ret = -EFAULT;
    struct ai_test_handler *ai;

    list_for_each_ai_handler(ai) {
        if (req->evt.ai_name && strcmp(ai->ai_sdk->name, req->evt.ai_name)) {
            continue;
        }
        if (ai->initialized) {
            continue;
        }
        printf("ai_connect: %s\n", ai->ai_sdk->name);
        int err = ai->ai_sdk->connect();
        if (err == 0) {
            ret = 0;
            ai->initialized = 1;
            ai->id = sys_timer_add(ai, ai_test_check_connect_state, 1000);
        }
    }

    return ret;
}

static int ai_test_req_disconnect(struct ai_server *server, union ai_test_req *req)
{
    int ret = -EFAULT;
    struct ai_test_handler *ai;

    list_for_each_ai_handler(ai) {
        if (req->evt.ai_name && strcmp(ai->ai_sdk->name, req->evt.ai_name)) {
            continue;
        }
        if (!ai->initialized) {
            continue;
        }
        printf("ai_disconnect: %s\n", ai->ai_sdk->name);
        if (ai->id) {
            sys_timer_del(ai->id);
            ai->id = 0;
        }
        int err = ai->ai_sdk->disconnect();
        if (err == 0) {
            ai->connect = 0;
            ai->initialized = 0;
            ret = 0;
        }
    }

    return ret;
}

static int ai_test_req_listen(struct ai_server *server, union ai_test_req *req)
{
    struct ai_test_handler *ai;

    if (req->lis.cmd == AI_LISTEN_START) {
        list_for_each_ai_handler(ai) {
            if (req->lis.ai_name && strcmp(ai->ai_sdk->name, req->lis.ai_name)) {
                continue;
            }
            if (ai->initialized && ai->connect) {
                ai->ai_sdk->do_event(AI_EVENT_RECORD_START, req->lis.arg);
            }
        }
    } else if (req->lis.cmd == AI_LISTEN_STOP) {
        list_for_each_ai_handler(ai) {
            if (req->lis.ai_name && strcmp(ai->ai_sdk->name, req->lis.ai_name)) {
                continue;
            }
            if (ai->initialized && ai->connect) {
                ai->ai_sdk->do_event(AI_EVENT_RECORD_STOP, req->lis.arg);
            }
        }
    }

    return 0;
}

static int ai_test_req_event(struct ai_server *server, union ai_test_req *req)
{
    struct ai_test_handler *ai;

    list_for_each_ai_handler(ai) {
        if (ai->initialized && !strcmp(ai->ai_sdk->name, req->evt.ai_name)) {
            return ai->ai_sdk->do_event(req->evt.event, req->evt.arg);
        }
    }

    return -EINVAL;
}


int ai_server_request(void *server, int req_type, union ai_test_req *req)
{
    int err = 0;
    struct ai_server *ai = ((struct server *)server)->server;

    switch (req_type) {
    case AI_REQ_CONNECT:
        err = ai_test_req_connect(ai, req);
        break;
    case AI_REQ_DISCONNECT:
        err = ai_test_req_disconnect(ai, req);
        break;
    case AI_REQ_LISTEN:
        err = ai_test_req_listen(ai, req);
        break;
    case AI_REQ_EVENT:
        err = ai_test_req_event(ai, req);
        break;
    default:
        break;
    }

    return err;
}



static int ai_test_server_init()
{
    memset(&AI_test_server, 0, sizeof(AI_test_server));

    return 0;
}
module_initcall(ai_test_server_init);

static void *ai_test_server_open(void *priv, void *name)
{
    int index;
    const struct ai_test_sdk_api *ai_sdk;

    list_for_each_ai_sdk(ai_sdk) {
        AI_test_server.ai[index].ai_sdk = ai_sdk;
        AI_test_server.ai[index].connect = 0;
        AI_test_server.ai[index].initialized = 0;
        printf("find ai_server: %s\n", ai_sdk->name);
    }

    AI_test_server.private_data = priv;

    return &AI_test_server;
}

static void ai_test_server_close(void *p)
{
    struct ai_test_handler *ai;

    list_for_each_ai_handler(ai) {
        if (ai->id) {
            sys_timer_del(ai->id);
            ai->id = 0;
        }
        if (ai->initialized && ai->ai_sdk->disconnect) {
            ai->ai_sdk->disconnect();
        }
        ai->connect = 0;
        ai->initialized = 0;
    }
}

SERVER_REGISTER(ai_test_server_info) = {
    .name = "ai_test_server",
    .reqlen = sizeof(union ai_test_req),
    .open = ai_test_server_open,
    .close = ai_test_server_close,
};

#endif //USE_AI_SDK_DEMO
