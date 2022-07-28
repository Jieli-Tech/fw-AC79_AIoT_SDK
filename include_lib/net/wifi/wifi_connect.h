#ifndef _WIFI_CONNECT_H_
#define _WIFI_CONNECT_H_

#include "generic/typedef.h"

///  \cond DO_NOT_DOCUMENT
enum WIFI_MODE {
    STA_MODE = 1,//STA_MODE位置必须为第一个
    AP_MODE,
    SMP_CFG_MODE,
    MP_TEST_MODE,
    P2P_MODE,
    NONE_MODE,
};

enum P2P_ROLE {
    P2P_GC_MODE = 1,
    P2P_GO_MODE,
};

struct wifi_store_info {
    enum WIFI_MODE mode;
    u8 pwd[2][64];
    u8 ssid[2][33];
    enum P2P_ROLE p2p_role;
    u8 sta_cnt;
    u8 	connect_best_network;
} __attribute__((packed));

struct wifi_stored_sta_info {
    u8 pwd[64];
    u8 ssid[33];
} __attribute__((packed));

struct wifi_scan_ssid_info {
    char ssid[32];
    unsigned int ssid_len;
    unsigned char mac_addr[6];
    char rssi;
    char snr;
    char rssi_db;
    char rssi_rsv;
    unsigned int channel_number;
    unsigned char	SignalStrength;//(in percentage)
    unsigned char	SignalQuality;//(in percentage)
    unsigned char   SupportedRates[16];
};

enum wifi_sta_connect_state {
    WIFI_STA_DISCONNECT,
    WIFI_STA_CONNECT_SUCC,
    WIFI_STA_CONNECT_TIMEOUT_NOT_FOUND_SSID,
    WIFI_STA_CONNECT_TIMEOUT_ASSOCIAT_FAIL,
    WIFI_STA_NETWORK_STACK_DHCP_SUCC,
    WIFI_STA_NETWORK_STACK_DHCP_TIMEOUT,
};

enum SMP_CFG_TYPE {
    PRIV_SMP_CFG1 = 1,
    PRIV_SMP_CFG2,
    AIRKISS_SMP_CFG,
};

struct smp_cfg_result {
    enum SMP_CFG_TYPE type;
    unsigned int ssid_crc;
    unsigned int random_val;
    char ssid[32];
    char passphrase[64];
};

enum WIFI_EVENT {
    WIFI_EVENT_MODULE_INIT,
    WIFI_EVENT_MODULE_START,
    WIFI_EVENT_MODULE_STOP,
    WIFI_EVENT_MODULE_START_ERR,

    WIFI_EVENT_AP_START,
    WIFI_EVENT_AP_STOP,
    WIFI_EVENT_STA_START,
    WIFI_EVENT_STA_STOP,
    WIFI_EVENT_STA_SCAN_COMPLETED,

    WIFI_EVENT_STA_CONNECT_SUCC,
    WIFI_EVENT_STA_CONNECT_TIMEOUT_NOT_FOUND_SSID,
    WIFI_EVENT_STA_CONNECT_TIMEOUT_ASSOCIAT_FAIL,
    WIFI_EVENT_STA_DISCONNECT,

    WIFI_EVENT_SMP_CFG_START,
    WIFI_EVENT_SMP_CFG_STOP,
    WIFI_EVENT_SMP_CFG_TIMEOUT,
    WIFI_EVENT_SMP_CFG_COMPLETED,

    WIFI_EVENT_STA_NETWORK_STACK_DHCP_SUCC,
    WIFI_EVENT_STA_NETWORK_STACK_DHCP_TIMEOUT,

    WIFI_EVENT_AP_ON_DISCONNECTED,
    WIFI_EVENT_AP_ON_ASSOC,

    WIFI_EVENT_MP_TEST_START,
    WIFI_EVENT_MP_TEST_STOP,

    WIFI_EVENT_P2P_START,
    WIFI_EVENT_P2P_STOP,
    WIFI_EVENT_P2P_GC_DISCONNECTED,
    WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_SUCC,
    WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_TIMEOUT,
    WIFI_EVENT_PM_SUSPEND,
    WIFI_EVENT_PM_RESUME,
    WIFI_FORCE_MODE_TIMEOUT,
};

struct wifi_mode_info {
    enum WIFI_MODE mode;
    char *ssid;
    char *pwd;
};

