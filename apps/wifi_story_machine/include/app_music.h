#ifndef _APP_MUSIC_H
#define _APP_MUSIC_H

#include "system/includes.h"
#include "app_config.h"
#include "server/audio_server.h"

struct music_dec_ops {
    int (*switch_dir)(int fsel_mode);
    int (*switch_file)(int fsel_mode);
    int (*dec_file)(void *file, int, void *, int);
    int (*dec_play_pause)(u8 notify);
    int (*dec_breakpoint)(int);
    int (*dec_stop)(int);
    int (*dec_volume)(int);
    int (*dec_progress)(int);
    int (*dec_seek)(int);
    int (*dec_status)(int);
};

struct net_breakpoint {
    u32 fptr;
    u16 url_len;
    char *url;
    const char *ai_name;
    struct audio_dec_breakpoint dec_bp;
};

enum {
    PICTURE_DET_EXIT_MODE = 0,
    PICTURE_DET_ENTER_MODE = 1,
};

enum {
    NET_MUSIC_MODE = 0, //网络播放模式，包括云识别还有绘本识别
    BT_MUSIC_MODE,      //蓝牙播放模式，包括媒体播放和打电话
    AUX_MUSIC_MODE,     //LINEIN播放模式
    LOCAL_MUSIC_MODE,   //本地flash播放
    SDCARD_MUSIC_MODE,  //sd卡播放
    UDISK_MUSIC_MODE,   //u盘播放
    FM_MUSIC_MODE,      //FM电台播放
};

enum {
    LOOP_PLAY_MODE = 0, //循环播放模式
    SINGLE_PLAY_MODE,   //单曲播放模式
    RANDOM_PLAY_MODE,   //随机播放模式
};

#ifdef CONFIG_UI_ENABLE
enum {
    UI_MSG_LAST_SONG = 1, //上一首
    UI_MSG_PAUSE_START,   //播放开
    UI_MSG_PAUSE_STOP,    //播放关
    UI_MSG_NEXT_SONG,     //下一首
    UI_MSG_SET_VOLUME,    //设置音量
    UI_MSG_SONG_NAME,     //显示歌曲名字
    UI_MSG_SD_LOG,        //SA卡图标状态
    UI_MSG_POWER_LOG,     //电池电量
    UI_MSG_ALL_TIME,      //播放总时间
    UI_MSG_CURRENT_TIME,  //当前播放时间
    UI_MSG_STARTUP_LOG,   //开机图片显示
    UI_MSG_UPDATE,        //ui升级
    UI_MSG_SHOW_PERSENT,  //ui显示百分比
    UI_MSG_UPGRADE_FAIL,  //ui升级失败
};
#endif //CONFIG_UI_ENABLE

