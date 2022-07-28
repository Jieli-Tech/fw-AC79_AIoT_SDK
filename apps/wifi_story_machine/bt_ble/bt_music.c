#include "system/app_core.h"
#include "server/server_core.h"
#include "server/audio_server.h"
#include "tone_player.h"
#include "app_config.h"
#include "action.h"
#include "btstack/avctp_user.h"
#include "btstack/le/ble_api.h"
#include "btstack/btstack_task.h"
#include "btctrler/btctrler_task.h"
#include "btcontroller_modules.h"
#include "app_music.h"
#include "bt_common.h"
#include "a2dp_media_codec.h"
#include "btstack/bluetooth.h"
#include "btstack/btstack_error.h"
#include "bt_ble/bt_emitter.h"
#include "event/bt_event.h"
#include "app_power_manage.h"
#include "syscfg/syscfg_id.h"
#include "wifi/wifi_connect.h"
#ifdef CONFIG_NET_ENABLE
#include "wbcp.h"
#endif

#ifdef CONFIG_BT_ENABLE

#define TIMEOUT_CONN_TIME         30 //超时断开之后回连的时间s
#define POWERON_AUTO_CONN_TIME    18  //开机去回连的时间,要取6的倍数

extern void *phone_call_begin(void *priv, u8 volume);
extern void phone_call_end(void);
extern void *earphone_a2dp_audio_codec_open(int media_type, u8 volume);
extern void earphone_a2dp_audio_codec_close(void);
extern u8 get_esco_coder_busy_flag(void);
extern void bredr_set_dut_enble(u8 en, u8 phone);
extern void get_remote_device_info_from_vm(void);
extern bool get_esco_busy_flag(void);
extern void bredr_close_all_scan(void);
extern int lmp_private_esco_suspend_resume(int flag);
extern const struct music_dec_ops *get_bt_music_dec_ops(void);
extern void set_app_music_dec_ops(const struct music_dec_ops *ops);
extern void switch_rf_coexistence_config_table(u8 index);
extern void __set_a2dp_auto_play_flag(u8 auto_en);
extern void __set_simple_pair_flag(bool flag);
extern void set_bt_dec_end_handler(void *handler);
extern u8 get_bt_connecting_flag(void);
extern void low_power_hw_unsleep_lock(void);
extern void low_power_hw_unsleep_unlock(void);

struct app_bt_opr {
    //phone
    u8 phone_ring_flag: 1;
    u8 phone_num_flag: 1;
    u8 phone_income_flag: 1;
    u8 phone_call_dec_begin: 1;
    u8 phone_con_sync_num_ring: 1;
    u8 phone_con_sync_ring: 1;
    u8 emitter_or_receiver: 2;

    u8 media_play_flag : 1;
    u8 call_flag : 1;	// 1-由于蓝牙打电话命令切回蓝牙模式
    u8 exit_flag : 1;	// 1-可以退出蓝牙标志
    u8 enable    : 1;
    u8 mute      : 1;
    u8 siri_stu;		// ios siri

    u8 call_volume;
    u8 media_volume;
    u8 inband_ringtone;
    u8 last_call_type;
    u8 auto_connection_addr[6];
    u8 income_phone_num[30];
    u8 income_phone_len;
    u16 phone_timer_id;
    u16 poweroff_timer_id;
    u16 a2dp_drop_frame_timer;
    u16 auto_connection_timer;
    u16 sniff_timer;
    int auto_connection_counter;
    void *dec_server;
};

static struct app_bt_opr app_bt_hdl;
#define __this 	(&app_bt_hdl)

#define SBC_FILTER_TIME_MS			2000	//后台音频过滤时间ms
#define SBC_ZERO_TIME_MS			500		//静音多长时间认为已经退出
#define NO_SBC_TIME_MS				100		//无音频时间ms

/*开关可发现可连接的函数接口*/
static void bt_wait_phone_connect_control(u8 enable)
{
    if (!__this->enable && enable) {
        return;
    }

    if (enable) {
        log_i("is_1t2_connection:%d \t total_conn_dev:%d\n", is_1t2_connection(), get_total_connect_dev());
        if (is_1t2_connection()) {
            /*达到最大连接数，可发现(0)可连接(0)*/
            user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
            user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
        } else {
            if (get_total_connect_dev() == 1) {
                /*支持连接2台，只连接一台的情况下，可发现(0)可连接(1)*/
                user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
                user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
            } else {
                /*可发现(1)可连接(1)*/
                user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
                user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
            }
        }
    } else {
        user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
    }
}

static void bt_init_ok_search_index(void)
{
    if (!__this->auto_connection_counter && get_current_poweron_memory_search_index(__this->auto_connection_addr)) {
        log_i("bt_wait_connect_and_phone_connect_switch\n");
        clear_current_poweron_memory_search_index(1);
        __this->auto_connection_counter = POWERON_AUTO_CONN_TIME * 1000; //8000ms
    }
}

static int bt_wait_connect_and_phone_connect_switch(void *p)
{
    int ret = 0;
    int timeout = 0;

    log_i("connect_switch: %d, %d\n", (int)p, __this->auto_connection_counter);

    if (!__this->enable) {
        return 0;
    }

    __this->auto_connection_timer = 0;

    if (__this->auto_connection_counter <= 0) {
        __this->auto_connection_counter = 0;
        user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);

        if (get_current_poweron_memory_search_index(NULL)) {
            bt_init_ok_search_index();
            return bt_wait_connect_and_phone_connect_switch(0);
        } else {
            bt_wait_phone_connect_control(1);
            return 0;
        }
    }
    /* log_i(">>>phone_connect_switch=%d\n",__this->auto_connection_counter ); */
    if (!p) {
        if (__this->auto_connection_counter) {
            timeout = 4000;
            bt_wait_phone_connect_control(0);
            switch_rf_coexistence_config_table(7);
            user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6, __this->auto_connection_addr);
            ret = 1;
        }
    } else {
        timeout = 2000;
        user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
        bt_wait_phone_connect_control(1);
        switch_rf_coexistence_config_table(0);
    }

    if (__this->auto_connection_counter) {
        __this->auto_connection_counter -= timeout;
        log_i("do=%d\n", __this->auto_connection_counter);
    }

    __this->auto_connection_timer = sys_timeout_add((void *)(!(int)p),
                                    (void (*)(void *))bt_wait_connect_and_phone_connect_switch, timeout);

    return ret;
}

#define SYS_BT_EVENT_TYPE_DECODE_STATUS (('D' << 24) | ('E' << 16) | ('C' << 8) | '\0')
#define BT_MUSIC_DEC_EVENT_VOL_SET	0x01
#define BT_MUSIC_DEC_EVENT_MUTE_SET	0x02

static void bt_set_music_device_volume(int volume)
{
    struct bt_event evt = {0};
    if (volume == 0xff) {
        return;
    }
    volume = volume * 100 / 127;
    printf("bt music set volume : %d\n", volume);
    __this->media_volume = volume;
    set_app_music_volume(volume, BT_MUSIC_MODE);
    evt.event = BT_MUSIC_DEC_EVENT_VOL_SET;
    evt.value = volume;
    bt_event_notify(BT_EVENT_FROM_USER, &evt);
}

