/**
* @file  tvs_exception_report.c
* @brief 异常上报
* @date  2020-5-14
*/

#include "tvs_executor_service.h"
#include "tvs_exception_report.h"
#define MESSAGE_MAX_LEN 		200

void tvs_exception_report_start(exception_report_type type, char *message, int message_len)
{
    if (message_len < 0 || message == NULL) {
        printf("message or len is error:%d", message_len);
        return ;
    }
    if (message_len > MESSAGE_MAX_LEN) {
        message_len = MESSAGE_MAX_LEN;
    }
    //因内存紧张，不再分配内存来拷贝字符了。
    char ch_tmp = message[message_len];
    //字符结束
    if (ch_tmp != '\0') {
        message[message_len] = '\0';
    }
    tvs_executor_start_exception_report(type, message);
    //恢复
    if (ch_tmp != '\0') {
        message[message_len] = ch_tmp;
    }
}


