#ifndef __VTTYPES_H__
#define __VTTYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

enum e_vt_httpMethod {GET = 0, POST};
typedef unsigned char uint8_t;
typedef char byte;
typedef unsigned short uint16_t;

/*vt_bk_get_recongstate()'s return value*/
#define VT_BK_STATE_STARTED  1
#define VT_BK_STATE_REQUEST_STOP   2
#define VT_BK_STATE_STOPED   3
#define VT_BK_STATE_PAUSE  4

/*vt_bk_register_error_cb errocode cb*/
typedef enum {
    //服务器识别错误码
    E_VTSTORYBK_RECOGNIZE_RESPONSE_WRONG_FAILED = -1310,    //识别JSON信息不全，识别错误
    E_VTSTORYBK_SERVER_RESPONSE_ERROR_10000 = -10000,       //找不到图片对应的书本信息
    E_VTSTORYBK_SERVER_RESPONSE_ERROR_10001 = -10001,       //找不到对应页信息
    E_VTSTORYBK_SERVER_RESPONSE_ERROR_10002 = -10002,       //找不到图片ID对应的文件
    E_VTSTORYBK_SERVER_RESPONSE_ERROR_10004 = -10004,       //模型不存在,识别失败
    E_VTSTORYBK_SERVER_RESPONSE_ERROR_10007 = -10007,       //识别失败
    E_VTSTORYBK_SERVER_RESPONSE_ERROR_10008 = -10008,       //识别文字失败
    E_VTSTORYBK_SERVER_RESPONSE_ERROR_10009 = -10009,       //识别结果图片ID不正确
    E_VTSTORYBK_SERVER_RESPONSE_ERROR_10010 = -10010,       //识别服务正在更新数据，再重试
    E_VTSTORYBK_SERVER_RESPONSE_ERROR_10011 = -10011,       //图片识别异常
    E_VTSTORYBK_SERVER_RESPONSE_ERROR_10012 = -10012,       //该书被禁用
    E_VTSTORYBK_SERVER_RESPONSE_ERROR_10013 = -10013,       //没有这本书
    E_VTSTORYBK_SERVER_RESPONSE_ERROR_10014 = -10014,       //书本对应的资源不存在
    E_VTSTORYBK_SERVER_RESPONSE_ERROR_10015 = -10015,       //上传图片格式不正确
    E_VTSTORYBK_SERVER_RESPONSE_ERROR_10016 = -10016,       //读取请求的图片异常
    E_VTSTORYBK_SERVER_RESPONSE_ERROR_10017 = -10017,       //FDS服务异常
} E_VTSTORYBK_ERRORCODE;

typedef enum {
    /*TASK 退出了状态*/
    E_VT_TASKS_EXITED = 0,
    /*TASK running 状态*/
    E_VT_TASKS_RUNNING = 1,
} VT_OS_TASK_STATE;

typedef enum {
    /*播放完成*/
    E_VTPLAYER_STATE_COMPLETE = 0,
    /*播放中*/
    E_VTPLAYER_STATE_PLAYING,
    /*暂停状态*/
    E_VTPLAYER_STATE_PAUSED,
    /*停止状态*/
    E_VTPLAYER_STATE_STOPED,
    /*播放出错*/
    E_VTPLAYER_STATE_ERROR,
} vt_mediaplayer_state;

typedef enum {
    E_COVER_PAGE = 1,
    E_TEXT_PAGE = 5,
    E_BACK_COVER_PAGE = 8,
} VT_RECONGNIZE_PAGE_TYPE;

/*网络请求参数*/
typedef struct _network_parameter {
    char *url;  /*待请求的地址已经携带token和相关参数，外部网络请求函数还需要设置cookie, cookie字段名可看Linux的适配DEMO*/
    enum e_vt_httpMethod method;    /*GET/POST*/
    int timeout;    /*超时时长*/
    void *mime_data;    /*mime数据*/
    unsigned int data_size; /*mime数据的长度*/
    char mime_type[16]; /*mime数据的类型*/
    char mime_filename[32]; /*mime数据在服务器端的文件名*/
    char mime_name[16]; /*mime数据在表单中的名称*/
} str_vt_networkPara;

