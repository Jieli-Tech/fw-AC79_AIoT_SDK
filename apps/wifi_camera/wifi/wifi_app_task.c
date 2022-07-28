#include "wifi/wifi_connect.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "os/os_api.h"
#include "system/init.h"
#include "lwip.h"
#include "dhcp_srv/dhcp_srv.h"
#include "lwip/dns.h"
#include "device/device.h"
#include "system/app_core.h"
#include "server/server_core.h"
#include "action.h"
#include "system/timer.h"
#include "asm/debug.h"
#include "app_config.h"
#include "http/http_cli.h"
#include "system/timer.h"
#include "database.h"
#include "dev_desc.h"
#include "http/http_server.h"
#include "server/ctp_server.h"
#include "server/net_server.h"
#include "video_rt_tcp.h"
#include "ftpserver/stupid-ftpd.h"
#include "streaming_media_server/fenice_config.h"
#include "syscfg/syscfg_id.h"

#define WIFI_APP_TASK_NAME "wifi_app_task"

#ifdef CONFIG_STATIC_IPADDR_ENABLE
const u8  IPV4_ADDR_CONFLICT_DETECT = 1;
#else
const u8  IPV4_ADDR_CONFLICT_DETECT = 0;
#endif

extern char get_MassProduction(void);
static void wifi_set_lan_setting_info(void);

static struct server *ctp = NULL;
static struct ctp_server_info server_info = {
    .ctp_vaild = true,
    .ctp_port = CTP_CTRL_PORT,
    .cdp_vaild = true,
    .cdp_port = CDP_CTRL_PORT,
    .k_alive_type = CTP_ALIVE,
    /*.k_alive_type = CDP_ALIVE,*/
};

