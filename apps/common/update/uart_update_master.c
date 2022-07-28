#include "app_config.h"
#if(USER_UART_UPDATE_ENABLE) && (UART_UPDATE_ROLE == UART_UPDATE_MASTER)
#include "typedef.h"
#include "update_loader_download.h"
#include "os/os_api.h"
#include "system/task.h"
#include "update.h"
#include "gpio.h"
#include "uart_update.h"
#include "asm/uart_dev.h"
#include "asm/clock.h"
#include "timer.h"
#include "system/fs/fs.h"

static volatile u32 uart_to_cnt = 0;
static volatile u32 uart_file_offset = 0;
static volatile u16 rx_cnt;  //收数据计数

typedef struct _uart_updte_ctl_t {
    OS_SEM sem;
    OS_SEM rx_sem;
    volatile u16 timemax;
    volatile u16 timeout;
    volatile u8 flag;
    volatile u8 err_code;
    u8 update_sta;
    u8 update_percent;
    u32 update_total_size;
    u32 update_send_size;
    u8 rx_cmd[0x30];
    u16 cmd_len;
    u16 uart_timer_hdl;
} uart_update_ctl_t;

static uart_update_ctl_t uart_update_ctl;
#define __this (&uart_update_ctl)

#define LOG_TAG             "[UART_UPDATE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#define RETRY_TIME          4//重试n次
#define PACKET_TIMEOUT      200//ms

#define FILE_READ_UNIT		512 //
#define UART_DEFAULT_BAUD	9600
#define UART_UPDATE_BAUD	(50*10000L)
#define TIME_TICK_UNIT		(10) //unit:ms
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
void *fd = NULL;

u32 uart_dev_receive_data(void *buf, u32 relen, u32 addr);
void uart_set_dir(u8 mode);
void uart_update_write(u8 *data, u32 len);
void uart_update_set_baud(u32 baudrate);
void uart_close_deal(void);
void uart_hw_init(uart_update_cfg update_cfg, void (*cb)(void *, u32));
void uart_data_decode(u8 *buf, u16 len);

/* enum { */
/*     SEEK_SET = 0x0, */
/*     SEEK_CUR = 0x1, */
/*     SEEK_END = 0X2, */
/* }; */

enum {
    CMD_UART_UPDATE_START = 0x1,
    CMD_UART_UPDATE_READ,
    CMD_UART_UPDATE_END,
    CMD_UART_UPDATE_UPDATE_LEN,
    CMD_UART_JEEP_ALIVE,
    CMD_UART_UPDATE_READY,
};

enum {
    RX_DATA_READY = 0,
    RX_DATA_TIMEOUT,
    RX_DATA_SUCC,
};

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
                    __this->timemax = 0;
                    if (protocal_frame.data.length <= sizeof(__this->rx_cmd)) {
                        memcpy(__this->rx_cmd, &(protocal_frame.data.data), protocal_frame.data.length);
                        __this->flag = RX_DATA_SUCC;
                        os_sem_post(&__this->rx_sem);
                    }
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
    uart_update_write((u8 *)&protocal_frame, length + SYNC_SIZE);
    uart_set_dir(1);
    return ret;
}

//read file by file operation handle
u32 ufw_data_read_api(u8 *buff, u32 addr, u32 size)
{
    //To do...
    if (fd) {
        fseek(fd, addr,  SEEK_SET);
        return fread(fd, buff, size);
    }
    return 0;
}

//open file and get file operation handle
bool ufw_file_op_init(char *update_path)
{
    if (fd) {
        fclose(fd);
    }
    //To do
    fd = fopen(update_path, "r");
    if (!fd) {
        return false;
    }
    return true;
}

//close the file and release resource
void ufw_file_op_close(void)
{
    if (fd) {
        fclose(fd);
        fd = NULL;
    }
    //To do
}

static u32 update_data_read_from_file(void *p, u32 addr, u32 len)
{
    u8 *buffer;
    struct file_info *p_file_info;
    u32 read_len = 0;

    if (len > FILE_READ_UNIT) {
        return (u32) - 1;
    }

    buffer = malloc(len + sizeof(struct file_info));
    if (buffer) {
        p_file_info = (struct file_info *)buffer;
        p_file_info->cmd = CMD_UART_UPDATE_READ;
        p_file_info->addr = addr;
        p_file_info->len = len;
        read_len = ufw_data_read_api(buffer + sizeof(struct file_info), addr, len);
    } else {
        return (u32) - 2;
    }

    uart_send_packet(buffer, len + sizeof(struct file_info));

    if (buffer) {
        free(buffer);
    }

    return read_len;
}

enum {
    UPDATE_STA_NONE = 0,
    UPDATE_STA_READY,
    UPDATE_STA_START,
    UPDATE_STA_TIMEOUT,
    UPDATE_STA_LOADER_DOWNLOAD_FINISH,
    UPDATE_STA_SUCC,
    UPDATE_STA_FAIL,
};

enum {
    UPDATE_ERR_NONE = 0,
    UPDATE_ERR_KEY,
    UPDATE_ERR_VERIFY,

    UPDATE_ERR_LOADER_DOWNLOAD_SUCC = 0x80,
};