/*http respond data, 外部申请相应内存存放网络返回的数据。SDK是调用方负责释放该内存,外部不需要再释放*/
typedef struct _network_respond {
    void *data;
    unsigned int size;
} str_vt_networkRespond;

/*param:网络请求的参数， respond:请求返回后的数据*/
typedef int (*pfun_vt_httpRequest_cb)(str_vt_networkPara *param, str_vt_networkRespond *respond);

/*TASK 主体函数, SDK 调用启动TASK接口注册到外部，由外部TASK线程体调用*/
typedef int (*pfun_vt_task_cb)(void *ctx, VT_OS_TASK_STATE state);

/*SDK 启动TASK接口， 外部实现注册进来SDK用来启动task, ctx为SDK传入的用户指针，调用pfun_vt_task_cb需要传回来*/
typedef int (*pfun_vt_start_task_cb)(pfun_vt_task_cb taskcb, void *ctx);

/*出错回调函数吗，出错后SDK调用到通知外部*/
typedef int (*pfun_vt_error_cb)(int errorCode, void *userp);

typedef struct vt_camera_param {
    /*预览宽度*/
    int m_preview_width;
    /*预览高度*/
    int m_preview_height;
    /*JPG图像宽度*/
    int m_picture_width;
    /*JPG图像高度*/
    int m_picture_height;
    /*帧率*/
    uint8_t m_fps;
    /*翻转模式*/
    uint8_t m_rotation;
    /*JPG图像质量*/
    uint8_t m_quality;
} vt_camera_param_t;

/*camera ops*/
/*预览回调函数*/
typedef void (*pfun_camera_preview_cb)(void *data, int size, void *userp);

/*拍照回调函数*/
typedef void (*pfun_camera_picture_cb)(void *data, int size, void *userp);

/*camera open 函数*/
typedef void *(*pfun_camera_open)();

/*camera 释放 函数， 释放后其他模块可操作*/
typedef int (*pfun_camera_release)(void *dev);

/*camera 设置参数 函数*/
typedef int (*pfun_camera_set_params)(void *dev, vt_camera_param_t *params);

/*camera 获取参数 函数*/
typedef int (*pfun_camera_get_params)(void *dev, vt_camera_param_t *params);

/*camera 设置预览回调 函数*/
typedef int (*pfun_camera_set_preview_cb)(void *dev, pfun_camera_preview_cb preview_cb, void *userp);

/*camera 设置拍照回调 函数*/
typedef int (*pfun_camera_set_picture_cb)(void *dev, pfun_camera_picture_cb picture_cb, void *userp);

/*camera 开始预览 函数*/
typedef int (*pfun_camera_start_preview)(void *dev);

/*camera 停止预览 函数*/
typedef int (*pfun_camera_stop_preview)(void *dev);

/*camera 拍照 函数*/
typedef int (*pfun_camera_take_picture)(void *dev);

/*player ops*/
/*播放状态回调函数接口， 由播放器回调通知SDK*/
typedef int (*pfun_vt_player_state_cb)(char *url, vt_mediaplayer_state state, void *userp);

/*播放器设置播放URL 函数*/
typedef int (*pfun_vt_player_init_url)(char *url);

/*播放器播放 函数*/
typedef int (*pfun_vt_player_play)();

/*播放器停止播放 函数*/
typedef int (*pfun_vt_player_stop)();

/*播放器暂停播放 函数*/
typedef int (*pfun_vt_player_pause)();

/*播放器恢复播放 函数*/
typedef int (*pfun_vt_player_resume)();

/*获取是否正在播放 函数， 必须要实现*/
typedef int (*pfun_vt_player_ispalying)(char *url);

/*播放器设置播放状态回调 函数*/
typedef int (*pfun_vt_player_set_state_cb)(pfun_vt_player_state_cb state_cb, void *userp);

