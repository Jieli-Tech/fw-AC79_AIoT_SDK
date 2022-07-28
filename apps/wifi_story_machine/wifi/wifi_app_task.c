#include "init.h"
#include "wifi/wifi_connect.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/ip.h"
#include "os/os_api.h"
#include "lwip.h"
#include "server/server_core.h"
#include "system/timer.h"
#include "asm/debug.h"
#include "app_config.h"
#include "system/timer.h"
#include "json_c/json_tokener.h"
#include "server/net_server.h"
#include "streaming_media_server/fenice_config.h"
#include "event/net_event.h"
#include "syscfg/syscfg_id.h"
#ifdef CONFIG_ASSIGN_MACADDR_ENABLE
#include "net/assign_macaddr.h"
#include "net/config_network.h"
#endif
#ifdef CONFIG_WIFIBOX_ENABLE
#include "client.h"
#endif

#define CONNECT_TIMEOUT_SEC	60

static struct {
    u32 use_static_ipaddr_flag : 1;
    u32 net_app_init_flag : 1;
    u32 request_connect_flag : 1;
    u32 save_ssid_flag : 1;
    u32 mac_addr_succ_flag : 1;
    u32 reserved : 26;
} wifi_app_hdl;

#define __this	(&wifi_app_hdl)

extern void airkiss_ssid_check(void);

int __attribute__((weak)) wifi_force_set_lan_setting_info(void)
{
    return 0;
}


#ifdef CONFIG_STATIC_IPADDR_ENABLE
const u8  IPV4_ADDR_CONFLICT_DETECT = 1;
#else
const u8  IPV4_ADDR_CONFLICT_DETECT = 0;
#endif

#ifdef CONFIG_STATIC_IPADDR_ENABLE

struct sta_ip_info {
    char ssid[33];
    u32 ip;
    u32 gw;
    u32 netmask;
    u32 dns;
    u8 gw_mac[6];
    u8 local_mac[6];
    u8 chanel;
};

static void wifi_set_lan_setting_info(void)
{
    struct sta_ip_info  sta_ip_info;
    syscfg_read(VM_STA_IPADDR_INDEX, (char *) &sta_ip_info, sizeof(struct sta_ip_info));

    struct lan_setting lan_setting_info = {

        .WIRELESS_IP_ADDR0  = (u8)(sta_ip_info.ip >> 0),
        .WIRELESS_IP_ADDR1  = (u8)(sta_ip_info.ip >> 8),
        .WIRELESS_IP_ADDR2  = (u8)(sta_ip_info.ip >> 16),
        .WIRELESS_IP_ADDR3  = (u8)(sta_ip_info.ip >> 24),

        .WIRELESS_NETMASK0  = (u8)(sta_ip_info.netmask >> 0),
        .WIRELESS_NETMASK1  = (u8)(sta_ip_info.netmask >> 8),
        .WIRELESS_NETMASK2  = (u8)(sta_ip_info.netmask >> 16),
        .WIRELESS_NETMASK3  = (u8)(sta_ip_info.netmask >> 24),

        .WIRELESS_GATEWAY0   = (u8)(sta_ip_info.gw >> 0),
        .WIRELESS_GATEWAY1   = (u8)(sta_ip_info.gw >> 8),
        .WIRELESS_GATEWAY2   = (u8)(sta_ip_info.gw >> 16),
        .WIRELESS_GATEWAY3   = (u8)(sta_ip_info.gw >> 24),
    };

    net_set_lan_info(&lan_setting_info);
}

