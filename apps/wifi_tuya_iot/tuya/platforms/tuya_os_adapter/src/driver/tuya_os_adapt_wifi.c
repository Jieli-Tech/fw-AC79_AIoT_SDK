/**
 * @file tuya_os_adapt_wifi.c
 * @brief wifi操作接口
 *
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 *
 */
#include "device/device.h"
#include "wifi/wifi_connect.h"
#include "printf.h"
#include "lwip.h"
#include "system/includes.h"
#include "lwip.h"
#include "dhcp_srv/dhcp_srv.h"
#include "event/net_event.h"


#include "tuya_os_adapt_wifi.h"
#include "tuya_os_adapter.h"
#include "tuya_os_adapter_error_code.h"
#include "tuya_os_adapt_memory.h"

#define SCAN_MAX_AP 64
static char save_ssid_flag, request_connect_flag;
static WIFI_REV_MGNT_CB mgnt_recv_cb = NULL;
static SNIFFER_CALLBACK sniffer_cb = NULL;
static char wifi_frame_cb_switch = 0;

static WF_WK_MD_E wf_mode = WWM_LOWPOWER; //WWM_LOWPOWER
//static WF_WK_MD_E wf_mode = WWM_STATION; //WWM_LOWPOWER

extern void get_gateway(u8_t lwip_netif, char *ipaddr);


/***********************************************************
*************************micro define***********************
***********************************************************/
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

typedef struct GNU_PACKED {
    /* Word 0 */
    u32  WirelessCliID: 8;
    u32  KeyIndex: 2;
    u32  BSSID: 3;
    u32  UDF: 3;
    u32  MPDUtotalByteCount: 12;
    u32  TID: 4;
    /* Word 1 */
    u32  FRAG: 4;
    u32  SEQUENCE: 12;
    u32  MCS: 7;
    u32  BW: 1;
    u32  ShortGI: 1;
    u32  STBC: 2;
    u32  rsv: 3;
    u32  PHYMODE: 2;             /* 1: this RX frame is unicast to me */
    /*Word2 */
    u32  RSSI0: 8;
    u32  RSSI1: 8;
    u32  RSSI2: 8;
    u32  rsv1: 8;
    /*Word3 */
    u32  SNR0: 8;
    u32  SNR1: 8;
    u32  FOFFSET: 8;
    u32  rsv2: 8;
    /*UINT32  rsv2:16;*/
} RXWI_STRUC, *PRXWI_STRUC;

//接收回调
static void wifi_rx_cb(void *rxwi, struct ieee80211_frame *wh, void *data, void *reserve)
{
    char *str_frm_type;
    switch (wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) {
    case IEEE80211_FC0_TYPE_MGT:
        switch (wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) {
        case IEEE80211_FC_STYPE_ASSOC_REQ:
            str_frm_type = "association req";
            break;
        case IEEE80211_FC_STYPE_ASSOC_RESP:
            str_frm_type = "association resp";
            break;
        case IEEE80211_FC_STYPE_REASSOC_REQ:
            str_frm_type = "reassociation req";
            break;
        case IEEE80211_FC_STYPE_REASSOC_RESP:
            str_frm_type = "reassociation resp";
            break;
        case IEEE80211_FC_STYPE_PROBE_REQ:
            str_frm_type = "probe req";
            break;
        case IEEE80211_FC_STYPE_PROBE_RESP:
            str_frm_type = "probe resp";
            break;
        case IEEE80211_FC_STYPE_BEACON:
            str_frm_type = "beacon";
            break;
        case IEEE80211_FC_STYPE_ATIM:
            str_frm_type = "atim";
            break;
        case IEEE80211_FC_STYPE_DISASSOC:
            str_frm_type = "disassociation";
            break;
        case IEEE80211_FC_STYPE_AUTH:
            str_frm_type = "authentication";
            break;
        case IEEE80211_FC_STYPE_DEAUTH:
            str_frm_type = "deauthentication";
            break;
        case IEEE80211_FC_STYPE_ACTION:
            str_frm_type = "action";
            break;
        default:
            str_frm_type = "unknown mgmt";
            break;
        }

        if (mgnt_recv_cb != NULL) {
            puts("wifi_rx_cb : mgnt_recv_cb\n");
            mgnt_recv_cb((void *)data, (int)reserve);
        }

        break;
    case IEEE80211_FC0_TYPE_CTL:
        str_frm_type = "control";
        break;
    case IEEE80211_FC0_TYPE_DATA:
        str_frm_type = "data";
        break;
    default:
        str_frm_type = "unknown";
        break;

    }

    printf("wifi recv:%s\n", str_frm_type);

    if (sniffer_cb != NULL) {
        PRXWI_STRUC pRx = (PRXWI_STRUC)rxwi;
        sniffer_cb(data, (int)reserve, pRx->RSSI0);
    }
}

