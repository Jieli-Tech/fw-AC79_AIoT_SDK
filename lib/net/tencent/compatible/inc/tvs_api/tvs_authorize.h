/**
* @file  tvs_authorize.h
* @brief TVS 授权接口
* @date  2019-5-10
*/

#ifndef __TVS_AUTHORIZE_H_FHUIEWRT__
#define __TVS_AUTHORIZE_H_FHUIEWRT__

#include "tvs_common_def.h"

/**
 * @brief 初始化authorize模块，已过时；
 *
 * @param callback 监听器
 * @return 0为初始化成功，其他值为失败
 */
int tvs_authorize_init(tvs_authorize_callback callback);

/**
 * @brief 设置之前生成的authorize结果，调用这个函数代表之前已经完成授权，仅仅需要对授权结果进行刷新操作；已过时；
 * @param auth_info authorize结果。
 * @param auth_info_len authorize结果的长度。
 * @return 0为成功，其他值为失败，错误码见TVS_API_ERROR_*
 */
int tvs_authorize_set_auth_info(char *auth_info, int auth_info_len);

/**
 * @brief 设置新的client id，调用此函数将清除之前的authorize结果，之后将利用新的client id重新执行authorize操作；已过时；
 *
 * @param client_id 这个ID可用由其他端（比如手机）利用QQ、微信账号或者第三方账号生成并传递到本终端，
 *                  也可以利用product id和dsn组合，根据终端账号方案决定
 * @return 0为成功，其他值为失败，错误码见TVS_API_ERROR_*
 */
int tvs_authorize_set_client_id(const char *client_id);

/**
 * @brief 清除authorize info，一般用于重新配对流程；已过时；
 *
 * @param
 * @return 0为初始化成功，其他值为失败
 */
int tvs_authorize_clear_auth_info();

/**
 * @brief 无账号方案中，利用product id和dsn组合client id；已过时；
 *
 * @param product_id
 * @param dsn  设备唯一标识
 * @return 组合所得client id，使用完毕后需要调用free,否则会产生内存泄漏
 */
char *tvs_authorize_generate_client_id(const char *product_id, const char *dsn);

/**
 * @brief 启动authorize流程；已过时；
 *
 * @param
 * @return 0为成功，其他值为失败
 */
int tvs_authorize_start();

/**
 * @brief 使用新的client id，重新授权；已过时；
 *
 * @param  client_id 新的client id
 * @return 0为成功，其他值为失败
 */
int tvs_authorize_restart(const char *client_id);


/**
 * @brief 授权模块初始化
 *
 * @param  product_id 产品ID
 * @param  dsn 设备唯一标识
 * @param  authorize_info 之前保存的授权信息，内容为json字符串；可以传NULL，代表之前从未授权
 * @param  authorize_info_size 授权信息的长度
 * @param  auth_callback 授权结果的监听器
 * @return 0为成功，其他值为失败
 */
int tvs_authorize_manager_initalize(const char *product_id, const char *dsn, const char *authorize_info, int authorize_info_size, tvs_authorize_callback auth_callback);

/**
 * @brief 触发访客授权流程
 *
 * @param
 * @return 0为成功，其他值为失败；
 *         取值TVS_API_ERROR_NETWORK_INVALID代表网络未连接，在网络连接后会自动进行授权;
 */
int tvs_authorize_manager_guest_login();

/**
 * @brief 在设备授权方案中，从手机APP端同步client id到设备端后，需要调用此函数设置client id;
 *
 * @param  client_id 从手机APP同步过来的client id
 * @return 0为成功，其他值为失败；
 */
int tvs_authorize_manager_set_client_id(const char *client_id);


/**
 * @brief 厂商账号授权方案,用来生成authReqInfo,然后使用接入DMSDK的厂商app来生成authRespInfo及ClientId,最后调用tvs_authorize_manager_set_manuf_client_id来授权
 *
 * @param  authReqInfo 用来存储生成的authReqInfo(长度不能少于120)
 * @param  len authReqInfo的长度
 * @return 0为成功，其他值为失败；
 */
int tvs_authorize_manager_build_auth_req(char *authReqInfo, int len);

/**
 * @brief 厂商账号授权方案，从手机APP端同步client id及authRespInfo到设备端后，需要调用此函数设置client id及authRespInfo;
 *
 * @param  client_id 从手机APP同步过来的client id
 * @param  auth_resp_info 从手机APP同步过来的authRespInfo
 * @return 0为成功，其他值为失败；
 */
int tvs_authorize_manager_set_manuf_client_id(const char *client_id, const char *auth_resp_info);


/**
 * @brief 触发设备授权流程
 *
 * @param
 * @return 0为成功，其他值为失败；
 *         取值TVS_API_ERROR_NETWORK_INVALID代表网络未连接，在网络连接后会自动进行授权;
 */
int tvs_authorize_manager_login();

/**
 * @brief 注销，用于重新授权的流程，调用此函数清除之前授权的信息
 *
 * @param
 * @return 0为成功，其他值为失败；
 */
int tvs_authorize_manager_logout();


#endif