static int compare_dhcp_ipaddr(void)
{
    __this->use_static_ipaddr_flag = 0;

    int ret;
    u8 local_mac[6];
    u8 gw_mac[6];
    u8 sta_channel;
    struct sta_ip_info  sta_ip_info;
    struct netif_info netif_info;
    ret = syscfg_read(VM_STA_IPADDR_INDEX, (char *) &sta_ip_info, sizeof(struct sta_ip_info));

    if (ret < 0) {
        puts("compare_dhcp_ipaddr NO VM_STA_IPADDR_INDEX\r\n");
        return -1;
    }

    lwip_get_netif_info(1, &netif_info);

    struct wifi_mode_info info;
    info.mode = STA_MODE;
    wifi_get_mode_cur_info(&info);

    sta_channel = wifi_get_channel();
    wifi_get_bssid(gw_mac);
    wifi_get_mac(local_mac);

    if (!strcmp(info.ssid, sta_ip_info.ssid)
        && !memcmp(local_mac, sta_ip_info.local_mac, 6)
        && !memcmp(gw_mac, sta_ip_info.gw_mac, 6)
        /*&& sta_ip_info.gw==sta_ip_info.dns//如果路由器没接网线/没联网,每次连接都去重新获取DHCP*/
       ) {
        __this->use_static_ipaddr_flag = 1;
        puts("compare_dhcp_ipaddr Match\r\n");
        return 0;
    }

    printf("compare_dhcp_ipaddr not Match!!! [%s][%s],[0x%x,0x%x][0x%x,0x%x],[0x%x] \r\n", info.ssid, sta_ip_info.ssid, local_mac[0], local_mac[5], sta_ip_info.local_mac[0], sta_ip_info.local_mac[5], sta_ip_info.dns);

    return -1;
}

static void store_dhcp_ipaddr(void)
{
    struct sta_ip_info  sta_ip_info = {0};
    u8 sta_channel;
    u8 local_mac[6];
    u8 gw_mac[6];

    if (__this->use_static_ipaddr_flag) { //记忆IP匹配成功,不需要重新保存
        return;
    }

    struct netif_info netif_info;
    lwip_get_netif_info(1, &netif_info);

    struct wifi_mode_info info;
    info.mode = STA_MODE;
    wifi_get_mode_cur_info(&info);

    sta_channel = wifi_get_channel();
    wifi_get_mac(local_mac);
    wifi_get_bssid(gw_mac);

    strcpy(sta_ip_info.ssid, info.ssid);
    memcpy(sta_ip_info.gw_mac, gw_mac, 6);
    memcpy(sta_ip_info.local_mac, local_mac, 6);
    sta_ip_info.ip =  netif_info.ip;
    sta_ip_info.netmask =  netif_info.netmask;
    sta_ip_info.gw =  netif_info.gw;
    sta_ip_info.chanel = sta_channel;
    sta_ip_info.dns = *(u32 *)dns_getserver(0);

    syscfg_write(VM_STA_IPADDR_INDEX, (char *) &sta_ip_info, sizeof(struct sta_ip_info));

    puts("store_dhcp_ipaddr\r\n");
}

void dns_set_server(u32 *dnsserver)
{
    struct sta_ip_info  sta_ip_info;
    if (syscfg_read(VM_STA_IPADDR_INDEX, (char *) &sta_ip_info, sizeof(struct sta_ip_info)) < 0) {
        *dnsserver = 0;
    } else {
        *dnsserver = sta_ip_info.dns;
    }
}

#endif

#if 0
#define INIT_MODE STA_MODE
#define FORCE_DEFAULT_MODE 1
#define INIT_SSID "WL80_TEST"
#define INIT_PWD  "12345678"
#else
#define INIT_MODE SMP_CFG_MODE
#define FORCE_DEFAULT_MODE 0 //配置wifi_on之后的模式,0为使用最后记忆的模式, 1为强制默认模式, 3-200为连接超时时间多少秒,如果超时都连接不上就连接最后记忆的或者最优网络
#define INIT_SSID "GJ1"
#define INIT_PWD  "8888888899"
#endif
#define INIT_STORED_SSID (INIT_MODE==STA_MODE)//配置STA模式情况下,把默认配置SSID也存储起来, 以后即使保存过其他SSID,也不会覆盖丢失,使用连接最优信号SSID策略的情况下可以匹配连接
#define INIT_CONNECT_BEST_SSID 1 //配置如果啟動WIFI后在STA模式下, 是否挑选连接记忆过的信号最优WIFI

