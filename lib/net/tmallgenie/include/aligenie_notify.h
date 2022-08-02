/*AliGenie SDK for FreeRTOS header*/
/*Ver.20180524*/

#ifndef _ALIGENIE_NOTIFY_HEADER_
#define _ALIGENIE_NOTIFY_HEADER_

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * 设备端接到新留言提醒后调用此函数，其中可以做一些界面显示或指示灯操作。
 * 注意：此函数不要长时间阻塞。
 */
extern void ag_notify_new_voice_msg();

/**
 * 设备收到URL类型的push指令后调用此函数。
 * 注意：此函数不要长时间阻塞。
 */
extern void ag_notify_push_command(const char *const cmd);

/**
 * 设备激活完成后调用此函数。
 * 注意：此函数不要长时间阻塞。
 */
extern void ag_notify_finish_init(const char *const uuid);

/**
 * 设备端收到“退出”指令后调用此函数。
 * 其中行为允许自定义。
 * 注意：此函数不要长时间阻塞。
 */
extern void ag_notify_cmd_exit();

/**
 * botid配置透传nlu结果后，nlu解析结果指令会从此函数透传。
 * 注意：此函数不要长时间阻塞。
 */
extern void ag_notify_nlu_result(const char *const cmd);

/**
* data result notification
*/
extern void ag_notify_data_result(const char *method, const char *action);

/**
 * “留言播放已完成”TTS指令收到时的回调，注意此时这条TTS尚未播放
 */
extern void ag_notify_voice_msg_about_to_play_finish();

/**
 * error code from AliGenie server.
 */
extern void ag_notify_server_error(int errorCode, char *errorMsg);

#ifdef __cplusplus
}
#endif

#endif /*#define _ALIGENIE_NOTIFY_HEADER_*/