// @brief 配置WIFI MP测试后校准后的 晶体频偏参数 ,PA参数 , 各个速率功率的数字增益参数
struct wifi_calibration_param {
    u8 xosc_l;
    u8 xosc_r;
    u8 pa_trim_data[7];
    u8 mcs_dgain[20];
};
extern const struct wifi_calibration_param wifi_calibration_param;

struct ieee80211_frame {
    u8		i_fc[2];
    u8		i_dur[2];
    u8		i_addr1[6];
    u8		i_addr2[6];
    u8		i_addr3[6];
    u8		i_seq[2];
    /* possibly followed by addr4[6]; */
} __packed;

/**
 * @brief IEEE 802.11 management frame subtype definition
 */
#define IEEE80211_FC_STYPE_SHIFT        4
#define IEEE80211_FC_STYPE_MASK         (0xf << IEEE80211_FC_STYPE_SHIFT)
#define IEEE80211_FC_STYPE_ASSOC_REQ    (0x0 << IEEE80211_FC_STYPE_SHIFT)
#define IEEE80211_FC_STYPE_ASSOC_RESP   (0x1 << IEEE80211_FC_STYPE_SHIFT)
#define IEEE80211_FC_STYPE_REASSOC_REQ  (0x2 << IEEE80211_FC_STYPE_SHIFT)
#define IEEE80211_FC_STYPE_REASSOC_RESP (0x3 << IEEE80211_FC_STYPE_SHIFT)
#define IEEE80211_FC_STYPE_PROBE_REQ    (0x4 << IEEE80211_FC_STYPE_SHIFT)
#define IEEE80211_FC_STYPE_PROBE_RESP   (0x5 << IEEE80211_FC_STYPE_SHIFT)
#define IEEE80211_FC_STYPE_BEACON       (0x8 << IEEE80211_FC_STYPE_SHIFT)
#define IEEE80211_FC_STYPE_ATIM         (0x9 << IEEE80211_FC_STYPE_SHIFT)
#define IEEE80211_FC_STYPE_DISASSOC     (0xa << IEEE80211_FC_STYPE_SHIFT)
#define IEEE80211_FC_STYPE_AUTH         (0xb << IEEE80211_FC_STYPE_SHIFT)
#define IEEE80211_FC_STYPE_DEAUTH       (0xc << IEEE80211_FC_STYPE_SHIFT)
#define IEEE80211_FC_STYPE_ACTION       (0xd << IEEE80211_FC_STYPE_SHIFT)
#define IEEE80211_FC0_TYPE_MASK         0x0c
#define IEEE80211_FC0_TYPE_MGT          0x00
#define IEEE80211_FC0_TYPE_CTL          0x04
#define IEEE80211_FC0_TYPE_DATA         0x08
#define IEEE80211_FC0_SUBTYPE_MASK      0xf0
/// \endcond

/**
 * @brief wifi_set_event_callback，用于注册（设置）WIFI事件回调函数
 *
 * @param cb 指向WIFI事件回调函数，一般为：wifi_event_callback
 */
extern void wifi_set_event_callback(int (*cb)(void *, enum WIFI_EVENT));

/**
 * @brief wifi_airkiss_calcrc_bytes，用于计算airkiss接收包crc校验
 *
 * @param p 指向设备端扫描到的空中SSID
 * @param num_of_bytes 设备端扫描到的空中SSID长度
 */
extern u8 wifi_airkiss_calcrc_bytes(u8 *p, unsigned int num_of_bytes);

/**
 * @brief wifi_set_frame_cb，用于注册（设置）WIFI底层接收到802.11数据帧回调函数
 *
 * @param cb 指向WIFI底层接收到802.11数据帧回调函数，一般为：wifi_rx_cb
 */
extern void wifi_set_frame_cb(void (*cb)(void *rxwi, struct ieee80211_frame *header, void *data, void *reserve));

/**
 * @brief wifi_set_pwr，用于设置WIFI 模拟功率等级
 *
 * @param pwr_sel 默认为6，范围为0-6
 *
 * @note 参数设为0，表示把WIFI模拟功率调整到最低档位节电
 */
extern void wifi_set_pwr(unsigned char pwr_sel);

/**
 * @brief wifi_on，用于启动WIFI
 */
extern int wifi_on(void);

/**
 * @brief wifi_is_on，查询WIFI是否启动
 */
extern int wifi_is_on(void);

/**
 * @brief wifi_off，关闭WIFI
 */
extern int wifi_off(void);

