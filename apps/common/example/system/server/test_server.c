#include "system/includes.h"
#include "server/server_core.h"
/* #include "../../../apps/common/example/server/test_server.h" */
#include "test_server.h"

#define TEST_SERVER_VERSION 0x10000000     //如果 test_server 打包成库, 这段操作用于把库生成一个对外接口: server_load(test_server);

#ifdef TEST_SERVER_VERSION
#define THIS_MODULE  test_server
MODULE_VERSION_EXPORT(THIS_MODULE, TEST_SERVER_VERSION);
MODULE_DEPEND_BEGIN()
MODULE_DEPEND_END()
#endif

static void test_server_task(void *p)
{
    int res;
    int msg[32];

    while (1) {
        res = os_task_pend("taskq", msg, ARRAY_SIZE(msg));
        switch (res) {
        case OS_TASKQ:
            switch (msg[0]) {
            case Q_EVENT:
                break;
            case Q_MSG:

                struct server_req *req = (struct server_req *)(msg[1]);
                struct test_server_req_parm *test_server_req_parm = (int *)req->arg;

                switch (req->type) {
                case TEST_REQ_DO1:
                    printf("TEST_REQ_DO1 run req_arg:0x%x \r\n", test_server_req_parm->cmd);
                    req->err = 0xaa;

                    if (test_server_req_parm->cmd == 0xa) {
                        //服务有事件要通知上层应用时调用此函数
                        int argv[2];
                        argv[0] = TEST_SERVER_EVENT;
                        argv[1] = 0x13579;
                        res = server_event_handler(req->server->server, sizeof(argv) / sizeof(argv[0]), argv);
                        if (res) {
                            printf("server_event_handler faild = %d!!!\n", res);
                        }
                    }

                    break;
                case TEST_REQ_DO2:
                    printf("TEST_REQ_DO2 run req_arg:0x%x \r\n", test_server_req_parm->cmd);
                    req->err = 0;
                    test_server_req_parm->cmd = 0xbb;
                    break;
                }

                server_req_complete(req); //请求响应完成，由服务调用通知server内核
                break;
            default:
                break;
            }
            break;
        case OS_TIMER:
            break;
        case OS_TIMEOUT:
            break;
        }
    }
}

static void *test_server_open(struct server *server, void *arg)
{
    printf("test_server_open run arg = %s \r\n", arg);

    //需要先创建一个与 SERVER_REGISTER name 同名的任务
    os_task_create(test_server_task, NULL, 10, 1000, 128, "test_server");

    return server;  //return server->server
}

static void test_server_close(void *p)
{
    printf("test_server_close run\r\n");
    os_task_del("test_server");
}


SERVER_REGISTER(test_server_info) = {
    .name = "test_server",
    .reqlen = sizeof(struct test_server_req_parm),
    .open = test_server_open,
    .close = test_server_close,

    /*
    如果只需要同步请求,设置为0;
    如果需要用到异步请求,reqnum就代表预申请最大同时可以排队多少个请求,
    一般调用server_request_async的任务很繁忙的情况下才需要配置,即使不预申请也没关系只是造成malloc碎片
    如果 发现打印出现  server_req_malloc, 需要增大reqnum
    */
    .reqnum = 0,
};
