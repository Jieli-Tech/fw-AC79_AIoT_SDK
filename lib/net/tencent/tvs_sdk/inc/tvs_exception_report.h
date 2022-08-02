#ifndef __TVS_EXCEPTION_H_FAEFAFWEFA__
#define __TVS_EXCEPTION_H_FAEFAFWEFA__
#include "tvs_http_client.h"

//上报错误类型
typedef enum  {
    /// 异常上报启始错误号(与其它SDK区分)
    EXCEPTION_START = 40000,
    /// 40001 系统调用错误
    EXCEPTION_SYSTEM_CALL,
    /// 40002 授权失败
    EXCEPTION_AUTH,
    /// 40003 echo失败
    EXCEPTION_ECHO,
    /// 40004 speech启动识别失败
    EXCEPTION_SPEECH,
    /**40005 会话过程中启动speex压缩失败*/
    EXCEPTION_AUDIO_ENCODER,
    /**40006 TTS解码失败*/
    EXCEPTION_TTS_DECODE
} exception_report_type;

void tvs_exception_report_start(exception_report_type type, char *message, int message_len);

#endif