/**
 * @brief wifi_get_mac，获取WIFI MAC地址
 *
 * @param mac 指向存储MAC地址的缓存数组，数组大小为6
 */
extern int wifi_get_mac(u8 *mac);

/**
 * @brief wifi_set_mac，设置WIFI MAC地址
 *
 * @param mac_addr 指向要设置的MAC地址缓存数组，数组大小为6
 */
extern int wifi_set_mac(char *mac_addr);

/**
 * @brief wifi_rxfilter_cfg，设置WIFI接收过滤
 *
 * @param mode 过滤模式
 *         |mode|说明|
 *         |- |- |
 *         |0|STA模式默认不过滤|
 *         |1|AP模式默认不过滤|
 *         |2|STA模式下使用,过滤广播,多播|
 *         |3|STA模式下使用,过滤not_my_bssid|
 *         |4|STA模式下使用,过滤广播+多播+not_my_bssid|
 *         |5|AP模式下使用,过滤广播,多播|
 *         |6|AP模式下使用,过滤not_my_bssid|
 *         |7|AP模式下使用,过滤广播+多播+not_my_bssid|
 */
extern void wifi_rxfilter_cfg(char mode);

/**
 * @brief wifi_set_tx_rate_control_tab，用于设置WIFI TX速率
 *
 * @param tab 速率表，不需要哪个速率就删除掉,可以动态设定
 */
extern void wifi_set_tx_rate_control_tab(u32 tab);

/**
 * @brief wifi_get_channel，用于获取WIFI当前信道
 */
extern u32 wifi_get_channel(void);

/**
 * @brief wifi_get_bssid，用于获取WIFI当前bssid
 */
extern void wifi_get_bssid(u8 bssid[6]);

/**
 * @brief wifi_get_upload_rate，用于获取WIFI上行速率
 */
extern u32 wifi_get_upload_rate(void);

/**
 * @brief wifi_get_download_rate，用于获取WIFI下行速率
 */
extern u32 wifi_get_download_rate(void);

/**
 * @brief wifi_get_mode_cur_info，用于获取WIFI当前是什么模式,或者当前指定模式下的配置信息
 *
 * @param info 指向一个wifi_mode_info类型的结构体，其包含了变量mode、ssid、pwd
 */
extern void wifi_get_mode_cur_info(struct wifi_mode_info *info);

/**
 * @brief wifi_get_mode_stored_info，用于获取WIFI最后记忆的是什么模式,或者最后记忆模式下的配置信息
 *
 * @param info 指向一个wifi_mode_info类型的结构体，其包含了变量mode、ssid、pwd
 */
extern int wifi_get_mode_stored_info(struct wifi_mode_info *info);

/**
 * @brief wifi_set_default_mode，用于设置WIFI启动默认模式配置
 *
 * @param parm 指向一个wifi_store_info类型的结构体
 * @param force 配置wifi_on之后的模式
 *         |force|说明|
 *         |- |- |
 *         |0|使用最后记忆的模式|
 *         |1|强制默认模式|
 *         |3-200|STA连接超时时间多少秒,如果超时都连接不上就连接最后记忆的或者最优网络|
 * @param store 选择是否存储默认配置的SSID
 *         |store|说明|
 *         |- |- |
 *         |0|不存储默认配置的SSID|
 *         |1|存储默认配置的SSID|
 *
 * @note 配置STA模式情况下,把默认配置SSID也存储起来,以后即使保存过其他SSID,也不会覆盖丢失,使用连接最优信号SSID策略的情况下可以匹配连接
 */
extern int wifi_set_default_mode(struct wifi_store_info *parm, char force, char store);

/**
 * @brief wifi_store_mode_info，用于保存WIFI模式配置信息,覆盖默认模式
 *
 * @param mode 配置的wifi模式
 * @param ssid 配置模式下的SSID
 * @param pwd 配置模式下的密码
 */
extern int wifi_store_mode_info(enum WIFI_MODE mode, char *ssid, char *pwd);

/**
 * @brief wifi_del_stored_ssid，用于删除STA模式下保存过的SSID,仅允许STA模式使用
 *
 * @param ssid 删除wifi_get_stored_sta_info存储中的ssid
 */
extern int wifi_del_stored_sta_info(char *ssid);

/**
 * @brief wifi_set_target_mac_for_agc_recv，用于设置目标接收地址,有利于提高抗干扰能力,专门针对目标地址的数据包进行接收AGC和频偏调整
 *
 * @param mac 目标接收地址
 */