static void wifi_app_timer_func(void *p)
{
    if (wifi_is_on()) {
        printf("WIFI U= %d KB/s, D= %d KB/s\r\n", wifi_get_upload_rate() / 1024, wifi_get_download_rate() / 1024);

#ifndef CONFIG_WIFI_STA_MODE
        for (int i = 0; i < 8; i++) {
            char *rssi;
            u8 *evm, *mac;
            if (wifi_get_sta_entry_rssi(i, &rssi, &evm, &mac)) {
                break;
            }
            if (*rssi) {
                printf("MAC[%x:%x:%x:%x:%x:%x],RSSI=%d,EVM=%d \r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], *rssi, *evm);
            }
        }
#endif
        malloc_stats();
    }
}

static void wifi_sta_to_ap_mode_change(void)//用在STA模式密码不对或者找不到SSID，自动切换AP模式
{
    u8 mac_addr[6];
    char ssid[64];
    struct wifi_mode_info info;
    info.mode = AP_MODE;
    wifi_get_mode_cur_info(&info);
    if (!strcmp("", info.ssid)) {
        wifi_get_mac(mac_addr);
        sprintf((char *)ssid, AP_WIFI_CAM_PREFIX"%02x%02x%02x%02x%02x%02x", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        info.ssid = ssid;
        info.pwd = AP_WIFI_CAM_WIFI_PWD;
    }

    wifi_set_lan_setting_info(); //STA转AP模式,需要重新配置一下IP信息和DHCP池起始分配地址
    wifi_enter_ap_mode(info.ssid, info.pwd);
    wifi_store_mode_info(AP_MODE, info.ssid, info.pwd);
}

static void wifi_set_lan_setting_info(void)
{
    struct lan_setting lan_setting_info = {

        .WIRELESS_IP_ADDR0  = 192,
        .WIRELESS_IP_ADDR1  = 168,
        .WIRELESS_IP_ADDR2  = 1,
        .WIRELESS_IP_ADDR3  = 1,

        .WIRELESS_NETMASK0  = 255,
        .WIRELESS_NETMASK1  = 255,
        .WIRELESS_NETMASK2  = 255,
        .WIRELESS_NETMASK3  = 0,

        .WIRELESS_GATEWAY0  = 192,
        .WIRELESS_GATEWAY1  = 168,
        .WIRELESS_GATEWAY2  = 1,
        .WIRELESS_GATEWAY3  = 1,

        .SERVER_IPADDR1  = 192,
        .SERVER_IPADDR2  = 168,
        .SERVER_IPADDR3  = 1,
        .SERVER_IPADDR4  = 1,

        .CLIENT_IPADDR1  = 192,
        .CLIENT_IPADDR2  = 168,
        .CLIENT_IPADDR3  = 1,
        .CLIENT_IPADDR4  = 2,

        .SUB_NET_MASK1   = 255,
        .SUB_NET_MASK2   = 255,
        .SUB_NET_MASK3   = 255,
        .SUB_NET_MASK4   = 0,
    };
    net_set_lan_info(&lan_setting_info);
}

static int wifi_event_callback(void *network_ctx, enum WIFI_EVENT event)
{

    switch (event) {

    case WIFI_EVENT_MODULE_INIT:

        wifi_set_lan_setting_info();

        u8 mac_addr[6];
        char ssid[64];
        int init_net_device_mac_addr(char *macaddr, char ap_mode); //如果AP模式需要配置SSID和MAC地址相关,需要在这里先产生MAC地址,否则就不用
        init_net_device_mac_addr((char *)mac_addr, 1);

        sprintf((char *)ssid, AP_WIFI_CAM_PREFIX"%02x%02x%02x%02x%02x%02x",
                mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        struct wifi_store_info wifi_default_mode_parm;
        memset(&wifi_default_mode_parm, 0, sizeof(struct wifi_store_info));

        wifi_default_mode_parm.mode = AP_MODE;
        if (wifi_default_mode_parm.mode == AP_MODE || wifi_default_mode_parm.mode == STA_MODE) {
            strncpy((char *)wifi_default_mode_parm.ssid[wifi_default_mode_parm.mode - STA_MODE],
                    (const char *)ssid,
                    sizeof(wifi_default_mode_parm.ssid[wifi_default_mode_parm.mode - STA_MODE]) - 1);
            strncpy((char *)wifi_default_mode_parm.pwd[wifi_default_mode_parm.mode - STA_MODE],
                    (const char *)AP_WIFI_CAM_WIFI_PWD,
                    sizeof(wifi_default_mode_parm.pwd[wifi_default_mode_parm.mode - STA_MODE]) - 1);
        }
#ifdef CONFIG_WIFI_STA_MODE
        wifi_default_mode_parm.mode = STA_MODE;
        if (wifi_default_mode_parm.mode == AP_MODE || wifi_default_mode_parm.mode == STA_MODE) {
            strncpy((char *)wifi_default_mode_parm.ssid[wifi_default_mode_parm.mode - STA_MODE],
                    (const char *)STA_WIFI_SSID,
                    sizeof(wifi_default_mode_parm.ssid[wifi_default_mode_parm.mode - STA_MODE]) - 1);
            strncpy((char *)wifi_default_mode_parm.pwd[wifi_default_mode_parm.mode - STA_MODE],
                    (const char *)STA_WIFI_PWD,
                    sizeof(wifi_default_mode_parm.pwd[wifi_default_mode_parm.mode - STA_MODE]) - 1);
        }
#endif
        wifi_set_default_mode(&wifi_default_mode_parm, 0, wifi_default_mode_parm.mode == STA_MODE); //配置STA模式情况下,把默>    认配置SSID也存储起来,以后即使保存过其他SSID,也不会覆盖丢失,使用连接最优信号SSID策略的情况下可以匹配连接
        break;

    case WIFI_EVENT_MODULE_START:
        if (!get_MassProduction()) {



            u32  tx_rate_control_tab = // 不需要哪个速率就删除掉,可以动态设定
                0
                /*|BIT(0) //0:CCK 1M*/
                /*|BIT(1) //1:CCK 2M*/
                /*| BIT(2) //2:CCK 5.5M*/
                /*| BIT(3) //3:OFDM 6M*/
                /*| BIT(4) //4:MCS0/7.2M*/
                /*| BIT(5) //5:OFDM 9M*/
                | BIT(6) //6:CCK 11M
                | BIT(7) //7:OFDM 12M
                | BIT(8) //8:MCS1/14.4M
                | BIT(9) //9:OFDM 18M
                | BIT(10) //10:MCS2/21.7M
                | BIT(11) //11:OFDM 24M
                | BIT(12) //12:MCS3/28.9M
                | BIT(13) //13:OFDM 36M
                | BIT(14) //14:MCS4/43.3M
                /*| BIT(15) //15:OFDM 48M*/
                /*| BIT(16) //16:OFDM 54M*/
                /*|BIT(17) //17:MCS5/57.8M */
                /*|BIT(18) //18:MCS6/65.0M */
                /*|BIT(19) //19:MCS7/72.2M */
                ;
            wifi_set_tx_rate_control_tab(tx_rate_control_tab);

        }
#if 0
        wifi_set_pwr(0);
#endif
        break;
    case WIFI_EVENT_MODULE_STOP:
        puts("|network_user_callback->WIFI_EVENT_MODULE_STOP\n");
        break;

    case WIFI_EVENT_AP_START:
        printf("|network_user_callback->WIFI_EVENT_AP_START,CH=%d\n", wifi_get_channel());
        //wifi_rxfilter_cfg(7); //过滤广播+多播+not_my_bssid
        break;
    case WIFI_EVENT_AP_STOP:
        puts("|network_user_callback->WIFI_EVENT_AP_STOP\n");
        break;

    case WIFI_EVENT_STA_START:
        puts("|network_user_callback->WIFI_EVENT_STA_START\n");
        wifi_set_sta_connect_timeout(30);//30s timeout
        break;
    case WIFI_EVENT_MODULE_START_ERR:
        puts("|network_user_callback->WIFI_EVENT_MODULE_START_ERR\n");
        break;
    case WIFI_EVENT_STA_STOP:
        puts("|network_user_callback->WIFI_EVENT_STA_STOP\n");
        break;
    case WIFI_EVENT_STA_DISCONNECT:
        puts("|network_user_callback->WIFI_STA_DISCONNECT\n");
        break;
    case WIFI_EVENT_STA_SCAN_COMPLETED:
        puts("|network_user_callback->WIFI_STA_SCAN_COMPLETED\n");
        break;
    case WIFI_EVENT_STA_CONNECT_SUCC:
        printf("|network_user_callback->WIFI_STA_CONNECT_SUCC,CH=%d\r\n", wifi_get_channel());
        break;

    case WIFI_EVENT_MP_TEST_START:
        puts("|network_user_callback->WIFI_EVENT_MP_TEST_START\n");
        break;
    case WIFI_EVENT_MP_TEST_STOP:
        puts("|network_user_callback->WIFI_EVENT_MP_TEST_STOP\n");
        break;

    case WIFI_EVENT_STA_CONNECT_TIMEOUT_NOT_FOUND_SSID:
        puts("|network_user_callback->WIFI_STA_CONNECT_TIMEOUT_NOT_FOUND_SSID\n");
#if 0	//打开STA模式扫描连接扫描不到wifi名称，自动切回AP模式，防止死循环扫描无法回AP模式
        static u8 cnt = 0;
        cnt++;

        if (cnt >= 2) {
            sys_timeout_add(NULL, wifi_sta_to_ap_mode_change, 100);//回调不能直接切换，此处放在app_core切换
        }

#endif
        break;

    case WIFI_EVENT_STA_CONNECT_TIMEOUT_ASSOCIAT_FAIL:
        puts("|network_user_callback->WIFI_STA_CONNECT_TIMEOUT_ASSOCIAT_FAIL .....\n");
#if 0	//打开STA模式连接路由器密码错误，自动切回AP模式，防止死循环连接路由器
        sys_timeout_add(NULL, wifi_sta_to_ap_mode_change, 100);//回调不能直接切换，此处放在app_core切换
#endif
        break;

    case WIFI_EVENT_STA_NETWORK_STACK_DHCP_SUCC:
        puts("|network_user_callback->WIFI_EVENT_STA_NETWPRK_STACK_DHCP_SUCC\n");

        //有些使用了加密的路由器刚获取IP地址后前几个包都会没响应，怀疑路由器没转发成功
        void connect_broadcast(void);
        connect_broadcast();

        void network_mssdp_init(void);
        network_mssdp_init();
        break;
    case WIFI_EVENT_STA_NETWORK_STACK_DHCP_TIMEOUT:
        puts("|network_user_callback->WIFI_EVENT_STA_NETWPRK_STACK_DHCP_TIMEOUT\n");
        break;

    case WIFI_EVENT_P2P_START:
        puts("|network_user_callback->WIFI_EVENT_P2P_START\n");
        break;
    case WIFI_EVENT_P2P_STOP:
        puts("|network_user_callback->WIFI_EVENT_P2P_STOP\n");
        break;
    case WIFI_EVENT_P2P_GC_DISCONNECTED:
        puts("|network_user_callback->WIFI_EVENT_P2P_GC_DISCONNECTED\n");
        break;
    case WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_SUCC:
        puts("|network_user_callback->WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_SUCC\n");
        break;
    case WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_TIMEOUT:
        puts("|network_user_callback->WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_TIMEOUT\n");
        break;

    case WIFI_EVENT_SMP_CFG_START:
        puts("|network_user_callback->WIFI_EVENT_SMP_CFG_START\n");
        break;
    case WIFI_EVENT_SMP_CFG_STOP:
        puts("|network_user_callback->WIFI_EVENT_SMP_CFG_STOP\n");
        break;
    case WIFI_EVENT_SMP_CFG_TIMEOUT:
        puts("|network_user_callback->WIFI_EVENT_SMP_CFG_TIMEOUT\n");
        break;
    case WIFI_EVENT_SMP_CFG_COMPLETED:
        puts("|network_user_callback->WIFI_EVENT_SMP_CFG_COMPLETED\n");
        break;

    case WIFI_EVENT_PM_SUSPEND:
        puts("|network_user_callback->WIFI_EVENT_PM_SUSPEND\n");
        break;
    case WIFI_EVENT_PM_RESUME:
        puts("|network_user_callback->WIFI_EVENT_PM_RESUME\n");
        break;
    case WIFI_EVENT_AP_ON_ASSOC:
        ;
        struct eth_addr *hwaddr = (struct eth_addr *)network_ctx;
        printf("WIFI_EVENT_AP_ON_ASSOC hwaddr = %02x:%02x:%02x:%02x:%02x:%02x \r\n\r\n",
               hwaddr->addr[0], hwaddr->addr[1], hwaddr->addr[2], hwaddr->addr[3], hwaddr->addr[4], hwaddr->addr[5]);
        break;
    case WIFI_EVENT_AP_ON_DISCONNECTED:
        hwaddr = (struct eth_addr *)network_ctx;
        struct ip4_addr ipaddr;
        dhcps_get_ipaddr(hwaddr->addr, &ipaddr);
        printf("WIFI_EVENT_AP_ON_DISCONNECTED hwaddr = %02x:%02x:%02x:%02x:%02x:%02x, ipaddr = [%d.%d.%d.%d] \r\n\r\n",
               hwaddr->addr[0], hwaddr->addr[1], hwaddr->addr[2], hwaddr->addr[3], hwaddr->addr[4], hwaddr->addr[5],
               ip4_addr1(&ipaddr), ip4_addr2(&ipaddr), ip4_addr3(&ipaddr), ip4_addr4(&ipaddr));
        ctp_keep_alive_find_dhwaddr_disconnect((struct eth_addr *)hwaddr->addr);
        cdp_keep_alive_find_dhwaddr_disconnect((struct eth_addr *)hwaddr->addr);
        break;
    default:
        break;
    }

    return 0;
}


void net_app_init(void)
{
#ifdef CONFIG_MASS_PRODUCTION_ENABLE
    if (get_MassProduction()) {
        wifi_enter_sta_mode(STA_WIFI_SSID, STA_WIFI_PWD);
#if 0
        /*     **代码段功能:修改RTSP的URL */
        /* **默认配置  :URL为rtsp://192.168.1.1/avi_pcm_rt/front.sd,//(avi_pcma_rt 传G7111音频)传JPEG实时流 */
        /* ** */
        /* * */
        const char *user_custom_name = "avi_pcm_rt";
        const char *user_custom_content =
            "stream\r\n"\
            "file_ext_name avi\r\n"\
            "media_source live\r\n"\
            "priority 1\r\n"\
            "payload_type 26\r\n"\
            "clock_rate 90000\r\n"\
            "encoding_name JPEG\r\n"\
            "coding_type frame\r\n"\
            "byte_per_pckt 1458\r\n"\
            "stream_end\r\n"\
            "stream\r\n"\
            "file_ext_name pcm\r\n"\
            "media_source live\r\n"\
            "priority 1\r\n"\
            "payload_type 8\r\n"\
            "encoding_name PCMA\r\n"\
            "clock_rate 8000\r\n"\
            "stream_end";
        extern void rtsp_modify_url(const char *user_custom_name, const char *user_custom_content);
        rtsp_modify_url(user_custom_name, user_custom_content);
#endif
        extern int stream_media_server_init(struct fenice_config * conf);
        extern int fenice_get_video_info(struct fenice_source_info * info);
        extern int fenice_get_audio_info(struct fenice_source_info * info);
        extern int fenice_set_media_info(struct fenice_source_info * info);
        extern int fenice_video_rec_setup(void);
        extern int fenice_video_rec_exit(void);
        struct fenice_config conf = {0};

        strncpy(conf.protocol, "UDP", 3);
        conf.exit = fenice_video_rec_exit;
        conf.setup = fenice_video_rec_setup;
        conf.get_video_info = fenice_get_video_info;
        conf.get_audio_info = fenice_get_audio_info;
        conf.set_media_info = fenice_set_media_info;
        conf.port = RTSP_PORT;  // 当为0时,用默认端口554
        stream_media_server_init(&conf);
    } else
#endif
    {
        ctp = server_open("ctp_server", (void *)&server_info);
        if (!ctp) {
            printf("ctp server fail\n");
        }
        puts("http server init\n");
        extern int http_virfile_reg(const char *path, const char *contents, unsigned long len);
        http_virfile_reg(DEV_DESC_PATH, DEV_DESC_CONTENT, strlen(DEV_DESC_CONTENT)); //注册虚拟文件描述文档,可在dev_desc.h修改
        http_get_server_init(HTTP_PORT); //8080
        video_rt_tcp_server_init(2229);


        printf("ftpd server init \n");
        extern void ftpd_vfs_interface_cfg(void);
        ftpd_vfs_interface_cfg();
        stupid_ftpd_init("MAXUSERS=2\nUSER=FTPX 12345678     0:/      2   A\n", NULL);
    }
#ifdef CONFIG_IPERF_ENABLE
//网络测试工具，使用iperf
    extern void iperf_test(void);
    iperf_test();
#endif
}

void net_app_uninit(void)
{
    puts("ctp server uninit\n");
    server_close(ctp);
    puts("http server uninit\n");
    http_get_server_uninit(); //8080
    stupid_ftpd_uninit();
    video_rt_tcp_server_uninit();
#ifdef CONFIG_MASS_PRODUCTION_ENABLE
    extern void stream_media_server_uninit(void);
    stream_media_server_uninit();
#endif
}

static void wifi_app_task(void *priv)
{
    int msg[32];
    int res;

    wifi_set_event_callback(wifi_event_callback);
    wifi_on();

    //非量产模式下的AP和ST设置
#ifndef CONFIG_MASS_PRODUCTION_ENABLE
    //以下在wifi启动之后可以设置模式、名称、密码，不使用则是默认AP模式、wifi_camera_wl8x_xxxx、12345678
#ifdef CONFIG_WIFI_STA_MODE
    wifi_enter_sta_mode(STA_WIFI_SSID, STA_WIFI_PWD);//设置wifi进入STA模式以及STA名称和密码
    wifi_store_mode_info(STA_MODE, STA_WIFI_SSID, STA_WIFI_PWD);//保存默认模式
#else
    //设置WIFI模式和名称密码，注意：不同AP和STA模式需要对应的库
    /*wifi_enter_ap_mode("test_123456", "12345678");//设置wifi进入AP模式以及AP名称和密码*/
    /*wifi_store_mode_info(AP_MODE, "test_123456", "12345678");//保存默认模式*/
#endif
#endif

    net_app_init();
    sys_timer_add_to_task("sys_timer", NULL, wifi_app_timer_func, 5 * 1000);
}

#ifdef CONFIG_WIFI_ENABLE
int wireless_net_init(void)//主要是create wifi 线程的
{
    return thread_fork(WIFI_APP_TASK_NAME, 10, 1500, 64, 0, wifi_app_task, NULL);
}
late_initcall(wireless_net_init);
#endif

const char *get_root_path(void)
{
    return CONFIG_ROOT_PATH;
}

