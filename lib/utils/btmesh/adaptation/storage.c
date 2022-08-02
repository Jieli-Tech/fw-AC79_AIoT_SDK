#include "adaptation.h"
#include "syscfg/syscfg_id.h"

#define LOG_TAG                 "[MESH-storage]"
/* #define LOG_INFO_ENABLE */
/* #define LOG_DEBUG_ENABLE */
/* #define LOG_WARN_ENABLE */
/* #define LOG_ERROR_ENABLE */
/* #define LOG_DUMP_ENABLE */
#include "mesh_log.h"

#define NODE_INFO_CLEAR_DEBUG_EN    1
#define NODE_INFO_STORE_DEBUG_EN    !NODE_INFO_CLEAR_DEBUG_EN


void node_info_store(int index, void *buf, u16 len)
{
    u32 ret = 0;
    u16 w_len = len + 1;

    BT_INFO("--func=%s", __FUNCTION__);

    /* vm_check_all(0); */

#if NODE_INFO_CLEAR_DEBUG_EN
    u8 temp_buf[64];
    if (buf == 0) {
        temp_buf[0] = 0;
    } else {
        temp_buf[0] = 1;
        memcpy(temp_buf + 1, (u8 *)buf, len);
    }
    ret = syscfg_write(index, (u8 *)temp_buf, w_len);
    if (ret != w_len) {
        BT_ERR("vm_write err");
        BT_ERR("ret = %d", ret);
    }
#else /*NODE_INFO_CLEAR_DEBUG_EN*/

#if !NODE_INFO_STORE_DEBUG_EN
    ret = syscfg_write(index, (u8 *)buf, len);
    if (ret != len) {
        BT_ERR("vm_write err");
        BT_ERR("ret = %d", ret);
    }
#endif /* NODE_INFO_STORE_DEBUG_EN */

#endif /* NODE_INFO_CLEAR_DEBUG_EN */
}

void node_info_clear(int index, u16 len)
{
    BT_INFO("--func=%s", __FUNCTION__);
    node_info_store(index, NULL, len);
}

bool node_info_load(int index, void *buf, u16 len)
{
    u32 ret = 0;
    u16 r_len = len + 1;

    BT_INFO("--func=%s", __FUNCTION__);
#if NODE_INFO_CLEAR_DEBUG_EN
    u8 temp_buf[64];
    ret = syscfg_read(index, (u8 *)temp_buf, r_len);
    if (ret != r_len) {
        BT_ERR("vm_read err");
        BT_ERR("ret = %d", ret);
        return 1;
    }
    if (temp_buf[0] == 0) {
        BT_INFO("vm_read clear");
        return 1;
    }
    memcpy((u8 *)buf, temp_buf + 1, len);

    return 0;
#else /*NODE_INFO_CLEAR_DEBUG_EN*/

#if NODE_INFO_STORE_DEBUG_EN
    return 1;
#else
    ret = syscfg_read(index, (u8 *)buf, len);
    if (ret != len) {
        BT_ERR("vm_read err");
        BT_ERR("ret = %d", ret);
        return 1;
    }

    return 0;
#endif /* NODE_INFO_STORE_DEBUG_EN */

#endif /* NODE_INFO_CLEAR_DEBUG_EN */
}
