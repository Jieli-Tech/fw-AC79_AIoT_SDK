#ifndef __TVS_AUTHORIZER_INFOS_H_FAEFAFWEFA__
#define __TVS_AUTHORIZER_INFOS_H_FAEFAFWEFA__


/**
* 厂商账号处理，首先在设备端通过tvs_auth_req_infos_build来生成authReqInfo(一个json格式)。
* 然后需要在厂商手机端apk(接入DMSDK)通过authReqInfo来获取authRespInfo(具体需要厂商手机app接入我们DMSDK来实现)，
* 然后再通过codeVerifier与authRespInfo中的authCode发给设备授权
* 流程请参考：https://dingdang.qq.com/doc/page/365
*/

/**
* 生成authReqInfos一个json格式
* {
    "codeChallenge": "",
    "sessionId": ""
  }
  codeChallenge生成:随机32个字节数字，然后按base64->codeVerifier->sha256->base64
  sessionId生成:随机32个字符
  authReqInfo:用来存储 authReqInfo 的bug
  len:authReqInfo的长度(不能少于120)
*/
int tvs_auth_req_infos_build(char *authReqInfo, int len);

/**
* 读取authCode,请参考上面厂商账号处理流程
*/
char *tvs_auth_get_authCode();

/**
* 读取session id,在dmsdk返回authcode信息时做校验
*/
char *tvs_auth_get_session_id();
/**
*设置authCode,这个需要使用手机DMSDK来生成,请参考上面厂商账号处理流程
*/
int tvs_auth_set_authCode(const char *authCode, int size);
/**
* 读取codeVerifier,参考tvs_auth_req_infos_build
*/
char *tvs_auth_get_codeVerifier();

#endif