void bt_music_set_mute_status(u8 mute)
{
    struct bt_event evt = {0};
    evt.event = BT_MUSIC_DEC_EVENT_MUTE_SET;
    evt.value = mute;
    bt_event_notify(BT_EVENT_FROM_USER, &evt);
}

int check_a2dp_media_if_mute(void)
{
    return __this->mute;
}

static int phone_get_device_vol(void)
{
    printf("bt music get volume\n");
    return get_app_music_volume() * 127 / 100;
}

static void phone_sync_vol(u8 volume)
{
    volume = (u16)volume * 15 / 100;
    user_send_cmd_prepare(USER_CTRL_HFP_CALL_SET_VOLUME, 1, &volume);
}

static void bt_read_remote_name(u8 status, u8 *addr, u8 *name)
{
    if (status) {
        printf("remote_name fail \n");
    } else {
        printf("remote_name : %s \n", name);
    }

    put_buf(addr, 6);

#if TCFG_USER_EMITTER_ENABLE
    emitter_search_noname(status, addr, (char *)name);
#endif
}

static int bt_get_battery_value()
{
    //取消默认蓝牙定时发送电量给手机，需要更新电量给手机使用USER_CTRL_HFP_CMD_UPDATE_BATTARY命令
    /*电量协议的是0-9个等级，请比例换算*/
    return get_app_music_battery_power() * 9 / 100;
}

static void bt_dut_api(u8 value)
{
    log_i("bt in dut\n");
    bredr_close_all_scan();
#if TCFG_USER_BLE_ENABLE && TRANS_DATA_EN
    bt_ble_adv_enable(0);
#endif
}

static void bredr_handle_register(void)
{
#if USER_SUPPORT_PROFILE_SPP
    extern void user_spp_data_handler(u8 packet_type, u16 ch, u8 * packet, u16 size);
    spp_data_deal_handle_register(user_spp_data_handler);
#endif

#if BT_SUPPORT_MUSIC_VOL_SYNC
    ///蓝牙音乐和通话音量同步
    music_vol_change_handle_register(bt_set_music_device_volume, phone_get_device_vol);
#endif
#if BT_SUPPORT_DISPLAY_BAT
    ///电量显示获取电量的接口
    get_battery_value_register(bt_get_battery_value);   /*电量显示获取电量的接口*/
#endif
    ///被测试盒链接上进入快速测试回调
    /* bt_fast_test_handle_register(bt_fast_test_api); */

    ///样机进入dut被测试仪器链接上回调
    bt_dut_test_handle_register(bt_dut_api);

    ///获取远端设备蓝牙名字回调
    read_remote_name_handle_register(bt_read_remote_name);

    ////获取歌曲信息回调
    /* bt_music_info_handle_register(user_get_bt_music_info); */

#if TCFG_USER_EMITTER_ENABLE
    ////发射器设置回调等
    inquiry_result_handle_register(emitter_search_result);
#endif
}

static void bt_function_select_init(void)
{
    /* __set_a2dp_auto_play_flag(1); */
    __set_user_ctrl_conn_num(TCFG_BD_NUM);
    __set_support_msbc_flag(1);
    __set_support_aac_flag(0);
#if BT_SUPPORT_DISPLAY_BAT
    __bt_set_update_battery_time(60);
#else
    __bt_set_update_battery_time(0);
#endif
    __set_page_timeout_value(8000); /*回连搜索时间长度设置,可使用该函数注册使用，ms单位,u16*/
    __set_super_timeout_value(8000); /*回连时超时参数设置。ms单位。做主机有效*/

    __set_simple_pair_flag(1); //是否打开简易配对功能，打开后不需要输入pincode
    ////设置蓝牙加密的level
    //io_capabilities ; /*0: Display only 1: Display YesNo 2: KeyboardOnly 3: NoInputNoOutput*/
    //authentication_requirements: 0:not protect  1 :protect
    __set_simple_pair_param(3, 0, 2);

#if (USER_SUPPORT_PROFILE_PBAP==1)
    ////设置蓝牙设备类型
    __change_hci_class_type(BD_CLASS_CAR_AUDIO);
#endif

#if (TCFG_BT_SNIFF_ENABLE == 0)
    void lmp_set_sniff_disable(void);
    lmp_set_sniff_disable();
#endif

#if TCFG_USER_BLE_ENABLE
    u8 tmp_ble_addr[6];
    extern const u8 *bt_get_mac_addr(void);
    extern void lib_make_ble_address(u8 * ble_address, u8 * edr_address);
    extern int le_controller_set_mac(void *addr);
#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV)
    memcpy(tmp_ble_addr, (u8 *)bt_get_mac_addr(), 6);
#else
    lib_make_ble_address(tmp_ble_addr, (u8 *)bt_get_mac_addr());
#endif //
    le_controller_set_mac((void *)tmp_ble_addr);
    printf("\n-----edr + ble 's address-----");
    put_buf((void *)bt_get_mac_addr(), 6);
    put_buf((void *)tmp_ble_addr, 6);
#if BT_NET_CENTRAL_EN || TRANS_MULTI_BLE_MASTER_NUMS
    extern void ble_client_config_init(void);
    ble_client_config_init();
#endif
#endif
}

/*配置通话时前面丢掉的数据包包数*/
#define ESCO_DUMP_PACKET_ADJUST		1	/*配置使能*/
#define ESCO_DUMP_PACKET_DEFAULT	0
#define ESCO_DUMP_PACKET_CALL		120 /*0~0xFF*/

static u8 esco_dump_packet = ESCO_DUMP_PACKET_CALL;

#if ESCO_DUMP_PACKET_ADJUST
u8 get_esco_packet_dump(void)
{
    //log_i("esco_dump_packet:%d\n", esco_dump_packet);
    return esco_dump_packet;
}
#endif


#define  SNIFF_CNT_TIME               5/////<空闲5S之后进入sniff模式

#define SNIFF_MAX_INTERVALSLOT        800
#define SNIFF_MIN_INTERVALSLOT        100
#define SNIFF_ATTEMPT_SLOT            4
#define SNIFF_TIMEOUT_SLOT            1

static int exit_sniff_timer = 0;

static void bt_check_exit_sniff(void)
{
    sys_timeout_del(exit_sniff_timer);
    exit_sniff_timer = 0;
    user_send_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
}