static void uart_update_err_code_handle(u8 code)
{
    if (code) {
        if (UPDATE_ERR_LOADER_DOWNLOAD_SUCC == code) {
            __this->update_percent = 100;
            __this->update_send_size = 0;
            __this->update_total_size = 0;
            __this->update_sta = UPDATE_STA_LOADER_DOWNLOAD_FINISH;
            log_info("loader dn succ\n");
        } else {
            __this->update_sta = UPDATE_STA_FAIL;
            log_error("update_err:%x\n", code);
        }
    } else {
        __this->update_percent = 100;
        __this->update_sta = UPDATE_STA_SUCC;
        log_info("update all succ\n");
    }
}


static int uart_update_wait_rev_data(timeout)
{
    u32 err;
    __this->flag = RX_DATA_READY;
    __this->timeout = 0;
    __this->timemax = (timeout + 1) / TIME_TICK_UNIT;

    err = os_sem_pend(&__this->rx_sem, 2000);

    if (OS_NO_ERR != err) {
        log_info("wait tm out\n");
    }

    __this->timemax = __this->timeout = 0;

    if (__this->flag == RX_DATA_SUCC) {
        return TRUE;
    }

    return FALSE;
}

int uart_update_api_write_then_read(u8 *buf, u8 length, u8 timeout)
{
    int ret = FALSE;

    uart_send_packet(buf, length);
    if (timeout) {
        ret = uart_update_wait_rev_data(timeout);
    }

    return ret;
}

bool uart_update_send_update_ready(char *file_update_path)
{
    u8 ut_cmd[1];
    ut_cmd[0] = CMD_UART_UPDATE_READY;

    if (ufw_file_op_init(file_update_path)) {
        __this->update_sta = UPDATE_STA_READY;
        uart_send_packet(ut_cmd, sizeof(ut_cmd));
        log_info("uart_update_send_update_ready\n");
        return TRUE;
    }

    return FALSE;
}

bool get_uart_update_sta(void)
{
    return (__this->update_sta == UPDATE_STA_READY || __this->update_sta == UPDATE_STA_START) ?  TRUE : FALSE;
}

static void update_process_run(void)
{
    update_baudrate = UART_DEFAULT_BAUD;
    uart_update_set_baud(update_baudrate);

    __this->update_total_size = 0;
    __this->update_percent = 0;
    __this->update_send_size = 0;

    u8 *pbuf = &__this->rx_cmd;
    while (1) {
        if (OS_NO_ERR != os_sem_pend(&__this->rx_sem, 800)) {
            log_info("uart_timeout\n");
            __this->update_sta = UPDATE_STA_TIMEOUT;
            update_baudrate = 9600;
            uart_update_set_baud(update_baudrate);
            continue;
        }

        //os_time_dly(1);

        switch (pbuf[0]) {
        case CMD_UPDATE_START:
            log_info("CMD_UPDATE_START\n");
            __this->update_sta = UPDATE_STA_START;
            update_baudrate = UART_UPDATE_BAUD;
            WRITE_LIT_U32(pbuf + 1, update_baudrate);
            uart_send_packet(pbuf, 1 + sizeof(u32));
            log_info("use baud:%x\n", update_baudrate);
            uart_update_set_baud(update_baudrate);
            break;

        case CMD_UPDATE_READ: {
            u32 addr = READ_LIT_U32(&pbuf[1]);
            u32 len = READ_LIT_U32(&pbuf[1 + sizeof(u32)]);
            if (__this->update_total_size) {
                __this->update_send_size += len;
                __this->update_percent = (__this->update_send_size * 100) / __this->update_total_size;
                if (__this->update_percent >= 99) {
                    __this->update_percent = 99;
                }
                log_info("send data process:%x\n", __this->update_percent);
            }
            log_info("CMD_UPDATE_READ\n");
            update_data_read_from_file(NULL, addr, len);
        }
        break;

        case CMD_UPDATE_END:
            log_info("CMD_UPDATE_END\n");
            uart_update_err_code_handle(pbuf[1]);
            uart_send_packet(pbuf, 1);
            break;

        case CMD_SEND_UPDATE_LEN:
            __this->update_total_size = READ_LIT_U32(&pbuf[1]);
            __this->update_percent = 0;
            __this->update_send_size = 0;
            log_info("update_total_size:%x\n", __this->update_total_size);
            uart_send_packet(pbuf, 1);
            break;

        case CMD_KEEP_ALIVE:
            uart_send_packet(pbuf, 1);
            break;

        }
    }
}

static void uart_timeout_handler(void *priv)
{
    if (__this->timemax) {
        __this->timeout++;
        if (__this->timeout > __this->timemax) {
            __this->timemax = 0;
            __this->flag = RX_DATA_TIMEOUT;
            os_sem_post(&__this->rx_sem);
        }
    }
}

static void update_loader_download_task(void *p)
{
    log_info("create %s task\n", THIS_TASK_NAME);
    os_sem_create(&__this->sem, 0);
    os_sem_create(&__this->rx_sem, 0);


    while (1) {
        update_process_run();
    }
}

void uart_update_init(uart_update_cfg *cfg)
{
    memcpy(&update_cfg, cfg, sizeof(uart_update_cfg));
    task_create(update_loader_download_task, NULL, THIS_TASK_NAME);
    uart_hw_init(update_cfg, uart_data_decode);

}

void uart_update_exit(void)
{
    log_info("uart update exit\n");

    if (__this->uart_timer_hdl) {
        sys_timer_del(__this->uart_timer_hdl);
    }

    os_sem_del(&__this->sem, 0);
    os_sem_del(&__this->rx_sem, 0);

    ufw_file_op_close();

    uart_close_deal();

    task_kill(THIS_TASK_NAME);
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
