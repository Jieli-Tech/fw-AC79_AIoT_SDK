#include "cfg_tool.h"
#include "event/device_event.h"
#include "usb/device/cdc.h"
#include "ioctl_cmds.h"
#include "boot.h"
#include "asm/crc16.h"
#include "app_config.h"

#define LOG_TAG_CONST       APP_CFG_TOOL
#define LOG_TAG             "[APP_CFG_TOOL]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_EQ_ENABLE && TCFG_EQ_ONLINE_ENABLE && (TCFG_COMM_TYPE == TCFG_USB_COMM)

struct cfg_tool_event {
    u8 event;
    u8 *packet;
    u8 size;
};

struct cfg_tool_info {
    R_QUERY_BASIC_INFO 		r_basic_info;
    R_QUERY_FILE_SIZE		r_file_size;
    R_QUERY_FILE_CONTENT	r_file_content;
    R_PREPARE_WRITE_FILE	r_prepare_write_file;
    R_READ_ADDR_RANGE		r_read_addr_range;
    R_ERASE_ADDR_RANGE      r_erase_addr_range;
    R_WRITE_ADDR_RANGE      r_write_addr_range;
    R_ENTER_UPGRADE_MODE    r_enter_upgrade_mode;

    S_QUERY_BASIC_INFO 		s_basic_info;
    S_QUERY_FILE_SIZE		s_file_size;
    S_PREPARE_WRITE_FILE    s_prepare_write_file;
};

static struct cfg_tool_info info = {
    .s_basic_info.protocolVer = PROTOCOL_VER_AT_OLD,
};

#define __this  (&info)
#define TEMP_BUF_SIZE	256

extern u8 *sdfile_get_burn_code(u8 *len);
extern void doe(u16 k, void *pBuf, u32 lenIn, u32 addr);
extern int norflash_erase(u32 cmd, u32 addr);
extern const char *sdk_version(void);
static int app_cfg_tool_event_handler(struct cfg_tool_event *cfg_tool_dev);

static u8 local_packet[TEMP_BUF_SIZE];
static const char fa_return[] = "FA";	//失败
static const char ok_return[] = "OK";	//成功
static const char er_return[] = "ER";	//不能识别的命令
static u32 size_total_write = 0;

#ifdef ALIGN
#undef ALIGN
#endif

#define ALIGN(a, b) \
	({ \
	 	int m = (u32)(a) & ((b)-1); \
		int ret = (u32)(a) + (m?((b)-m):0);	 \
		ret;\
	})

static u32 cfg_tool_encode_data_by_user_key(u16 key, u8 *buff, u16 size, u32 dec_addr, u8 dec_len)
{
    u16 key_addr;
    u16 r_len;

    while (size) {
        r_len = (size > dec_len) ? dec_len : size;
        key_addr = (dec_addr >> 2)^key;
        doe(key_addr, buff, r_len, 0);
        buff += r_len;
        dec_addr += r_len;
        size -= r_len;
    }
    return dec_addr;
}

static u8 parse_seq = 0;
static void ci_send_packet_new(u32 id, u8 *packet, int size)
{
#if APP_ONLINE_DEBUG
    app_online_db_ack(parse_seq, packet, size);
#endif/*APP_ONLINE_DEBUG*/
}

struct cfg_tool_event spp_packet;
static int cfg_tool_effect_tool_spp_rx_data(u8 *buf, u32 len)
{
    parse_seq = buf[2];
    u32 event = (buf[3] | (buf[4] << 8) | (buf[5] << 16) | (buf[6] << 24));
    spp_packet.event = event;
    spp_packet.packet = buf;
    spp_packet.size = len;
    return (app_cfg_tool_event_handler(&spp_packet));
}

void all_assemble_package_send_to_pc(u8 id, u8 sq, u8 *buf, u32 len)
{
    u8 *send_buf = NULL;
    u16 crc16_data;

    send_buf = (u8 *)malloc(TEMP_BUF_SIZE);
    if (send_buf == NULL) {
        log_error("send_buf malloc err!");
        return;
    }

    send_buf[0] = 0x5A;
    send_buf[1] = 0xAA;
    send_buf[2] = 0xA5;
    send_buf[5] = 2 + len;/*L*/
    send_buf[6] = id;/*T*/
    send_buf[7] = sq;/*SQ*/
    memcpy(send_buf + 8, buf, len);

    crc16_data = CRC16(&send_buf[5], len + 3);
    send_buf[3] = crc16_data & 0xff;
    send_buf[4] = (crc16_data >> 8) & 0xff;

    /* printf_buf(send_buf, len + 8); */

#if TCFG_EQ_ONLINE_ENABLE
#if (TCFG_COMM_TYPE == TCFG_USB_COMM)
    cdc_write_data(0, send_buf, len + 8);
#elif (TCFG_COMM_TYPE == TCFG_UART_COMM)
    /* ci_uart_write(send_buf, len + 8); */
    /* ci_dev_send_packet(send_buf, len + 8); */
    ci_send_packet(0, send_buf, len + 8);
#elif (TCFG_COMM_TYPE == TCFG_SPP_COMM)
    ci_send_packet_new(INITIATIVE_STYLE, buf, len);
#endif
#endif

    free(send_buf);
}

