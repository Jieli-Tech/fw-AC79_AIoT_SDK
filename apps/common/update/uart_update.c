#include "app_config.h"
#if(USER_UART_UPDATE_ENABLE) && (UART_UPDATE_ROLE == UART_UPDATE_SLAVE)
#include "typedef.h"
#include "update_loader_download.h"
#include "os/os_api.h"
#include "system/task.h"
#include "update.h"
#include "gpio.h"
#include "uart_update.h"
#include "asm/uart_dev.h"
#include "asm/clock.h"

static volatile u32 uart_to_cnt = 0;
static volatile u32 uart_file_offset = 0;
static volatile u16 rx_cnt;  //收数据计数
static void (*uart_update_resume_hdl)(void *priv) = NULL;
static int (*uart_update_sleep_hdl)(void *priv) = NULL;

#define LOG_TAG             "[UART_UPDATE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#define RETRY_TIME          4//重试n次
#define PACKET_TIMEOUT      200//ms

//命令
#define CMD_UPDATE_START    0x01
#define CMD_UPDATE_READ     0x02
#define CMD_UPDATE_END      0x03
#define CMD_SEND_UPDATE_LEN 0x04
#define CMD_KEEP_ALIVE      0x05


#define READ_LIT_U16(a)   (*((u8*)(a))  + (*((u8*)(a)+1)<<8))
#define READ_LIT_U32(a)   (*((u8*)(a))  + (*((u8*)(a)+1)<<8) + (*((u8*)(a)+2)<<16) + (*((u8*)(a)+3)<<24))

#define WRITE_LIT_U16(a,src)   {*((u8*)(a)+1) = (u8)(src>>8); *((u8*)(a)+0) = (u8)(src&0xff); }
#define WRITE_LIT_U32(a,src)   {*((u8*)(a)+3) = (u8)((src)>>24);  *((u8*)(a)+2) = (u8)(((src)>>16)&0xff);*((u8*)(a)+1) = (u8)(((src)>>8)&0xff);*((u8*)(a)+0) = (u8)((src)&0xff);}

#define THIS_TASK_NAME "uart_update"


static protocal_frame_t protocal_frame __attribute__((aligned(4)));
u32 update_baudrate = 9600;             //初始波特率
static uart_update_cfg  update_cfg;

u32 uart_dev_receive_data(void *buf, u32 relen, u32 addr);
void uart_set_dir(u8 mode);
void uart_update_write(u8 *data, u32 len);
void uart_update_set_baud(u32 baudrate);
void uart_close_deal(void);
void uart_hw_init(uart_update_cfg update_cfg, void (*cb)(void *, u32));
void uart_data_decode(u8 *buf, u16 len);

enum {
    SEEK_SET = 0x0,
    SEEK_CUR = 0x1,
    SEEK_END = 0X2,
};

enum {
    CMD_UART_UPDATE_START = 0x1,
    CMD_UART_UPDATE_READ,
    CMD_UART_UPDATE_END,
    CMD_UART_UPDATE_UPDATE_LEN,
    CMD_UART_JEEP_ALIVE,
    CMD_UART_UPDATE_READY,
};


static void uart_update_hdl_register(void (*resume_hdl)(void *priv), int (*sleep_hdl)(void *priv))
{
    uart_update_resume_hdl = resume_hdl;
    uart_update_sleep_hdl  = sleep_hdl;

}

/*----------------------------------------------------------------------------*/
/**@brief   填充升级结构体私有参数
  @param   p: 升级结构体指针(UPDATA_PARM)
  @return  void
  @note
 */
/*----------------------------------------------------------------------------*/
static void uart_ufw_update_private_param_fill(UPDATA_PARM *p)
{
    UPDATA_UART uart_param = {.control_baud = update_baudrate, .control_io_tx = update_cfg.tx, .control_io_rx = update_cfg.rx};
    memcpy(p->parm_priv, &uart_param, sizeof(uart_param));
}

/*----------------------------------------------------------------------------*/
/**@brief   固件升级校验流程完成, cpu reset跳转升级新的固件
  @param   type: 升级类型
  @return  void
  @note
 */
/*----------------------------------------------------------------------------*/
static void uart_ufw_update_before_jump_handle(int type)
{
    printf("soft reset to update >>>");
    cpu_reset(); //复位让主控进入升级内置flash
}