/*camera 操作接口*/
typedef struct camera_ops {
    pfun_camera_open camera_open;
    pfun_camera_release camera_release;
    pfun_camera_set_params camera_set_params;
    pfun_camera_get_params camera_get_params;
    pfun_camera_set_preview_cb camera_set_preview_cb;
    pfun_camera_set_picture_cb camera_set_picture_cb;
    pfun_camera_start_preview camera_start_preview;
    pfun_camera_stop_preview  camera_stop_preview;
    pfun_camera_take_picture  camera_take_picture;
    pfun_vt_start_task_cb     camera_start_task;
} camera_ops_t;

/*mediaplayer 操作接口*/
typedef struct media_player_ops {
    pfun_vt_player_init_url init;
    pfun_vt_player_play play;
    pfun_vt_player_stop stop;
    pfun_vt_player_pause pause;
    pfun_vt_player_resume resume;
    pfun_vt_player_ispalying playing;
    pfun_vt_player_set_state_cb set_state_cb;
    pfun_vt_start_task_cb player_start_task;
} media_player_ops_t;


//cond event
typedef void *(*vt_create_event)(uint8_t process_shared); /*创建 事件event, process_shared[是否进程共享Linux需要实现] 返回一个事件实例*/

typedef int (*vt_post_event)(void *event);   /*触发通知event*/

typedef int (*vt_wait_event)(void *event, void *mutex, int time_ms);   /*等待event, time_ms【等待的毫秒数，如果为-1则阻塞等待】*/

typedef int (*vt_destory_event)(void *event);   /*销毁事件event*/


//semaphone event
typedef void *(*vt_create_semaphone)(uint8_t process_shared, int max_count, int init_count); /*创建 信号量, process_shared[是否进程共享Linux需要实现] 返回一个信号量实例*/

typedef int (*vt_post_semaphone)(void *semaphone);   /*发送信号量+1*/

typedef int (*vt_wait_semaphone)(void *semaphone, int time_ms);   /*等待信号量 time_ms【等待的毫秒数，如果为-1则阻塞等待】*/

typedef int (*vt_destory_semaphone)(void *semaphone);   /*销毁信号量*/


//mutex
typedef void *(*vt_create_mutex)(uint8_t process_shared); /*创建互斥锁 process_shared[是否进程共享Linux需要实现] 返回一个锁实例*/

typedef int (*vt_mutex_lock)(void *mutex);   /*锁定*/

typedef int (*vt_mutex_unlock)(void *mutex);   /*解锁*/

typedef int (*vt_destory_mutex)(void *mutex);   /*销毁锁*/


//timer
typedef void (*vt_timer_cb)(void *timer);  /*定时器回调函数*/

typedef void *(*vt_create_timer)(vt_timer_cb cb, int period_ms, uint8_t one_short); /*创建 定时器, process_shared[是否进程共享Linux需要实现] 返回一个定时器实例*/

typedef int (*vt_start_timer)(void *timer);   /*开始定时器*/

typedef int (*vt_stop_timer)(void *timer);   /*停止定时器*/

typedef int (*vt_delete_timer)(void *timer);   /*销毁定时器*/


//message queue
typedef void *(*vt_create_message_queue)(int queue_size, int item_length); /*创建一个数据元素长度为item_length， 元素个数为queue_size的消息队列，返回消息队列实例*/

typedef int (*vt_send_message)(void *queue, const void *data, int wait_time_ms, uint8_t send_front);   /*发送消息，queue是消息队列实例,data为数据, wait_time_ms消息队列为满时最多等待的毫秒数，send_front【默认为0发送到队列尾部，为1插入队列头部】参数详见RTOS队列函数*/

typedef int (*vt_recv_message)(void *queue, void *data, int wait_time_ms, uint8_t peek_msg);   /*接收消息 queue是消息队列实例,data为存放数据指针, wait_time_ms消息队列为空时最多等待的毫秒数，peek_msg【默认为0直接收取队列数据，为1时只探测数据不把数据从队列取出相当于队列数据不减少】参数详见RTOS队列函数*/

