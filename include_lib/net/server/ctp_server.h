#ifndef  __CTP_SERVER_H__
#define  __CTP_SERVER_H__


#include <stdarg.h>
//#include "server/server_core.h"
#include "ctp.h"
#include "cdp.h"
#include "generic/typedef.h"
#define CTP_PUT_COMMAND     0x1
#define CTP_GET_COMMAND     0x2
#define CTP_NOTIFY_COMMAND  0x3
#define CDP_NOTIFY_COMMAND  0x4

enum {
    CTP_NO_ERR = 0x0,
    CTP_SDCARD,         // SD卡错误
    CTP_SD_OFFLINE,     // SD卡离线
    CTP_ACCESS_RFU,     // 拒绝访问
    CTP_REQUEST,        // 请求错误
    CTP_VER_UMATCH,     // 版本不匹配
    CTP_NO_TOPIC,       // Topic未实现
    CTP_IN_USB,         // 正处于USB模式
    CTP_IN_VIDEO,       // 正在录像
    CTP_IN_BROWSE,      // 正在浏览模式
    CTP_IN_PARKING,     // 正在停车
    CTP_OPEN_FILE,      // 打开文件失败
    CTP_SYS_EXCEP,      // 系统异常
    CTP_SET_PRARM,      // 设置参数失败
    CTP_NET_ERR,        // 网络异常
    CTP_PULL_OFFLINE,    //   后拉不在线
    CTP_PULL_NOSUPPORT,  //后拉不支持
    CTP_RT_OPEN_FAIL,     //实时流打开失败
    CTP_XX_XXXXX  = 0xff,
};
#define CTP_NO_ERR_MSG         "\"msg\":\"CTP_NO_ERR\""
#define CTP_SDCARD_MSG         "\"msg\":\"CTP_SDCARD_MSG\""    // SD卡错误
#define CTP_SD_OFFLINE_MSG     "\"msg\":\"CTP_SD_OFFLINE_MSG\""  // SD卡离线
#define CTP_ACCESS_RFU_MSG     "\"msg\":\"CTP_ACCESS_RFU_MSG\"" // 拒绝访问
#define CTP_REQUEST_MSG        "\"msg\":\"CTP_REQUEST_MSG\"" // 请求错误
#define CTP_VER_UMATCH_MSG     "\"msg\":\"CTP_VER_UMATCH_MSG\""  // 版本不匹配
#define CTP_NO_TOPIC_MSG       "\"msg\":\"CTP_NO_TOPIC_MSG\""  // Topic未实现
#define CTP_IN_USB_MSG         "\"msg\":\"CTP_IN_USB_MSG\""   // 正处于USB模式
#define CTP_IN_VIDEO_MSG       "\"msg\":\"CTP_IN_VIDEO_MSG\""   // 正在录像
#define CTP_IN_BROWSE_MSG      "\"msg\":\"CTP_IN_BROWSE_MSG\""   // 正在浏览模式
#define CTP_IN_PARKING_MSG     "\"msg\":\"CTP_IN_PARKING_MSG\""  // 正在停车
#define CTP_OPEN_FILE_MSG      "\"msg\":\"CTP_OPEN_FILE_MSG\""   // 打开文件失败
#define CTP_SYS_EXCEP_MSG      "\"msg\":\"CTP_SYS_EXCEP_MSG\""   // 系统异常
#define CTP_SET_PRARM_MSG      "\"msg\":\"CTP_SET_PRARM_MSG\""    // 设置参数失败
#define CTP_NET_ERR_MSG        "\"msg\":\"CTP_NET_ERR_MSG\""    // 网络异常
#define CTP_PULL_OFFLINE_MSG     "\"msg\":\"CTP_PULL_OFFLINE\""     //   后拉不在线
#define CTP_PULL_NOSUPPORT_MSG   "\"msg\":\"CTP_PULL_NOSUPPORT\""   //  后拉不支持
#define CTP_RT_OPEN_FAIL_MSG       "\"msg\":\"CTP_RT_OPEN_FAIL\""   // 实时流打开失败





/*
{
	  "errno":0,        // 错误返回，若无可省略此字段
	    "ct":[
	      "key_0":value0,    //操作类型，value0包括“PUT","GET","NOTIFY"
    "key_1":value1,    // value1-n皆对应相应参数，若无可省略相关字段
    "key_2":value2,
    "key_3":value3,
    "key_4":value4,
    ......
	  ]
}
*/



#define PARM_MAX 8
struct ctp_req {
    u32 parm_count;
    void *cli;
    const char *topic;
    char *parm;
};

struct ctp_arg { //为了sys_event_notify
    void *cli;
    char topic[64];
    char *content;
    int num;//app_accept_num
    void *dest_addr;
};



enum keep_alive_type {
    NOT_USE_ALIVE = 0x0,
    CTP_ALIVE = 0x1,
    CDP_ALIVE = 0x2,
};
struct ctp_server_info {
    u8 ctp_vaild; //是否开启CTP协议
    u16 ctp_port;//CTP端口
    u8 cdp_vaild;// 是否开启CDP协议
    u16 cdp_port; //CDP端口
    enum keep_alive_type k_alive_type;
};


struct ctp_map_entry {
    const char *dev_cmd;
    const char *ctp_command;
    int (*get)(void *, char *);
    int (*put)(void *, char *);
    u8 sync; //防止APP瞬发多个CTP命令
};


//总
extern  struct ctp_map_entry ctp_mapping_tab_begin[], ctp_mapping_tab_end[];
//系统
extern  struct ctp_map_entry ctp_mapping_tab_system_cmd_begin[], ctp_mapping_tab_system_cmd_end[];
//video
extern  struct ctp_map_entry ctp_mapping_tab_video_cmd_begin[], ctp_mapping_tab_video_cmd_end[];
//photo
extern  struct ctp_map_entry ctp_mapping_tab_photo_cmd_begin[], ctp_mapping_tab_photo_cmd_end[];


#define list_for_ctp_mapping_tab(p) \
    for (p=ctp_mapping_tab_begin; p<ctp_mapping_tab_end; p++)

#define list_for_ctp_system_tab(p) \
    for (p=ctp_mapping_tab_system_cmd_begin; p<ctp_mapping_tab_system_cmd_end; p++)


#define list_for_ctp_video_tab(p) \
    for (p=ctp_mapping_tab_video_cmd_begin; p<ctp_mapping_tab_video_cmd_end; p++)


#define list_for_ctp_photo_tab(p) \
    for (p=ctp_mapping_tab_photo_cmd_begin; p<ctp_mapping_tab_photo_cmd_end; p++)




int ctp_cmd_analysis(const char *topic, char *content, void *priv);
struct server *get_ctp_server_hander();


void __find_get_system_cmd_run(const char *topic);
void __find_get_video_cmd_run(const char *topic);
void __find_get_photo_cmd_run(const char *topic);
int CTP_CMD_COMBINED(void *priv, u32 err, const char *_req, const char *mothod, char *str);   //第一参数：topic
int CTP_ERR(int err); //第一参数：topic
void CTP_CMD_RELEASE(char *buf);


#endif  /*CTP_SERVER_H*/