#define WIFI_FORCE_DEFAULT_MODE  FORCE_DEFAULT_MODE
#define WIFI_INIT_MODE  INIT_MODE
#define WIFI_INIT_SSID INIT_SSID
#define WIFI_INIT_PWD INIT_PWD
#define WIFI_INIT_CONNECT_BEST_SSID  INIT_CONNECT_BEST_SSID
#define WIFI_INIT_STORED_SSID  INIT_STORED_SSID

void wifi_return_sta_mode(void)
{
    if (!wifi_is_on()) {
        return;
    }
    int ret;
    struct wifi_mode_info info;
    info.mode = STA_MODE;
    ret = wifi_get_mode_stored_info(&info);
    if (ret) {//如果没保存过SSID
        info.ssid = WIFI_INIT_SSID;
        info.pwd = WIFI_INIT_PWD;
    }
    wifi_clear_scan_result(); //根据扫描结果连接信号最优ssid之前先清除之前结果,防止之前最优信号的ssid已下线
    wifi_set_sta_connect_best_ssid(1);
    __this->save_ssid_flag = 0;
    wifi_enter_sta_mode(info.ssid, info.pwd);
}

static int wifi_is_in_smp_cfg_mode(void)
{
    struct wifi_mode_info info;
    info.mode = NONE_MODE;
    wifi_get_mode_cur_info(&info);
    return info.mode == SMP_CFG_MODE;
}

static void wifi_sta_save_ssid(void)
{
    if (__this->save_ssid_flag) {
        __this->save_ssid_flag = 0;

        struct wifi_mode_info info;
        info.mode = STA_MODE;
        wifi_get_mode_cur_info(&info);
        wifi_store_mode_info(STA_MODE, info.ssid, info.pwd);
    }
}

#ifdef CONFIG_ELINK_QLINK_NET_CFG
extern u8 elink_qlink_is_start(void);
extern u8 elink_qlink_is_start_reset(void);
static u8 elink_find_hide_ssid = 0;

u8 is_elink_find_hid_ssid(void)
{
    return elink_find_hide_ssid;
}

u8 is_elink_find_hid_ssid_reset(void)
{
    elink_find_hide_ssid = 0;
    return elink_find_hide_ssid;
}

void elink_qlink_timeout(void)
{
    struct sys_event net;
    net.arg = "net";
    net.event = NET_EVENT_SMP_CFG_TIMEOUT;
    net_event_notify(NET_EVENT_FROM_USER, &net);
}

int is_chinaNet_qlink(void)
{
    struct cfg_info info = {0};
    if (elink_qlink_is_start()) {
        struct wifi_mode_info info;
        info.mode = STA_MODE;
        wifi_get_mode_cur_info(&info);
        if (0 == strcmp("ChinaNet-QLINK", info.ssid)) {
            elink_find_hide_ssid = 1;
            elink_qlink_is_start_reset();
            extern int elink_qlink_task(void);
            elink_qlink_task();
            return 0;
        }
    }
    return -1;
}
#endif