static void uart_update_state_cbk(int type, u32 state, void *priv)
{
    update_ret_code_t *ret_code = (update_ret_code_t *)priv;

    if (ret_code) {
        printf("state:%x err:%x\n", ret_code->stu, ret_code->err_code);
    }

    switch (state) {
    case UPDATE_CH_EXIT:
        if ((0 == ret_code->stu) && (0 == ret_code->err_code)) {
            //update_mode_api(BT_UPDATA);
            update_mode_api_v2(UART_UPDATA,
                               uart_ufw_update_private_param_fill,
                               uart_ufw_update_before_jump_handle);
        }

        break;

    default:
        break;
    }
}
static void uart_update_callback(void *priv, u8 type, u8 cmd)
{
    /* printf("cmd:%x\n", cmd); */
    /* if (cmd == UPDATE_LOADER_OK) { */
    /*     update_mode_api(type, update_baudrate, update_cfg.tx, update_cfg.rx); */
    /* } else { */
    /*     //失败将波特率设置回初始,并重置变量 */
    /*     rx_cnt = 0; */
    /*     update_baudrate = 9600; */
    /*     uart_file_offset = 0; */
    /*     uart_update_set_baud(update_baudrate); */
    /*     //uart_hw_init(update_cfg, uart_data_decode); */
    /* } */
}

void uart_data_decode(u8 *buf, u16 len)
{
    u16 crc, crc0, i, ch;
    /* printf("decode_len:%d\n", len); */
    /* put_buf(buf, len); */
    for (i = 0; i < len; i++) {
        ch = buf[i];
__recheck:
        if (rx_cnt == 0) {
            if (ch == SYNC_MARK0) {
                protocal_frame.raw_data[rx_cnt++] = ch;
            }
        } else if (rx_cnt == 1) {
            protocal_frame.raw_data[rx_cnt++] = ch;
            if (ch != SYNC_MARK1) {
                rx_cnt = 0;
                goto __recheck;
            }
        } else if (rx_cnt < 4) {
            protocal_frame.raw_data[rx_cnt++] = ch;
        } else {
            protocal_frame.raw_data[rx_cnt++] = ch;
            if (rx_cnt == (protocal_frame.data.length + SYNC_SIZE)) {
                rx_cnt = 0;
                extern u16 CRC16(void *ptr, u32 len);
                crc = CRC16(protocal_frame.raw_data, protocal_frame.data.length + SYNC_SIZE - 2);
                memcpy(&crc0, &protocal_frame.raw_data[protocal_frame.data.length + SYNC_SIZE - 2], 2);
                if (crc0 == crc) {
                    switch (protocal_frame.data.data[0]) {
                    case CMD_UART_UPDATE_START:
                        log_info("CMD_UART_UPDATE_START\n");
                        os_taskq_post_msg(THIS_TASK_NAME, 1, MSG_UART_UPDATE_START_RSP);
                        break;
                    case CMD_UART_UPDATE_READ:
                        log_info("CMD_UART_UPDATE_READ\n");
                        if (uart_update_resume_hdl) {
                            uart_update_resume_hdl(NULL);
                        }
                        break;
                    case CMD_UART_UPDATE_END:
                        log_info("CMD_UART_UPDATE_END\n");
                        break;
                    case CMD_UART_UPDATE_UPDATE_LEN:
                        log_info("CMD_UART_UPDATE_LEN\n");
                        break;
                    case CMD_UART_JEEP_ALIVE:
                        log_info("CMD_UART_KEEP_ALIVE\n");
                        break;
                    case CMD_UART_UPDATE_READY:
                        log_info("CMD_UART_UPDATE_READY\n");
                        os_taskq_post_msg(THIS_TASK_NAME, 1, MSG_UART_UPDATE_READY);
                        break;
                    default:
                        log_info("unkown cmd...\n");
                        break;
                    }
                } else {
                    rx_cnt = 0;
                }
            }
        }
    }
}

static bool uart_send_packet(u8 *buf, u16 length)
{
    bool ret;
    u16 crc;
    u8 *buffer;

    buffer = (u8 *)&protocal_frame;
    protocal_frame.data.mark0 = SYNC_MARK0;
    protocal_frame.data.mark1 = SYNC_MARK1;
    protocal_frame.data.length = length;
    memcpy((char *)&buffer[4], buf, length);
    crc = CRC16(buffer, length + SYNC_SIZE - 2);
    memcpy(buffer + 4 + length, &crc, 2);
    uart_set_dir(0);//设为输出
    put_buf((u8 *)&protocal_frame, length + SYNC_SIZE);
    uart_update_write((u8 *)&protocal_frame, length + SYNC_SIZE);
    uart_set_dir(1);
    return ret;
}

u32 uart_dev_receive_data(void *buf, u32 relen, u32 addr)
{
    u8 i;
    struct file_info file_cmd;
    for (i = 0; i < RETRY_TIME; i++) {
        if (i > 0) {
            putchar('r');
        }
        file_cmd.cmd = CMD_UPDATE_READ;
        file_cmd.addr = addr;
        file_cmd.len = relen;
        uart_send_packet(&file_cmd, sizeof(file_cmd));
        if (uart_update_sleep_hdl) {
            if (uart_update_sleep_hdl(NULL) == OS_TIMEOUT) {
                printf("uart_sleep\n");
                continue;
            }
        }
        memcpy(&file_cmd, protocal_frame.data.data, sizeof(file_cmd));
        if ((file_cmd.cmd != CMD_UPDATE_READ) || (file_cmd.addr != addr) || (file_cmd.len != relen)) {
            continue;
        }
        memcpy(buf, &protocal_frame.data.data[sizeof(file_cmd)], protocal_frame.data.length - sizeof(file_cmd));
        return (protocal_frame.data.length - sizeof(file_cmd));
    }
    putchar('R');
    return relen;
}