static void bt_check_enter_sniff(void *priv)
{
    struct sniff_ctrl_config_t sniff_ctrl_config;
    u8 addr[12];
    u8 conn_cnt = 0;
    u8 i = 0;
    /*putchar('H');*/
    conn_cnt = bt_api_enter_sniff_status_check(SNIFF_CNT_TIME, addr);

    ASSERT(conn_cnt <= 2);

    for (i = 0; i < conn_cnt; i++) {
        log_i("-----USER SEND SNIFF IN %d %d\n", i, conn_cnt);
        sniff_ctrl_config.sniff_max_interval = SNIFF_MAX_INTERVALSLOT;
        sniff_ctrl_config.sniff_mix_interval = SNIFF_MIN_INTERVALSLOT;
        sniff_ctrl_config.sniff_attemp = SNIFF_ATTEMPT_SLOT;
        sniff_ctrl_config.sniff_timeout  = SNIFF_TIMEOUT_SLOT;
        memcpy(sniff_ctrl_config.sniff_addr, addr + i * 6, 6);
        user_send_cmd_prepare(USER_CTRL_SNIFF_IN, sizeof(struct sniff_ctrl_config_t), (u8 *)&sniff_ctrl_config);
    }
}

static void sys_auto_sniff_controle(u8 enable, u8 *addr)
{
#if (TCFG_BT_SNIFF_ENABLE == 0)
    return;
#endif

    if (addr) {
        if (bt_api_conn_mode_check(enable, addr) == 0) {
            log_i("sniff ctr not change\n");
            return;
        }
    }

    if (enable) {
        if (get_total_connect_dev() == 0) {
            return;
        }

        if (addr) {
            log_i("sniff cmd timer init\n");
            user_cmd_timer_init();
        }

        if (__this->sniff_timer == 0) {
            log_i("check_sniff_enable\n");
            __this->sniff_timer = sys_timer_add(NULL, bt_check_enter_sniff, 1000);
        }
    } else {
        if (get_total_connect_dev() > 0) {
            return;
        }

        if (addr) {
            log_i("sniff cmd timer remove\n");
            remove_user_cmd_timer();
        }

        if (__this->sniff_timer) {
            log_i("check_sniff_disable\n");
            sys_timeout_del(__this->sniff_timer);
            __this->sniff_timer = 0;

            if (exit_sniff_timer == 0) {
                /* exit_sniff_timer = sys_timer_add(NULL, bt_check_exit_sniff, 5000); */
            }
        }
    }
}

static const u32 num0_9[] = {
    (u32)TONE_NUM_0,
    (u32)TONE_NUM_1,
    (u32)TONE_NUM_2,
    (u32)TONE_NUM_3,
    (u32)TONE_NUM_4,
    (u32)TONE_NUM_5,
    (u32)TONE_NUM_6,
    (u32)TONE_NUM_7,
    (u32)TONE_NUM_8,
    (u32)TONE_NUM_9,
} ;

#if 0
static u8 check_phone_income_idle(void)
{
    if (__this->phone_ring_flag) {
        return 0;
    }
    return 1;
}

REGISTER_LP_TARGET(phone_incom_lp_target) = {
    .name       = "phone_check",
    .is_idle    = check_phone_income_idle,
};
#endif

static void number_to_play_list(char *num, u32 *lst)
{
    u8 i = 0;

    if (num) {
        for (; i < strlen(num); i++) {
            lst[i] = num0_9[num[i] - '0'] ;
        }
    }
    lst[i++] = (u32)TONE_REPEAT_BEGIN(-1);
    lst[i++] = (u32)TONE_RING;
    lst[i++] = (u32)TONE_REPEAT_END();
    lst[i++] = (u32)NULL;
}

static void phone_num_play_timer(void *priv)
{
    if (get_call_status() == BT_CALL_HANGUP) {
        log_i("hangup,--phone num play return\n");
        return;
    }

    if (__this->phone_num_flag) {
        u32 *len_lst = malloc(4 * 34);
        number_to_play_list((char *)(__this->income_phone_num), len_lst);
        /* tone_file_list_play((const char **)len_lst); */
    } else {
        /*电话号码还没有获取到，定时查询*/
        __this->phone_timer_id = sys_timeout_add(NULL, phone_num_play_timer, 200);
    }
}

static void phone_num_play_start(void)
{
    /* check if support inband ringtone */
    if (!__this->inband_ringtone) {
        __this->phone_num_flag = 0;
        __this->phone_timer_id = sys_timeout_add(NULL, phone_num_play_timer, 500);
    }
}

static void phone_ring_play_timer(void *priv)
{
    struct intent it = {0};
    it.name = "app_music";
    /* it.action = ACTION_MUSIC_PLAY_FILE; */
    /* it.data = CONFIG_VOICE_PROMPT_FILE_PATH"ring.mp3"; */
    it.action = ACTION_MUSIC_PLAY_VOICE_PROMPT;
    it.data = "ring.mp3";
    start_app(&it);
    //通话铃声逻辑暂未处理
    __this->phone_timer_id = sys_timeout_add(NULL, phone_ring_play_timer, 1000);
}

static void phone_ring_play_start(void)
{
    if (get_call_status() == BT_CALL_HANGUP) {
        log_i("hangup,--phone ring play return\n");
        return;
    }
    /* check if support inband ringtone */
    if (!__this->inband_ringtone) {
        __this->phone_timer_id = sys_timeout_add(NULL, phone_ring_play_timer, 1000);
    }
}

static void a2dp_drop_frame(void *p)
{
    int len;
    u8 *frame;
    int num = a2dp_media_get_packet_num();
    if (num > 1) {
        for (int i = 0; i < (num - 1); i++) {
            len = a2dp_media_get_packet(&frame);
            if (len <= 0) {
                break;
            }
            /* log_i("a2dp_drop_frame: %d\n", len); */
            a2dp_media_free_packet(frame);
        }
    }
}

static void a2dp_audio_codec_open(void)
{
    if (__this->a2dp_drop_frame_timer) {
        sys_timeout_del(__this->a2dp_drop_frame_timer);
        __this->a2dp_drop_frame_timer = 0;
    }
    if (!__this->dec_server) {
        __this->dec_server = earphone_a2dp_audio_codec_open(0x0, __this->media_volume);//A2DP_CODEC_SBC);
        if (__this->dec_server) {
            __this->media_play_flag = 1;
        }
    }
}

void bt_ble_module_init(void)
{
    bt_function_select_init();
    bredr_handle_register();
    btstack_init();
}

void bt_connection_enable(void)
{
    if (__this->enable) {
        return;
    }
    if (__this->poweroff_timer_id) {
        sys_timeout_del(__this->poweroff_timer_id);
        __this->poweroff_timer_id = 0;
    }
#ifdef CONFIG_LOW_POWER_ENABLE
    low_power_hw_unsleep_lock();
    btctrler_task_init_bredr();
    low_power_hw_unsleep_unlock();
#endif
    __this->enable = 1;
    __this->auto_connection_counter = 0;
    get_remote_device_info_from_vm();
    bt_init_ok_search_index();

#if ((CONFIG_BT_MODE == BT_BQB)||(CONFIG_BT_MODE == BT_PER))
    bt_wait_phone_connect_control(1);
#else
#if TCFG_USER_EMITTER_ENABLE || defined CONFIG_DUI_SDK_ENABLE
    bt_wait_phone_connect_control(1);
#else
    bt_wait_connect_and_phone_connect_switch(0);
#endif
#endif
}