static int wifi_event_callback(void *network_ctx, enum WIFI_EVENT event)
{
    int ret = 0;
    struct net_event net = {0};

    switch (event) {

    case WIFI_EVENT_MODULE_INIT:

        struct wifi_store_info wifi_default_mode_parm;
        memset(&wifi_default_mode_parm, 0, sizeof(struct wifi_store_info));

        wifi_default_mode_parm.mode = WIFI_INIT_MODE;
        if (wifi_default_mode_parm.mode == AP_MODE || wifi_default_mode_parm.mode == STA_MODE) {
            strncpy((char *)wifi_default_mode_parm.ssid[wifi_default_mode_parm.mode - STA_MODE], (const char *)WIFI_INIT_SSID, sizeof(wifi_default_mode_parm.ssid[wifi_default_mode_parm.mode - STA_MODE]) - 1);
            strncpy((char *)wifi_default_mode_parm.pwd[wifi_default_mode_parm.mode - STA_MODE], (const char *)WIFI_INIT_PWD, sizeof(wifi_default_mode_parm.pwd[wifi_default_mode_parm.mode - STA_MODE]) - 1);
        }
        wifi_default_mode_parm.connect_best_network = WIFI_INIT_CONNECT_BEST_SSID;
        wifi_set_default_mode(&wifi_default_mode_parm, WIFI_FORCE_DEFAULT_MODE, WIFI_INIT_STORED_SSID);

#ifdef CONFIG_WIFIBOX_ENABLE
        wifi_freq_adjust();
#endif
        break;

    case WIFI_EVENT_MODULE_START:
        puts("|network_user_callback->WIFI_EVENT_MODULE_START\n");

#if (defined(CONFIG_WIFI_ENABLE) && defined(WB_ASSIST_ENABLE))
        int wifibox_client_init(void);
        wifibox_client_init();
#endif

#ifndef CONFIG_MASS_PRODUCTION_ENABLE
        struct wifi_mode_info info;
        info.mode = NONE_MODE;
        wifi_get_mode_cur_info(&info);
        if (info.mode == SMP_CFG_MODE) {
            net.arg = "net";
            net.event = NET_EVENT_SMP_CFG_FIRST;
            net_event_notify(NET_EVENT_FROM_USER, &net);
        }
#endif
#if 1
        u32  tx_rate_control_tab = // 不需要哪个速率就删除掉,可以动态设定
            0
            | BIT(0) //0:CCK 1M
            | BIT(1) //1:CCK 2M
            | BIT(2) //2:CCK 5.5M
            | BIT(3) //3:OFDM 6M
            | BIT(4) //4:MCS0/7.2M
            | BIT(5) //5:OFDM 9M
            | BIT(6) //6:CCK 11M
            | BIT(7) //7:OFDM 12M
            | BIT(8) //8:MCS1/14.4M
            | BIT(9) //9:OFDM 18M
            | BIT(10) //10:MCS2/21.7M
            | BIT(11) //11:OFDM 24M
            | BIT(12) //12:MCS3/28.9M
            | BIT(13) //13:OFDM 36M
            | BIT(14) //14:MCS4/43.3M
            | BIT(15) //15:OFDM 48M
            | BIT(16) //16:OFDM 54M
            | BIT(17) //17:MCS5/57.8M
            | BIT(18) //18:MCS6/65.0M
            | BIT(19) //19:MCS7/72.2M
            ;
        wifi_set_tx_rate_control_tab(tx_rate_control_tab);
#endif
        break;

    case WIFI_EVENT_MODULE_STOP:
        puts("|network_user_callback->WIFI_EVENT_MODULE_STOP\n");
        break;

    case WIFI_EVENT_AP_START:
        puts("|network_user_callback->WIFI_EVENT_AP_START\n");
        break;

    case WIFI_EVENT_AP_STOP:
        puts("|network_user_callback->WIFI_EVENT_AP_STOP\n");
        break;

    case WIFI_EVENT_STA_START:
        puts("|network_user_callback->WIFI_EVENT_STA_START\n");
        break;

    case WIFI_EVENT_MODULE_START_ERR:
        puts("|network_user_callback->WIFI_EVENT_MODULE_START_ERR\n");
        break;

    case WIFI_EVENT_STA_STOP:
        puts("|network_user_callback->WIFI_EVENT_STA_STOP\n");
        break;

    case WIFI_EVENT_STA_SCAN_COMPLETED:
        puts("|network_user_callback->WIFI_STA_SCAN_COMPLETED\n");
#ifdef CONFIG_AIRKISS_NET_CFG
        airkiss_ssid_check();
#endif
        break;

    case WIFI_EVENT_STA_CONNECT_SUCC:
        printf("|network_user_callback->WIFI_STA_CONNECT_SUCC,CH=%d\r\n", wifi_get_channel());

        /*wifi_rxfilter_cfg(3);    //过滤not_my_bssid,如果需要使用扫描空中SSID就不要过滤*/

        if (wifi_force_set_lan_setting_info()) {
            ret = 1;
        } else {
#ifdef CONFIG_STATIC_IPADDR_ENABLE
            if (0 == compare_dhcp_ipaddr()) {
                wifi_set_lan_setting_info();
                ret = 1;
            }
#endif
        }
        break;

    case WIFI_EVENT_MP_TEST_START:
        puts("|network_user_callback->WIFI_EVENT_MP_TEST_START\n");
        break;

    case WIFI_EVENT_MP_TEST_STOP:
        puts("|network_user_callback->WIFI_EVENT_MP_TEST_STOP\n");
        break;

    case WIFI_EVENT_STA_CONNECT_TIMEOUT_NOT_FOUND_SSID:
        puts("|network_user_callback->WIFI_STA_CONNECT_TIMEOUT_NOT_FOUND_SSID\n");
        net.arg = "net";
        net.event = NET_CONNECT_TIMEOUT_NOT_FOUND_SSID;
        net_event_notify(NET_EVENT_FROM_USER, &net);
        break;

    case WIFI_EVENT_STA_CONNECT_TIMEOUT_ASSOCIAT_FAIL:
        puts("|network_user_callback->WIFI_STA_CONNECT_TIMEOUT_ASSOCIAT_FAIL\n");
        net.arg = "net";
        net.event = NET_CONNECT_TIMEOUT_ASSOCIAT_FAIL;
        net_event_notify(NET_EVENT_FROM_USER, &net);
        break;

    case WIFI_EVENT_STA_NETWORK_STACK_DHCP_SUCC:
        puts("|network_user_callback->WIFI_EVENT_STA_NETWPRK_STACK_DHCP_SUCC\n");

        //有些使用了加密的路由器刚获取IP地址后前几个包都会没响应，怀疑路由器没转发成功
        void connect_broadcast(void);
        connect_broadcast();

        wifi_sta_save_ssid();

#ifdef CONFIG_ASSIGN_MACADDR_ENABLE
        if (!is_server_assign_macaddr_ok()) { //如果使用服务器分配MAC地址的情况,需要确认MAC地址有效才发送连接成功事件到APP层,否则先访问服务器分配mac地址
            server_assign_macaddr(wifi_return_sta_mode);
            break;
        }
        __this->mac_addr_succ_flag = 1;
#endif

#ifdef CONFIG_MASS_PRODUCTION_ENABLE
        void network_mssdp_init(void);
        network_mssdp_init();
#endif
#ifdef CONFIG_STATIC_IPADDR_ENABLE
        store_dhcp_ipaddr();
#endif

#ifdef CONFIG_ELINK_QLINK_NET_CFG
        int ret = is_chinaNet_qlink();
        if (ret == 0) {
            break;
        }
#endif
        wifi_set_sta_connect_best_ssid(0);
        __this->request_connect_flag = 0;

        net.arg = "net";
        net.event = NET_EVENT_CONNECTED;
        net_event_notify(NET_EVENT_FROM_USER, &net);
        break;

    case WIFI_EVENT_STA_DISCONNECT:
        puts("|network_user_callback->WIFI_STA_DISCONNECT\n");

        /*wifi_rxfilter_cfg(0);*/

#ifdef CONFIG_ASSIGN_MACADDR_ENABLE
        if (!__this->mac_addr_succ_flag) {
            break;
        }
#endif
        net.arg = "net";
        net.event = NET_EVENT_DISCONNECTED;
        net_event_notify(NET_EVENT_FROM_USER, &net);

        if (!wifi_is_in_smp_cfg_mode() && !__this->request_connect_flag) {
            memset(&net, 0, sizeof(net));
            net.arg = "net";
            net.event = NET_EVENT_DISCONNECTED_AND_REQ_CONNECT;
            net_event_notify(NET_EVENT_FROM_USER, &net);
        }
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
        net.arg = "net";
        net.event = NET_EVENT_SMP_CFG_TIMEOUT;
        net_event_notify(NET_EVENT_FROM_USER, &net);
        break;

    case WIFI_EVENT_SMP_CFG_COMPLETED:
        puts("|network_user_callback->WIFI_EVENT_SMP_CFG_COMPLETED\n");
        net.arg = "net";
        net.event = NET_SMP_CFG_COMPLETED;
        net_event_notify(NET_EVENT_FROM_USER, &net);
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
        printf("WIFI_EVENT_AP_ON_DISCONNECTED hwaddr = %02x:%02x:%02x:%02x:%02x:%02x \r\n\r\n",
               hwaddr->addr[0], hwaddr->addr[1], hwaddr->addr[2], hwaddr->addr[3], hwaddr->addr[4], hwaddr->addr[5]);
        break;

    default:
        break;
    }

    return ret;
}