extern void wifi_return_sta_mode(void);

/***********************************************************
*************************variable define********************
***********************************************************/
static const TUYA_OS_WIFI_INTF m_tuya_os_wifi_intfs = {
    .all_ap_scan                  = tuya_os_adapt_wifi_all_ap_scan,
    .assign_ap_scan               = tuya_os_adapt_wifi_assign_ap_scan,
    .release_ap                   = tuya_os_adapt_wifi_release_ap,
    .set_cur_channel              = tuya_os_adapt_wifi_set_cur_channel,
    .get_cur_channel              = tuya_os_adapt_wifi_get_cur_channel,
    .sniffer_set                  = tuya_os_adapt_wifi_sniffer_set,
    .get_ip                       = tuya_os_adapt_wifi_get_ip,
    .set_mac                      = tuya_os_adapt_wifi_set_mac,
    .get_mac                      = tuya_os_adapt_wifi_get_mac,
    .set_work_mode                = tuya_os_adapt_wifi_set_work_mode,
    .get_work_mode                = tuya_os_adapt_wifi_get_work_mode,
    .ap_start                     = tuya_os_adapt_wifi_ap_start,
    .ap_stop                      = tuya_os_adapt_wifi_ap_stop,
    .get_connected_ap_info_v2     = NULL,
    .fast_station_connect_v2      = NULL,
    .station_connect              = tuya_os_adapt_wifi_station_connect,
    .station_disconnect           = tuya_os_adapt_wifi_station_disconnect,
    .station_get_conn_ap_rssi     = tuya_os_adapt_wifi_station_get_conn_ap_rssi,
    .get_bssid                    = tuya_os_adapt_wifi_get_bssid,
    .station_get_status           = tuya_os_adapt_wifi_station_get_status,
    .set_country_code             = tuya_os_adapt_wifi_set_country_code,
    .send_mgnt                    = tuya_os_adapt_wifi_send_mgnt,
    .register_recv_mgnt_callback  = tuya_os_adapt_wifi_register_recv_mgnt_callback,
    .set_lp_mode                  = tuya_os_adapt_set_wifi_lp_mode,
    .rf_calibrated				  = tuya_os_adapt_wifi_rf_calibrated,
    //.err_status_get               = tuya_os_adapt_wifi_err_status_get,
};