static void bt_power_off(void *p)
{
    if (is_1t2_connection()) {
        __this->poweroff_timer_id = sys_timeout_add(NULL, bt_power_off, 100);
        return;
    }
    __this->poweroff_timer_id = 0;
    user_send_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);
#ifdef CONFIG_LOW_POWER_ENABLE
    os_time_dly(50);
    low_power_hw_unsleep_lock();
    btctrler_task_close_bredr();
    low_power_hw_unsleep_unlock();
    lmp_set_sniff_establish_by_remote(0);
#endif
}

void bt_delete_power_off_timer(void)
{
    if (__this->poweroff_timer_id) {
        sys_timeout_del(__this->poweroff_timer_id);
        __this->poweroff_timer_id = 0;
    }
}

//关闭蓝牙
void bt_connection_disable(void)
{
#if !TCFG_USER_EMITTER_ENABLE
    if (!__this->enable) {
        return;
    }
#else
    emitter_or_receiver_switch(0);
#endif
    __this->enable = 0;
    if (__this->auto_connection_timer) {
        sys_timer_del(__this->auto_connection_timer);
        __this->auto_connection_timer = 0;
    }
    switch_rf_coexistence_config_table(0);
    bt_wait_phone_connect_control(0);
    user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_CONNECTION_CANCEL, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, NULL);
    if (!__this->poweroff_timer_id) {
        __this->poweroff_timer_id = sys_timeout_add(NULL, bt_power_off, 500);
    }
    /* user_send_cmd_prepare(USER_CTRL_INQUIRY_CANCEL, 0, NULL); */
}

void bredr_a2dp_open_and_close(void)
{
    if (get_curr_channel_state() & A2DP_CH) {
        puts("start to disconnect a2dp ");
        user_send_cmd_prepare(USER_CTRL_DISCONN_A2DP, 0, NULL);
    } else {
        puts("start to connect a2dp ");
        user_send_cmd_prepare(USER_CTRL_CONN_A2DP, 0, NULL);
    }
}

void bredr_hfp_open_and_close(void)
{
    if (get_curr_channel_state() & HFP_CH) {
        user_send_cmd_prepare(USER_CTRL_HFP_DISCONNECT, 0, NULL);
    } else {
        user_send_cmd_prepare(USER_CTRL_HFP_CMD_BEGIN, 0, NULL);
    }
}

/*
 * 对应原来的状态处理函数，连接，电话状态等
 */