static void net_app_init(void)
{
    if (__this->net_app_init_flag) {
        return;
    }
    __this->net_app_init_flag = 1;

#ifdef CONFIG_VIDEO_ENABLE
#ifdef CONFIG_MASS_PRODUCTION_ENABLE
    extern char get_MassProduction(void);
    if (get_MassProduction()) {
        wifi_enter_sta_mode(STA_WIFI_SSID, STA_WIFI_PWD);
        /*
         **代码段功能:修改RTSP的URL
         **默认配置  :URL为rtsp://192.168.1.1/avi_pcm_rt/front.sd,//(avi_pcma_rt 传G7111音频)传JPEG实时流
         **
         **/
        const char *user_custom_name = "avi_pcma_rt";
        const char *user_custom_content =
            "stream\r\n \
            file_ext_name avi\r\n \
            media_source live\r\n \
            priority 1\r\n \
            payload_type 26\r\n \
            clock_rate 90000\r\n \
            encoding_name JPEG\r\n \
            coding_type frame\r\n \
            byte_per_pckt 1458\r\n \
            stream_end\r\n \
            stream\r\n \
            file_ext_name pcm\r\n \
            media_source live\r\n \
            priority 1\r\n \
            payload_type 8\r\n \
            encoding_name PCMA\r\n \
            clock_rate 8000\r\n \
            stream_end";
        extern void rtsp_modify_url(const char *user_custom_name, const char *user_custom_content);
        rtsp_modify_url(user_custom_name, user_custom_content);

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

        printf("network mssdp init\n");
        /*void network_mssdp_init(void);*/
        /*network_mssdp_init();*/
    }
#endif
#endif

#ifdef CONFIG_IPERF_ENABLE
//网络测试工具，使用iperf
    extern void iperf_test(void);
    iperf_test();
#endif

#if defined CONFIG_ALI_SDK_ENABLE && defined CONFIG_ALL_ADC_CHANNEL_OPEN_ENABLE
    extern void set_ag_rec_sample_source(const char *sample_source);
    extern void set_ag_rec_channel_bit_map(u8 channel_bit_map);
    set_ag_rec_sample_source(CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_PLNK0 ? "plnk0" : "mic");
    set_ag_rec_channel_bit_map(BIT(CONFIG_AISP_MIC0_ADC_CHANNEL));
#endif

#if defined CONFIG_TVS_SDK_ENABLE && defined CONFIG_ALL_ADC_CHANNEL_OPEN_ENABLE
    extern void set_tvs_rec_sample_source(const char *sample_source);
    extern void set_tvs_rec_channel_bit_map(u8 channel_bit_map);
    set_tvs_rec_sample_source(CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_PLNK0 ? "plnk0" : "mic");
    set_tvs_rec_channel_bit_map(BIT(CONFIG_AISP_MIC0_ADC_CHANNEL));
#endif
}