typedef int (*vt_get_message_size)(void *queue);   /*获取消息队列size*/

typedef int (*vt_destory_msgqueue)(void *queue);   /*销毁消息队列*/


//task
typedef void (*vt_task_delay)(int time_ms);  /*系统延时函数，time_ms为毫秒数，适配系统需要做相应转换*/
typedef unsigned long (*vt_get_timestamp)(void);   /*获取时间戳 毫秒级*/

//memory
typedef void *(*vt_malloc)(unsigned long size);

typedef void (*vt_free)(void *ptr);

typedef void *(*vt_calloc)(unsigned long nmemb, unsigned long size);

typedef void *(*vt_realloc)(void *ptr, unsigned long size);


//26*4 bytes
/*操作系统功能函数, 必须全部正确实现*/
typedef struct {
    vt_create_event create_event;
    vt_post_event post_event;
    vt_wait_event wait_event;
    vt_destory_event destory_event;
    vt_create_semaphone create_semaphone;
    vt_post_semaphone post_semaphone;
    vt_wait_semaphone wait_semaphone;
    vt_destory_semaphone destory_semaphone;
    vt_create_mutex create_mutex;
    vt_mutex_lock mutex_lock;
    vt_mutex_unlock mutex_unlock;
    vt_destory_mutex destory_mutex;
    vt_create_timer create_timer;
    vt_start_timer  start_timer;
    vt_stop_timer   stop_timer;
    vt_delete_timer delete_timer;
    vt_create_message_queue create_message_queue;
    vt_send_message send_message;
    vt_recv_message recv_message;
    vt_get_message_size get_message_size;
    vt_destory_msgqueue destory_msgqueue;
    vt_task_delay   task_delay;
    /*sdk use only*/
    vt_malloc vt_plat_malloc;
    vt_free vt_plat_free;
    vt_calloc vt_plat_calloc;
    vt_realloc vt_plat_realloc;
    /*algorithm use only*/
    vt_malloc vt_alg_malloc;
    vt_free vt_alg_free;
    vt_realloc vt_alg_realloc;
    vt_get_timestamp get_timestamp;
} vt_os_ops_t;

typedef struct _recog_ops {
    /*识别网络请求 函数*/
    pfun_vt_httpRequest_cb request_func;
    /*识别TASK启动 函数，SDK调用此接口启动TASK*/
    pfun_vt_start_task_cb  recog_start_task;
} _recog_ops_t;

typedef struct StatedetParamEx {
    int width, height, channels;    // Image width and height.
    int x, y, w, h;                 // ROI rectangle.
    int ds_factor;                  // Downsample factor.
    int in_type;                    // Input image type. /*1: 灰度图; 2: NV21的YUV图片 可配置*/
    int thresh;                     // Threshold. /*缩放比例因子，一般赋值为4，可配置*/
    int fds;                        //set to 0
    int handle_edge;
    int propose_hand;
} StatedetParamEx;


/*资源切换列表菜单*/
typedef struct _recongnize_resinfo {
    /*当前书本资源库*/
    int current_res_id;
    /*资源库列表,包含当前资源库*/
    int *res_id;
    byte *res_languare;
    byte *res_sound_type;
    /*资源库列表大小*/
    byte res_size;
} _recongnize_resinfo_t;

typedef struct _recongnize_page_info {
    int BookID;
    int PageID;
    int PageType; /*VT_RECONGNIZE_PAGE_TYPE*/
} _recongnize_page_info_t;

/*pageinfo识别书本ID信息, resinfo[暂时无效废弃]*/
typedef int (*pfun_vt_recongnize_info_cb)(_recongnize_page_info_t *pageinfo, _recongnize_resinfo_t *resinfo, void *recong_userp);





#ifdef __cplusplus
}
#endif
#endif //__VTTYPES_H__