/***********************************************************
*************************function define********************
***********************************************************/
int tuya_os_adapt_wifi_all_ap_scan(AP_IF_S **ap_ary, unsigned int *num)
{
    int scan_cnt = 0;
    int timeout = 0 ;
    AP_IF_S *item;
    AP_IF_S *array = NULL;
    struct wifi_scan_ssid_info *aplist = NULL;
    int i;

    if ((NULL == ap_ary) || (NULL == num)) {
        printf("all_ap_scan argerr\n");
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    wifi_scan_req();

    msleep(3 * 1000);

    aplist = wifi_get_scan_result(&scan_cnt);

    if (aplist && scan_cnt) {
        if (scan_cnt > SCAN_MAX_AP) {
            scan_cnt = SCAN_MAX_AP;
        }

        array = (AP_IF_S *)tuya_os_adapt_system_malloc(SIZEOF(AP_IF_S) * scan_cnt);
        if (NULL == array) {
            tuya_os_adapt_system_free(aplist);
            aplist = NULL;
            printf("all_ap_scan:malloc faile\n");
            return OPRT_OS_ADAPTER_COM_ERROR;
        }

        memset(array, 0, SIZEOF(AP_IF_S) * scan_cnt);

        for (i = 0; i < scan_cnt; i++) {
            item = &array[i];

            item->channel = aplist[i].channel_number;
            item->rssi = aplist[i].rssi;

            memcpy(item->bssid, aplist[i].mac_addr, 6);
            strncpy((char *)item->ssid, aplist[i].ssid, aplist[i].ssid_len);
            item->s_len = aplist[i].ssid_len;

            /*************************调试信息**************************/
            printf("ssid : %s\n", item->ssid);
            printf("channel : %d\n", item->channel);
            printf("rssi: %d\n", item->rssi);
            printf("baaid : \n");
            put_buf(item->bssid, 6);
            /*************************调试信息**************************/
        }

        *ap_ary = array;
        *num = scan_cnt & 0xff;

        tuya_os_adapt_system_free(aplist);
        printf("all_ap_scan ok :%d\n", *num);

        return  OPRT_OS_ADAPTER_OK;
    }

    printf("all_ap_scan fail %d %d\n", aplist, scan_cnt);

    return OPRT_OS_ADAPTER_COM_ERROR;
}

int tuya_os_adapt_wifi_assign_ap_scan(const signed char *ssid, AP_IF_S **ap)
{
    int scan_cnt;
    AP_IF_S *array = NULL;
    struct wifi_scan_ssid_info *aplist = NULL;
    int ret = OPRT_INVALID_PARM;
    int i, j = 0;

    if ((NULL == ssid) || (NULL == ap)) {
        return OPRT_INVALID_PARM;
    }

    wifi_scan_req();
    vTaskDelay(3 * 100);

    aplist = wifi_get_scan_result(&scan_cnt);
    if (NULL == aplist) {
        return OPRT_INVALID_PARM;
    }

    array = (AP_IF_S *)tuya_os_adapt_system_malloc(SIZEOF(AP_IF_S));
    if (NULL == array) {
        tuya_os_adapt_system_free(aplist);
        aplist = NULL;
        printf("wifi_assign_ap_scan:malloc faile\n");
        return OPRT_OS_ADAPTER_COM_ERROR;
    }

    memset(array, 0, sizeof(AP_IF_S));
    array->rssi = -100;

    for (i = 0; i < scan_cnt; i++) {
        if (strcmp(aplist[i].ssid, ssid)) {
            continue;
        }

        if (aplist[i].rssi < array->rssi) {
            continue;
        }

        array->channel = aplist[i].channel_number; //信道
        array->rssi = aplist[i].rssi;              //rssi强度

        memcpy(array->bssid, aplist[i].mac_addr, 6); //bssid
        strncpy((char *)array->ssid, aplist[i].ssid, aplist[i].ssid_len); //ssid
        array->s_len = aplist[i].ssid_len; //ssid长度

        j++;
    }

    if (j == 0) {
        goto SCAN_ERR;
    }

    *ap = array;
    if (aplist != NULL) {
        tuya_os_adapt_system_free(aplist);
    }

    return OPRT_OS_ADAPTER_OK;

SCAN_ERR:
    if (aplist != NULL) {
        tuya_os_adapt_system_free(aplist);
        aplist = NULL;
    }

    if (array) {
        tuya_os_adapt_system_free(array);
        array = NULL;
    }

    return OPRT_OS_ADAPTER_COM_ERROR;
}

/**
 * @brief release the memory malloced in <tuya_os_adapt_wifi_all_ap_scan>
 *        and <tuya_os_adapt_wifi_assign_ap_scan> if needed. tuya-sdk
 *        will call this function when the ap info is no use.
 *
 * @param[in]       ap          the ap info
 * @return  OPRT_OS_ADAPTER_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_release_ap(AP_IF_S *ap)
{
    if (NULL == ap) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    tuya_os_adapt_system_free(ap);
    ap = NULL;

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief set wifi interface work channel
 *
 * @param[in]       chan        the channel to set
 * @return  OPRT_OK: success  Other: fail
 */
static int set_channel = 1;
int tuya_os_adapt_wifi_set_cur_channel(const unsigned char chan)
{
    /* printf("%s", os_current_task()); */
    if ((chan > 14) || (chan < 1)) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    printf("adapt_set_wifi_channel:%d\n", chan);
    /* wifi_set_channel(chan); */
    set_channel = chan;
    /* os_time_dly(20); */

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief get wifi interface work channel
 *
 * @param[out]      chan        the channel wifi works
 * @return  OPRT_OS_ADAPTER_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_get_cur_channel(unsigned char *chan)
{
    *chan = (unsigned char)wifi_get_channel();
    printf("get current channel :%d\n", *chan);
    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief get wifi ip info.when wifi works in
 *        ap+station mode, wifi has two ips.
 *
 * @param[in]       wf          wifi function type
 * @param[out]      ip          the ip addr info
 * @return  OPRT_OS_ADAPTER_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_get_ip(const WF_IF_E wf, NW_IP_S *ip)
{
    char ipaddr[16] = {0};
    char gateway[16] = {0};

    Get_IPAddress(1, ipaddr);
    get_gateway(1, gateway);

    strcpy(ip->ip, ipaddr);
    strcpy(ip->mask, "255.255.255.0");
    strcpy(ip->gw, gateway);

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief set wifi mac info.when wifi works in
 *        ap+station mode, wifi has two macs.
 *
 * @param[in]       wf          wifi function type
 * @param[in]       mac         the mac info
 * @return  OPRT_OS_ADAPTER_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_set_mac(const WF_IF_E wf, const NW_MAC_S *mac)
{
    printf("adapt_set_wifi_mac:0x%x\n", mac->mac[0]);

    wifi_set_mac((char *)mac);

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief get wifi mac info.when wifi works in
 *        ap+station mode, wifi has two macs.
 *
 * @param[in]       wf          wifi function type
 * @param[out]      mac         the mac info
 * @return  OPRT_OS_ADAPTER_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_get_mac(const WF_IF_E wf, NW_MAC_S *mac)
{
    puts("tuya_os_adapt_wifi_get_mac");
    u8 mac_t[6];

    wifi_get_mac(mac_t);

    memcpy(mac, mac_t, sizeof(mac_t));

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief set wifi work mode
 *
 * @param[in]       mode        wifi work mode
 * @return  OPRT_OS_ADAPTER_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_set_work_mode(const WF_WK_MD_E mode)
{
    OPERATE_RET ret = OPRT_OS_ADAPTER_OK;
    WF_WK_MD_E current_mode;

    ret = tuya_os_adapt_wifi_get_work_mode(&current_mode);
    if ((OPRT_OS_ADAPTER_OK == ret) && (current_mode != mode)) {
    }

    switch (mode) {
    case WWM_LOWPOWER :
        puts("adapt_wifi_set_work_mode[WWM_LOWPOWER]\n");
        break;

    case WWM_SNIFFER :
        puts("adapt_wifi_set_work_mode[WWM_SNIFFER]\n");
        break;

    case WWM_STATION :
        puts("adapt_wifi_set_work_mode[WWM_STATION]\n");
        break;

    case WWM_SOFTAP :
        puts("adapt_wifi_set_work_mode[WWM_SOFTAP]\n");
        break;

    case WWM_STATIONAP :
        puts("adapt_wifi_set_work_mode[WWM_STATIONAP]\n");
        break;

    default:
        puts("adapt_wifi_set_work_mode[default]\n");
        break;
    }

    wf_mode = mode;

    return OPRT_OS_ADAPTER_OK;
}


/**
 * @brief get wifi work mode
 *
 * @param[out]      mode        wifi work mode
 * @return  OPRT_OS_ADAPTER_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_get_work_mode(WF_WK_MD_E *mode)
{
    *mode = wf_mode;
    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief enable / disable wifi sniffer mode.
 *        if wifi sniffer mode is enabled, wifi recv from
 *        packages from the air, and user shoud send these
 *        packages to tuya-sdk with callback <cb>.
 *
 * @param[in]       en          enable or disable
 * @param[in]       cb          notify callback
 * @return  OPRT_OS_ADAPTER_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_sniffer_set(const bool en, const SNIFFER_CALLBACK cb)
{
    printf("sniffer_set: %d \n", en);
    WF_WK_MD_E mode;
    if (en) {
        mode = WWM_LOWPOWER;
        tuya_os_adapt_wifi_get_work_mode(&mode);
        if ((mode == WWM_SOFTAP) || (mode == WWM_STATIONAP)) {
        } else {
        }

        //sniffer_cb = cb;
        //wifi_set_frame_cb(wifi_rx_cb);  //注册接收802.11数据帧回调
    } else {
        wifi_set_frame_cb(NULL);
        sniffer_cb = NULL;

        //fix me
        //重新进入ap配网时，需要切换回原来的信道，否则连接不上。
        //wifi_set_channel(11);
        //os_time_dly(3);
    }

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief start a soft ap
 *
 * @param[in]       cfg         the soft ap config
 * @return  OPRT_OS_ADAPTER_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_ap_start(const WF_AP_CFG_IF_S *cfg)
{
    printf("adapt_wifi_set:%s\n", cfg->ssid);

    //设置ap ip gw mask
    wifi_set_lan_setting_info();

    wifi_enter_ap_mode((char *)cfg->ssid, (char *)cfg->passwd);

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief stop a soft ap
 *
 * @return  OPRT_OS_ADAPTER_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_ap_stop(void)
{
    puts("tuya_os_adapt_wifi_ap_stop");
    wifi_return_sta_mode();
    return OPRT_OS_ADAPTER_OK;
}


/**
 * @brief : get ap info for fast connect
 * @param[out]      fast_ap_info
 * @return  OPRT_OS_ADAPTER_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_get_connected_ap_info_v2(FAST_WF_CONNECTED_AP_INFO_V2_S **fast_ap_info)
{
    puts("tuya_os_adapt_wifi_get_connected_ap_info_v2 : Not supported yet!\n");

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief : fast connect
 * @param[in]      fast_ap_info
 * @return  OPRT_OS_ADAPTER_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_fast_station_connect_v2(const FAST_WF_CONNECTED_AP_INFO_V2_S *fast_ap_info)
{
    puts("tuya_os_adapt_wifi_fast_station_connect_v2 : Not supported yet!\n");

    return OPRT_OS_ADAPTER_OK;
}
//#endif

/**
 * @brief connect wifi with ssid and passwd
 *
 * @param[in]       ssid
 * @param[in]       passwd
 * @return  OPRT_OS_ADAPTER_OK: success  Other: fail
 */
// only support wap/wap2
int tuya_os_adapt_wifi_station_connect(const signed char *ssid, const signed char *passwd)
{
    printf("wifi connecting (ssid : %s, passwd : %s)\n", ssid, passwd);
    if ((NULL == ssid) || (NULL == passwd)) {
        printf("tuya_os_adapt_wifi_station_connect : [INVALID PARM]\n");
        return -1;
    }

    printf("adapt_wifi_station_connect :%s\n", ssid);

    wifi_enter_sta_mode(ssid, passwd);

    return OPRT_OS_ADAPTER_OK;
}

int tuya_os_adapt_wifi_station_disconnect(void)
{
    puts("tuya_os_adapt_wifi_station_disconnect : Not supported yet!\n");
    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief get wifi connect rssi
 *
 * @param[out]      rssi        the return rssi
 * @return  OPRT_OS_ADAPTER_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_station_get_conn_ap_rssi(signed char *rssi)
{
    *rssi = (signed char)wifi_get_rssi();
    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief get wifi bssid
 *
 * @param[out]      mac         uplink mac
 * @return  OPRT_OS_ADAPTER_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_get_bssid(unsigned char *mac)
{
    unsigned char bssid[6];
    wifi_get_bssid(bssid);
    memcpy(mac, bssid, 6);

    return OPRT_OS_ADAPTER_OK;
}


/**
 * @brief get wifi station work status
 *
 * @param[out]      stat        the wifi station work status
 * @return  OPRT_OS_ADAPTER_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_station_get_status(WF_STATION_STAT_E *stat)
{
    enum wifi_sta_connect_state state;

    state = wifi_get_sta_connect_state();
    switch (state) {
    case WIFI_STA_CONNECT_TIMEOUT_NOT_FOUND_SSID :
        *stat = WSS_NO_AP_FOUND;
        break;
    case WIFI_STA_CONNECT_SUCC :
        *stat = WSS_CONN_SUCCESS;
        break;
    case WIFI_STA_NETWORK_STACK_DHCP_SUCC :
        *stat = WSS_GOT_IP;
        break;
    case WIFI_STA_DISCONNECT :
    case WIFI_STA_CONNECT_TIMEOUT_ASSOCIAT_FAIL :
    case WIFI_STA_NETWORK_STACK_DHCP_TIMEOUT :
        *stat = WSS_CONN_FAIL;
        break;
    default :
        printf("tuya_os_adapt_wifi_station_get_status : [NOT DEFAULT]\n");
        break;
    }

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief set wifi country code
 *
 * @param[in]       ccode  country code
 * @return  OPRT_OS_ADAPTER_OK: success  Other: fail
 */
int tuya_os_adapt_wifi_set_country_code(const COUNTRY_CODE_E ccode)
{
    puts("tuya_os_adapt_wifi_set_country_code : Not supported yet!\n");
    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief set wifi lowerpower mode
 *
 * @param[in]		 en
 * @param[in]		 dtim
 * @return  OPRT_OS_ADAPTER_OK: success  Other: fail
 */
int tuya_os_adapt_set_wifi_lp_mode(const bool en, const unsigned char dtim)
{
    puts("tuya_os_adapt_set_wifi_lp_mode : Not supported yet!\n");
    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief send wifi management
 *
 * @param[in]       buf         pointer to buffer
 * @param[in]       len         length of buffer
 * @return  OPRT_OS_ADAPTER_OK: success  Other: fail
 */
//for test
unsigned char send_data[182] = {
    0x80, 0x00, 0x3A, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1C, 0x23, 0xD8, 0xF4, 0x4A, 0x45,
    0x1C, 0x23, 0xD8, 0xF4, 0x4A, 0x45, 0xB0, 0x01, 0xA0, 0x53, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x64, 0x00, 0x21, 0x00, 0x00, 0x0E, 0x53, 0x6D, 0x61, 0x72, 0x74, 0x4C, 0x69, 0x66, 0x65, 0x2D,
    0x34, 0x41, 0x34, 0x35, 0x01, 0x08, 0x82, 0x84, 0x8B, 0x96, 0x0C, 0x12, 0x18, 0x24, 0x03, 0x01,
    0x06, 0x05, 0x04, 0x00, 0x01, 0x00, 0x00, 0x2A, 0x01, 0x00, 0x32, 0x04, 0x30, 0x48, 0x60, 0x6C,
    0x2D, 0x1A, 0x20, 0x00, 0x01, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3D, 0x16, 0x05, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xDD, 0x18, 0x00, 0x50, 0xF2, 0x02, 0x01, 0x01, 0x00, 0x00, 0x03, 0xA4,
    0x00, 0x00, 0x27, 0xA4, 0x00, 0x00, 0x42, 0x43, 0x5E, 0x00, 0x62, 0x32, 0x2F, 0x00, 0xDD, 0x0E,
    0x00, 0x50, 0xF2, 0x04, 0x10, 0x4A, 0x00, 0x01, 0x20, 0x10, 0x44, 0x00, 0x01, 0x02, 0xDD, 0x06,
    0x00, 0xE0, 0x4C, 0x02, 0x01, 0x10,
};

__attribute__((aligned(4))) static u8 wifi_send_pkg[1564] =
{0xc6, 0x00, 0x00, 0x04, 0xB0, 0x00, 0x04, 0x80, 0x35, 0x01, 0xB6, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,};

int tuya_os_adapt_wifi_send_mgnt(const unsigned char *buf, const unsigned int len)
{
    printf("the set channel : %d\n", set_channel);
    /* wifi_set_channel(set_channel); */
    os_time_dly(30);

#if 0
    //put_buf(buf, len);
    u16 *PktLen = &wifi_send_pkg[0];
    u16 *MPDUtotalByteCount = &wifi_send_pkg[10];
    *PktLen = len + 20 + 4 - 8;
    *MPDUtotalByteCount = len;
    memcpy(&wifi_send_pkg[20], buf, len);
    wifi_send_data(wifi_send_pkg, len + 20 + 4, WIFI_TXRATE_11M);
    /* put_buf(wifi_send_pkg, len+20+4); */
#endif

#if 0
    u16 *PktLen = &wifi_send_pkg[0];
    u16 *MPDUtotalByteCount = &wifi_send_pkg[10];
    *PktLen = sizeof(send_data) + 20 + 4 - 8;
    *MPDUtotalByteCount = sizeof(send_data);
    memcpy(&wifi_send_pkg[20], send_data, sizeof(send_data));
    wifi_send_data(wifi_send_pkg, sizeof(send_data) + 20 + 4, WIFI_TXRATE_1M);
    /* put_buf(wifi_send_pkg, sizeof(send_data)+20+4); */
#endif

    return OPRT_OS_ADAPTER_OK;
}

int tuya_os_adapt_wifi_register_recv_mgnt_callback(const bool enable, const WIFI_REV_MGNT_CB recv_cb)
{
    puts("tuya_os_adapt_wifi_register_recv_mgnt_callback");
    printf("mgnt_set: %d \n", enable);
    if (enable) {
        WF_WK_MD_E mode;
        int ret = tuya_os_adapt_wifi_get_work_mode(&mode);
        if (OPRT_OS_ADAPTER_OK != ret) {
            return OPRT_OS_ADAPTER_COM_ERROR;
        }

        //|| (mode == WWM_SNIFFER)
        if ((mode == WWM_LOWPOWER)) {
            return OPRT_OS_ADAPTER_COM_ERROR;
        }

        //mgnt_recv_cb = recv_cb;
    } else {
        mgnt_recv_cb = NULL;
        if (sniffer_cb == NULL) {
            wifi_set_frame_cb(NULL);
        }
    }

    return OPRT_OS_ADAPTER_OK;
}

bool tuya_os_adapt_wifi_rf_calibrated(void)
{
    puts("tuya_os_adapt_wifi_rf_calibrated : Not supported yet!\n");
    return true;
}

int tuya_os_adapt_wifi_err_status_get(WF_STATION_STAT_E *stat)
{
    puts("tuya_os_adapt_wifi_err_status_get : Not supported yet!\n");
    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief tuya_os_adapt_reg_wifi_intf 接口注册
 * @return int
 */
int tuya_os_adapt_reg_wifi_intf(void)
{
    extern int wifi_is_on(void);
    while (!wifi_is_on()) {
        printf("WIFI NOT START UP, WAITING");
        os_time_dly(5);
    }

    return tuya_os_adapt_reg_intf(INTF_WIFI, (void *)&m_tuya_os_wifi_intfs);
}