static int bt_connction_status_event_handler(struct bt_event *bt)
{
    struct intent it = {0};
    log_i("-----------------------bt_connction_status_event_handler %d\n", bt->event);

    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        /*
         * 蓝牙初始化完成
         */
        log_i("===================BT_STATUS_INIT_OK\n");
#if CONFIG_POWER_ON_ENABLE_BT
        bt_connection_enable();
#else
#if ((CONFIG_BT_MODE == BT_BQB)||(CONFIG_BT_MODE == BT_PER))
        bt_wait_phone_connect_control(1);
#endif
#ifdef CONFIG_LOW_POWER_ENABLE
        btctrler_task_close_bredr();
#endif
#endif
#if BT_NET_CENTRAL_EN && !CONFIG_POWER_ON_ENABLE_BLE
        extern void bt_master_ble_init(void);
        bt_master_ble_init();
#endif
#if TCFG_USER_BLE_ENABLE && CONFIG_POWER_ON_ENABLE_BLE
        if (BT_MODE_IS(BT_BQB)) {
            ble_bqb_test_thread_init();
        } else {
            bt_ble_init();
        }
#endif
#if CONFIG_BT_BREDR_DUT_MODE_ENABLE
        bredr_set_dut_enble(1, 1);
#endif
#if (TCFG_USER_EDR_ENABLE && SPP_TRANS_DATA_EN)
        extern void transport_spp_init(void);
        transport_spp_init();
        extern void bt_wait_phone_connect_control_ext(u8 inquiry_en, u8 page_scan_en);
        bt_wait_phone_connect_control_ext(1, 1);
#endif
#if CONFIG_BLE_MESH_ENABLE
        void bt_ble_mesh_init(void);
        bt_ble_mesh_init();
#endif
        break;
    case BT_STATUS_START_CONNECTED:
        log_i(" BT_STATUS_START_CONNECTED\n");
        break;
    case BT_STATUS_ENCRY_COMPLETE:
        log_i(" BT_STATUS_ENCRY_COMPLETE\n");
        break;
    case BT_STATUS_SECOND_CONNECTED:
        clear_current_poweron_memory_search_index(0);
    case BT_STATUS_FIRST_CONNECTED:
        log_i("BT_STATUS_CONNECTED\n");
#ifdef CONFIG_WIFI_ENABLE
        switch_rf_coexistence_config_table(6);
#endif
        sys_auto_sniff_controle(1, NULL);
        sys_power_auto_shutdown_pause();
        /* user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_SEND_VOL, 0, NULL); */
        it.name = "app_music";
        it.action = ACTION_MUSIC_PLAY_VOICE_PROMPT;
        it.data = "BtSucceed.mp3";
        it.exdata = 1;
        start_app(&it);
        break;
    case BT_STATUS_FIRST_DISCONNECT:
    case BT_STATUS_SECOND_DISCONNECT:
        log_i("BT_STATUS_DISCONNECT\n");
        __this->call_flag = 0;
        __this->media_play_flag = 0;
        sys_auto_sniff_controle(0, NULL);
#if TCFG_USER_EMITTER_ENABLE
        bt_emitter_stu_set(0);
#endif
        sys_power_auto_shutdown_resume();
#ifdef CONFIG_WIFI_ENABLE
        switch_rf_coexistence_config_table(0);
#endif
        it.name = "app_music";
        it.action = ACTION_MUSIC_PLAY_VOICE_PROMPT;
        it.data = "BtDisc.mp3";
        it.exdata = 1;
        start_app(&it);
        break;
    //phone status deal
    case BT_STATUS_PHONE_INCOME:
        log_i("BT_STATUS_PHONE_INCOME\n");
        //此处要关掉混响
        esco_dump_packet = ESCO_DUMP_PACKET_CALL;

        u8 tmp_bd_addr[6];
        memcpy(tmp_bd_addr, bt->args, 6);
        /*
         *(1)1t2有一台通话的时候，另一台如果来电不要提示
         *(2)1t2两台同时来电，先来的提示，后来的不播
         */
        if ((check_esco_state_via_addr(tmp_bd_addr) != BD_ESCO_BUSY_OTHER) && (__this->phone_ring_flag == 0)) {
#if BT_INBAND_RINGTONE
            extern u8 get_device_inband_ringtone_flag(void);
            __this->inband_ringtone = get_device_inband_ringtone_flag();
#else
            __this->inband_ringtone = 0 ;
            lmp_private_esco_suspend_resume(3);
#endif
            __this->phone_ring_flag = 1;
            __this->phone_income_flag = 1;
#if BT_PHONE_NUMBER
            phone_num_play_start();
#else
            phone_ring_play_start();
#endif
            user_send_cmd_prepare(USER_CTRL_HFP_CALL_CURRENT, 0, NULL); //发命令获取电话号码
        } else {
            log_i("SCO busy now:%d,%d\n", check_esco_state_via_addr(tmp_bd_addr), __this->phone_ring_flag);
        }
        break;
    case BT_STATUS_PHONE_OUT:
        log_i("BT_STATUS_PHONE_OUT\n");
        lmp_private_esco_suspend_resume(4);
        esco_dump_packet = ESCO_DUMP_PACKET_CALL;
        __this->phone_income_flag = 0;
        user_send_cmd_prepare(USER_CTRL_HFP_CALL_CURRENT, 0, NULL); //发命令获取电话号码
        break;
    case BT_STATUS_PHONE_ACTIVE:
        log_i("BT_STATUS_PHONE_ACTIVE\n");
        if (__this->phone_call_dec_begin) {
            log_i("call_active,dump_packet clear\n");
            esco_dump_packet = ESCO_DUMP_PACKET_DEFAULT;
        }
        if (__this->phone_ring_flag) {
            __this->phone_ring_flag = 0;
            if (__this->phone_timer_id) {
                sys_timeout_del(__this->phone_timer_id);
                __this->phone_timer_id = 0;
            }
        }
        lmp_private_esco_suspend_resume(4);
        __this->phone_income_flag = 0;
        __this->phone_num_flag = 0;
        __this->phone_con_sync_num_ring = 0;
        __this->phone_con_sync_ring = 0;
        log_i("phone_active:%d\n", __this->call_volume);
        break;
    case BT_STATUS_PHONE_HANGUP:
        esco_dump_packet = ESCO_DUMP_PACKET_CALL;
        log_i("phone_handup\n");
        if (__this->phone_ring_flag) {
            __this->phone_ring_flag = 0;
            if (__this->phone_timer_id) {
                sys_timeout_del(__this->phone_timer_id);
                __this->phone_timer_id = 0;
            }
        }
        lmp_private_esco_suspend_resume(4);
        __this->phone_num_flag = 0;
        __this->phone_con_sync_num_ring = 0;
        __this->phone_con_sync_ring = 0;
        break;
    case BT_STATUS_PHONE_NUMBER:
        log_i("BT_STATUS_PHONE_NUMBER\n");
        u8 *phone_number = (u8 *)bt->value;
        if (__this->phone_num_flag == 1) {
            break;
        }
        __this->income_phone_len = 0;
        memset(__this->income_phone_num, '\0', sizeof(__this->income_phone_num));
        for (int i = 0; i < strlen((const char *)phone_number); i++) {
            if (phone_number[i] >= '0' && phone_number[i] <= '9') {
                //过滤，只有数字才能报号
                __this->income_phone_num[__this->income_phone_len++] = phone_number[i];
                if (__this->income_phone_len >= sizeof(__this->income_phone_num)) {
                    break;    /*buffer 空间不够，后面不要了*/
                }
            }
        }
        if (__this->income_phone_len > 0) {
            __this->phone_num_flag = 1;
        } else {
            log_i("PHONE_NUMBER len err\n");
        }
        break;
    case BT_STATUS_INBAND_RINGTONE:
        log_i("BT_STATUS_INBAND_RINGTONE\n");
#if BT_INBAND_RINGTONE
        __this->inband_ringtone = bt->value;
#else
        __this->inband_ringtone = 0;
#endif
        break;
    case BT_STATUS_BEGIN_AUTO_CON:
        log_i("BT_STATUS_BEGIN_AUTO_CON\n");
        break;
    case BT_STATUS_A2DP_MEDIA_START:
        log_i(" BT_STATUS_A2DP_MEDIA_START");
        __this->call_flag = 0;
        __this->mute = 0;
        if (get_bt_connecting_flag()) {
            __this->a2dp_drop_frame_timer = sys_timer_add(NULL, a2dp_drop_frame, 100);
            set_bt_dec_end_handler(a2dp_audio_codec_open);
            break;
        }
        if (!__this->dec_server) {
            __this->dec_server = earphone_a2dp_audio_codec_open(0x0, __this->media_volume);//A2DP_CODEC_SBC);
            if (__this->dec_server) {
                __this->media_play_flag = 1;
            }
        }
        break;
    case BT_STATUS_A2DP_MEDIA_STOP:
        log_i(" BT_STATUS_A2DP_MEDIA_STOP");
        if (__this->a2dp_drop_frame_timer) {
            sys_timeout_del(__this->a2dp_drop_frame_timer);
            __this->a2dp_drop_frame_timer = 0;
        }
        if (get_bt_connecting_flag()) {
            set_bt_dec_end_handler(NULL);
        }
        if (__this->dec_server) {
            __this->dec_server = NULL;
            earphone_a2dp_audio_codec_close();
        }
        __this->media_play_flag = 0;
        break;
    case BT_STATUS_SCO_STATUS_CHANGE:
        log_i(" BT_STATUS_SCO_STATUS_CHANGE len:%d ,type:%d", (bt->value >> 16), (bt->value & 0x0000ffff));
        if (bt->value != 0xff) {
            if (!__this->dec_server) {
                __this->dec_server = phone_call_begin(&bt->value, __this->call_volume);
            }
            __this->call_flag = 1;
            __this->phone_call_dec_begin = 1;
            if (get_call_status() == BT_CALL_ACTIVE) {
                log_i("dec_begin,dump_packet clear\n");
                esco_dump_packet = ESCO_DUMP_PACKET_DEFAULT;
            }
        } else {
            __this->phone_call_dec_begin = 0;
            esco_dump_packet = ESCO_DUMP_PACKET_CALL;
            if (__this->dec_server) {
                phone_call_end();
                __this->dec_server = NULL;
            }
            __this->call_flag = 0;
        }
        break;
    case BT_STATUS_CALL_VOL_CHANGE:
        log_i(" BT_STATUS_CALL_VOL_CHANGE %d", bt->value);
        u8 volume = 100 * bt->value / 15;
        u8 call_status = get_call_status();
        __this->call_volume = volume;
        if ((call_status == BT_CALL_ACTIVE) || (call_status == BT_CALL_OUTGOING) || __this->siri_stu) {
            struct bt_event evt = {0};
            evt.event = BT_MUSIC_DEC_EVENT_VOL_SET;
            evt.value = volume;
            bt_event_notify(BT_EVENT_FROM_USER, &evt);
        } else if (call_status != BT_CALL_HANGUP) {
            /*只保存，不设置到dac*/
            __this->call_volume = volume;
        }
        break;
    case BT_STATUS_SNIFF_STATE_UPDATE:
        log_i(" BT_STATUS_SNIFF_STATE_UPDATE %d\n", bt->value);    //0退出SNIFF
        if (bt->value == 0) {
            sys_auto_sniff_controle(1, bt->args);
        } else {
            sys_auto_sniff_controle(0, bt->args);
        }
        break;
    case BT_STATUS_LAST_CALL_TYPE_CHANGE:
        log_i("BT_STATUS_LAST_CALL_TYPE_CHANGE:%d\n", bt->value);
        __this->last_call_type = bt->value;
        break;
    case BT_STATUS_CONN_A2DP_CH:
#if TCFG_USER_EMITTER_ENABLE && BT_SUPPORT_EMITTER_AUTO_A2DP_START
        //收到对方的开始播放指令才打开A2DP流
        bt_emitter_stu_sw();
#endif
        break;
    case BT_STATUS_CONN_HFP_CH:
        if ((!is_1t2_connection()) && (get_current_poweron_memory_search_index(NULL))) { //回连下一个device
            if (get_esco_coder_busy_flag()) {
                clear_current_poweron_memory_search_index(0);
            } else {
                user_send_cmd_prepare(USER_CTRL_START_CONNECTION, 0, NULL);
            }
        }
        break;
    case BT_STATUS_PHONE_MANUFACTURER:
        log_i("BT_STATUS_PHONE_MANUFACTURER:%d\n", bt->value);
        extern const u8 hid_conn_depend_on_dev_company;
        if (hid_conn_depend_on_dev_company) {
            if (bt->value) {
                //user_send_cmd_prepare(USER_CTRL_HID_CONN, 0, NULL);
            } else {
                user_send_cmd_prepare(USER_CTRL_HID_DISCONNECT, 0, NULL);
            }
        }
        break;
    case BT_STATUS_VOICE_RECOGNITION:
        log_i(" BT_STATUS_VOICE_RECOGNITION:%d \n", bt->value);
        esco_dump_packet = ESCO_DUMP_PACKET_DEFAULT;
        /* put_buf(bt, sizeof(struct bt_event)); */
        __this->siri_stu = bt->value;
        break;
    case BT_STATUS_AVRCP_INCOME_OPID:
#define AVC_VOLUME_UP			0x41
#define AVC_VOLUME_DOWN			0x42
        log_i("BT_STATUS_AVRCP_INCOME_OPID:%d\n", bt->value);
        if (bt->value == AVC_VOLUME_UP) {

        }
        if (bt->value == AVC_VOLUME_DOWN) {

        }
        break;
    case BT_STATUS_RECONN_OR_CONN:
        log_i(" BT_STATUS_RECONN_OR_CONN\n");
        break;
    default:
        log_i(" BT STATUS DEFAULT\n");
        break;
    }

    return 0;
}