extern void wifi_set_target_mac_for_agc_recv(char *mac);


/*-------------------------------------AP MODE--------------------------------------*/
/**
 * @brief wifi_enter_ap_mode，用于设置WIFI进入AP模式
 *
 * @param ap_ssid AP模式下的SSID
 * @param ap_pwd AP模式下的密码
 */
extern int wifi_enter_ap_mode(char *ap_ssid, char *ap_pwd);

/**
 * @brief wifi_get_sta_entry_rssi，用于AP模式下获取接入的每个STA信号质量和MAC地址
 */
extern int wifi_get_sta_entry_rssi(char wcid, char **rssi, u8 **evm, u8 **mac);

/**
 * @brief wifi_disconnect_station，用于AP模式下断开指定MAC地址的STA
 *
 * @param mac 要断开设备的mac地址
 * @param reason 断开的原因，通常填8:Deauthenticated because sending station is leaving
 */
extern void wifi_disconnect_station(char *mac, u16 reason);


/*-------------------------------------STA MODE--------------------------------------*/

/**
 * @brief wifi_set_store_ssid_cnt，用于设置WIFI最多保存多少个连接的SSID数目
 *
 * @param cnt sta模式下最多保存的SSID数目，工程中默认为5
 */
extern void wifi_set_store_ssid_cnt(u32 cnt);

/**
 * @brief wifi_set_sta_connect_best_ssid，用于设置WIFI进入STA模式的时候如果匹配到信号最好的网络就去连接
 *
 * @param enable 自动连接保存过的信号最好的SSID使能端,置1：使能，置0：不使能
 */
extern void wifi_set_sta_connect_best_ssid(u8 enable);

/**
 * @brief wifi_set_sta_connect_timeout，用于设置WIFI连接STA超时时间
 *
 * @param sec WIFI连接STA超时时间，单位为秒，工程中默认为60秒
 */
extern void wifi_set_sta_connect_timeout(int sec);

/**
 * @brief wifi_set_connect_sta_block，用于设置WIFI连接STA是否阻塞, 默认非阻塞通过事件通知
 *
 * @param block 使能端，置1：阻塞，置为0：不阻塞
 */
extern void wifi_set_connect_sta_block(int block);

/**
 * @brief wifi_enter_sta_mode，用于设置WIFI进入STA模式
 *
 * @param sta_ssid 配置STA模式的SSID
 * @param sta_pwd 配置STA模式的密码
 */
extern int wifi_enter_sta_mode(char *sta_ssid, char *sta_pwd);

/**
 * @brief wifi_scan_req，用于WIFI STA模式或者AP模式下启动一次扫描空中SSID
 * @note STA模式下调用返回-1代表WIFI OFF/正在扫描/连接中,无需启动扫描, 可以等待几秒或者扫描完成事件到来获取结果
 */
extern int wifi_scan_req(void);

/**
 * @brief wifi_get_scan_result，用于启动一次扫描空中SSID后,获取扫描结果
 */
extern struct wifi_scan_ssid_info *wifi_get_scan_result(u32 *ssid_cnt);

/**
 * @brief wifi_clear_scan_result，用于获取扫描结果后,清空上次扫描结果
 */
extern void wifi_clear_scan_result(void);

/**
 * @brief wifi_get_stored_sta_info，用于获取WIFI保存过的SSID
 *
 * @note 返回WIFI保存过的SSID数目
 */
extern int wifi_get_stored_sta_info(struct wifi_stored_sta_info wifi_stored_sta_info[]);

/**
 * @brief wifi_get_rssi，用于获取WIFI连接的STA接收信号强度
 *
 * @note 返回WIFI连接的STA接收信号强度
 */
extern char wifi_get_rssi(void);

/**
 * @brief wifi_get_cqi，用于获取WIFI连接的STA信号质量
 *
 * @note 返回WIFI连接的STA通信丢包质量,0-100,一般认为大于50为较好,20-50之间为一般,小于20较差, decide ChannelQuality based on: 1)last BEACON received time, 2)last RSSI权重50, 3)TxPER权重30, and 4)RxPER权重20
 */
extern char wifi_get_cqi(void);

/**
 * @brief wifi_get_sta_connect_state，用于获取WIFI是连接STA状态
 *
 * @note 返回WIFI连接STA的状态
 */
enum wifi_sta_connect_state wifi_get_sta_connect_state(void);