typedef enum {
    /*01*/    APP_LOCAL_PROMPT_POWER_ON = 1,           //开机提示音
    /*02*/    APP_LOCAL_PROMPT_POWER_OFF,              //关机提示音
    /*03*/    APP_LOCAL_PROMPT_PLAY_DOMAIN_MUSIC,      //领域播放：儿歌
    /*04*/    APP_LOCAL_PROMPT_PLAY_DOMAIN_STORY,      //领域播放：故事
    /*05*/    APP_LOCAL_PROMPT_PLAY_DOMAIN_SINOLOGY,   //领域播放：国学
    /*06*/    APP_LOCAL_PROMPT_PLAY_DOMAIN_ENGLISH,    //领域播放：英语
    /*07*/    APP_LOCAL_PROMPT_BT_OPEN,                //打开蓝牙
    /*08*/    APP_LOCAL_PROMPT_BT_CLOSE,               //关闭蓝牙
    /*09*/    APP_LOCAL_PROMPT_BT_CONNECTED,           //蓝牙连接成功
    /*10*/    APP_LOCAL_PROMPT_BT_DISCONNECT,          //蓝牙断开连接
    /*11*/    APP_LOCAL_PROMPT_WIFI_CONNECTING,        //wifi正在连接
    /*12*/    APP_LOCAL_PROMPT_WIFI_CONNECT_SUCCESS,   //wifi连接成功
    /*13*/    APP_LOCAL_PROMPT_WIFI_CONNECT_FAIL,      //wifi连接失败
    /*14*/    APP_LOCAL_PROMPT_WIFI_DISCONNECT,        //wifi断开连接
    /*15*/    APP_LOCAL_PROMPT_WIFI_CONFIG_START,      //进入配网模式
    /*16*/    APP_LOCAL_PROMPT_WIFI_CONFIG_INFO_RECV,  //配网信息接收成功
    /*17*/    APP_LOCAL_PROMPT_WIFI_FIRST_CONFIG,      //提示进行第一次配网
    /*18*/    APP_LOCAL_PROMPT_WIFI_CONFIG_TIMEOUT,    //配网超时
    /*19*/    APP_LOCAL_PROMPT_LIGHT_OPEN,             //打开灯光
    /*20*/    APP_LOCAL_PROMPT_LIGHT_CLOSE,            //关闭灯光
    /*21*/    APP_LOCAL_PROMPT_AI_ASR_FAIL,            //服务器暂未连接成功，云端识别请求失败
    /*22*/    APP_LOCAL_PROMPT_AI_TRANSLATE_FAIL,      //服务器暂未连接成功，翻译请求失败
    /*23*/    APP_LOCAL_PROMPT_AI_PICTURE_FAIL,        //服务器暂未连接成功，绘本请求失败
    /*24*/    APP_LOCAL_PROMPT_SEND_MESSAGE_FAIL,      //服务器暂未连接成功，留言发送失败
    /*25*/    APP_LOCAL_PROMPT_RECV_NEW_MESSAGE,       //接收到新的留言信息
    /*26*/    APP_LOCAL_PROMPT_MESSAGE_TOO_SHORT,      //发送的留言过短
    /*27*/    APP_LOCAL_PROMPT_MESSAGE_EMPTY,          //没有新的留言信息
    /*28*/    APP_LOCAL_PROMPT_LOW_BATTERY_LEVEL,      //电池电量不足
    /*29*/    APP_LOCAL_PROMPT_LOW_BATTERY_SHUTDOWN,   //低电关机
    /*30*/    APP_LOCAL_PROMPT_ENTER_AI_TRANSLATE_MODE,//进入翻译模式
    /*31*/    APP_LOCAL_PROMPT_ENTER_AI_ASR_MODE,      //进入云端识别模式
    /*32*/    APP_LOCAL_PROMPT_ENTER_AI_PICTURE_MODE,  //进入绘本识别模式
    /*33*/    APP_LOCAL_PROMPT_EXIT_AI_PICTURE_MODE,   //退出绘本识别模式
    /*34*/    APP_LOCAL_PROMPT_PICTURE_RECOGNIZE_FAIL, //绘本识别失败
    /*35*/    APP_LOCAL_PROMPT_SERVER_CONNECTED,       //服务器连接成功
    /*36*/    APP_LOCAL_PROMPT_SERVER_DISCONNECT,      //服务器连接断开
    /*37*/    APP_LOCAL_PROMPT_ASR_EMPTY,              //ASR识别结果为空，提示再说一遍
    /*38*/    APP_LOCAL_PROMPT_KWS_ENABLE,             //打开打断唤醒功能
    /*39*/    APP_LOCAL_PROMPT_KWS_DISABLE,            //关闭打断唤醒功能
    /*40*/    APP_LOCAL_PROMPT_REVERB_ENABLE,          //进入混响模式
    /*41*/    APP_LOCAL_PROMPT_REVERB_DISABLE,         //退出混响模式
    /*42*/    APP_LOCAL_PROMPT_ENTER_NETWORK_MODE,     //进入网络媒体播放模式
    /*43*/    APP_LOCAL_PROMPT_ENTER_FLASH_MUSIC_MODE, //进入FLASH音乐播放模式
    /*44*/    APP_LOCAL_PROMPT_ENTER_SDCARD_MUSIC_MODE,//进入SD卡音乐播放模式
    /*45*/    APP_LOCAL_PROMPT_ENTER_UDISK_MUSIC_MODE, //进入U盘音乐播放模式
    /*46*/    APP_LOCAL_PROMPT_ENTER_FM_MUSIC_MODE,    //进入FM音乐播放模式
    /*47*/    APP_LOCAL_PROMPT_PHONE_RING,             //来电提示
    /*48*/    APP_LOCAL_PROMPT_AI_LISTEN_START,        //开始云端识别
    /*49*/    APP_LOCAL_PROMPT_AI_LISTEN_STOP,         //结束云端识别
    /*50*/    APP_LOCAL_PROMPT_VOLUME_SET,             //设置音量
    /*51*/    APP_LOCAL_PROMPT_VOLUME_FULL,            //音量已经设置最大值
    /*52*/    APP_LOCAL_PROMPT_DEVICE_BIND_SUCCESS,    //设备绑定成功
    /*53*/    APP_LOCAL_PROMPT_DEVICE_BIND_FAIL,       //设备绑定失败
    /*54*/    APP_LOCAL_PROMPT_DEVICE_UNBIND_DEFAULT,  //设备解绑
    /*55*/    APP_LOCAL_PROMPT_DEVICE_UNBIND_SUCCESS,  //设备解绑成功
    /*56*/    APP_LOCAL_PROMPT_DEVICE_UNBIND_FAIL,     //设备解绑失败
    /*57*/    APP_LOCAL_PROMPT_DEVICE_INFO_RESTORE,    //恢复出厂设置
    /*58*/    APP_LOCAL_PROMPT_DEVICE_RESTART,         //设备重启
    /*59*/    APP_LOCAL_PROMPT_OTA_UPGRADE_START,      //OTA升级开始
    /*60*/    APP_LOCAL_PROMPT_OTA_UPGRADE_SUCCESS,    //OTA升级成功
    /*61*/    APP_LOCAL_PROMPT_OTA_UPGRADE_FAIL,       //OTA升级失败
    /*62*/    APP_LOCAL_PROMPT_ALARM,                  //闹铃
    /*63*/    APP_LOCAL_PROMPT_SCHEDULE,               //时刻表
    /*64*/    APP_LOCAL_PROMPT_FORMAT_NOT_SUPPORT,     //播放器不支持的媒体格式
    /*65*/    APP_LOCAL_PROMPT_SEEK_UNSUPPORTED,       //不支持快进快退的本地提示音
    /****/    APP_LOCAL_PROMPT_NUMM,
} APP_LOCAL_PROMPT_TYPE_E;