static void sys_time_auto_connection_deal(void *arg)
{
    if (__this->auto_connection_counter) {
        __this->auto_connection_counter--;
        user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6, __this->auto_connection_addr);
    }
}

static void bt_send_pair(u8 en)
{
    user_send_cmd_prepare(USER_CTRL_PAIR, 1, &en);
}

static u8 bt_hci_event_filter(struct bt_event *bt)
{
    if (bt->event == HCI_EVENT_VENDOR_REMOTE_TEST) {
        if (0 == bt->value) {
            set_remote_test_flag(0);
            /* log_debug("clear_test_box_flag"); */
            return 0;
        } else {
#if TCFG_USER_BLE_ENABLE && TRANS_DATA_EN
            //1:edr con;2:ble con;
            if (1 == bt->value) {
                extern void bt_ble_adv_enable(u8 enable);
                bt_ble_adv_enable(0);
            }
#endif
        }
    }

    if ((bt->event != HCI_EVENT_CONNECTION_COMPLETE) ||
        ((bt->event == HCI_EVENT_CONNECTION_COMPLETE) && (bt->value != ERROR_CODE_SUCCESS))) {
#if TCFG_TEST_BOX_ENABLE
        if (chargestore_get_testbox_status()) {
            if (get_remote_test_flag()) {
                chargestore_clear_connect_status();
            }
            //return 0;
        }
#endif
        if (get_remote_test_flag() \
            && !(HCI_EVENT_DISCONNECTION_COMPLETE == bt->event) \
            && !(HCI_EVENT_VENDOR_REMOTE_TEST == bt->event)) {
            log_i("cpu reset\n");
            /* cpu_reset(); */
        }
    }

    return 1;
}

