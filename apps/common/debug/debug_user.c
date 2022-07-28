#include "app_config.h"
#include "typedef.h"
#include "syscfg/syscfg_id.h"

//#define CONFIG_SDCARD_DEBUG_ENABLE  //开关SD卡记录打印信息
//#define CONFIG_NETWORK_DEBUG_ENABLE //可以触发打印信息上传到服务器端
//#define CONFIG_USB_DEBUG_ENABLE     //开关USB虚拟串口打印信息


#if (defined CONFIG_DEBUG_ENABLE && defined CONFIG_NETWORK_DEBUG_ENABLE)
#include "os/os_api.h"
#include "spinlock.h"
#include "wifi/wifi_connect.h"
#include "device/device.h"
#include "http/http_cli.h"
#include "json_c/json_object.h"
extern unsigned int time_lapse(unsigned int *handle, unsigned int time_out);
extern int lwip_dhcp_bound(void);
extern struct json_object *json_tokener_parse(const char *str);
static u32 log_cnt, log_time_hdl;
static char log_part, uploading_log_flag;
static spinlock_t lock;
static char log_buf[100 * 1024];

static int tmp_cnt = 0;
static u8 mac[6];
static u8 keycode_legal = 0;
static u8 check_keycode_cnt = 0;
#define LOG_BRAND 	"jieli"
#define LOG_NAME 	"APP"
#define	LOG_VERSION	"1.0.0"
#define LOG_UUID	"12345678"
#define LOG_KEYCODE	""		//客户随机填写12-16个字符串，用于日志查询