/*-------------------------------------MONITOR MODE--------------------------------------*/
/**
 * @brief wifi_enter_smp_cfg_mode，用于设置WIFI进入配网模式/monitor模式
 */
extern int wifi_enter_smp_cfg_mode(void);

/**
 * @brief wifi_set_smp_cfg_timeout，用于设置WIFI配网超时事件时间
 *
 * @param sec WIFI配网超时时间，单位为秒
 */
extern void wifi_set_smp_cfg_timeout(int sec);

/**
 * @brief wifi_set_smp_cfg_scan_all_channel，用于设置WIFI配网模式/monitor模式下是否扫描全部信道,默认否
 *
 * @param onoff 全扫描开关，1:开启，0:关闭
 */
extern void wifi_set_smp_cfg_scan_all_channel(char onoff);

/**
 * @brief wifi_set_smp_cfg_airkiss_recv_ssid，用于设置WIFI airkiss配网模式下是否接收完整的SSID,默认否,有助于加快配网时效,有概率配网失败
 *
 * @param onoff 全扫描开关，1:开启，0:关闭
 */
extern void wifi_set_smp_cfg_airkiss_recv_ssid(char onoff);

/**
 * @brief wifi_set_smp_cfg_just_monitor_mode，用于设置WIFI进入配网/monitor模式后,是否只保留monitor模式,默认否
 *
 * @param onoff 全扫描开关，1:开启，0:关闭
 */
extern void wifi_set_smp_cfg_just_monitor_mode(char onoff);

/**
 * @brief wifi_set_monitor_mode_scan_channel_time，用于在WIFI只保留monitor模式的情况下,设置扫描每个信道的时间间隔
 *
 * @param time_ms 扫描每个信道的时间间隔，单位为ms
 */
extern void wifi_set_monitor_mode_scan_channel_time(int time_ms);

/**
 * @brief wifi_set_airkiss_key，用于设置WIFI配网模式下airkiss的key
 *
 * @param key 为WIFI配网模式下airkiss的key数组
 */
extern void wifi_set_airkiss_key(u8 key[16]);

/**
 * @brief wifi_get_cfg_net_result，用于配网成功后获取WIFI配网信息
 */
extern int wifi_get_cfg_net_result(struct smp_cfg_result *smp_cfg);

// @brief 设置WIFI当前信道
/**
 * @brief wifi_set_channel，用于设置WIFI当前信道
 *
 * @param ch 要设置的WIFI信道
 */
extern void wifi_set_channel(u8 ch);

///  \cond DO_NOT_DOCUMENT
// @brief 使用WIFI底层接口直接发送数据帧
enum wifi_tx_rate {
    WIFI_TXRATE_1M = 0,
    WIFI_TXRATE_2M,
    WIFI_TXRATE_5M,
    WIFI_TXRATE_6M,
    WIFI_TXRATE_7M,
    WIFI_TXRATE_9M,
    WIFI_TXRATE_11M,
    WIFI_TXRATE_12M,
    WIFI_TXRATE_14M,
    WIFI_TXRATE_18M,
    WIFI_TXRATE_21M,
    WIFI_TXRATE_24M,
    WIFI_TXRATE_28M,
    WIFI_TXRATE_36M,
    WIFI_TXRATE_43M,
    WIFI_TXRATE_48M,
    WIFI_TXRATE_54M,
    WIFI_TXRATE_57M,
    WIFI_TXRATE_65M,
    WIFI_TXRATE_72M,
};
/// \endcond

/**
 * @brief wifi_send_data，用于使用WIFI底层接口直接发送数据帧
 *
 * @param pkg 要发送的数组
 * @param len 发送数据的长度
 * @param rate WIFI发送速率
 */
extern void wifi_send_data(char *pkg, int len, enum wifi_tx_rate rate);

/**
 * @brief wifi_set_long_retry，用于设置WIFI底层长帧重传次数
 *
 * @param retry 为重传次数
 */
extern void wifi_set_long_retry(u8  retry);

/**
 * @brief wifi_set_short_retry，用于设置WIFI底层短帧重传次数
 *
 * @param retry 为重传次数
 */
extern void wifi_set_short_retry(u8  retry);

/**
 * @brief wifi 最大backoff范围设置
 *
 * @param cwmax 为最大backoff范围
 */
extern void wifi_backoff_cfg(u8 cwmax);

#endif  //_WIFI_CONNECT_H_