static void hex2text(u8 *buf, u8 *out)
{
    sprintf(out, "%02x%02x-%02x%02x%02x%02x", buf[5], buf[4], buf[3], buf[2], buf[1], buf[0]);
}

static u32 packet_combined(u8 *packet, u8 num)
{
    u32 _packet = 0;
    _packet = (packet[num] | (packet[num + 1] << 8) | (packet[num + 2] << 16) | (packet[num + 3] << 24));
    return _packet;
}

static FILE *cfg_open_file(u32 file_id)
{
    FILE *cfg_fp = NULL;
    if (file_id <= CFG_EQ_FILEID) {
        if (file_id == CFG_TOOL_FILEID) {
            cfg_fp = fopen(CFG_TOOL_FILE, "r");
            log_info("open cfg_tool.bin\n");
        } else if (file_id == CFG_OLD_EQ_FILEID) {
            cfg_fp = fopen(CFG_OLD_EQ_FILE, "r");
            log_info("open old eq_cfg_hw.bin\n");
        } else if (file_id == CFG_OLD_EFFECT_FILEID) {
            cfg_fp = fopen(CFG_OLD_EFFECT_FILE, "r");
            log_info("open effects_cfg.bin\n");
        } else if (file_id == CFG_EQ_FILEID) {
            cfg_fp = fopen(CFG_EQ_FILE, "r");
            log_info("open eq_cfg_hw.bin\n");
        }
    }
    return cfg_fp;
}

extern void nvram_set_boot_state(u32 state);
extern void hw_mmu_disable(void);
extern void ram_protect_close(void);
AT(.volatile_ram_code)
void cfg_tool_go_mask_usb_updata()
{
    local_irq_disable();
    ram_protect_close();
    hw_mmu_disable();
    nvram_set_boot_state(2);

    JL_CLOCK->PWR_CON |= (1 << 4);
    /* chip_reset(); */
    /* cpu_reset(); */
    while (1);
}

/*延时复位防止工具升级进度条显示错误*/
static void delay_cpu_reset(void *priv)
{
    extern void cpu_reset();
    cpu_reset();
}