struct app_music_hdl {
#ifdef CONFIG_NET_ENABLE
    u8 rec_again : 1;
    u8 net_connected : 1;
    u8 picture_det_mode : 1;
    u8 wifi_config_state : 1;
    u8 ai_connected : 1;
    u8 upgrading : 1;
    u8 wait_download_suspend : 1;
    u8 wechat_flag : 1;

    u8 listening : 2;
    u8 play_tts : 1;
    u8 reconnecting : 1;
    u8 net_cfg_fail : 1;
    u8 net_connected_frist : 1;
    u8 _net_dhcp_ready : 1;
    u8 reserve : 1;
    u8 voice_mode;
    u8 wechat_url_num;
    u16 wait_download;
    u16 coexistence_timer;
    u16 cancel_dns_timer;
    int download_ready;
    void *net_file;
    void *save_url;
    const char *url;
    const char *ai_name;
    struct server *ai_server;
    struct net_breakpoint net_bp;
    struct list_head wechat_list;
#endif

    u8 mode;
    u8 last_mode;

    u8 led_mode : 4;
    u8 last_play_status : 2;
    u8 force_switch_file : 1;
    u8 bt_connectted_prompt_playing_flag : 1;

    u8 bt_emitter_enable : 1;
    u8 led_breath_on : 1;
    u8 mute : 1;
    u8 local_play_all : 1;	//全盘播放
    u8 local_play_mode : 2;
    u8 last_local_play_mode : 2;

    u8 play_voice_prompt : 1;
    u8 bt_music_enable : 1;
    u8 reverb_enable : 1;
    u8 wakeup_support : 1;
    u8 call_flag : 1;
    u8 bt_connecting : 1;
    u8 key_disable : 2;

    u8 light_intensity;
    u8 battery_per;
    u8 media_dev_online;	//bit0:sd_card   bit1:udisk   bit2:card_reader   bit7:first_dec_success
    char volume;
    u16 wait_sd;
    u16 wait_udisk;
    int udisk_upgrade_timer;
    int seek_step;
    int play_time;
    int save_play_time;
    int total_time;
    int save_total_time;
    int wait_switch_file;
    void *cmp_file;
    FILE *local_bp_file;
    FILE *file;
    int dec_end_args[4];
    void *dec_end_file;
    void *dec_end_handler;
    struct vfscan *fscan;
    struct vfscan *dir_list;
    struct server *dec_server;
    struct audio_dec_breakpoint local_bp;
    const char *local_path;
    const struct music_dec_ops *dec_ops;
    struct server *led_ui;
    void *eq_hdl;
    void *digital_vol_hdl;
};

void set_app_music_volume(int volume, u8 mode);
int get_app_music_volume(void);
int get_app_music_playtime(void);
int get_app_music_total_time(void);
int get_app_music_light_intensity(void);
int get_app_music_battery_power(void);

#endif