static void upload_log_task(void *priv)
{
#define LOG_BOUNDARY        			"----385424468743051437018271"
#define LOG_HTTP_HEAD_OPTION_TWO        \
    "POST /status/v1/log/file HTTP/1.1\r\n"\
    "cache-control: no-cache\r\n"\
    "Host: log.jieliapp.com\r\n"\
    "content-type: multipart/form-data; boundary="LOG_BOUNDARY"\r\n"\
    "content-length: %lu\r\n"\
    "Connection: close\r\n\r\n"

#define LOG_BODY_OPTION_THREE                                                     \
    "--"LOG_BOUNDARY"\r\n"                                                          \
    "Content-Disposition: form-data; name=\"file\"; filename=\"%s.txt\"\r\n"  \
    "Content-Type: text/plain\r\n\r\n"

#define LOG_BODY_OPTION_FOUR       \
    "\r\n--"LOG_BOUNDARY"\r\n"\
    "Content-Disposition: form-data; name=\"filename\"\r\n\r\n"\
    "%s.txt"\
    "\r\n--"LOG_BOUNDARY"\r\n"\
    "Content-Disposition: form-data; name=\"platform\"\r\n\r\n"\
    "device"\
    "\r\n--"LOG_BOUNDARY"\r\n"\
    "Content-Disposition: form-data; name=\"brand\"\r\n\r\n"\
    ""LOG_BRAND""\
    "\r\n--"LOG_BOUNDARY"\r\n"\
    "Content-Disposition: form-data; name=\"name\"\r\n\r\n"\
    ""LOG_NAME""\
    "\r\n--"LOG_BOUNDARY"\r\n"\
    "Content-Disposition: form-data; name=\"version\"\r\n\r\n"\
    ""LOG_VERSION""\
    "\r\n--"LOG_BOUNDARY"\r\n"\
    "Content-Disposition: form-data; name=\"mac\"\r\n\r\n"\
    "%s"\
    "\r\n--"LOG_BOUNDARY"\r\n"\
    "Content-Disposition: form-data; name=\"uuid\"\r\n\r\n"\
    ""LOG_UUID""\
    "\r\n--"LOG_BOUNDARY"\r\n"\
    "Content-Disposition: form-data; name=\"keycode\"\r\n\r\n"\
    ""LOG_KEYCODE""\
    "\r\n--"LOG_BOUNDARY"--\r\n"

    int ret = 0;
    char http_body_begin[512] = {0};
    char user_head_buf[512] = {0};
    char http_body_end[1024] = {0};
    char recv_buf[1024] = {0};

    if (mac[0] == '\0') {
        wifi_get_mac(mac);
    }
    char temp_buf1[20] = {0}, temp_buf2[20] = {0};
    sprintf(temp_buf1, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    sprintf(temp_buf2, "%s-%d", temp_buf1, tmp_cnt++);
    sprintf(http_body_begin, LOG_BODY_OPTION_THREE, temp_buf2);
    sprintf(http_body_end, LOG_BODY_OPTION_FOUR, temp_buf2, temp_buf1);



    httpcli_ctx ctx = {0};
    //mode data的传输方式
    int http_more_data_addr[3] = {0};
    int http_more_data_len[3 + 1] = {0};
    http_more_data_addr[0] = (int)(http_body_begin);
    http_more_data_addr[1] = (int)log_buf;
    http_more_data_addr[2] = (int)(http_body_end);
    http_more_data_len[0] = strlen(http_body_begin);
    http_more_data_len[1] = strlen(log_buf);
    http_more_data_len[2] = strlen(http_body_end);
    http_more_data_len[3] = 0;

    ctx.more_data = http_more_data_addr;
    ctx.more_data_len = http_more_data_len;

    http_body_obj http_recv_body = {0};
    http_recv_body.recv_len = 0;
    http_recv_body.buf_len = 1024;
    http_recv_body.buf_count = 1;
    http_recv_body.p = recv_buf;

    ctx.timeout_millsec = 5000;
    ctx.priv = &http_recv_body;
    ctx.url = "http://log.jieliapp.com/status/v1/log/file";

    sprintf(user_head_buf, LOG_HTTP_HEAD_OPTION_TWO,  strlen(http_body_begin) + strlen(log_buf) + strlen(http_body_end));
    ctx.user_http_header = user_head_buf;
    ret = httpcli_post(&ctx);
    /*  extern void efgh(int i); */
    /* if (ret) { */
    /* efgh(1); */
    /* } else { */
    /* efgh(0); */
    /* } */


    memset(log_buf, 0, sizeof(log_buf));
    log_cnt = 0;



    uploading_log_flag = 0;
}

static void check_keycode(void)
{
    char url[128] = {0};
    char recv_buf[1024] = {0};
    http_body_obj http_recv_body = {0};
    http_recv_body.recv_len = 0;
    http_recv_body.buf_len = 1024;
    http_recv_body.buf_count = 1;
    http_recv_body.p = recv_buf;
    httpcli_ctx ctx = {0};
    sprintf(url, "http://log.jieliapp.com/status/v1/log/keycode/check?keycode=%s", LOG_KEYCODE);
    ctx.timeout_millsec = 5000;
    ctx.priv = &http_recv_body;
    ctx.url = url;
    ctx.connection = "close";
    httpcli_post(&ctx);
    json_object *root_node = NULL, *first_node = NULL, *second_node = NULL;
    root_node = json_tokener_parse((unsigned char *)http_recv_body.p);
    if (strcmp(json_object_get_string(json_object_object_get(root_node, "data")), "true") == 0) {
        keycode_legal = 1;
    }
    json_object_put(root_node);

}


void upload_log_trig(void)
{
    if (!lwip_dhcp_bound()) {
        return;
    }

    while (keycode_legal == 0 && check_keycode_cnt < 5) {
        check_keycode();
        check_keycode_cnt++;
    }
    if (keycode_legal == 1) {
        spin_lock(&lock);
        if (!uploading_log_flag) {
            uploading_log_flag = 1;
            spin_unlock(&lock);
            if (thread_fork("upload_log_task", 11, 2048, 0, 0, upload_log_task, NULL)) {
                uploading_log_flag = 0;
            }
            return;
        }
        spin_unlock(&lock);
    }
}

#endif



#if (defined CONFIG_DEBUG_ENABLE && defined CONFIG_SDCARD_DEBUG_ENABLE)
#include "system/includes.h"//cbuffer_t
#include "os/os_api.h"//创建线程，系统延时
#include "storage_device.h"//检查SD卡
#include "system/init.h"//early_initcall
#include "fs/fs.h"//操作写文件

/* 定义初始化循环BUF的大小 */
#define DATA_BUF_SIZE (20*1024)
/* CBUF数据多于DATA_BUF_SIZE/2就写入一次数据 */
#define WRITE_SIZE     ( DATA_BUF_SIZE/2)
/* 定义需要使用到的结构体 */
/* 定义不写入数据标记值  */
/* 定义写命令 */

static  u8 write_time;  	//记录写的次数
static  u32 all_time;       //记录写的总次数
static  char lg_buf[DATA_BUF_SIZE]; //打印信息缓存值
static  u8 sd_state, write_busy;       //SD卡状态
static char file_name[64];           //定义路径

static void change_file_name(void)
{
    u32 log_num = 0;           //文件序号变量

    syscfg_write(VM_SD_LOG_INDEX, (char *)&log_num, sizeof(log_num)); //从VM读出
    snprintf(file_name, sizeof(file_name) - 1, CONFIG_ROOT_PATH"log%u.txt", log_num);
    log_num++;
    syscfg_write(VM_SD_LOG_INDEX, (char *)&log_num, sizeof(log_num)); //从VM写入

}
/* 写入数据到SD卡 */
static void write_log_to_sd(char *data, u32 len)
{
    FILE *file;            //写入数据的文件指针
    file = fopen(file_name, "w+");
    if (!file) {
        return;
    }
    fseek(file, all_time * len, SEEK_SET);
    fwrite(write_time ? data : (data + len), 1, len, file);
    fclose(file);
    /* 下一次写数据文件指针偏移 */
    all_time++;
}
static void sd_log_task(void *priv)
{
    u8 last_write_time = 0;
    change_file_name();

    while (1) {
        if (!storage_device_ready()) { //SD卡未就绪
            if (sd_state) {
                sd_state = 0;
                all_time = 0;//清除文件指针偏移量
                write_busy = 0 ; //清空标记位
                change_file_name();
            }
        } else {//SD卡就绪
            sd_state = 1;

            if (last_write_time != write_time) {
                last_write_time = write_time; //记录上一包数据号
                write_log_to_sd(lg_buf, WRITE_SIZE);
                write_busy = 0 ; //标记已经写入数据
            }
            if (write_busy && last_write_time == write_time) { //需要写入数据并且数据包和上一包一样
                puts("SD_WRITE_[ERROR],Packet loss!!!!!");
            }
        }
        os_time_dly(123);
    }
}
static int sd_log_task_create(void)
{
#define SD_LOG_STK_SIZE 512
    static u8 sd_log_tcb_stk_q[sizeof(StaticTask_t) + SD_LOG_STK_SIZE * 4] ALIGNE(4);
    os_task_create_static(sd_log_task, 0, 10, SD_LOG_STK_SIZE, 0, "sd_log_task", sd_log_tcb_stk_q);
    return 0;
}
module_initcall(sd_log_task_create);

#endif



#if (defined CONFIG_DEBUG_ENABLE && defined CONFIG_USB_DEBUG_ENABLE)
#include "os/os_api.h"//创建线程，系统延时
#include "system/init.h"//early_initcall

extern u32 cdc_write_data(int usb_id, u8 *buf, u32 len);

#define DATA_BUF_SIZE (20 * 1024)

static u32 lg_cnt, last_cnt;
static char lg_buf[DATA_BUF_SIZE]; //打印信息缓存值

static void usb_log_task(void *priv)
{
    os_time_dly(300);

    while (1) {
        if (lg_cnt > last_cnt && last_cnt != lg_cnt) {
            if (lg_cnt / sizeof(lg_buf) > last_cnt / sizeof(lg_buf)) {
                cdc_write_data(0, (u8 *)lg_buf + (last_cnt % sizeof(lg_buf)), sizeof(lg_buf) - last_cnt % sizeof(lg_buf));
                cdc_write_data(1, (u8 *)lg_buf + (last_cnt % sizeof(lg_buf)), sizeof(lg_buf) - last_cnt % sizeof(lg_buf));
                cdc_write_data(0, (u8 *)lg_buf, lg_cnt - last_cnt - (sizeof(lg_buf) - last_cnt % sizeof(lg_buf)));
                cdc_write_data(1, (u8 *)lg_buf, lg_cnt - last_cnt - (sizeof(lg_buf) - last_cnt % sizeof(lg_buf)));
            } else {
                cdc_write_data(0, (u8 *)lg_buf + (last_cnt % sizeof(lg_buf)), lg_cnt - last_cnt);
                cdc_write_data(1, (u8 *)lg_buf + (last_cnt % sizeof(lg_buf)), lg_cnt - last_cnt);
            }
        }
        last_cnt = lg_cnt;
        os_time_dly(10);
    }
}

static int usb_log_task_create(void)
{
#define USB_LOG_STK_SIZE 256
    static u8 usb_log_tcb_stk_q[sizeof(StaticTask_t) + USB_LOG_STK_SIZE * 4 + sizeof(StaticQueue_t)] ALIGNE(4);
    return os_task_create_static(usb_log_task, 0, 1, USB_LOG_STK_SIZE, 0, "usb_log_task", usb_log_tcb_stk_q);
}
module_initcall(usb_log_task_create);
#endif



//////////////////////////////////////////

#if (defined CONFIG_NETWORK_DEBUG_ENABLE || defined CONFIG_SDCARD_DEBUG_ENABLE || defined CONFIG_USB_DEBUG_ENABLE)
void putbyte(char c)
{
#if defined CONFIG_NETWORK_DEBUG_ENABLE
    log_buf[(log_cnt++) % (sizeof(log_buf))] = c;
#endif
#if defined CONFIG_SDCARD_DEBUG_ENABLE
    static  u32 lg_cnt = 0;         //记录数据个数
    u32 t;
    t = (lg_cnt++) % (sizeof(lg_buf));
    lg_buf[t] = c;
    if (!(t % WRITE_SIZE)) {//判断t为WRITE_SIZE的整数倍
        write_busy = 1 ;//标记需要写入数据
        write_time = t / WRITE_SIZE;//write_time 只为 0 或 1  为1时是第一包数据 为0时是第二包数据
    }
#endif
#if defined CONFIG_USB_DEBUG_ENABLE
    if (c == '\r') {
        return;
    }
    if (c == '\n') {
        lg_buf[(lg_cnt++) % (sizeof(lg_buf))] = '\r';
    }
    lg_buf[(lg_cnt++) % (sizeof(lg_buf))] = c;
#endif
}
#endif


#if (!defined CONFIG_DEBUG_ENABLE || !defined CONFIG_USER_DEBUG_ENABLE)
//关闭user_printf用户打印
int user_putchar(int a)
{
    return a;
}
int user_puts(const char *out)
{
    return 0;
}
int user_printf(const char *format, ...)
{
    return 0;
}
int user_vprintf(const char *restrict format, va_list arg)
{
    return 0;
}
void user_put_buf(const u8 *buf, int len)
{
}
void user_put_u8hex(u8 dat)
{
}
void user_put_u16hex(u16 dat)
{
}
void user_put_u32hex(u32 dat)
{
}
#endif