static int app_cfg_tool_event_handler(struct cfg_tool_event *cfg_tool_dev)
{
    u8 *buf = NULL;
    u8 *buf_temp = NULL;
    u8 *buf_temp_0 = NULL;
    u8 valid_cmd = 1;
    u32 erase_cmd;
    u32 write_len;
    u32 send_len = 0;
    u8 crc_temp_len, sdkname_temp_len;
    const struct tool_interface *p;
    struct vfs_attr attr;
    FILE *cfg_fp = NULL;

    /* if(cfg_tool_dev->packet[3] != 0x04){ */
    /* printf("-------------------\n\r"); */
    /* printf_buf(cfg_tool_dev->packet, cfg_tool_dev->size); */
    /* }else{ */
    /* putbyte('a');	 */
    /* } */

    buf = (u8 *)malloc(TEMP_BUF_SIZE);
    if (buf == NULL) {
        log_error("buf malloc err!");
        return 0;
    }
    buf_temp_0 = (u8 *)malloc(TEMP_BUF_SIZE);
    if (buf_temp_0 == NULL) {
        free(buf);
        log_error("buf_temp_0 malloc err!");
        return 0;
    }
    buf_temp_0 = (u8 *)ALIGN(buf_temp_0, 4);
    memset(buf_temp_0, 0, TEMP_BUF_SIZE);
    memcpy(buf_temp_0 + 1, cfg_tool_dev->packet, cfg_tool_dev->size);

    /*数据进行分发*/
    list_for_each_tool_interface(p) {
        if (p->id == cfg_tool_dev->packet[1]) {
            p->tool_message_deal(buf_temp_0 + 2, cfg_tool_dev->size - 1);
            free(buf_temp_0);
            free(buf);
            return 0;
        }
    }

    memset(buf, 0, TEMP_BUF_SIZE);

    switch (cfg_tool_dev->event) {
    case ONLINE_SUB_OP_QUERY_BASIC_INFO:
        u8 *p = sdfile_get_burn_code(&crc_temp_len);
        memcpy(__this->s_basic_info.progCrc, p + 8, 6);
        hex2text(__this->s_basic_info.progCrc, __this->s_basic_info.progCrc);

        sdkname_temp_len = strlen(sdk_version());
        memcpy(__this->s_basic_info.sdkName, sdk_version(), sdkname_temp_len);
        printf("name = %s\n\r", __this->s_basic_info.sdkName);

        struct flash_head flash_head_for_pid_vid;
        for (u8 i = 0; i < 5; i++) {
            norflash_read(NULL, (u8 *)&flash_head_for_pid_vid, 32, 0x1000 * i);
            doe(0xffff, (u8 *)&flash_head_for_pid_vid, 32, 0);
            if (flash_head_for_pid_vid.crc == 0xffff) {
                continue;
            } else {
                log_info("flash head addr = 0x%x\n", 0x1000 * i);
                break;
            }
        }
        if (flash_head_for_pid_vid.crc == 0xffff) {
            log_error("Can't find flash head addr");
            break;
        }

        memcpy(__this->s_basic_info.pid, &(flash_head_for_pid_vid.pid), sizeof(flash_head_for_pid_vid.pid));
        memcpy(__this->s_basic_info.vid, &(flash_head_for_pid_vid.vid), sizeof(flash_head_for_pid_vid.vid));
        for (u8 i = 0; i < sizeof(__this->s_basic_info.pid); i++) {
            if (__this->s_basic_info.pid[i] == 0xff) {
                __this->s_basic_info.pid[i] = 0x00;
            }
        }

        send_len = sizeof(__this->s_basic_info);
        memcpy(buf, &(__this->s_basic_info), send_len);
        break;

    case ONLINE_SUB_OP_QUERY_FILE_SIZE:
        __this->r_file_size.file_id = packet_combined(cfg_tool_dev->packet, 7);
        cfg_fp = cfg_open_file(__this->r_file_size.file_id);

        if (cfg_fp == NULL) {
            log_error("file open error!\n");
            goto _exit_;
        }

        fget_attrs(cfg_fp, &attr);
        __this->s_file_size.file_size = attr.fsize;
        send_len = sizeof(__this->s_file_size.file_size);//长度
        memcpy(buf, &(__this->s_file_size.file_size), 4);
        fclose(cfg_fp);
        break;

    case ONLINE_SUB_OP_QUERY_FILE_CONTENT:
        __this->r_file_content.file_id = packet_combined(cfg_tool_dev->packet, 7);
        __this->r_file_content.offset = packet_combined(cfg_tool_dev->packet, 11);
        __this->r_file_content.size = packet_combined(cfg_tool_dev->packet, 15);

        cfg_fp = cfg_open_file(__this->r_file_content.file_id);
        if (cfg_fp == NULL) {
            log_error("file open error!\n");
            goto _exit_;
        }
        fget_attrs(cfg_fp, &attr);

        if (__this->r_file_content.size > attr.fsize) {
            fclose(cfg_fp);
            log_error("reading size more than actual size!\n");
            break;
        }

        u32 flash_addr = sdfile_cpu_addr2flash_addr(attr.sclust);
        buf_temp = (u8 *)malloc(__this->r_file_content.size);
        norflash_read(NULL, (void *)buf_temp, __this->r_file_content.size, flash_addr + __this->r_file_content.offset);
        send_len = __this->r_file_content.size;
        memcpy(buf, buf_temp, __this->r_file_content.size);

        if (buf_temp) {
            free(buf_temp);
        }
        fclose(cfg_fp);
        break;

    case ONLINE_SUB_OP_PREPARE_WRITE_FILE:
        __this->r_prepare_write_file.file_id = packet_combined(cfg_tool_dev->packet, 7);
        __this->r_prepare_write_file.size = packet_combined(cfg_tool_dev->packet, 11);

        cfg_fp = cfg_open_file(__this->r_prepare_write_file.file_id);
        if (cfg_fp == NULL) {
            log_error("file open error!\n");
            break;
        }
        fget_attrs(cfg_fp, &attr);

        __this->s_prepare_write_file.file_size = attr.fsize;
        __this->s_prepare_write_file.file_addr = sdfile_cpu_addr2flash_addr(attr.sclust);
        __this->s_prepare_write_file.earse_unit = boot_info.vm.align * 256;
        send_len = sizeof(__this->s_prepare_write_file);
        memcpy(buf, &(__this->s_prepare_write_file), send_len);
        fclose(cfg_fp);
        break;

    case ONLINE_SUB_OP_READ_ADDR_RANGE:
        __this->r_read_addr_range.addr = packet_combined(cfg_tool_dev->packet, 7);
        __this->r_read_addr_range.size = packet_combined(cfg_tool_dev->packet, 11);
        buf_temp = (u8 *)malloc(__this->r_read_addr_range.size);
        norflash_read(NULL, (void *)buf_temp, __this->r_read_addr_range.size, __this->r_read_addr_range.addr);
        send_len = __this->r_read_addr_range.size;
        memcpy(buf, buf_temp, __this->r_read_addr_range.size);

        if (buf_temp) {
            free(buf_temp);
        }
        break;

    case ONLINE_SUB_OP_ERASE_ADDR_RANGE:
        __this->r_erase_addr_range.addr = packet_combined(cfg_tool_dev->packet, 7);
        __this->r_erase_addr_range.size = packet_combined(cfg_tool_dev->packet, 11);

        switch (__this->s_prepare_write_file.earse_unit) {
        case 256:
            erase_cmd = IOCTL_ERASE_PAGE;
            break;
        case (4*1024):
            erase_cmd = IOCTL_ERASE_SECTOR;
            break;
        case (64*1024):
            erase_cmd = IOCTL_ERASE_BLOCK;
            break;
defualt:
            memcpy(buf, fa_return, sizeof(fa_return));
            log_error("erase error!");
            break;
        }

        for (u8 i = 0; i < (__this->r_erase_addr_range.size / __this->s_prepare_write_file.earse_unit); i ++) {
            u8 ret = norflash_erase(erase_cmd, __this->r_erase_addr_range.addr + (i * __this->s_prepare_write_file.earse_unit));
            if (ret) {
                memcpy(buf, fa_return, sizeof(fa_return));
                log_error("erase error!");
                break;
            } else {
                memcpy(buf, ok_return, sizeof(ok_return));
            }
        }
        send_len = sizeof(fa_return);
        break;

    case ONLINE_SUB_OP_WRITE_ADDR_RANGE:
        __this->r_write_addr_range.addr = packet_combined(cfg_tool_dev->packet, 7);
        __this->r_write_addr_range.size = packet_combined(cfg_tool_dev->packet, 11);
        buf_temp = (u8 *)malloc(__this->r_write_addr_range.size);
        memcpy(buf_temp, cfg_tool_dev->packet + 15, __this->r_write_addr_range.size);
        cfg_tool_encode_data_by_user_key(boot_info.chip_id, buf_temp, __this->r_write_addr_range.size, __this->r_write_addr_range.addr - boot_info.sfc.sfc_base_addr, 0x20);
        write_len = norflash_write(NULL, buf_temp, __this->r_write_addr_range.size, __this->r_write_addr_range.addr);

        if (write_len != __this->r_write_addr_range.size) {
            memcpy(buf, fa_return, sizeof(fa_return));
            log_error("write error!");
        } else {
            memcpy(buf, ok_return, sizeof(ok_return));
        }
        send_len = sizeof(fa_return);

        if (buf_temp) {
            free(buf_temp);
        }

        if (__this->r_prepare_write_file.file_id == CFG_TOOL_FILEID) {
            size_total_write += __this->r_write_addr_range.size;
            if (size_total_write >= __this->r_erase_addr_range.size) {
                size_total_write = 0;
                log_info("cpu_reset\n");
                extern u16 sys_timeout_add(void *priv, void (*func)(void *priv), u32 msec);
                sys_timeout_add(NULL, delay_cpu_reset, 500);
            }
        }
        break;

    case ONLINE_SUB_OP_ENTER_UPGRADE_MODE:
        log_info("event_ONLINE_SUB_OP_ENTER_UPGRADE_MODE\n");
        cfg_tool_go_mask_usb_updata();
        break;

    default:
        valid_cmd = 0;
        log_error("invalid data\n");
        memcpy(buf, er_return, sizeof(er_return));//不认识的命令返回ER
        send_len = sizeof(er_return);
        break;
_exit_:
        memcpy(buf, fa_return, sizeof(fa_return));//文件打开失败返回FA
        send_len = sizeof(fa_return);
        break;
    }

    all_assemble_package_send_to_pc(REPLY_STYLE, cfg_tool_dev->packet[2], buf, send_len);

    free(buf_temp_0);
    free(buf);
    return (valid_cmd);
}