static void net_app_uninit(void)
{
}

static void wifi_and_network_on(void)
{
    if (wifi_is_on()) {
        return;
    }
    wifi_on();
    net_app_init();
}

void wifi_and_network_off(void)
{
    if (wifi_is_on() == 0) {
        return;
    }
    net_app_uninit();

    wifi_off();
}

void wifi_sta_connect(char *ssid, char *pwd, char save, void *priv)
{
    if (save) {
        struct wifi_mode_info info;
        info.mode = STA_MODE;
        wifi_get_mode_cur_info(&info);
        if (strcmp(info.ssid, ssid) || strcmp(info.pwd, pwd)) {
            __this->save_ssid_flag = 1;    //防止输错密码还保存ssid, 等待确保连接上路由器再保存
        }
    }

    __this->request_connect_flag = 1;
    if (save) {
        /* wifi_store_mode_info(STA_MODE,ssid,pwd); */
    }

    wifi_enter_sta_mode(ssid, pwd);
#if defined CONFIG_BT_ENABLE
    //卸载wifi驱动后需要重新设置一下共存参数才能生效
    u8 get_rf_coexistence_config_index(void);
    void switch_rf_coexistence_config_table(u8 index);
    switch_rf_coexistence_config_table(get_rf_coexistence_config_index());
#endif
}