static int bt_hci_event_handler(struct bt_event *bt)
{
    //对应原来的蓝牙连接上断开处理函数  ,bt->value=reason
    log_i("------------------------bt_hci_event_handler reason %x", bt->event);

#ifdef CONFIG_WIFIBOX_ENABLE
    if (bt->event == HCI_EVENT_VENDOR_REMOTE_TEST) {
        int wifibox_bt_connect_post(void);
        wifibox_bt_connect_post();
    }
#endif

    if (bt_hci_event_filter(bt) == 0) {
        return 0;
    }

    switch (bt->event) {
    case HCI_EVENT_INQUIRY_COMPLETE:
        log_i(" HCI_EVENT_INQUIRY_COMPLETE \n");
#if TCFG_USER_EMITTER_ENABLE
        emitter_search_stop(bt->value);
#endif
        break;
    case HCI_EVENT_IO_CAPABILITY_REQUEST:
        log_i(" HCI_EVENT_IO_CAPABILITY_REQUEST \n");
        break;
    case HCI_EVENT_USER_CONFIRMATION_REQUEST:
        log_i(" HCI_EVENT_USER_CONFIRMATION_REQUEST \n");
        ///<可通过按键来确认是否配对 1：配对   0：取消
        bt_send_pair(1);
        break;
    case HCI_EVENT_USER_PASSKEY_REQUEST:
        log_i(" HCI_EVENT_USER_PASSKEY_REQUEST \n");
        ///<可以开始输入6位passkey
        break;
    case HCI_EVENT_USER_PRESSKEY_NOTIFICATION:
        log_i(" HCI_EVENT_USER_PRESSKEY_NOTIFICATION %x\n", bt->value);
        ///<可用于显示输入passkey位置 value 0:start  1:enrer  2:earse   3:clear  4:complete
        break;
    case HCI_EVENT_PIN_CODE_REQUEST :
        log_i("HCI_EVENT_PIN_CODE_REQUEST  \n");
        bt_send_pair(1);
        break;
    case HCI_EVENT_VENDOR_NO_RECONN_ADDR :
        log_i("HCI_EVENT_VENDOR_NO_RECONN_ADDR \n");
    case HCI_EVENT_DISCONNECTION_COMPLETE :
        log_i("HCI_EVENT_DISCONNECTION_COMPLETE \n");
#if TCFG_USER_EMITTER_ENABLE
        if (bt_emitter_disconnect()) {
            bt_wait_phone_connect_control(1);
        }
#else
#ifndef CONFIG_DUI_SDK_ENABLE
        bt_wait_phone_connect_control(1);
#else
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
#endif
#endif
        break;
    case BTSTACK_EVENT_HCI_CONNECTIONS_DELETE:
    case HCI_EVENT_CONNECTION_COMPLETE:
        log_i(" HCI_EVENT_CONNECTION_COMPLETE \n");
        switch (bt->value) {
        case ERROR_CODE_SUCCESS :
            log_i("ERROR_CODE_SUCCESS  \n");
            if (__this->auto_connection_timer) {
                sys_timeout_del(__this->auto_connection_timer);
                __this->auto_connection_timer = 0;
            }
            __this->auto_connection_counter = 0;
            bt_wait_phone_connect_control(0);
            user_send_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
            break;

        case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION   :
#ifdef CONFIG_WIFIBOX_ENABLE
            if (get_wbcp_connect_status()) {
                break;
            }
#endif
        case ERROR_CODE_PIN_OR_KEY_MISSING:
            log_i(" ERROR_CODE_PIN_OR_KEY_MISSING \n");
        case ERROR_CODE_SYNCHRONOUS_CONNECTION_LIMIT_TO_A_DEVICE_EXCEEDED :
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES:
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR:
        case ERROR_CODE_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED  :
        case ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST :
        case ERROR_CODE_AUTHENTICATION_FAILURE :
        case CUSTOM_BB_AUTO_CANCEL_PAGE:
#if TCFG_USER_EMITTER_ENABLE
            if (bt_emitter_disconnect()) {
                bt_wait_phone_connect_control(1);
            }
#else
            bt_wait_phone_connect_control(1);
#endif
            break;
        case ERROR_CODE_PAGE_TIMEOUT:
            log_i(" ERROR_CODE_PAGE_TIMEOUT \n");
            if (__this->auto_connection_timer) {
                sys_timer_del(__this->auto_connection_timer);
                __this->auto_connection_timer = 0;
            }
#if TCFG_USER_EMITTER_ENABLE
            int ret = bt_emitter_page_timeout();
            if (ret) {
                if (ret == 1) {
                    __this->enable = 1;
                }
                bt_wait_phone_connect_control(1);
            }
#else
            bt_wait_phone_connect_control(1);
#endif
            break;
        case ERROR_CODE_CONNECTION_TIMEOUT:
            log_i(" ERROR_CODE_CONNECTION_TIMEOUT \n");
#ifdef CONFIG_WIFIBOX_ENABLE
            if (get_wbcp_connect_status()) {
                break;
            }
#endif
            if (!get_remote_test_flag() && !get_esco_busy_flag()) {
                __this->auto_connection_counter = (TIMEOUT_CONN_TIME * 1000);
                memcpy(__this->auto_connection_addr, bt->args, 6);
                if (__this->auto_connection_timer) {
                    sys_timer_del(__this->auto_connection_timer);
                    __this->auto_connection_timer = 0;
                }
#if ((CONFIG_BT_MODE == BT_BQB)||(CONFIG_BT_MODE == BT_PER))
                bt_wait_phone_connect_control(1);
#else
                user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
                bt_wait_connect_and_phone_connect_switch(0);
#endif
                //user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6, bt->args);
            } else {
                bt_wait_phone_connect_control(1);
            }
            break;
        case ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS  :
            log_i("ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS   \n");
            if (!get_remote_test_flag() && !get_esco_busy_flag()) {
                __this->auto_connection_counter = (8 * 1000);
                memcpy(__this->auto_connection_addr, bt->args, 6);
                if (__this->auto_connection_timer) {
                    sys_timer_del(__this->auto_connection_timer);
                    __this->auto_connection_timer = 0;
                }
                bt_wait_connect_and_phone_connect_switch(0);
            } else {
                bt_wait_phone_connect_control(1);
            }
            break;
        default:
            break;
        }

        break;
    default:
        break;
    }

    return 0;
}

#if (TCFG_USER_EDR_ENABLE && SPP_TRANS_DATA_EN)
void bt_wait_phone_connect_control_ext(u8 inquiry_en, u8 page_scan_en)
{
    if (inquiry_en) {
        user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
    } else {
        user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
    }

    if (page_scan_en) {
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
    } else {
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
    }
}

static int bt_hci_spp_event_handler(struct bt_event *bt)
{
    //对应原来的蓝牙连接上断开处理函数  ,bt->value=reason
    log_i("------------------------bt_hci_event_handler reason %x %x", bt->event, bt->value);

    switch (bt->event) {
    case HCI_EVENT_INQUIRY_COMPLETE:
        log_i(" HCI_EVENT_INQUIRY_COMPLETE \n");
        /* bt_hci_event_inquiry(bt); */
        break;
    case HCI_EVENT_USER_CONFIRMATION_REQUEST:
        log_i(" HCI_EVENT_USER_CONFIRMATION_REQUEST \n");
        ///<可通过按键来确认是否配对 1：配对   0：取消
        bt_send_pair(1);
        break;
    case HCI_EVENT_USER_PASSKEY_REQUEST:
        log_i(" HCI_EVENT_USER_PASSKEY_REQUEST \n");
        ///<可以开始输入6位passkey
        break;
    case HCI_EVENT_USER_PRESSKEY_NOTIFICATION:
        log_i(" HCI_EVENT_USER_PRESSKEY_NOTIFICATION %x\n", bt->value);
        ///<可用于显示输入passkey位置 value 0:start  1:enrer  2:earse   3:clear  4:complete
        break;
    case HCI_EVENT_PIN_CODE_REQUEST :
        log_i("HCI_EVENT_PIN_CODE_REQUEST  \n");
        bt_send_pair(1);
        break;
    case HCI_EVENT_VENDOR_NO_RECONN_ADDR :
        log_i("HCI_EVENT_VENDOR_NO_RECONN_ADDR \n");
        bt_wait_phone_connect_control_ext(1, 1);
        break;
    case HCI_EVENT_DISCONNECTION_COMPLETE :
        log_i("HCI_EVENT_DISCONNECTION_COMPLETE \n");
        bt_wait_phone_connect_control_ext(1, 1);
        break;
    case BTSTACK_EVENT_HCI_CONNECTIONS_DELETE:
    case HCI_EVENT_CONNECTION_COMPLETE:
        log_i(" HCI_EVENT_CONNECTION_COMPLETE \n");
        switch (bt->value) {
        case ERROR_CODE_SUCCESS :
            log_i("ERROR_CODE_SUCCESS  \n");
            bt_wait_phone_connect_control_ext(0, 0);
            break;
        case ERROR_CODE_PIN_OR_KEY_MISSING:
            log_i(" ERROR_CODE_PIN_OR_KEY_MISSING \n");
        case ERROR_CODE_SYNCHRONOUS_CONNECTION_LIMIT_TO_A_DEVICE_EXCEEDED :
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES:
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR:
        case ERROR_CODE_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED  :
        case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION   :
        case ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST :
        case ERROR_CODE_AUTHENTICATION_FAILURE :
        case CUSTOM_BB_AUTO_CANCEL_PAGE:
            bt_wait_phone_connect_control_ext(1, 1);
            break;
        case ERROR_CODE_PAGE_TIMEOUT:
            log_i(" ERROR_CODE_PAGE_TIMEOUT \n");
            bt_wait_phone_connect_control_ext(1, 1);
            break;
        case ERROR_CODE_CONNECTION_TIMEOUT:
            log_i(" ERROR_CODE_CONNECTION_TIMEOUT \n");
            bt_wait_phone_connect_control_ext(1, 1);
            break;
        case ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS  :
            log_i("ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS   \n");
            bt_wait_phone_connect_control_ext(1, 1);
            break;
        default:
            break;
        }
        break;
    default:
        break;

    }

    return 0;
}
#endif