static void cfg_tool_event_to_user(u8 *packet, u32 type, u8 event, u8 size)
{
    struct cfg_tool_event cfg_tool;
    if (packet != NULL) {
        if ((u32)size > sizeof(local_packet)) {
            return;
        }
        cfg_tool.packet = local_packet;
        memcpy(cfg_tool.packet, packet, size);
    }
    cfg_tool.size = size;
    cfg_tool.event = event;

    app_cfg_tool_event_handler(&cfg_tool);
}

static void online_cfg_tool_data_deal(void *buf, u32 len)
{
    u8 *data_buf = (u8 *)buf;
    u16 crc16_data;

    /* printf_buf(buf, len); */

    if ((data_buf[0] != 0x5a) || (data_buf[1] != 0xaa) || (data_buf[2] != 0xa5)) {
        log_error("Header check error, receive an invalid message!\n");
        return;
    }
    crc16_data = (data_buf[4] << 8) | data_buf[3];
    if (crc16_data != CRC16(data_buf + 5, len - 5)) {
        log_error("CRC16 check error, receive an invalid message!\n");
        return;
    }

    u32 cmd = packet_combined(data_buf, 8);
    switch (cmd) {
    case ONLINE_SUB_OP_QUERY_BASIC_INFO:
    case ONLINE_SUB_OP_QUERY_FILE_SIZE:
    case ONLINE_SUB_OP_QUERY_FILE_CONTENT:
    case ONLINE_SUB_OP_PREPARE_WRITE_FILE:
    case ONLINE_SUB_OP_READ_ADDR_RANGE:
    case ONLINE_SUB_OP_ERASE_ADDR_RANGE:
    case ONLINE_SUB_OP_WRITE_ADDR_RANGE:
    case ONLINE_SUB_OP_ENTER_UPGRADE_MODE:
        cfg_tool_event_to_user(&data_buf[5], DEVICE_EVENT_FROM_CFG_TOOL, cmd, data_buf[5] + 1);
        break;
    default:
        cfg_tool_event_to_user(&data_buf[5], DEVICE_EVENT_FROM_CFG_TOOL, DEFAULT_ACTION, data_buf[5] + 1);
        break;
    }
}

