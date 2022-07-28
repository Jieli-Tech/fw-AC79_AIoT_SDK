#include "adaptation.h"
#include "net/buf.h"
#include "btmesh/access.h"

#define LOG_TAG         "[mesh-API]"
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_WARN_ENABLE
#define LOG_ERROR_ENABLE
#define LOG_DUMP_ENABLE
#include "mesh_log.h"

#if MESH_RAM_AND_CODE_MAP_DETAIL
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ble_mesh_joint_bss")
#pragma data_seg(".ble_mesh_joint_data")
#pragma const_seg(".ble_mesh_joint_const")
#pragma code_seg(".ble_mesh_joint_code")
#endif
#else /* MESH_RAM_AND_CODE_MAP_DETAIL */
#pragma bss_seg(".ble_mesh_bss")
#pragma data_seg(".ble_mesh_data")
#pragma const_seg(".ble_mesh_const")
#pragma code_seg(".ble_mesh_code")
#endif /* MESH_RAM_AND_CODE_MAP_DETAIL */

u8 *buffer_add_u8_at_tail(void *buf, u8 val)
{
    return net_buf_simple_add_u8((struct net_buf_simple *)buf, val);
}

void buffer_add_le16_at_tail(void *buf, u16 val)
{
    net_buf_simple_add_le16((struct net_buf_simple *)buf, val);
}

void buffer_add_be16_at_tail(void *buf, u16 val)
{
    net_buf_simple_add_be16((struct net_buf_simple *)buf, val);
}

void buffer_add_le32_at_tail(void *buf, u32 val)
{
    net_buf_simple_add_le32((struct net_buf_simple *)buf, val);
}

void buffer_add_be32_at_tail(void *buf, u32 val)
{
    net_buf_simple_add_be32((struct net_buf_simple *)buf, val);
}

u8 buffer_pull_u8_from_head(void *buf)
{
    return net_buf_simple_pull_u8((struct net_buf_simple *)buf);
}

u16 buffer_pull_le16_from_head(void *buf)
{
    return net_buf_simple_pull_le16((struct net_buf_simple *)buf);
}

u16 buffer_pull_be16_from_head(void *buf)
{
    return net_buf_simple_pull_be16((struct net_buf_simple *)buf);
}

u32 buffer_pull_le32_from_head(void *buf)
{
    return net_buf_simple_pull_le32((struct net_buf_simple *)buf);
}

u32 buffer_pull_be32_from_head(void *buf)
{
    return net_buf_simple_pull_be32((struct net_buf_simple *)buf);
}

void *buffer_memcpy(void *buf, const void *mem, u32 len)
{
    return net_buf_simple_add_mem((struct net_buf_simple *)buf, mem, len);
}

void *buffer_memset(struct net_buf_simple *buf, u8 val, u32 len)
{
    return memset(net_buf_simple_add(buf, len), val, len);
}

u32 buffer_head_init(u32 opcode)
{
    if (opcode < 0x100) {
        /* 1-byte OpCode */
        return opcode;
    }

    if (opcode < 0x10000) {
        /* 2-byte OpCode */
        return sys_cpu_to_be16(opcode);
    }

    /* 3-byte OpCode */
    u8 b0 = (opcode >> 16) & 0xff;
    u16 cid = sys_cpu_to_le16(opcode & 0xffff);

    return cid << 8 | b0;
}

void mesh_node_primary_addr_reset(u16 addr)
{
    bt_mesh_comp_provision(addr);
}

void mesh_setup(void (*init_cb)(void))
{
    BT_INFO("--func=%s", __FUNCTION__);

    hci_core_init();

    //< mesh init
    if (init_cb) {
        init_cb();
    }
}
