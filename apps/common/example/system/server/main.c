#include "app_config.h"
#include "system/includes.h"
#include "server/server_core.h"
/* #include "../../../apps/common/example/server/test_server.h" */
#include "test_server.h"

#ifdef USE_SERVER_TEST_DEMO

static void test_server_event_handler(void *priv, int argc, int *argv)
{
    switch (argv[0]) {
    case TEST_SERVER_EVENT:
        printf("TEST_SERVER_EVENT[in %s task] run : priv=%s, argc=0x%x, argv[1]=0x%x\n", os_current_task(), priv, argc, argv[1]);
        break;
    default:
        break;
    }
}

static int test_server_completed_func(void *priv, int err)
{
    printf("test_server_completed_func[in %s task] run : 0x%x, err = 0x%x\n", os_current_task(), priv, err);

    return 0;
}


static void server_example_task(void *p)
{
    int test_cnt = 1;
    int ret;
    int msg[32];
    int argv[2];
    struct test_server_req_parm test_server_req_parm;


    //如果 test_server 打包成库, 这句话用于把库链接进来, 不打包成库就不需要
    server_load(test_server);

    //打开服务
    struct server *test_server_hdl = server_open("test_server", "server_open_arg");
    if (!test_server_hdl) {
        puts("server_test_task server_open faild!!!\n");
    }

    //注册服务事件通知回调
    server_register_event_handler(test_server_hdl, "test_priv", test_server_event_handler);

    while (1) {
        puts("\r\n\r\n");

        //__os_taskq_pend 是为了让 test_server_event_handler 和 test_server_completed_func 能够在里面运行
        ret = __os_taskq_pend(msg, ARRAY_SIZE(msg), 200);   //200个tick超时返回 OS_Q_EMPTY

        if (test_cnt-- == 0) {
            break;
        }

        //同步请求服务做一件事情
        test_server_req_parm.cmd = 0xa;
        ret = server_request(test_server_hdl, TEST_REQ_DO1, &test_server_req_parm);
        if (ret) {
            printf("server_request faild = 0x%x!!!\n", ret);
        }

        test_server_req_parm.cmd = 0xb;
        ret = server_request(test_server_hdl, TEST_REQ_DO2, &test_server_req_parm);
        if (ret) {
            printf("server_request faild = %d!!!\n", ret);
        }
        printf("server_request TEST_REQ_DO2 test_server_req_parm.cmd = 0x%x \r\n", test_server_req_parm.cmd);

        //异步请求服务做一件事情
        test_server_req_parm.cmd = 0xc;
        ret = server_request_async(test_server_hdl, TEST_REQ_DO1, &test_server_req_parm);
        if (ret) {
            printf("server_request_async faild = %d!!!\n", ret);
        }

        //异步请求服务做一件事情, 并且完成后调用回调
        test_server_req_parm.cmd = 0xd;
        ret = server_request_async(test_server_hdl, TEST_REQ_DO1 | REQ_COMPLETE_CALLBACK, &test_server_req_parm, test_server_completed_func, 0x2468);
        if (ret) {
            printf("server_request_async faild = %d!!!\n", ret);
        }
        test_server_req_parm.cmd = 0xe;
        ret = server_request_async(test_server_hdl, TEST_REQ_DO2 | REQ_COMPLETE_CALLBACK, &test_server_req_parm, test_server_completed_func, 0x246A);
        if (ret) {
            printf("server_request_async faild = %d!!!\n", ret);
        }

    }

    //清除服务队列还没执行的 server_event_handler 事件通知, 参数必须与server_event_handler指定事件相同
    argv[0] = TEST_SERVER_EVENT;
    argv[1] = 0x13579;
    ret = server_event_handler_del(test_server_hdl, sizeof(argv) / sizeof(argv[0]), argv);
    if (ret && ret != OS_Q_ERR) {
        printf("server_event_handler_del faild = %d!!!\n", ret);
    }

    //关闭服务,释放资源
    server_close(test_server_hdl);
}



static int c_main(void)
{
    printf("\r\n\r\n\r\n\r\n\r\n -----------server_example run %s -------------\r\n\r\n\r\n\r\n\r\n", __TIME__);

    thread_fork("server_example_task", 13, 1000, 128, NULL, server_example_task, NULL);

    return 0;
}
late_initcall(c_main);
#endif
