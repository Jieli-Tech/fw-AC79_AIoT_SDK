#ifndef __TVS_AUTH_MANAGER_H__
#define __TVS_AUTH_MANAGER_H__


void init_authorize_on_boot();

//通过client id授权
void start_authorize_with_client_id(const char *client_id);
//通过厂商账号授权(需要厂商手机app接入我们DMSDK)
void start_authorize_with_manuf(const char *client_id, const char *authRespInfo);

//保存授权信息
void save_auth_info(char *account_info, int len);

#endif