void usb_cdc_read_data_handler(struct device_event *event)
{
    struct usb_device_t *usb_device = event->arg;

    const usb_dev usb_id = usb_device2id(usb_device);
    static u8 buf_rx[256] = {0};
    static u8 rx_len_total = 0;
    u8 buf[64] = {0};
    u32 rlen;

    rlen = cdc_read_data(usb_id, buf, 64);

    /* put_buf(buf, rlen); */
    /* printf("rlen = %d\n\r",rlen); */
    /* cdc_write_data(usb_id, buf, rlen); */

    if ((buf[0] == 0x5A) && (buf[1] == 0xAA) && (buf[2] == 0xA5)) {
        memset(buf_rx, 0, 256);
        memcpy(buf_rx, buf, rlen);
        /* log_info("need len = %d\n", buf_rx[5] + 6); */
        /* log_info("rx len = %d\n", rlen); */
        if ((buf_rx[5] + 6) == rlen) {
            rx_len_total = 0;
#if	TCFG_EQ_ONLINE_ENABLE
#if (TCFG_COMM_TYPE == TCFG_USB_COMM)
            /* put_buf(buf_rx, rlen); */
            online_cfg_tool_data_deal(buf_rx, rlen);
#else
            put_buf(buf, rlen);
            cdc_write_data(usb_id, buf, rlen);
#endif
#endif
        } else {
            rx_len_total += rlen;
        }
    } else {
        if ((rx_len_total + rlen) > 256) {
            memset(buf_rx, 0, 256);
            rx_len_total = 0;
            return;
        }
        memcpy(buf_rx + rx_len_total, buf, rlen);
        /* log_info("need len = %d\n", buf_rx[5] + 6); */
        /* log_info("rx len = %d\n", rx_len_total + rlen); */
        if ((buf_rx[5] + 6) == (rx_len_total + rlen)) {
#if	TCFG_EQ_ONLINE_ENABLE
#if (TCFG_COMM_TYPE == TCFG_USB_COMM)
            /* put_buf(buf_rx, rx_len_total + rlen); */
            online_cfg_tool_data_deal(buf_rx, rx_len_total + rlen);
#else
            put_buf(buf, rlen);
            cdc_write_data(usb_id, buf, rlen);
#endif
#endif
            rx_len_total = 0;
        } else {
            rx_len_total += rlen;
        }
    }
}

#endif