int get_wifi_scan_ssid_info(struct wifi_scan_ssid_info **ssid_info)
{
    struct wifi_scan_ssid_info *sta_ssid_info;
    u32 sta_ssid_num = 0;
    sta_ssid_info = wifi_get_scan_result(&sta_ssid_num);
    wifi_clear_scan_result();

    for (u32 i = 0; i < sta_ssid_num; i++) {
        printf("find scan ssid = [%s]\r\n", sta_ssid_info[i].ssid);
    }

    *ssid_info = sta_ssid_info;
    return sta_ssid_num;
}

static void wifi_app_task(void *priv)  //主要是create wifi 线程的
{
    wifi_set_store_ssid_cnt(NETWORK_SSID_INFO_CNT);

#ifdef CONFIG_DUER_SDK_ENABLE
    u8 airkiss_aes_key[16] = {
        0x65, 0x31, 0x63, 0x33, 0x36, 0x31, 0x63, 0x63,
        0x32, 0x39, 0x65, 0x34, 0x33, 0x66, 0x62, 0x38
    };
    wifi_set_airkiss_key(airkiss_aes_key);
#endif

#if 0
    wifi_set_smp_cfg_scan_all_channel(1);
    wifi_set_smp_cfg_airkiss_recv_ssid(1);
#endif

#ifdef CONFIG_WIFIBOX_ENABLE
    set_wbcp_mode(WB_CLIENT_MODE);
    wifi_set_frame_cb(wbcp_rx_frame_cb);
#endif

    wifi_set_sta_connect_timeout(CONNECT_TIMEOUT_SEC);

    wifi_set_event_callback(wifi_event_callback);
    wifi_and_network_on();

#if defined CONFIG_BT_ENABLE
    extern void bt_ble_module_init(void);
    bt_ble_module_init();
#endif
}

#ifdef CONFIG_WIFI_ENABLE
static int wireless_net_init(void)   //主要是create wifi 线程的
{
    puts("wireless_net_init \n\n");
    return thread_fork("wifi_app_task", 10, 1792, 0, 0, wifi_app_task, NULL);
}
late_initcall(wireless_net_init);
#endif

void lwip_netflow(int in_out, int proto_type)//inout:0->in,1->out;proto_type: IP_PROTO_UDP IP_PROTO_TCP
{
    /*if(in_out==1||proto_type==IP_PROTO_UDP|| proto_type==IP_PROTO_TCP)*/
}

#include "asm/power_interface.h"

#if TCFG_LOWPOWER_LOWPOWER_SEL == 0
void wf_low_power_request(void *priv, u32 usec)
{

}
#endif