static int bt_music_decode_event_handler(struct bt_event *bt)
{
    switch (bt->event) {
    case BT_MUSIC_DEC_EVENT_VOL_SET:
        if (__this->dec_server) {
            union audio_req req = {0};
            req.dec.cmd     = AUDIO_DEC_SET_VOLUME;
            req.dec.volume  = bt->value;
            server_request(__this->dec_server, AUDIO_REQ_DEC, &req);
        }
        break;
    case BT_MUSIC_DEC_EVENT_MUTE_SET:
        if (__this->dec_server) {
            puts("bt set no mute\n");
            set_app_music_dec_ops(get_bt_music_dec_ops());
            __this->mute = 0;
            union audio_req req = {0};
            req.dec.cmd = AUDIO_DEC_DIGITAL_GAIN_SET;
            req.dec.digital_gain_mul = 0;
            req.dec.digital_gain_div = 0;
            return server_request(__this->dec_server, AUDIO_REQ_DEC, &req);
        }
        break;
    default:
        break;
    }

    return 0;
}

int app_music_bt_event_handler(struct sys_event *event)
{
    if (event->from == BT_EVENT_FROM_CON) {
        return bt_connction_status_event_handler((struct bt_event *)event->payload);
    } else if (event->from == BT_EVENT_FROM_HCI) {
#if (TCFG_USER_EDR_ENABLE && SPP_TRANS_DATA_EN)
        /* return bt_hci_spp_event_handler((struct bt_event *)event->payload); */
#endif
        return bt_hci_event_handler((struct bt_event *)event->payload);
    } else if (event->from == BT_EVENT_FROM_USER) {
        return bt_music_decode_event_handler((struct bt_event *)event->payload);
    }
    return false;
}

static const struct music_dec_ops bt_music_dec_ops;

static int bt_music_dec_play_pause(u8 notify)
{
#if TCFG_USER_EMITTER_ENABLE
    if (bt_emitter_role_get() == BT_EMITTER_EN) {
        return bt_emitter_stu_sw();
    }
#endif
    if (__this->phone_ring_flag) {
        user_send_cmd_prepare(USER_CTRL_HFP_CALL_ANSWER, 0, NULL);
    } else if (__this->call_flag) {
        user_send_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
    } else {
        puts("bt_music_dec_play\n");
        if (notify) {
            user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
        }
    }

    return 0;
}

static int bt_music_dec_breakpoint(int priv)
{
    union audio_req req = {0};

    puts("bt_music_dec_play_breakpoint\n");

    if (__this->dec_server) {
        req.dec.cmd = AUDIO_DEC_DIGITAL_GAIN_SET;
        req.dec.digital_gain_mul = 0;
        req.dec.digital_gain_div = 0;
        return server_request(__this->dec_server, AUDIO_REQ_DEC, &req);
    }

    return 0;
}

static int bt_music_dec_stop(int save_breakpoint)
{
    union audio_req req = {0};

    puts("bt_music_dec_play_stop\n");

    if (__this->dec_server && save_breakpoint >= 0) {
        req.dec.cmd = AUDIO_DEC_DIGITAL_GAIN_SET;
        req.dec.digital_gain_mul = 0xff;
        req.dec.digital_gain_div = 0xff;
        return server_request(__this->dec_server, AUDIO_REQ_DEC, &req);
    }

    if (save_breakpoint == -1 && __this->media_play_flag) {
        /* user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_STOP, 0, NULL); */
        user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_PAUSE, 0, NULL);
        os_time_dly(10);
        __this->mute = 1;
    }

    return 0;
}

static int bt_music_dec_switch_file(int fsel_mode)
{
    puts("bt_music_dec_switch_file\n");

    if (!__this->call_flag) {
        if (fsel_mode == FSEL_NEXT_FILE) {
            user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_NEXT, 0, NULL);
        } else if (fsel_mode == FSEL_PREV_FILE) {
            user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_PREV, 0, NULL);
        }
    }

    return 0;
}

static int bt_music_dec_volume(int step)
{
    union audio_req req = {0};

    if (__this->media_play_flag) {
        if (step > 0) {
            user_send_cmd_prepare(USER_CTRL_CMD_SYNC_VOL_INC, 0, NULL);
        } else {
            user_send_cmd_prepare(USER_CTRL_CMD_SYNC_VOL_DEC, 0, NULL);
        }
        if (__this->dec_server) {
            req.dec.cmd     = AUDIO_DEC_SET_VOLUME;
            req.dec.volume  = get_app_music_volume();
            return server_request(__this->dec_server, AUDIO_REQ_DEC, &req);
        }
#if 0
    } else if (__this->call_flag) {
        if (step > 0) {
            user_send_cmd_prepare(USER_CTRL_HID_VOL_UP, 0, NULL);
        } else {
            user_send_cmd_prepare(USER_CTRL_HID_VOL_DOWN, 0, NULL);
        }
        if (__this->dec_server) {
            req.dec.cmd     = AUDIO_DEC_SET_VOLUME;
            req.dec.volume  = __this->call_volume;
            return server_request(__this->dec_server, AUDIO_REQ_DEC, &req);
        }
#endif
    }

    return 0;
}

static int bt_music_get_dec_status(int priv)
{
#if 1
    return __this->media_play_flag ? AUDIO_DEC_START : AUDIO_DEC_PAUSE;
#else
    union audio_req req = {0};
    req.dec.cmd     = AUDIO_DEC_GET_STATUS;
    server_request(__this->dec_server, AUDIO_REQ_DEC, &req);

    return req.dec.status == AUDIO_DEC_START ? AUDIO_DEC_START : AUDIO_DEC_PAUSE;
#endif
}

static const struct music_dec_ops bt_music_dec_ops = {
    .switch_dir     = NULL,
    .switch_file    = bt_music_dec_switch_file,
    .dec_file       = NULL,
    .dec_breakpoint = bt_music_dec_breakpoint,
    .dec_play_pause = bt_music_dec_play_pause,
    .dec_volume     = bt_music_dec_volume,
    .dec_progress   = NULL,
    .dec_stop       = bt_music_dec_stop,
    .dec_seek       = NULL,
    .dec_status     = bt_music_get_dec_status,
};

const struct music_dec_ops *get_bt_music_dec_ops(void)
{
    return &bt_music_dec_ops;
}

#endif
