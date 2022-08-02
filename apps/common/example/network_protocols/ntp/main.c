#include "wifi/wifi_connect.h"
#include "system/includes.h"

#ifdef USE_NTP_TEST

static void ntp_test2()
{
    //填NULL, 请求所有ntp_list列表里面的NTP服务器
    ntp_client_get_time(NULL);
}
static void ntp_test()
{

    enum wifi_sta_connect_state state;
    while (1) {
        printf("Connecting to the network...\n");
        state = wifi_get_sta_connect_state();
        if (WIFI_STA_NETWORK_STACK_DHCP_SUCC == state) {
            printf("Network connection is successful!\n");
            break;
        }

        os_time_dly(20);
    }

    thread_fork("test", 10, 512, 0, 0, ntp_test2, NULL);


    //退出当前ntp请求,主要用于弱网情况下卡ntp请求过久
    ntp_client_get_time_exit();

    os_time_dly(20);

    //获取请求状态
    u8 status = ntp_client_get_time_status();
    printf("status = %d\n\r", status);

    //清除请求状态
    ntp_client_get_time_clear();

    //非NULL, 则只请求指定NTP服务器
    char *ntp_server = "s2c.time.edu.cn";
    ntp_client_get_time(ntp_server);

    //获取请求状态
    status = ntp_client_get_time_status();
    printf("status = %d\n\r", status);


}

void c_main()
{
    if (thread_fork("ntp_test", 10, 2 * 1024, 0, NULL, ntp_test, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}
late_initcall(c_main);

#endif
