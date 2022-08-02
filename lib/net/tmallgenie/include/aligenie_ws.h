#ifndef _ALIGENIE_WS_HEADER_
#define _ALIGENIE_WS_HEADER_

#include "stdint.h"
#include "stdbool.h"

typedef enum {
    AG_WS_BIN_DATA_START,
    AG_WS_BIN_DATA_CONTINUE,
    AG_WS_BIN_DATA_FINISH,
} AG_WS_BIN_DATA_TYPE_T;

typedef void(*ag_ws_cb_on_connect)(void);
typedef void(*ag_ws_cb_on_disconnect)(void);
/**
 * The next 2 callback need to block the websocket transportation if it doesn't return.
 */
typedef void(*ag_ws_cb_on_recv_text)(char *, uint32_t);
typedef void(*ag_ws_cb_on_recv_bin)(void *, uint32_t, AG_WS_BIN_DATA_TYPE_T);

typedef struct {
    ag_ws_cb_on_connect     cb_on_connect;
    ag_ws_cb_on_disconnect  cb_on_disconnect;
    ag_ws_cb_on_recv_text   cb_on_recv_text;
    ag_ws_cb_on_recv_bin    cb_on_recv_bin;
} AG_WS_CALLBACKS_T;

typedef struct {
    char *server;
    uint16_t port;
    char *schema;
    char *cacert;
    char *path;
    AG_WS_CALLBACKS_T *callbacks;
} AG_WS_CONNECT_INFO_T;

typedef enum {
    AG_WS_STATUS_DISCONNECTED = 0,
    AG_WS_STATUS_CONNECTING,
    AG_WS_STATUS_CONNECTED,
} AG_WS_STATUS_E;

#define AG_WS_RET_OK        (0)
#define AG_WS_RET_ERROR     (-1)

/**
 * ATTENTION: Websocket needs to create a new task.
 * DO NOT IMPLEMENT WS CONNECT IN THIS FUNCTION
 * Because the stack size of caller is very tiny.
 *
 * return AG_WS_RET_OK: connecting, AG_WS_RET_ERROR: error
 */
extern int32_t ag_ws_connect(AG_WS_CONNECT_INFO_T *info);

/**
 * disconnect request
 * return AG_WS_RET_OK: disconnecting, AG_WS_RET_ERROR: error
 */
extern int32_t ag_ws_disconnect();

/**
 * get connection status
 * return: enum AG_WS_STATUS_E
 */
extern AG_WS_STATUS_E ag_ws_get_connection_status();

/**
 * send text
 * return: AG_WS_RET_OK for success, AG_WS_RET_ERROR for error.
 */
extern int32_t ag_ws_send_text(char *text, uint32_t len);

/**
 * send binary
 * return: AG_WS_RET_OK for success, AG_WS_RET_ERROR for error.
 */
extern int32_t ag_ws_send_binary(void *data, uint32_t len, AG_WS_BIN_DATA_TYPE_T type);

#endif /*_ALIGENIE_WS_HEADER_*/

