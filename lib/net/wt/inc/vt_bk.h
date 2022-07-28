#ifndef __VT_BK_H__
#define __VT_BK_H__

#include "vt_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct vt_bk_handle;

/*
* 创建单例绘本实例
*/
struct vt_bk_handle *vt_bk_instance();

/*
* 绘本登陆环境初始化
* in: handle[返回的绘本实例] license_raw_text [license.lcs 裸数据] device_id[设备不会变化的唯一ID] modle_type[机型参数]
* 0：成功 其他失败
*/
int vt_bk_env_init(struct vt_bk_handle *handle, const char *license_raw_text, const char *device_id, const char *modle_type);

/*登陆
* in: handle[返回的绘本实例] cb [设备登陆时网络请求的回调函数，外部实现好注册进来]
* 0：成功 其他失败, 失败重试的顺序是先调用vt_bk_env_init 设置参数再重新调用vt_bk_login
*/
int vt_bk_login(struct vt_bk_handle *handle, pfun_vt_httpRequest_cb cb);

/*
* 注册相机操作
* in: handle[返回的绘本实例] ops [相机操作接口，外部实现好注册进来，stoppreview 等接口支持重复调用无副作用]
* 0：成功 其他失败
*/
int vt_bk_register_camera_ops(struct vt_bk_handle *handle, const camera_ops_t *ops);

/*
* 注册播放器操作
* in: handle[返回的绘本实例] ops [播放器操作接口，外部实现好注册进来， 接口最好支持重复调用无副作用]
* 0：成功 其他失败
*/
int vt_bk_register_mediaplayer_ops(struct vt_bk_handle *handle, const media_player_ops_t *ops);

/*
* 注册适配的系统功能函数
* in: handle[返回的绘本实例] ops [适配的系统功能接口，外部实现好注册进来]
* 0：成功 其他失败
*/
int vt_bk_register_os_wrapper(struct vt_bk_handle *handle, const vt_os_ops_t *ops);


/*
* 注册识别的网络请求函数和识别task启动函数
* in: handle[返回的绘本实例] ops [网络请求函数和启动TASK的接口，外部实现好注册进来] StatedetParam [翻页算法参数]
* 0：成功 其他失败
*/
int vt_bk_register_recognize_ops(struct vt_bk_handle *handle, const _recog_ops_t *ops, void *dectparams);

/*
*/
int vt_bk_register_common_request(struct vt_bk_handle *handle, pfun_vt_httpRequest_cb cb);

/*
* 开始识别TASK
* in: handle[返回的绘本实例]
* 0：成功 其他失败
*/
int vt_bk_start_recognize(struct vt_bk_handle *handle);

/*
* 停止识别TASK
* in: handle[返回的绘本实例]
* 0：成功 其他失败
*/
int vt_bk_stop_recognize(struct vt_bk_handle *handle);

/*
* 暂停识别TASK
* in: handle[返回的绘本实例]
* 0：成功 其他失败
*/
int vt_bk_pause(struct vt_bk_handle *handle);

/*
* 恢复识别TASK
* in: handle[返回的绘本实例]
* 0：成功 其他失败
*/
int vt_bk_resume(struct vt_bk_handle *handle);


/*
 * 获取识别TASK状态, TASK必须要是PAUSE状态 resume才会有效
*/
int vt_bk_get_recongstate(struct vt_bk_handle *handle);

/*
* 注册错误回调
* in: handle[返回的绘本实例] cb[出错后回调的函数接口] userp[用户指针]
* 0：成功 其他失败
*/
int vt_bk_register_error_cb(struct vt_bk_handle *handle, pfun_vt_error_cb cb, void *userp);

/*
* 注册识别信息回调
* in: handle[返回的绘本实例] cb[识别回调的函数接口] recong_userp[识别注册的用户指针，回调时会回传此指针]
* 0：成功 其他失败
*/
int vt_bk_register_reconginfo_cb(struct vt_bk_handle *handle, pfun_vt_recongnize_info_cb cb, void *recong_userp);

/*
* 切换资源
* in: handle[返回的绘本实例] cb[切换资源需要调用的网络接口] res_id[资源库ID通过pfun_vt_recongnize_info_cb的回调获取]
* 0：成功 其他失败
*/
int vt_bk_select_resource(struct vt_bk_handle *handle, pfun_vt_httpRequest_cb cb, int res_id);

/*
* 设置参数【函数为空实现，暂未实现】
* in: handle[返回的绘本实例] value[设置的值]
* 0：成功 其他失败
*/
int vt_bk_set_params(struct vt_bk_handle *handle, int value);

/*
* 获取参数【函数为空实现，暂未实现】
* in: handle[返回的绘本实例] value[存放参数]
* 0：成功 其他失败
*/
int vt_bk_get_params(struct vt_bk_handle *handle, int *value);

/*
* 销毁绘本实例
* in: handle[返回的绘本实例]
* 0：成功 其他失败
*/
int vt_bk_destory(struct vt_bk_handle *handle);

#ifdef __cplusplus
}
#endif
#endif // __VT_BK_H__