bool uart_update_cmd(u8 cmd, u8 *buf, u32 len)
{
    u8 *pbuf, i;
    //for (i = 0; i < RETRY_TIME; i++)
    {
        pbuf = protocal_frame.data.data;
        pbuf[0] = cmd;
        if (buf) {
            memcpy(pbuf + 1, buf, len);
        }
        uart_send_packet(pbuf, len + 1);
    }
    return TRUE;
}

extern const update_op_api_t uart_ch_update_op;
void uart_update_recv(u8 cmd, u8 *buf, u32 len)
{
    u32 baudrate = 9600;
    switch (cmd) {
    case CMD_UPDATE_START:
        memcpy(&baudrate, buf, 4);
        g_printf("CMD_UPDATE_START:%d\n", baudrate);
        if (update_baudrate != baudrate) {
            update_baudrate = baudrate;
            uart_update_set_baud(baudrate);
            uart_update_cmd(CMD_UPDATE_START, &update_baudrate, 4);
        } else {
            update_mode_info_t info = {
                .type = UART_UPDATA,
                .state_cbk =  uart_update_state_cbk,
                .p_op_api = &uart_ch_update_op,
                .task_en = 1,
            };
            app_active_update_task_init(&info);
            /* app_update_loader_downloader_init(              //设置完波特率后开始升级 */
            /*     UART_UPDATA, */
            /*     uart_update_callback, */
            /*     NULL, */
            /*     &uart_ch_update_op); */
        }
        break;

    case CMD_UPDATE_END:
        break;

    case CMD_SEND_UPDATE_LEN:
        break;

    default:
        break;
    }
}


bool uart_send_update_len(u32 update_len)
{
    u8 cmd[4];
    WRITE_LIT_U32(&cmd[0], update_len);
    return uart_update_cmd(CMD_SEND_UPDATE_LEN, cmd, 4);
}


u16 uart_f_open(void)
{
    return 1;
}

u16 uart_f_read(void *handle, void *buf, u32 relen)
{
    u32 len;
    len = uart_dev_receive_data(buf, relen, uart_file_offset);
    if (len == -1) {
        log_info("uart_f_read err\n");
        return -1;
    }
    uart_file_offset += len;
    return len;
}

int uart_f_seek(void *fp, u8 type, u32 offset)
{
    if (type == SEEK_SET) {
        uart_file_offset = offset;
    } else if (type == SEEK_CUR) {
        uart_file_offset += offset;
    }
    return 0;//FR_OK;
}

u16 uart_f_stop(u8 err)
{
    uart_update_cmd(CMD_UPDATE_END, &err, 1);
    update_baudrate = 9600;             //把波特率设置回9600
    return 0;
}



const update_op_api_t uart_ch_update_op = {
    .ch_init = uart_update_hdl_register,
    .f_open  = uart_f_open,
    .f_read  = uart_f_read,
    .f_seek  = uart_f_seek,
    .f_stop  = uart_f_stop,
};

static void update_loader_download_task(void *p)
{
    int ret;
    int msg[8];
    static u8 update_start = 0;
    const uart_bus_t *uart_bus;
    u32 uart_rxcnt = 0;

    while (1) {
        ret = os_task_pend("taskq", msg, ARRAY_SIZE(msg));
        if (ret != OS_TASKQ) {
            continue;
        }
        if (msg[0] != Q_MSG) {
            continue;
        }
        switch (msg[1]) {
        case MSG_UART_UPDATE_READY:
            uart_update_cmd(CMD_UPDATE_START, NULL, 0);
            break;

        case MSG_UART_UPDATE_START_RSP:         //收到START_RSP 进行波特率设置设置
            log_info("MSG_UART_UPDATE_START_RSP\n");
            uart_update_recv(protocal_frame.data.data[0], &protocal_frame.data.data[1], protocal_frame.data.length - 1);
            break;

        case MSG_UART_UPDATE_READ_RSP:
            log_info("MSG_UART_UPDATE_READ_RSP\n");
            break;

        default:
            log_info("unkown msg..............\n");
            break;
        }
    }
}

void uart_update_init(uart_update_cfg *cfg)
{
    memcpy(&update_cfg, cfg, sizeof(uart_update_cfg));
    task_create(update_loader_download_task, NULL, THIS_TASK_NAME);
    uart_hw_init(update_cfg, uart_data_decode);
    printf(">>>%s\n", __func__);
}


static void clock_critical_enter(void)
{
}

static void clock_critical_exit(void)
{
    uart_update_set_baud(update_baudrate);
}
CLOCK_CRITICAL_HANDLE_REG(uart_update, clock_critical_enter, clock_critical_exit)







#endif
