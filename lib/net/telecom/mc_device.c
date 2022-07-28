#include "sock_api/sock_api.h"
#include "lwip.h"
#include <string.h>
#include "stdlib.h"
#include "time.h"
#include "json_c/json_object.h"
#include "json_c/json_tokener.h"
#include "server/ai_server.h"
#include "system/includes.h"
#include "asm/crc16.h"
#include "common/base64.h"
#include "common/aes.h"
#include "fs/fs.h"
#include "asm/sfc_norflash_api.h"

#ifdef TELECOM_MC_SDK_VERSION
MODULE_VERSION_EXPORT(telecom_mc_sdk, TELECOM_MC_SDK_VERSION);
#endif

#if 1
#define log_info(x, ...)    printf("\n>>>>>>[mc_device]\n" x " ", ## __VA_ARGS__)
#else
#define log_info(...)
#endif

#define DEV_CTEI_LOGIN_ENABLE

#define CT_LOGIN_HOST		"smarthome.ctdevice.ott4china.com:9012"
#define	CT_DEVICE_VERSION	"11111.222.333.444"
#define	CT_PARENT_DEV_MAC	"000000000000"
#define	CT_VERSION			"000.000.000.001"
#define CT_SUB_DEVICE_ID	"123123123123112312"
#define CT_SUB_DEVICE_CTEI	"123123123312312312"
#define CT_SUB_DEVICE_MAC	"123312312312312"

#define FLASH_BUF_LEN 		1024
#define FLASH_BUF_START		12 * 1024

//定时器ID
static int heartbeat_timeout_id;
static int heartbeat_id;
static int device_status_id;
//线程PID
static int server_func_id;
static int dev_login_pid;

static int recv_sequence_num;	//接收随机数
static u8 report_all;		//发送完整状态成功标志位
static u8 CT_connect_flag;	//SDK可连接标志位
static u8 heart_send;		//心跳已发送标志,处理了心跳包发送但先收到非心跳包的情况

static int tcp_device_init(char *tcp_host);
static void tcp_send_function(int code);
static void tcp_recv_handler(void *priv);
static void device_status_init();
static void info_init();
static void timeout_func(void *priv);
static void device_status_func(void *priv);
static void heartbeat_ctl_func(void *priv);
static void status_set_func(int serial, char *name, char *param);

enum SEVER_TO_DEVICE_EVENT {
    RECV_MSG,
    CONNECT_CLOSE,
};

struct CT_Mc_device_info {
    struct sockaddr_in local;
    struct sockaddr_in dest;
    void *tcp_sock;
    void *udp_sock;

    char ipaddr[20];

    char tcpHost[64];
    char tcp_port[10];

    char udpHost[64];
    char udp_port[10];

    u16	serial_num;
    char deviceId[50];
    char sessionKey[20];
    char pin[20];
    char iv[20];
    char mac[20];
    char ctei[20];
    char model[20];

    char token[40];

    int heartBeat;
    int authInterval;

    u8 login_flag;
};
static struct CT_Mc_device_info device = {0};

const struct ai_sdk_api CT_MC_api;

/************************************************/
//statusSerials
struct extraStatusParam {
    int test;			//#测试使用
};

struct dev_status {
    struct list_head entry;
    char statusName[20];
    int statusValue;

    u8 total;			//共有同类型设备数量
    u8 id_num;			//设备单/多路序号
    u8 upload_flag;		//设备状态上报标志

    u8 param_vaild;		//是否有扩展项
    struct extraStatusParam param;
};
static struct dev_status status_head;

/***********************************************/
//resourceSerials
struct resourceInfoParam {
    int consum_ID;
    int remain_life;
    char consum_name[20];
};

struct dev_res {
    struct list_head entry;
    struct resourceInfoParam param;
    char resourceName[20];

    u8 total;			//共有同类型设备数量
    u8 id_num;			//设备单/多路序号
    u8 upload_flag;		//设备状态上报标志
};
static struct dev_res res_head;

/************************************************/
//event                           //#没测试到
struct eventInfoParam {
    char ID_name[20];
    int ID_value;
    char TYPE_name[20];
    int TYPE_value;
};

struct device_eventInfo {
    struct list_head entry;
    char eventName[20];
    struct eventInfoParam param;
};
static struct device_eventInfo event_head;
/************************************************/


static struct dev_status *get_alarm_info()
{
    return NULL;
}

#define PROFILE_PATH "mnt/sdfile/app/exif"

//设备信息读取
static int device_read_flash()
{
    printf("%s", __func__);
    int ret = -1;

    struct list_head *pos = NULL;
    struct dev_status *status_info = NULL;
    struct dev_res *res_info = NULL;

    u8 *device_data = (u8 *)calloc(1, FLASH_BUF_LEN);
    if (!device_data) {
        return ret;
    }

    u32 profile_addr;
    FILE *profile_fp = fopen(PROFILE_PATH, "r");
    if (profile_fp == NULL) {
        free(device_data);
        return ret;
    }

    struct vfs_attr file_attr;
    fget_attrs(profile_fp, &file_attr);
    profile_addr = sdfile_cpu_addr2flash_addr(file_attr.sclust);
    fclose(profile_fp);

    norflash_read(NULL, device_data, FLASH_BUF_LEN, profile_addr);

    u16 *device_crc = (u16 *)&device_data[0];
    u16 *device_len	= (u16 *)&device_data[2];
    if (*device_len	> FLASH_BUF_LEN) {
        printf("------%s------%d, no data ,device_len = %d\r\n", __func__, __LINE__, *device_len);
        goto read_exit;
    }

    //CRC校验比较
    if (*device_crc != CRC16((const void *)&device_data[4], (u32)*device_len)) {
        printf("-----%s--------%d, CRC16 failed\r\n", __func__, __LINE__);
        goto read_exit;
    }

    u16 *status_num		= (u16 *)&device_data[4];
    u16 *resource_num	= (u16 *)&device_data[6];
    u8 *device_buffer = &device_data[8];

    for (int i = 0; i < (u32)*status_num; i++) {
        status_info = (struct dev_status *)calloc(1, sizeof(struct dev_status));
        if (status_info == NULL) {
            goto read_exit;
        }
        memcpy(status_info, device_buffer, sizeof(struct dev_status));
        device_buffer += sizeof(struct dev_status);
        list_add_tail(&status_info->entry, &status_head.entry);
    }

    for (int i = 0; i < (u32)*resource_num; i++) {
        res_info = (struct dev_res *)calloc(1, sizeof(struct dev_res));
        if (res_info == NULL) {
            goto read_exit;
        }
        memcpy(res_info, device_buffer, sizeof(struct dev_res));
        device_buffer += sizeof(struct dev_res);
        list_add_tail(&res_info->entry, &res_head.entry);
    }

    ret = 0;

read_exit:
    //关闭flash
    free(device_data);
    device_data = NULL;
    return ret;
}

//擦除flash函数,测试使用
void device_erase_flash()
{
    printf("%s", __func__);

    u32 profile_addr;
    FILE *profile_fp = fopen(PROFILE_PATH, "r");
    if (profile_fp == NULL) {
        return;
    }

    struct vfs_attr file_attr;
    fget_attrs(profile_fp, &file_attr);
    profile_addr = sdfile_cpu_addr2flash_addr(file_attr.sclust);
    fclose(profile_fp);

    norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, profile_addr);
}

//设备信息存储
void device_write_flash()
{
    printf("%s", __func__);

    struct list_head *pos = NULL;
    struct dev_status *status_info = NULL;
    struct dev_res *res_info = NULL;

    u8 *device_data = (u8 *)calloc(1, FLASH_BUF_LEN);
    if (!device_data) {
        return;
    }

    u32 profile_addr;
    FILE *profile_fp = fopen(PROFILE_PATH, "r");
    if (profile_fp == NULL) {
        free(device_data);
        return;
    }

    struct vfs_attr file_attr;
    fget_attrs(profile_fp, &file_attr);
    profile_addr = sdfile_cpu_addr2flash_addr(file_attr.sclust);
    fclose(profile_fp);

    u16 *device_crc 	= (u16 *)&device_data[0];
    u16 *device_len	 	= (u16 *)&device_data[2];
    u16 *status_num 	= (u16 *)&device_data[4];
    u16 *resource_num	= (u16 *)&device_data[6];

    list_for_each(pos, &status_head.entry) {
        (*status_num)++;
    }
    list_for_each(pos, &res_head.entry) {
        (*resource_num)++;
    }

    u8 *device_buffer = &device_data[8];
    *device_len = 4;

    list_for_each(pos, &status_head.entry) {
        status_info = list_entry(pos, struct dev_status, entry);
        memcpy(device_buffer, status_info, sizeof(struct dev_status));
        device_buffer += sizeof(struct dev_status);
        *device_len += sizeof(struct dev_status);
    }

    list_for_each(pos, &res_head.entry) {
        res_info = list_entry(pos, struct dev_res, entry);
        memcpy(device_buffer, res_info, sizeof(struct dev_res));
        device_buffer += sizeof(struct dev_res);
        *device_len += sizeof(struct dev_res);
    }

    *device_crc 	= CRC16((const void *)&device_data[4], (u32) * device_len);

    norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, profile_addr);
    norflash_write(NULL, device_data, (u32)*device_len + 4, profile_addr);
    free(device_data);
    device_data = NULL;
}

//设备信息初始化
static void device_status_init()
{
    struct list_head *pos = NULL;
    int total = 0;
    char tmp_buf[32];
    struct dev_status *status_info = NULL;
    struct dev_res *res_info = NULL;

//-----------------------设备状态初始化-------------------------------//
    /************************************/
    memset(tmp_buf, 0, sizeof(tmp_buf));
    strcpy(tmp_buf, "POWER");
    list_for_each(pos, &status_head.entry) {
        status_info = list_entry(pos, struct dev_status, entry);
        if (!strcmp(status_info->statusName, tmp_buf)) {
            status_info->total += 1;

            if (status_info->id_num == 0) {
                status_info->id_num = 1;
            }
        }
    }
    //获取同类别设备状态总数
    if (status_info != NULL) {
        total = status_info->total;
    } else {
        total = 0;
    }

    status_info = (struct dev_status *)calloc(1, sizeof(struct dev_status));
    if (status_info == NULL) {
        printf("%s---%dcalloc failed \r\n", __func__, __LINE__);
        return;
    }

    strcpy(status_info->statusName, tmp_buf);
    status_info->statusValue = 0;

    status_info->total 	= (total == 0) ? 1 : total;
    status_info->id_num = (total == 0) ? 0 : total;
    status_info->upload_flag = 1;		//创建时默认要上传给服务器端
    list_add_tail(&status_info->entry, &status_head.entry);
    status_info = NULL;
    total = 0;
    /************************************/

    /************************************/
    memset(tmp_buf, 0, sizeof(tmp_buf));
    strcpy(tmp_buf, "LIGHT_TEMP");
    list_for_each(pos, &status_head.entry) {
        status_info = list_entry(pos, struct dev_status, entry);
        if (!strcmp(status_info->statusName, tmp_buf)) {
            status_info->total += 1;
            total = status_info->total;

            if (status_info->id_num == 0) {
                status_info->id_num = 1;
            }
        }
    }

    status_info = (struct dev_status *)calloc(1, sizeof(struct dev_status));
    if (status_info == NULL) {
        printf("%s---%dcalloc failed \r\n", __func__, __LINE__);
        return;
    }

    strcpy(status_info->statusName, tmp_buf);
    status_info->statusValue = 12000;

    status_info->total 	= (total == 0) ? 1 : total;
    status_info->id_num = (total == 0) ? 0 : total;
    status_info->upload_flag = 1;		//创建时默认要上传给服务器端
    list_add_tail(&status_info->entry, &status_head.entry);
    status_info = NULL;
    total = 0;
    /************************************/

    /************************************/
    memset(tmp_buf, 0, sizeof(tmp_buf));
    strcpy(tmp_buf, "LIGHTNESS");
    list_for_each(pos, &status_head.entry) {
        status_info = list_entry(pos, struct dev_status, entry);
        if (!strcmp(status_info->statusName, tmp_buf)) {
            status_info->total += 1;
            total = status_info->total;

            if (status_info->id_num == 0) {
                status_info->id_num = 1;
            }
        }
    }

    status_info = (struct dev_status *)calloc(1, sizeof(struct dev_status));
    if (status_info == NULL) {
        printf("%s---%dcalloc failed \r\n", __func__, __LINE__);
        return;
    }

    strcpy(status_info->statusName, tmp_buf);
    status_info->statusValue = 100;

    status_info->total 	= (total == 0) ? 1 : total;
    status_info->id_num = (total == 0) ? 0 : total;
    status_info->upload_flag = 1;		//创建时默认要上传给服务器端
    list_add_tail(&status_info->entry, &status_head.entry);
    status_info = NULL;
    total = 0;
    /************************************/

    /************************************/
    memset(tmp_buf, 0, sizeof(tmp_buf));
    strcpy(tmp_buf, "LIGHTNESS");
    list_for_each(pos, &status_head.entry) {
        status_info = list_entry(pos, struct dev_status, entry);
        if (!strcmp(status_info->statusName, tmp_buf)) {
            status_info->total += 1;
            total = status_info->total;

            if (status_info->id_num == 0) {
                status_info->id_num = 1;
            }
        }
    }

    status_info = (struct dev_status *)calloc(1, sizeof(struct dev_status));
    if (status_info == NULL) {
        printf("%s---%dcalloc failed \r\n", __func__, __LINE__);
        return;
    }

    strcpy(status_info->statusName, tmp_buf);
    status_info->statusValue = 50;

    status_info->total 	= (total == 0) ? 1 : total;
    status_info->id_num = (total == 0) ? 0 : total;
    status_info->upload_flag = 1;		//创建时默认要上传给服务器端
    list_add_tail(&status_info->entry, &status_head.entry);
    status_info = NULL;
    total = 0;
    /************************************/

//-----------------------资源状态初始化-------------------------------//
    /************************************/
    memset(tmp_buf, 0, sizeof(tmp_buf));
    strcpy(tmp_buf, "Electricity");
    list_for_each(pos, &res_head.entry) {
        res_info = list_entry(pos, struct dev_res, entry);
        if (!strcmp(res_info->resourceName, tmp_buf)) {
            res_info->total += 1;
            total = res_info->total;

            if (res_info->id_num == 0) {
                res_info->id_num = 1;
            }
        }
    }

    res_info = (struct dev_res *)calloc(1, sizeof(struct dev_res));
    if (res_info == NULL) {
        printf("%s---%dcalloc failed \r\n", __func__, __LINE__);
        return;
    }

    strcpy(res_info->resourceName, tmp_buf);
    res_info->param.consum_ID = 123456;
    strcpy(res_info->param.consum_name, "temperature");
    res_info->param.remain_life = 30;

    res_info->total 	= (total == 0) ? 1 : total;
    res_info->id_num = (total == 0) ? 0 : total;
    res_info->upload_flag = 1;		//创建时默认要上传给服务器端
    list_add_tail(&res_info->entry, &res_head.entry);
    res_info = NULL;
    total = 0;
    /************************************/



    /************************************/
    memset(tmp_buf, 0, sizeof(tmp_buf));
    strcpy(tmp_buf, "Electricity");
    list_for_each(pos, &res_head.entry) {
        res_info = list_entry(pos, struct dev_res, entry);
        if (!strcmp(res_info->resourceName, tmp_buf)) {
            res_info->total += 1;
            total = res_info->total;

            if (res_info->id_num == 0) {
                res_info->id_num = 1;
            }
        }
    }

    res_info = (struct dev_res *)calloc(1, sizeof(struct dev_res));
    if (res_info == NULL) {
        printf("%s---%dcalloc failed \r\n", __func__, __LINE__);
        return;
    }

    strcpy(res_info->resourceName, tmp_buf);
    res_info->param.consum_ID = 123456;
    strcpy(res_info->param.consum_name, "temperature");
    res_info->param.remain_life = 20;

    res_info->total 	= (total == 0) ? 1 : total;
    res_info->id_num = (total == 0) ? 0 : total;
    res_info->upload_flag = 1;		//创建时默认要上传给服务器端
    list_add_tail(&res_info->entry, &res_head.entry);
    res_info = NULL;
    total = 0;
    /************************************/

    /************************************/
    memset(tmp_buf, 0, sizeof(tmp_buf));
    strcpy(tmp_buf, "Water");
    list_for_each(pos, &res_head.entry) {
        res_info = list_entry(pos, struct dev_res, entry);
        if (!strcmp(res_info->resourceName, tmp_buf)) {
            res_info->total += 1;
            total = res_info->total;

            if (res_info->id_num == 0) {
                res_info->id_num = 1;
            }
        }
    }

    res_info = (struct dev_res *)calloc(1, sizeof(struct dev_res));
    if (res_info == NULL) {
        printf("%s---%dcalloc failed \r\n", __func__, __LINE__);
        return;
    }

    strcpy(res_info->resourceName, tmp_buf);
    res_info->param.consum_ID = 1122;
    strcpy(res_info->param.consum_name, "LEVEL");
    res_info->param.remain_life = 10;

    res_info->total 	= (total == 0) ? 1 : total;
    res_info->id_num = (total == 0) ? 0 : total;
    res_info->upload_flag = 1;		//创建时默认要上传给服务器端
    list_add_tail(&res_info->entry, &res_head.entry);
    res_info = NULL;
    total = 0;
    /************************************/
}

//错误信息处理
static void CT_error_fun(int error)
{
    switch (error) {
    case 1001:
        log_info("Token invalid...");
        break;
    case 1002:
        log_info("Request parameter error...");
        break;
    case 1003:
        log_info("Device does not exist...");
    case 9999:
        log_info("Failed, System abnormality...");
    default:
        break;
    }
}

//初始化上报信息标志位
static void init_upload_flag()
{
    struct dev_status *status_info = NULL;
    struct dev_res *res_info = NULL;
    struct list_head *pos = NULL;
    list_for_each(pos, &status_head.entry) {
        status_info = list_entry(pos, struct dev_status, entry);
        status_info->upload_flag = 1;
    }

    list_for_each(pos, &res_head.entry) {
        res_info = list_entry(pos, struct dev_res, entry);
        res_info->upload_flag = 1;
    }

}

//deviceID登录请求
static char *CT_dev_login_ID_request(void)
{
    json_object *root_node = NULL, *first_node = NULL, *second_node = NULL;
    first_node = json_object_new_object();
    device.serial_num = rand32();
    json_object_object_add(first_node, "sequence", json_object_new_int(++device.serial_num));
    json_object_object_add(first_node, "deviceId", json_object_new_string(device.deviceId));
    json_object_object_add(first_node, "devMac", json_object_new_string(device.mac));
    json_object_object_add(first_node, "devCTEI", json_object_new_string(device.ctei));
    json_object_object_add(first_node, "devVersion", json_object_new_string(CT_DEVICE_VERSION));
#if 0 //存在父设备开启

    json_object_object_add(first_node, "parentDevMac", json_object_new_string(CT_PARENT_DEV_MAC));

#endif
    if (device.ipaddr[0] != '\0') {
        json_object_object_add(first_node, "ip", json_object_new_string(device.ipaddr));
    }
    json_object_object_add(first_node, "version", json_object_new_string(CT_VERSION));
    json_object_object_add(first_node, "time", json_object_new_int(time(NULL)));


    //encrypt
    char *encrypt_buf = NULL, *encode_buf = NULL;
    int encrypt_len;
    encrypt_buf = (char *)calloc(1, strlen(json_object_get_string(first_node)) + 16);
    if (!encrypt_buf) {
        json_object_put(first_node);
        return NULL;
    }
    CT_AES_CBC_Encrypt((u8 *)json_object_get_string(first_node), strlen(json_object_get_string(first_node)), device.pin, 16, device.iv, 16, encrypt_buf, &encrypt_len);

    encode_buf = base64_encode(encrypt_buf, encrypt_len, NULL);

    root_node = json_object_new_object();
    json_object_object_add(root_node, "code", json_object_new_int(1002));
    json_object_object_add(root_node, "deviceId", json_object_new_string(device.deviceId));
    json_object_object_add(root_node, "data", json_object_new_string(encode_buf));

    char *send_buf = (char *)calloc(1, strlen(json_object_get_string(root_node)) + 4);
    strcpy(send_buf, "CTS");
    strcat(send_buf, json_object_to_json_string_ext(root_node, JSON_C_TO_STRING_NOSLASHESCAPE));
    strcat(send_buf, "\r\n");

    if (encrypt_buf) {
        free(encrypt_buf);
    }
    if (encode_buf) {
        free(encode_buf);
    }
    json_object_put(root_node);
    json_object_put(first_node);

    return send_buf;
}

//CTEI登录请求
static char *CT_dev_login_CTEI_request(void)
{
    json_object *root_node = NULL, *first_node = NULL;
    first_node = json_object_new_object();
    device.serial_num = rand32();
    json_object_object_add(first_node, "sequence", json_object_new_int(++device.serial_num));
    json_object_object_add(first_node, "devCTEI", json_object_new_string(device.ctei));
    json_object_object_add(first_node, "devMac", json_object_new_string(device.mac));
    json_object_object_add(first_node, "devVersion", json_object_new_string(CT_DEVICE_VERSION));
#if 0 //存在父设备开启

    json_object_object_add(first_node, "parentDevMac", json_object_new_string(CT_PARENT_DEV_MAC));

#endif
    if (*device.ipaddr) {
        json_object_object_add(first_node, "ip", json_object_new_string(device.ipaddr));
    }
    json_object_object_add(first_node, "version", json_object_new_string(CT_VERSION));
    json_object_object_add(first_node, "time", json_object_new_int(time(NULL)));

    //encrypt
    char *encrypt_buf = NULL, *encode_buf = NULL;
    int encrypt_len;
    encrypt_buf = (char *)calloc(1, strlen(json_object_get_string(first_node)) + 16);
    if (!encrypt_buf) {
        printf("encrypt_buf malloc failed ----%s-----%d\r\n", __func__, __LINE__);
        json_object_put(first_node);
        return NULL;
    }
    CT_AES_CBC_Encrypt((u8 *)json_object_get_string(first_node), strlen(json_object_get_string(first_node)), device.pin, 16, device.iv, 16, encrypt_buf, &encrypt_len);

    encode_buf = base64_encode(encrypt_buf, encrypt_len, NULL);

    root_node = json_object_new_object();
    json_object_object_add(root_node, "code", json_object_new_int(1102));
    json_object_object_add(root_node, "devCTEI", json_object_new_string(device.ctei));

    json_object_object_add(root_node, "data", json_object_new_string(encode_buf));

    char *send_buf = (char *)calloc(1, strlen(json_object_get_string(root_node)) + 4);
    strcpy(send_buf, "CTS");
    strcat(send_buf, json_object_to_json_string_ext(root_node, JSON_C_TO_STRING_NOSLASHESCAPE));
    strcat(send_buf, "\r\n");

    if (encrypt_buf) {
        free(encrypt_buf);
    }
    if (encode_buf) {
        free(encode_buf);
    }
    json_object_put(root_node);
    json_object_put(first_node);

    return send_buf;
}

//业务服务器连接请求
static char *CT_dev_connect_request(void)
{
    json_object *root_node = NULL, *first_node = NULL;
    first_node = json_object_new_object();
    json_object_object_add(first_node, "sequence", json_object_new_int(++device.serial_num));
    json_object_object_add(first_node, "token", json_object_new_string(device.token));
    json_object_object_add(first_node, "devVersion", json_object_new_string(CT_DEVICE_VERSION));
    json_object_object_add(first_node, "model", json_object_new_string(device.model));
    json_object_object_add(first_node, "time", json_object_new_int(time(NULL)));

    //encrypt
    char *encrypt_buf = NULL, *encode_buf = NULL;
    int encrypt_len;
    encrypt_buf = (char *)calloc(1, strlen(json_object_get_string(first_node)) + 16);
    if (!encrypt_buf) {
        printf("encrypt_buf malloc failed ----%s-----%d\r\n", __func__, __LINE__);
        json_object_put(first_node);
        return NULL;
    }
    CT_AES_CBC_Encrypt((u8 *)json_object_get_string(first_node), strlen(json_object_get_string(first_node)), device.sessionKey, 16, device.sessionKey, 16, encrypt_buf, &encrypt_len);

    encode_buf = base64_encode(encrypt_buf, encrypt_len, NULL);

    root_node = json_object_new_object();
    json_object_object_add(root_node, "code", json_object_new_int(1004));
    json_object_object_add(root_node, "token", json_object_new_string(device.token));
    json_object_object_add(root_node, "data", json_object_new_string(encode_buf));


    char *send_buf = (char *)calloc(1, strlen(json_object_get_string(root_node)) + 4);
    strcpy(send_buf, "CTS");
    strcat(send_buf, json_object_to_json_string_ext(root_node, JSON_C_TO_STRING_NOSLASHESCAPE));
    strcat(send_buf, "\r\n");

    if (encrypt_buf) {
        free(encrypt_buf);
    }
    if (encode_buf) {
        free(encode_buf);
    }
    json_object_put(root_node);
    json_object_put(first_node);

    return send_buf;
}

//心跳请求
static char *CT_dev_heartbeat_request(void)
{
    json_object *root_node = NULL, *first_node = NULL;
    first_node = json_object_new_object();
    json_object_object_add(first_node, "sequence", json_object_new_int(++device.serial_num));
    json_object_object_add(first_node, "token", json_object_new_string(device.token));
    json_object_object_add(first_node, "time", json_object_new_int(time(NULL)));

    heart_send = 1;	//心跳包创建发送

    //encrypt
    char *encrypt_buf = NULL, *encode_buf = NULL;
    int encrypt_len;
    encrypt_buf = (char *)calloc(1, strlen(json_object_get_string(first_node)) + 16);
    if (!encrypt_buf) {
        printf("encrypt_buf malloc failed ----%s-----%d\r\n", __func__, __LINE__);
        json_object_put(first_node);
        return NULL;
    }
    CT_AES_CBC_Encrypt((u8 *)json_object_get_string(first_node), strlen(json_object_get_string(first_node)), device.sessionKey, 16, device.sessionKey, 16, encrypt_buf, &encrypt_len);

    encode_buf = base64_encode(encrypt_buf, encrypt_len, NULL);

    root_node = json_object_new_object();
    json_object_object_add(root_node, "code", json_object_new_int(1000));
    json_object_object_add(root_node, "token", json_object_new_string(device.token));
    json_object_object_add(root_node, "data", json_object_new_string(encode_buf));

    char *send_buf = (char *)calloc(1, strlen(json_object_get_string(root_node)) + 4);
    strcpy(send_buf, "CTS");
    strcat(send_buf, json_object_to_json_string_ext(root_node, JSON_C_TO_STRING_NOSLASHESCAPE));
    strcat(send_buf, "\r\n");

    if (encrypt_buf) {
        free(encrypt_buf);
    }
    if (encode_buf) {
        free(encode_buf);
    }
    json_object_put(root_node);
    json_object_put(first_node);

    return send_buf;
}

//设备控制回复
static char *CT_dev_control_reply(void)	//device->server
{
    json_object *root_node = NULL, *first_node = NULL;
    first_node = json_object_new_object();
    json_object_object_add(first_node, "result", json_object_new_int(0));
    json_object_object_add(first_node, "sequence", json_object_new_int(recv_sequence_num));
    json_object_object_add(first_node, "dscp", json_object_new_string("操作成功"));
    json_object_object_add(first_node, "deviceId", json_object_new_string(device.deviceId));
    json_object_object_add(first_node, "time", json_object_new_int(time(NULL)));

    //encrypt
    char *encrypt_buf = NULL, *encode_buf = NULL;
    int encrypt_len;
    encrypt_buf = (char *)calloc(1, strlen(json_object_get_string(first_node)) + 16);
    if (!encrypt_buf) {
        printf("encrypt_buf malloc failed ----%s-----%d\r\n", __func__, __LINE__);
        json_object_put(first_node);
        return NULL;
    }
    CT_AES_CBC_Encrypt((u8 *)json_object_get_string(first_node), strlen(json_object_get_string(first_node)), device.sessionKey, 16, device.sessionKey, 16, encrypt_buf, &encrypt_len);

    encode_buf = base64_encode(encrypt_buf, encrypt_len, NULL);

    root_node = json_object_new_object();
    json_object_object_add(root_node, "code", json_object_new_int(2004));
    json_object_object_add(root_node, "token", json_object_new_string(device.token));
    json_object_object_add(root_node, "data", json_object_new_string(encode_buf));

    char *send_buf = (char *)calloc(1, strlen(json_object_get_string(root_node)) + 4);
    strcpy(send_buf, "CTS");
    strcat(send_buf, json_object_to_json_string_ext(root_node, JSON_C_TO_STRING_NOSLASHESCAPE));
    strcat(send_buf, "\r\n");

    if (encrypt_buf) {
        free(encrypt_buf);
    }
    if (encode_buf) {
        free(encode_buf);
    }
    json_object_put(root_node);
    json_object_put(first_node);

    return send_buf;
}

//设备信息上报
static char *CT_dev_report_request(void)
{
    int status_serialId_num = 0, resource_serialId_num = 0;
    int total ;
    json_object *root_node = NULL, *first_node = NULL, *second_node[6] = {0}, *third_node[6] = {0};
    struct list_head *pos = NULL;
    struct dev_status *status_info = NULL;
    struct dev_res *res_info = NULL;

    if (!report_all) {
        init_upload_flag();
    }

    first_node = json_object_new_object();
    json_object_object_add(first_node, "sequence", json_object_new_int(++device.serial_num));
    json_object_object_add(first_node, "deviceId", json_object_new_string(device.deviceId));

    //statusSerials
    status_info = list_first_entry(&status_head.entry, struct dev_status, entry);
    if (*(status_info->statusName)) {
        second_node[0] = json_object_new_array();
        total = 1;
        list_for_each(pos, &status_head.entry) {
            status_info = list_entry(pos, struct dev_status, entry);
            //先统计要上报的状态总数
            if (status_info->upload_flag == 1) {
                total = status_info->total > total ? status_info->total + 1 : total;
            }
        }
        for (int i = 0; i < total; i++) {
            second_node[3] = NULL;
            second_node[1] = json_object_new_object();
            second_node[2] = json_object_new_array();
            list_for_each(pos, &status_head.entry) {
                status_info = list_entry(pos, struct dev_status, entry);
                if (status_info->id_num == i && status_info->upload_flag == 1) {
                    status_info->upload_flag = 0;
                    second_node[3] = json_object_new_object();
                    json_object_object_add(second_node[3], "statusName", json_object_new_string(status_info->statusName));
                    json_object_object_add(second_node[3], "curStatusValue", json_object_new_int(status_info->statusValue));
                    if (status_info->param_vaild) {
                        printf("%s into param\r\n", status_info->statusName);
                        second_node[4] = json_object_new_array();

                        //if  multple , use list_head -- 1
                        second_node[5] = json_object_new_object();
                        json_object_object_add(second_node[5], "param1", json_object_new_string("123"));
                        json_object_object_add(second_node[5], "param2", json_object_new_string("456"));

                        json_object_array_add(second_node[4], second_node[5]);
                        //if  multple , use list_head--end -- 1
                        json_object_object_add(second_node[3], "extraStatusParam", second_node[4]);
                    }
                    json_object_array_add(second_node[2], second_node[3]);
                }
            }
            if (!second_node[3]) {
                json_object_put(second_node[1]);
                second_node[1] = NULL;
                json_object_put(second_node[2]);
                second_node[2] = NULL;
            } else {
                json_object_object_add(second_node[1], "serialId", json_object_new_int(i));
                json_object_object_add(second_node[1], "statusSerial", second_node[2]);
                json_object_array_add(second_node[0], second_node[1]);
            }
        }
        if (!second_node[3]) {
            json_object_put(second_node[0]);
            second_node[0] = NULL;
        } else {
            json_object_object_add(first_node, "statusSerials", second_node[0]);
        }
    }

    //resourceSerials
    res_info = list_first_entry(&res_head.entry, struct dev_res, entry);
    if (*(res_info->resourceName)) {
        third_node[0] = json_object_new_array();
        total = 1;
        list_for_each(pos, &res_head.entry) {
            res_info = list_entry(pos, struct dev_res, entry);
            //先统计要上报的资源总数
            if (res_info->upload_flag == 1) {
                total  = res_info->total > total ? res_info->total + 1 : total;
            }
        }
        for (int i = 0; i < total; i++) {
            third_node[3] = NULL;
            third_node[1] = json_object_new_object();
            third_node[2] = json_object_new_array();
            list_for_each(pos, &res_head.entry) {
                res_info = list_entry(pos, struct dev_res, entry);
                if (res_info->id_num == i && res_info->upload_flag == 1) {
                    res_info->upload_flag = 0;
                    third_node[3] = json_object_new_object();
                    json_object_object_add(third_node[3], "resourceName", json_object_new_string(res_info->resourceName));
                    third_node[4] = json_object_new_array();

                    //if  multple , use list_head -- 2
                    third_node[5] = json_object_new_object();
                    json_object_object_add(third_node[5], "CONSUM_ID", json_object_new_int(res_info->param.consum_ID));
                    json_object_object_add(third_node[5], "CONSUM_NAME", json_object_new_string(res_info->param.consum_name));
                    json_object_object_add(third_node[5], "REMAIN_LIFE", json_object_new_int(res_info->param.remain_life));
                    json_object_array_add(third_node[4], third_node[5]);
                    //if  multple , use list_head --end -- 2

                    json_object_object_add(third_node[3], "resourceInfo", third_node[4]);
                    json_object_array_add(third_node[2], third_node[3]);
                }
            }
            if (!third_node[3]) {
                json_object_put(third_node[1]);
                third_node[1] = NULL;
                json_object_put(third_node[2]);
                third_node[2] = NULL;
            } else {
                json_object_object_add(third_node[1], "serialId", json_object_new_int(i));
                json_object_object_add(third_node[1], "resourceSerial", third_node[2]);
                json_object_array_add(third_node[0], third_node[1]);
            }

        }
        if (!third_node[3]) {
            json_object_put(third_node[0]);
            third_node[0] = NULL;
        } else {
            json_object_object_add(first_node, "resourceSerials", third_node[0]);
        }
    }
    //time
    json_object_object_add(first_node, "time", json_object_new_int(time(NULL)));

    //encrypt
    char *encrypt_buf = NULL, *encode_buf = NULL;
    int encrypt_len;
    encrypt_buf = (char *)calloc(1, strlen(json_object_get_string(first_node)) + 16);
    if (!encrypt_buf) {
        printf("encrypt_buf malloc failed ----%s-----%d\r\n", __func__, __LINE__);
        json_object_put(first_node);
        return NULL;
    }
    CT_AES_CBC_Encrypt((u8 *)json_object_get_string(first_node), strlen(json_object_get_string(first_node)), device.sessionKey, 16, device.sessionKey, 16, encrypt_buf, &encrypt_len);

    encode_buf = base64_encode(encrypt_buf, encrypt_len, NULL);

    root_node = json_object_new_object();
    json_object_object_add(root_node, "code", json_object_new_int(2006));
    json_object_object_add(root_node, "token", json_object_new_string(device.token));
    json_object_object_add(root_node, "data", json_object_new_string(encode_buf));
    char *send_buf = (char *)calloc(1, strlen(json_object_get_string(root_node)) + 4);
    strcpy(send_buf, "CTS");
    strcat(send_buf, json_object_to_json_string_ext(root_node, JSON_C_TO_STRING_NOSLASHESCAPE));
    strcat(send_buf, "\r\n");

    /* printf("\nlen = %d\nbuf = %s\r\n", strlen(json_object_get_string(first_node)), json_object_get_string(first_node)); */

    if (encrypt_buf) {
        free(encrypt_buf);
    }
    if (encode_buf) {
        free(encode_buf);
    }
    json_object_put(root_node);
    json_object_put(first_node);

    return send_buf;
}

#if 0	//#没测试到，待修改

static void CT_dev_status_reply(void)	//device->server
{
    char encrypt_buf[512];
    char *encode_buf;
    unsigned int encode_len;
    int encrypt_len;

    json_object *root_node = NULL, *first_node = NULL;
    first_node = json_object_new_object();
    json_object_object_add(first_node, "result", json_object_new_int(0));
    json_object_object_add(first_node, "sequence", json_object_new_int(device.serial_num));
    json_object_object_add(first_node, "deviceId", json_object_new_string(device.deviceId));
    json_object_object_add(first_node, "time", json_object_new_int(time(NULL)));

    //encrypt
    CT_AES_CBC_Encrypt((u8 *)json_object_get_string(first_node), strlen(json_object_get_string(first_node)), device.sessionKey, strlen(device.sessionKey), device.iv, strlen(device.iv), encrypt_buf, &encrypt_len);

    root_node = json_object_new_object();
    json_object_object_add(root_node, "code", json_object_new_int(2002));
    json_object_object_add(first_node, "token", json_object_new_string(device.token));
    json_object_object_add(root_node, "data", json_object_new_string(encrypt_buf));


    printf("***********************\r\n");
    printf("*********%s**************\r\n", __func__);
    printf("%s\r\n", json_object_get_string(root_node));

    encode_buf = base64_encode(json_object_get_string(root_node), strlen(json_object_get_string(root_node)), &encode_len);

    printf("%s\r\n", encode_buf);

    free(encode_buf);
    return;
}

static void CT_dev_manage_reply(void)	//device->server
{

    char encrypt_buf[512];
    char *encode_buf;
    unsigned int encode_len;
    int encrypt_len;

    json_object *root_node = NULL, *first_node = NULL;
    first_node = json_object_new_object();
    json_object_object_add(first_node, "result", json_object_new_int(0));
    json_object_object_add(first_node, "sequence", json_object_new_int(device.serial_num));
    json_object_object_add(first_node, "deviceId", json_object_new_string(device.deviceId));
    json_object_object_add(first_node, "time", json_object_new_int(time(NULL)));

    //encrypt
    CT_AES_CBC_Encrypt((u8 *)json_object_get_string(first_node), strlen(json_object_get_string(first_node)), device.sessionKey, strlen(device.sessionKey), device.iv, strlen(device.iv), encrypt_buf, &encrypt_len);

    root_node = json_object_new_object();
    json_object_object_add(root_node, "code", json_object_new_int(2030));
    json_object_object_add(first_node, "token", json_object_new_string(device.token));
    json_object_object_add(root_node, "data", json_object_new_string(encrypt_buf));

    printf("***********************\r\n");
    printf("*********%s**************\r\n", __func__);
    printf("%s\r\n", json_object_get_string(root_node));

    encode_buf = base64_encode(json_object_get_string(root_node), strlen(json_object_get_string(root_node)), &encode_len);

    printf("%s\r\n", encode_buf);

    free(encode_buf);
}

static void CT_dev_resource_request(void)
{
    char encrypt_buf[512];
    char *encode_buf;
    unsigned int encode_len;
    int encrypt_len;

    int resource_serialId_num = 0;
    json_object *root_node = NULL, *first_node = NULL, *second_node[5], *third_node[5];
    struct list_head *pos = NULL;
    struct dev_res *res_info = NULL;

    first_node = json_object_new_object();
    json_object_object_add(first_node, "sequence", json_object_new_int(device.serial_num++));
    json_object_object_add(first_node, "deviceId", json_object_new_string(device.deviceId));

    //resourceSerials
    res_info = list_first_entry(&res_head.entry, struct dev_res, entry);
    if (res_info != &res_head) {
        third_node[0] = json_object_new_array();
        if (list_first_entry(&res_info->entry, struct dev_res, entry) != &res_head) {
            resource_serialId_num = 1;
        } else {
            resource_serialId_num = 0;
        }
        list_for_each(pos, &res_head.entry) {
            res_info = list_entry(pos, struct dev_res, entry);
            third_node[1] = json_object_new_object();
            json_object_object_add(third_node[1], "serialId", json_object_new_int(resource_serialId_num++));
            third_node[2] = json_object_new_array();

            //if  multple , use list_head -- 1
            third_node[3] = json_object_new_object();
            json_object_object_add(second_node[3], "resourceName", json_object_new_string(res_info->resourceName));
            third_node[4] = json_object_new_array();


            //if  multple , use list_head -- 2
            third_node[5] = json_object_new_object();
            json_object_object_add(third_node[5], "CONSUM_ID", json_object_new_int(res_info->param.consum_ID));
            json_object_object_add(third_node[5], "CONSUM_NAME", json_object_new_string(res_info->param.consum_name));
            json_object_object_add(third_node[5], "REMAIN_LIFE", json_object_new_int(res_info->param.remain_life));
            json_object_array_add(third_node[4], third_node[5]);
            //if  multple , use list_head --end -- 2

            json_object_object_add(third_node[3], "resourceInfo", third_node[4]);
            json_object_array_add(third_node[2], third_node[3]);
            //if  multple , use list_head --end -- 1

            json_object_object_add(third_node[1], "resourceSerial", third_node[2]);
            json_object_array_add(third_node[0], third_node[1]);
        }
        json_object_object_add(first_node, "resourceSerials", third_node[0]);
    }
    //time
    json_object_object_add(first_node, "time", json_object_new_int(time(NULL)));

    printf("%s %d\r\n", device.sessionKey, strlen(device.sessionKey));

    //encrypt
    CT_AES_CBC_Encrypt((u8 *)json_object_get_string(first_node), strlen(json_object_get_string(first_node)), device.sessionKey, strlen(device.sessionKey), device.iv, strlen(device.iv), encrypt_buf, &encrypt_len);

    root_node = json_object_new_object();
    json_object_object_add(root_node, "code", json_object_new_int(2028));
    json_object_object_add(root_node, "token", json_object_new_string(device.token));
    json_object_object_add(root_node, "data", json_object_new_string(encrypt_buf));

    printf("***********************\r\n");
    printf("*********%s**************\r\n", __func__);
    printf("%s\r\n", json_object_get_string(first_node));
    printf("%s\r\n", json_object_get_string(root_node));

    encode_buf = base64_encode(json_object_get_string(root_node), strlen(json_object_get_string(root_node)), &encode_len);

    printf("%s\r\n", encode_buf);
    printf("***********************\r\n");

    free(encode_buf);
    json_object_put(root_node);
    json_object_put(first_node);
}

void CT_dev_message_request(void)
{
    char encrypt_buf[512];
    char *encode_buf;
    unsigned int encode_len;
    int encrypt_len;

    int event_num = 0, event_serialId_num = 0;
    json_object *root_node = NULL, *first_node = NULL, *second_node[5], *third_node[5];
    struct list_head *pos = NULL;
    struct device_eventInfo *event_info = NULL;

    first_node = json_object_new_object();
    json_object_object_add(first_node, "sequence", json_object_new_int(device.serial_num++));
    json_object_object_add(first_node, "deviceId", json_object_new_string(device.deviceId));

    //event
    second_node[0] = json_object_new_array();
    list_for_each(pos, &event_head.entry) {
        event_info = list_entry(pos, struct device_eventInfo, entry);
        second_node[1] = json_object_new_object();
        json_object_object_add(second_node[1], "eventId", json_object_new_int(event_num++));
        json_object_object_add(second_node[1], "serialId", json_object_new_int(event_serialId_num++));
        json_object_object_add(second_node[1], "eventNmame", json_object_new_string(event_info->eventName));
        second_node[2] = json_object_new_array();

        //if  multple , use list_head -- 1
        second_node[3] = json_object_new_object();
        json_object_object_add(second_node[3], event_info->param.ID_name, json_object_new_int(event_info->param.ID_value));
        json_object_object_add(second_node[3], event_info->param.TYPE_name, json_object_new_int(event_info->param.TYPE_value));
        json_object_object_add(second_node[3], "TIME", json_object_new_int(time(NULL)));


        json_object_array_add(second_node[2], second_node[3]);
        //if  multple , use list_head --end -- 1

        json_object_object_add(second_node[1], "eventInfo", second_node[2]);
        json_object_array_add(second_node[0], second_node[1]);
    }
    json_object_object_add(first_node, "event", second_node[0]);

    //time
    json_object_object_add(first_node, "time", json_object_new_int(time(NULL)));

    //encrypt
    CT_AES_CBC_Encrypt((u8 *)json_object_get_string(first_node), strlen(json_object_get_string(first_node)), device.sessionKey, strlen(device.sessionKey), device.iv, strlen(device.iv), encrypt_buf, &encrypt_len);

    root_node = json_object_new_object();
    json_object_object_add(root_node, "code", json_object_new_int(2032));
    json_object_object_add(root_node, "token", json_object_new_string(device.token));
    json_object_object_add(root_node, "data", json_object_new_string(encrypt_buf));

    printf("***********************\r\n");
    printf("*********%s**************\r\n", __func__);
    printf("%s\r\n", json_object_get_string(first_node));
    printf("%s\r\n", json_object_get_string(root_node));

    encode_buf = base64_encode(json_object_get_string(root_node), strlen(json_object_get_string(root_node)), &encode_len);

    printf("%s\r\n", encode_buf);
    printf("***********************\r\n");

    free(encode_buf);
    json_object_put(root_node);
    json_object_put(first_node);
}

void CT_dev_alarm_request(void)
{
    char encrypt_buf[512];
    char *encode_buf;
    unsigned int encode_len;
    int encrypt_len;

    int alarm_serialId_num = 0;
    struct dev_status *alarm_info = NULL;
    alarm_info = get_alarm_info();

    json_object *root_node = NULL, *first_node = NULL, *second_node[5];
    first_node = json_object_new_object();
    json_object_object_add(first_node, "sequence", json_object_new_int(device.serial_num++));
    json_object_object_add(first_node, "deviceId", json_object_new_string(device.deviceId));

    //alarm
    second_node[0] = json_object_new_object();
    json_object_object_add(second_node[0], "serialId", json_object_new_int(alarm_serialId_num++));
    json_object_object_add(second_node[0], "statusName", json_object_new_string(alarm_info->statusName));
    json_object_object_add(second_node[0], "curStatusValue", json_object_new_int(alarm_info->statusValue));
    json_object_object_add(second_node[0], "dscp", json_object_new_string("qwasdsadasdas"));
    json_object_object_add(second_node[0], "alarmtime", json_object_new_int(time(NULL)));

    json_object_object_add(first_node, "alarm", second_node[0]);
    json_object_object_add(first_node, "time", json_object_new_int(time(NULL)));

    //encrypt
    CT_AES_CBC_Encrypt((u8 *)json_object_get_string(first_node), strlen(json_object_get_string(first_node)), device.sessionKey, strlen(device.sessionKey), device.iv, strlen(device.iv), encrypt_buf, &encrypt_len);

    root_node = json_object_new_object();
    json_object_object_add(root_node, "code", json_object_new_int(2008));
    json_object_object_add(first_node, "token", json_object_new_string(device.token));
    json_object_object_add(root_node, "data", json_object_new_string(encrypt_buf));

    printf("***********************\r\n");
    printf("*********%s**************\r\n", __func__);
    printf("%s\r\n", json_object_get_string(root_node));

    encode_buf = base64_encode(json_object_get_string(root_node), strlen(json_object_get_string(root_node)), &encode_len);

    printf("%s\r\n", encode_buf);

    free(encode_buf);
}

void CT_dev_fault_request(void)
{
    char encrypt_buf[512];
    char *encode_buf;
    unsigned int encode_len;
    int encrypt_len;

    int error_serialId_num = 0;

    json_object *root_node = NULL, *first_node = NULL, *second_node[5];
    first_node = json_object_new_object();
    json_object_object_add(first_node, "sequence", json_object_new_int(device.serial_num++));
    json_object_object_add(first_node, "deviceId", json_object_new_string(device.deviceId));

    //error
    second_node[0] = json_object_new_object();
    json_object_object_add(second_node[0], "serialId", json_object_new_int(error_serialId_num++));
    json_object_object_add(second_node[0], "errorCode", json_object_new_int(100001));
    json_object_object_add(second_node[0], "errorInfo", json_object_new_string("qwasdsadasdas"));
    json_object_object_add(second_node[0], "errortime", json_object_new_int(time(NULL)));

    json_object_object_add(first_node, "error", second_node[0]);
    json_object_object_add(first_node, "time", json_object_new_int(time(NULL)));

    //encrypt
    CT_AES_CBC_Encrypt((u8 *)json_object_get_string(first_node), strlen(json_object_get_string(first_node)), device.sessionKey, strlen(device.sessionKey), device.iv, strlen(device.iv), encrypt_buf, &encrypt_len);

    root_node = json_object_new_object();
    json_object_object_add(root_node, "code", json_object_new_int(2010));
    json_object_object_add(first_node, "token", json_object_new_string(device.token));
    json_object_object_add(root_node, "data", json_object_new_string(encrypt_buf));

    printf("***********************\r\n");
    printf("*********%s**************\r\n", __func__);
    printf("%s\r\n", json_object_get_string(root_node));

    encode_buf = base64_encode(json_object_get_string(root_node), strlen(json_object_get_string(root_node)), &encode_len);

    printf("%s\r\n", encode_buf);

    free(encode_buf);
}

void CT_sub_dev_ID_auth_request(void)
{
    char encrypt_buf[512];
    char *encode_buf;
    unsigned int encode_len;
    int encrypt_len;

    json_object *root_node = NULL, *first_node = NULL, *second_node[5];
    first_node = json_object_new_object();
    json_object_object_add(first_node, "sequence", json_object_new_int(device.serial_num++));
    json_object_object_add(first_node, "deviceId", json_object_new_string(device.deviceId));
    json_object_object_add(first_node, "subDeviceId", json_object_new_string(CT_SUB_DEVICE_ID));
    json_object_object_add(first_node, "pin", json_object_new_string(device.pin));
#if (defined CT_SUB_DEVICE_MAC)
    json_object_object_add(first_node, "subDevMac", json_object_new_string(CT_SUB_DEVICE_MAC));
#endif

#if (defined CT_SUB_DEVICE_CTEI)
    json_object_object_add(first_node, "subDevCTEI", json_object_new_string(CT_SUB_DEVICE_CTEI));
#endif

#if (defined CT_PARENT_DEV_MAC)
    json_object_object_add(first_node, "parentDevMac", json_object_new_string(CT_PARENT_DEV_MAC));
#endif

#if (defined CT_SUB_DEVICE_IP)
    json_object_object_add(first_node, "subDevIp", json_object_new_string(CT_SUB_DEVICE_IP));
#endif

#if (defined CT_SUB_DEVICE_VERSION)
    json_object_object_add(first_node, "subVersion", json_object_new_string(CT_SUB_DEVICE_VERSION));
#endif

    json_object_object_add(first_node, "time", json_object_new_int(time(NULL)));

    //encrypt
    CT_AES_CBC_Encrypt((u8 *)json_object_get_string(first_node), strlen(json_object_get_string(first_node)), device.sessionKey, strlen(device.sessionKey), device.iv, strlen(device.iv), encrypt_buf, &encrypt_len);

    root_node = json_object_new_object();
    json_object_object_add(root_node, "code", json_object_new_int(2012));
    json_object_object_add(first_node, "token", json_object_new_string(device.token));
    json_object_object_add(root_node, "data", json_object_new_string(encrypt_buf));

    printf("***********************\r\n");
    printf("*********%s**************\r\n", __func__);
    printf("%s\r\n", json_object_get_string(root_node));

    encode_buf = base64_encode(json_object_get_string(root_node), strlen(json_object_get_string(root_node)), &encode_len);

    printf("%s\r\n", encode_buf);

    free(encode_buf);
}

void CT_sub_dev_CTEI_auth_request(void)
{
    char encrypt_buf[512];
    char *encode_buf;
    unsigned int encode_len;
    int encrypt_len;

    json_object *root_node = NULL, *first_node = NULL, *second_node[5];
    first_node = json_object_new_object();
    json_object_object_add(first_node, "sequence", json_object_new_int(device.serial_num++));
    json_object_object_add(first_node, "deviceId", json_object_new_string(device.deviceId));
#if (defined CT_SUB_DEVICE_MAC)
    json_object_object_add(first_node, "subDevMac", json_object_new_string(CT_SUB_DEVICE_MAC));
#endif
#if (defined CT_SUB_DEVICE_VERSION)
    json_object_object_add(first_node, "subVersion", json_object_new_string(CT_SUB_DEVICE_VERSION));
#endif
    json_object_object_add(first_node, "pin", json_object_new_string(device.pin));
    json_object_object_add(first_node, "time", json_object_new_int(time(NULL)));

    //encrypt
    CT_AES_CBC_Encrypt((u8 *)json_object_get_string(first_node), strlen(json_object_get_string(first_node)), device.sessionKey, strlen(device.sessionKey), device.iv, strlen(device.iv), encrypt_buf, &encrypt_len);

    root_node = json_object_new_object();
    json_object_object_add(root_node, "code", json_object_new_int(2112));
    json_object_object_add(first_node, "token", json_object_new_string(device.token));
    json_object_object_add(root_node, "data", json_object_new_string(encrypt_buf));

    printf("***********************\r\n");
    printf("*********%s**************\r\n", __func__);
    printf("%s\r\n", json_object_get_string(root_node));

    encode_buf = base64_encode(json_object_get_string(root_node), strlen(json_object_get_string(root_node)), &encode_len);

    printf("%s\r\n", encode_buf);

    free(encode_buf);
}

void CT_sub_dev_unbind_reply(void)
{
    char encrypt_buf[512];
    char *encode_buf;
    unsigned int encode_len;
    int encrypt_len;

    json_object *root_node = NULL, *first_node = NULL, *second_node[5];
    first_node = json_object_new_object();
    json_object_object_add(first_node, "result", json_object_new_int(0));
    json_object_object_add(first_node, "sequence", json_object_new_int(device.serial_num));
    json_object_object_add(first_node, "deviceId", json_object_new_string(device.deviceId));
    json_object_object_add(first_node, "subDeviceId", json_object_new_string(CT_SUB_DEVICE_ID));
    json_object_object_add(first_node, "time", json_object_new_int(time(NULL)));

    //encrypt
    CT_AES_CBC_Encrypt((u8 *)json_object_get_string(first_node), strlen(json_object_get_string(first_node)), device.sessionKey, strlen(device.sessionKey), device.iv, strlen(device.iv), encrypt_buf, &encrypt_len);

    root_node = json_object_new_object();
    json_object_object_add(root_node, "code", json_object_new_int(2014));
    json_object_object_add(first_node, "token", json_object_new_string(device.token));
    json_object_object_add(root_node, "data", json_object_new_string(encrypt_buf));

    printf("***********************\r\n");
    printf("*********%s**************\r\n", __func__);
    printf("%s\r\n", json_object_get_string(root_node));

    encode_buf = base64_encode(json_object_get_string(root_node), strlen(json_object_get_string(root_node)), &encode_len);

    printf("%s\r\n", encode_buf);

    free(encode_buf);
}

void CT_sub_dev_bind_request(void)
{
    char encrypt_buf[512];
    char *encode_buf;
    unsigned int encode_len;
    int encrypt_len;

    json_object *root_node = NULL, *first_node = NULL, *second_node[5];
    first_node = json_object_new_object();
    json_object_object_add(first_node, "sequence", json_object_new_int(device.serial_num++));
    json_object_object_add(first_node, "deviceId", json_object_new_string(device.deviceId));
    json_object_object_add(first_node, "subDeviceId", json_object_new_string(CT_SUB_DEVICE_ID));
    json_object_object_add(first_node, "time", json_object_new_int(time(NULL)));

    //encrypt
    CT_AES_CBC_Encrypt((u8 *)json_object_get_string(first_node), strlen(json_object_get_string(first_node)), device.sessionKey, strlen(device.sessionKey), device.iv, strlen(device.iv), encrypt_buf, &encrypt_len);

    root_node = json_object_new_object();
    json_object_object_add(root_node, "code", json_object_new_int(2016));
    json_object_object_add(first_node, "token", json_object_new_string(device.token));
    json_object_object_add(root_node, "data", json_object_new_string(encrypt_buf));

    printf("***********************\r\n");
    printf("*********%s**************\r\n", __func__);
    printf("%s\r\n", json_object_get_string(root_node));

    encode_buf = base64_encode(json_object_get_string(root_node), strlen(json_object_get_string(root_node)), &encode_len);

    printf("%s\r\n", encode_buf);

    free(encode_buf);
}

void CT_sub_dev_unbind_request(void)
{
    char encrypt_buf[512];
    char *encode_buf;
    unsigned int encode_len;
    int encrypt_len;

    json_object *root_node = NULL, *first_node = NULL, *second_node[5];
    first_node = json_object_new_object();
    json_object_object_add(first_node, "sequence", json_object_new_int(device.serial_num++));
    json_object_object_add(first_node, "deviceId", json_object_new_string(device.deviceId));
    json_object_object_add(first_node, "subDeviceId", json_object_new_string(CT_SUB_DEVICE_ID));
    json_object_object_add(first_node, "time", json_object_new_int(time(NULL)));

    //encrypt
    CT_AES_CBC_Encrypt((u8 *)json_object_get_string(first_node), strlen(json_object_get_string(first_node)), device.sessionKey, strlen(device.sessionKey), device.iv, strlen(device.iv), encrypt_buf, &encrypt_len);

    root_node = json_object_new_object();
    json_object_object_add(root_node, "code", json_object_new_int(2018));
    json_object_object_add(first_node, "token", json_object_new_string(device.token));
    json_object_object_add(root_node, "data", json_object_new_string(encrypt_buf));

    printf("***********************\r\n");
    printf("*********%s**************\r\n", __func__);
    printf("%s\r\n", json_object_get_string(root_node));

    encode_buf = base64_encode(json_object_get_string(root_node), strlen(json_object_get_string(root_node)), &encode_len);

    printf("%s\r\n", encode_buf);

    free(encode_buf);
}

void CT_sub_dev_online_request(void)
{
    char encrypt_buf[512];
    char *encode_buf;
    unsigned int encode_len;
    int encrypt_len;

    json_object *root_node = NULL, *first_node = NULL, *second_node[5];
    first_node = json_object_new_object();
    json_object_object_add(first_node, "sequence", json_object_new_int(device.serial_num++));
    json_object_object_add(first_node, "deviceId", json_object_new_string(device.deviceId));
    json_object_object_add(first_node, "subDeviceId", json_object_new_string(CT_SUB_DEVICE_ID));
    json_object_object_add(first_node, "onLine", json_object_new_int(1));
    json_object_object_add(first_node, "time", json_object_new_int(time(NULL)));

    //encrypt
    CT_AES_CBC_Encrypt((u8 *)json_object_get_string(first_node), strlen(json_object_get_string(first_node)), device.sessionKey, strlen(device.sessionKey), device.iv, strlen(device.iv), encrypt_buf, &encrypt_len);

    root_node = json_object_new_object();
    json_object_object_add(root_node, "code", json_object_new_int(2020));
    json_object_object_add(first_node, "token", json_object_new_string(device.token));
    json_object_object_add(root_node, "data", json_object_new_string(encrypt_buf));


    printf("***********************\r\n");
    printf("*********%s**************\r\n", __func__);
    printf("%s\r\n", json_object_get_string(root_node));

    encode_buf = base64_encode(json_object_get_string(root_node), strlen(json_object_get_string(root_node)), &encode_len);

    printf("%s\r\n", encode_buf);

    free(encode_buf);
}

void CT_dev_version_request(void)
{
    char encrypt_buf[512];
    char *encode_buf;
    unsigned int encode_len;
    int encrypt_len;

    json_object *root_node = NULL, *first_node = NULL, *second_node[5];
    first_node = json_object_new_object();
    json_object_object_add(first_node, "sequence", json_object_new_int(device.serial_num++));
    json_object_object_add(first_node, "deviceId", json_object_new_string(device.deviceId));
    json_object_object_add(first_node, "model", json_object_new_string(device.model));

    second_node[0] = json_object_new_object();
    json_object_object_add(second_node[0], "version", json_object_new_string(CT_DEVICE_VERSION));
    json_object_object_add(first_node, "device", second_node[0]);

    if (0) {		//have modules
        second_node[1] = json_object_new_array();

        //if  multple , use list_head
        second_node[2] = json_object_new_object();
        json_object_object_add(second_node[2], "name", json_object_new_string("123"));
        json_object_object_add(second_node[2], "version", json_object_new_string("1.2.3"));
        json_object_array_add(second_node[1], second_node[2]);
        //if  multple , use list_head --end
        json_object_object_add(first_node, "modules", second_node[1]);

    }
    json_object_object_add(first_node, "time", json_object_new_int(time(NULL)));

    //encrypt
    CT_AES_CBC_Encrypt((u8 *)json_object_get_string(first_node), strlen(json_object_get_string(first_node)), device.sessionKey, strlen(device.sessionKey), device.iv, strlen(device.iv), encrypt_buf, &encrypt_len);

    root_node = json_object_new_object();
    json_object_object_add(root_node, "code", json_object_new_int(2022));
    json_object_object_add(first_node, "token", json_object_new_string(device.token));
    json_object_object_add(root_node, "data", json_object_new_string(encrypt_buf));

    printf("***********************\r\n");
    printf("*********%s**************\r\n", __func__);
    printf("%s\r\n", json_object_get_string(root_node));

    encode_buf = base64_encode(json_object_get_string(root_node), strlen(json_object_get_string(root_node)), &encode_len);

    printf("%s\r\n", encode_buf);

    free(encode_buf);
}

void CT_dev_upgrade_reply(void)	//device->server
{

    char encrypt_buf[512];
    char *encode_buf;
    unsigned int encode_len;
    int encrypt_len;

    json_object *root_node = NULL, *first_node = NULL;
    first_node = json_object_new_object();
    json_object_object_add(first_node, "result", json_object_new_int(0));
    json_object_object_add(first_node, "sequence", json_object_new_int(device.serial_num));
    json_object_object_add(first_node, "deviceId", json_object_new_string(device.deviceId));
    json_object_object_add(first_node, "time", json_object_new_int(time(NULL)));

    //encrypt
    CT_AES_CBC_Encrypt((u8 *)json_object_get_string(first_node), strlen(json_object_get_string(first_node)), device.sessionKey, strlen(device.sessionKey), device.iv, strlen(device.iv), encrypt_buf, &encrypt_len);

    root_node = json_object_new_object();
    json_object_object_add(root_node, "code", json_object_new_int(2024));
    json_object_object_add(first_node, "token", json_object_new_string(device.token));
    json_object_object_add(root_node, "data", json_object_new_string(encrypt_buf));


    printf("***********************\r\n");
    printf("*********%s**************\r\n", __func__);
    printf("%s\r\n", json_object_get_string(root_node));

    encode_buf = base64_encode(json_object_get_string(root_node), strlen(json_object_get_string(root_node)), &encode_len);

    printf("%s\r\n", encode_buf);

    free(encode_buf);
}

void CT_dev_upgrade_result_request(void)
{
    char encrypt_buf[512];
    char *encode_buf;
    unsigned int encode_len;
    int encrypt_len;
    int type = 0, upgrade_status = 0;

    json_object *root_node = NULL, *first_node = NULL, *second_node[5];
    first_node = json_object_new_object();
    json_object_object_add(first_node, "sequence", json_object_new_int(device.serial_num++));
    json_object_object_add(first_node, "deviceId", json_object_new_string(device.deviceId));
    json_object_object_add(first_node, "type", json_object_new_int(type));
    if (type == 1) {
        json_object_object_add(first_node, "moduleName", json_object_new_string("1asdas"));
    }
    json_object_object_add(first_node, "status", json_object_new_int(upgrade_status));
    json_object_object_add(first_node, "version", json_object_new_string(CT_DEVICE_VERSION));
    json_object_object_add(first_node, "percent", json_object_new_string("20\%"));
    json_object_object_add(first_node, "time", json_object_new_int(time(NULL)));

    //encrypt
    CT_AES_CBC_Encrypt((u8 *)json_object_get_string(first_node), strlen(json_object_get_string(first_node)), device.sessionKey, strlen(device.sessionKey), device.iv, strlen(device.iv), encrypt_buf, &encrypt_len);

    root_node = json_object_new_object();
    json_object_object_add(root_node, "code", json_object_new_int(2026));
    json_object_object_add(first_node, "token", json_object_new_string(device.token));
    json_object_object_add(root_node, "data", json_object_new_string(encrypt_buf));

    printf("***********************\r\n");
    printf("*********%s**************\r\n", __func__);
    printf("%s\r\n", json_object_get_string(root_node));

    encode_buf = base64_encode(json_object_get_string(root_node), strlen(json_object_get_string(root_node)), &encode_len);

    printf("%s\r\n", encode_buf);

    free(encode_buf);
}

void CT_voice_control_request(void)
{
    char encrypt_buf[512];
    char *encode_buf;
    unsigned int encode_len;
    int encrypt_len;
    int extensions = 0;

    json_object *root_node = NULL, *first_node = NULL, *second_node[5];
    first_node = json_object_new_object();
    //需要配合sn，islast判断是否加1
    json_object_object_add(first_node, "sequence", json_object_new_int(device.serial_num++));
    json_object_object_add(first_node, "deviceId", json_object_new_string(device.deviceId));

    second_node[0] = json_object_new_object();
    if (1) {
        json_object_object_add(second_node[0], "format", json_object_new_string("amr"));
    }
    if (1) {
        json_object_object_add(second_node[0], "rate", json_object_new_int(8000));
    }
    if (1) {
        json_object_object_add(second_node[0], "channel", json_object_new_int(1));
    }
    if (1) {
        json_object_object_add(second_node[0], "vocieId", json_object_new_string("12312321"));
    }
    if (1) {
        json_object_object_add(second_node[0], "sn", json_object_new_int(1));
    }
    if (1) {
        json_object_object_add(second_node[0], "isLast", json_object_new_int(0));
    }
    json_object_object_add(second_node[0], "audioData", json_object_new_string(" "));
    if (extensions == 1) {
        second_node[1] = json_object_new_array();
        //if  multple , use list_head
        second_node[2] = json_object_new_object();
        json_object_object_add(second_node[2], "key", json_object_new_string("213"));
        json_object_object_add(second_node[2], "value", json_object_new_int(1));
        json_object_array_add(second_node[1], second_node[2]);
        //if  multple , use list_head --end

        json_object_object_add(second_node[0], "extensions", second_node[1]);
    }
    json_object_object_add(first_node, "CtrlParam", second_node[0]);

    json_object_object_add(first_node, "time", json_object_new_int(time(NULL)));

    //encrypt
    CT_AES_CBC_Encrypt((u8 *)json_object_get_string(first_node), strlen(json_object_get_string(first_node)), device.sessionKey, strlen(device.sessionKey), device.iv, strlen(device.iv), encrypt_buf, &encrypt_len);

    root_node = json_object_new_object();
    json_object_object_add(root_node, "code", json_object_new_int(3002));
    json_object_object_add(first_node, "token", json_object_new_string(device.token));
    json_object_object_add(root_node, "data", json_object_new_string(encrypt_buf));

    printf("***********************\r\n");
    printf("*********%s**************\r\n", __func__);
    printf("%s\r\n", json_object_get_string(root_node));

    encode_buf = base64_encode(json_object_get_string(root_node), strlen(json_object_get_string(root_node)), &encode_len);

    printf("%s\r\n", encode_buf);

    free(encode_buf);
}

void CT_speech_synthesis_request(void)
{
    char encrypt_buf[512];
    char *encode_buf;
    unsigned int encode_len;
    int encrypt_len;
    int extensions = 0;

    json_object *root_node = NULL, *first_node = NULL, *second_node[5];
    first_node = json_object_new_object();
    json_object_object_add(first_node, "sequence", json_object_new_int(device.serial_num++));
    json_object_object_add(first_node, "deviceId", json_object_new_string(device.deviceId));

    second_node[0] = json_object_new_object();
    json_object_object_add(second_node[0], "content", json_object_new_string("已经为你打开空调!"));
    if (1) {
        json_object_object_add(second_node[0], "anchor", json_object_new_string("xiaoyan"));
    }
    if (1) {
        json_object_object_add(second_node[0], "speed", json_object_new_int(123));
    }
    if (1) {
        json_object_object_add(second_node[0], "volume", json_object_new_int(123));
    }
    if (1) {
        json_object_object_add(second_node[0], "pitch", json_object_new_int(123));
    }
    if (1) {
        json_object_object_add(second_node[0], "format", json_object_new_string("amr"));
    }
    if (1) {
        json_object_object_add(second_node[0], "rate", json_object_new_int(8000));
    }
    if (1) {
        json_object_object_add(second_node[0], "channel", json_object_new_int(1));
    }
    json_object_object_add(first_node, "ttsParam", second_node[0]);

    json_object_object_add(first_node, "time", json_object_new_int(time(NULL)));

    //encrypt
    CT_AES_CBC_Encrypt((u8 *)json_object_get_string(first_node), strlen(json_object_get_string(first_node)), device.sessionKey, strlen(device.sessionKey), device.iv, strlen(device.iv), encrypt_buf, &encrypt_len);

    root_node = json_object_new_object();
    json_object_object_add(root_node, "code", json_object_new_int(3000));
    json_object_object_add(first_node, "token", json_object_new_string(device.token));
    json_object_object_add(root_node, "data", json_object_new_string(encrypt_buf));

    printf("***********************\r\n");
    printf("*********%s**************\r\n", __func__);
    printf("%s\r\n", json_object_get_string(root_node));

    encode_buf = base64_encode(json_object_get_string(root_node), strlen(json_object_get_string(root_node)), &encode_len);

    printf("%s\r\n", encode_buf);

    free(encode_buf);
}

void CT_dev_data_download_reply(void)
{
    char encrypt_buf[512];
    char *encode_buf;
    unsigned int encode_len;
    int encrypt_len;

    int error_serialId_num = 0;

    json_object *root_node = NULL, *first_node = NULL, *second_node[5];
    first_node = json_object_new_object();
    json_object_object_add(first_node, "result", json_object_new_int(0));
    json_object_object_add(first_node, "sequence", json_object_new_int(device.serial_num));
    json_object_object_add(first_node, "deviceId", json_object_new_string(device.deviceId));
    json_object_object_add(first_node, "time", json_object_new_int(time(NULL)));
    json_object_object_add(first_node, "downDataResp", json_object_new_string("1232132131"));

    //encrypt
    CT_AES_CBC_Encrypt((u8 *)json_object_get_string(first_node), strlen(json_object_get_string(first_node)), device.sessionKey, strlen(device.sessionKey), device.iv, strlen(device.iv), encrypt_buf, &encrypt_len);

    root_node = json_object_new_object();
    json_object_object_add(root_node, "code", json_object_new_int(9000));
    json_object_object_add(first_node, "token", json_object_new_string(device.token));
    json_object_object_add(root_node, "data", json_object_new_string(encrypt_buf));

    printf("***********************\r\n");
    printf("*********%s**************\r\n", __func__);
    printf("%s\r\n", json_object_get_string(root_node));

    encode_buf = base64_encode(json_object_get_string(root_node), strlen(json_object_get_string(root_node)), &encode_len);

    printf("%s\r\n", encode_buf);

    free(encode_buf);
}

void CT_dev_data_upload_request(void)
{
    char encrypt_buf[512];
    char *encode_buf;
    unsigned int encode_len;
    int encrypt_len;

    int error_serialId_num = 0;

    json_object *root_node = NULL, *first_node = NULL, *second_node[5];
    first_node = json_object_new_object();
    json_object_object_add(first_node, "sequence", json_object_new_int(device.serial_num++));
    json_object_object_add(first_node, "deviceId", json_object_new_string(device.deviceId));
    json_object_object_add(first_node, "time", json_object_new_int(time(NULL)));
    json_object_object_add(first_node, "upData", json_object_new_string("1232132131"));

    //encrypt
    CT_AES_CBC_Encrypt((u8 *)json_object_get_string(first_node), strlen(json_object_get_string(first_node)), device.sessionKey, strlen(device.sessionKey), device.iv, strlen(device.iv), encrypt_buf, &encrypt_len);

    root_node = json_object_new_object();
    json_object_object_add(root_node, "code", json_object_new_int(9002));
    json_object_object_add(first_node, "token", json_object_new_string(device.token));
    json_object_object_add(root_node, "data", json_object_new_string(encrypt_buf));

    printf("***********************\r\n");
    printf("*********%s**************\r\n", __func__);
    printf("%s\r\n", json_object_get_string(root_node));

    encode_buf = base64_encode(json_object_get_string(root_node), strlen(json_object_get_string(root_node)), &encode_len);

    printf("%s\r\n", encode_buf);

    free(encode_buf);
}

#endif

static void CT_server_to_device_fun(void *priv)
{
    u8 connecting = 1;
    char cmdName[32] = {0}, cmdParam[128] = {0}, parse_buf[64] = {0};
    unsigned int decrypt_len;
    int parse_len, decode_len, code, msg[32], err, serial_num;
    char *decode_buf = NULL, *decrypt_buf = NULL;

    json_object *root_node = NULL, *first_node = NULL, *second_node = NULL;

    while (connecting) {
        err = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (err != OS_TASKQ || msg[0] != Q_USER) {
            continue;
        }

        switch (msg[1]) {
        case RECV_MSG:

            root_node = json_tokener_parse(strstr((char *)msg[2], "{"));
            if (!root_node) {
                log_info("%d buf1 root_node error", __LINE__);
                goto exit;
            }
            free((char *)msg[2]);

            code = json_object_get_int(json_object_object_get(root_node, "code"));

            if (!json_object_object_get_ex(root_node, "data", &first_node)) {
                goto exit;
            }
            json_object_get(first_node);
            json_object_put(root_node);
            root_node = NULL;

            decode_buf = base64_decode(json_object_get_string(first_node), strlen(json_object_get_string(first_node)), &decode_len);
            decrypt_buf = (char *)calloc(1, decode_len + 1);
            if (!decrypt_buf) {
                log_info("------%s-----------%d------malloc failed", __func__, __LINE__);
                goto exit;
            }
            if ((code == 1003) || (code == 1103)) {
                CT_AES_CBC_Decrypt(decode_buf, decode_len, device.pin, 16, device.iv, 16, decrypt_buf, &decrypt_len);
            } else {
                CT_AES_CBC_Decrypt(decode_buf, decode_len, device.sessionKey, 16, device.sessionKey, 16, decrypt_buf, &decrypt_len);
            }

            log_info("code = %d\nbuf = %s", code, decrypt_buf);
            //json
            root_node = json_tokener_parse(decrypt_buf);
            if (!root_node) {
                log_info("%d root_node NULL", __LINE__);
                goto exit;
            }
            if (decode_buf) {
                free(decode_buf);
                decode_buf = NULL;
            }
            if (decrypt_buf) {
                free(decrypt_buf);
                decrypt_buf = NULL;
            }

            if (code == 2003 || code == 2005 || code == 2031 || code == 2015 || code == 2025 || code == 9001) {
                recv_sequence_num = json_object_get_int(json_object_object_get(root_node, "sequence"));
            } else {
                if (0 != json_object_get_int(json_object_object_get(root_node, "result"))) {
                    log_info("%s------%d result not 0", __func__, __LINE__);
                    if (code == 2007) {
                        init_upload_flag();
                    }
                    CT_error_fun(json_object_get_int(json_object_object_get(root_node, "result")));
                    goto exit;
                }

                serial_num = json_object_get_int(json_object_object_get(root_node, "sequence"));
                if (code == 1001) {
                    heart_send = 0;
                }
                if ((device.serial_num - heart_send) != serial_num) {
                    log_info("%d serial_num not match\n\n\n", __LINE__);
                    goto exit;
                }
            }

            switch (code) {
            //-----------设备通道层接口
            case 1003: //设备ID登录响应
                if (*json_object_get_string(json_object_object_get(root_node, "sessionKey"))) {
                    strcpy(device.sessionKey, json_object_get_string(json_object_object_get(root_node, "sessionKey")));
                }


                if (*json_object_get_string(json_object_object_get(root_node, "tcpHost"))) {
                    strcpy(parse_buf, json_object_get_string(json_object_object_get(root_node, "tcpHost")));
                    parse_len = strchr(parse_buf, ':') - parse_buf	+ 1;
                    strncpy(device.tcpHost, parse_buf, parse_len - 1);
                    strcpy(device.tcp_port, parse_buf + parse_len);
                    memset(parse_buf, 0, sizeof(parse_buf));
                }

                if (*json_object_get_string(json_object_object_get(root_node, "udpHost"))) {
                    strcpy(parse_buf, json_object_get_string(json_object_object_get(root_node, "udpHost")));
                    parse_len = strchr(parse_buf, ':') - parse_buf	+ 1;
                    strncpy(device.udpHost, parse_buf, parse_len - 1);
                    strcpy(device.udp_port, parse_buf + parse_len);
                    memset(parse_buf, 0, sizeof(parse_buf));
                }

                if (*json_object_get_string(json_object_object_get(root_node, "token"))) {
                    strcpy(device.token, json_object_get_string(json_object_object_get(root_node, "token")));
                }

                log_info("sessionkey = %s, tcpHost = %s, tcp_port = %s,udpHost = %s,udp_port = %s, token = %s\r\n", device.sessionKey, device.tcpHost, device.tcp_port, device.udpHost, device.udp_port, device.token);

                for (int i = 0; i < 3; i++) {
                    if (0 == tcp_device_init(NULL)) {		//连接业务服务器
                        thread_fork("msg_recv_handler", 8, 2048, 0, NULL, tcp_recv_handler, NULL);
                        //发送信息
                        tcp_send_function(1004);
                        break;
                    } else {
                        printf("------%s-----%d connect server failed ,i = %d\r\n", __func__, __LINE__, i);
                    }
                }
                break;

            case 1005:	//设备连接响应
                device.login_flag = 1;	//设备登录标志位置1
                //heatBeat
                if (*json_object_get_string(json_object_object_get(root_node, "heartBeat"))) {
                    device.heartBeat = json_object_get_int(json_object_object_get(root_node, "heartBeat"));
                } else {
                    device.heartBeat = 30;
                }

                //authInterval  如果不设置就默认为0， 前3次60s 后续600s
                if (*json_object_get_string(json_object_object_get(root_node, "authInterval"))) {
                    device.authInterval = json_object_get_int(json_object_object_get(root_node, "authInterval"));
                }
                //上报设备状态跟资源信息 code:2006
                tcp_send_function(2006);
                if (!heartbeat_id) {
                    heartbeat_id = sys_timer_add_to_task("app_core", NULL, heartbeat_ctl_func, device.heartBeat * 1000);
                }

                if (heartbeat_timeout_id) {
                    sys_timer_modify(heartbeat_timeout_id, (device.heartBeat * 3 + 10) * 1000);
                } else {
                    heartbeat_timeout_id = sys_timeout_add_to_task("app_core", NULL, timeout_func, (device.heartBeat * 3 + 10) * 1000);
                }

                break;

            case 1001:	//设备心跳响应
                //将心跳计数标志位清零 重新计数超时
                if (heartbeat_timeout_id) {
                    sys_timer_re_run(heartbeat_timeout_id);
                }

                break;

            case 1103:	//设备ctei登录响应
                if (*json_object_get_string(json_object_object_get(root_node, "sessionKey"))) {
                    strcpy(device.sessionKey, json_object_get_string(json_object_object_get(root_node, "sessionKey")));
                }

                if (*json_object_get_string(json_object_object_get(root_node, "tcpHost"))) {
                    strcpy(parse_buf, json_object_get_string(json_object_object_get(root_node, "tcpHost")));
                    parse_len = strchr(parse_buf, ':') - parse_buf	+ 1;
                    strncpy(device.tcpHost, parse_buf, parse_len - 1);
                    strcpy(device.tcp_port, parse_buf + parse_len);
                    memset(parse_buf, 0, sizeof(parse_buf));
                }

                if (*json_object_get_string(json_object_object_get(root_node, "udpHost"))) {
                    strcpy(parse_buf, json_object_get_string(json_object_object_get(root_node, "udpHost")));
                    parse_len = strchr(parse_buf, ':') - parse_buf	+ 1;
                    strncpy(device.udpHost, parse_buf, parse_len - 1);
                    strcpy(device.udp_port, parse_buf + parse_len);
                    memset(parse_buf, 0, sizeof(parse_buf));
                }
                if (*json_object_get_string(json_object_object_get(root_node, "deviceId"))) {
                    strcpy(device.deviceId, json_object_get_string(json_object_object_get(root_node, "deviceId")));
                }

                if (*json_object_get_string(json_object_object_get(root_node, "token"))) {
                    strcpy(device.token, json_object_get_string(json_object_object_get(root_node, "token")));
                }

                log_info("sessionkey = %s, tcpHost = %s, tcp_port = %s,udpHost = %s,udp_port = %s,deviceId = %s token = %s\r\n", device.sessionKey, device.tcpHost, device.tcp_port, device.udpHost, device.udp_port, device.deviceId, device.token);

                for (int i = 0; i < 3; i++) {
                    if (0 == tcp_device_init(NULL)) {		//连接业务服务器
                        thread_fork("msg_recv_handler", 8, 2048, 0, NULL, tcp_recv_handler, NULL);
                        //发送信息
                        tcp_send_function(1004);
                        break;
                    } else {
                        printf(" %d connect server failed\r\n", i);
                    }
                }
                break;

            //------------设备业务层接口
            case 2003:	//设备状态查询请求 #
                if (device.deviceId == json_object_get_string(json_object_object_get(root_node, "deviceId"))) {
                    //服务器是查找本设备，10s内上报设备最新状态
                } else {
                    //查询设备绑定的子设备是否存在ID与deviceId相同，若存在则将该命令转换来查询对应的子设备
                }
                break;

            case 2005:	//设备状态控制请求 #
                if (!strcmp(device.deviceId, json_object_get_string(json_object_object_get(root_node, "deviceId")))) {
                    //服务器是查找本设备，10s内上报设备最新状态
                    log_info("into deviceid\r\n");

                    int tmp_serial = json_object_get_int(json_object_object_get(root_node, "serialId"));

                    for (int i = 0;; i++) {
                        second_node = json_object_array_get_idx(json_object_object_get(root_node, "cmd"), i);
                        if (second_node == NULL) {
                            break;
                        }
                        strcpy(cmdName, json_object_get_string(json_object_object_get(second_node, "cmdName")));
                        strcpy(cmdParam, json_object_get_string(json_object_object_get(second_node, "cmdParam")));
                        status_set_func(tmp_serial, cmdName, cmdParam);
                        memset(cmdName, 0, sizeof(cmdName));
                        memset(cmdParam, 0, sizeof(cmdParam));

                    }

                    tcp_send_function(2004);
                    if (device_status_id) {
                        sys_timer_re_run(device_status_id);
                    } else {
                        //延迟5S后汇报
                        device_status_id = sys_timeout_add(NULL, device_status_func, 5000);
                    }
                } else {
                    //查询设备绑定的子设备是否存在ID与deviceId相同，若存在则将该命令转换来查询对应的子设备
                }
                break;

            case 2007:	//设备状态/资源上报响应
                report_all = 1;
                //可选择性的在此处进行flash更新
                thread_fork("write_flash", 20, 1024, 0, NULL, device_write_flash, NULL);
                if (!strcmp(device.deviceId, json_object_get_string(json_object_object_get(root_node, "deviceId")))) {
                    //服务器是查找本设备
                } else {
                    //查询设备绑定的子设备是否存在ID与deviceId相同，若存在则将该命令转换来响应对应的子设备
                }
                break;
#if 0	//#没测试到，待修改
            case 2031:	//设备资源管理请求 #
                if (!strcmp(device.deviceId, json_object_get_string(json_object_object_get(root_node, "deviceId")))) {
                    //服务器是查找本设备，10s内上报设备最新状态
                } else {
                    //查询设备绑定的子设备是否存在ID与deviceId相同，若存在则将该命令转换来查询对应的子设备
                }
                break;

            case 2029:	//设备资源变化上报响应
                if (!strcmp(device.deviceId, json_object_get_string(json_object_object_get(root_node, "deviceId")))) {
                    //服务器是查找本设备，10s内上报设备最新状态
                } else {
                    //查询设备绑定的子设备是否存在ID与deviceId相同，若存在则将该命令转换来查询对应的子设备
                }
                break;

            case 2033:	//设备信息上报响应
                if (!strcmp(device.deviceId, json_object_get_string(json_object_object_get(root_node, "deviceId")))) {
                    //服务器是查找本设备，10s内上报设备最新状态
                } else {
                    //查询设备绑定的子设备是否存在ID与deviceId相同，若存在则将该命令转换来查询对应的子设备
                }
                break;

            case 2009:	//设备状态告警响应
                if (!strcmp(device.deviceId, json_object_get_string(json_object_object_get(root_node, "deviceId")))) {
                    //服务器是查找本设备，10s内上报设备最新状态
                } else {
                    //查询设备绑定的子设备是否存在ID与deviceId相同，若存在则将该命令转换来查询对应的子设备
                }
                break;

            case 2011:	//设备故障反馈响应
                if (!strcmp(device.deviceId, json_object_get_string(json_object_object_get(root_node, "deviceId")))) {
                    //服务器是查找本设备，10s内上报设备最新状态
                } else {
                    //查询设备绑定的子设备是否存在ID与deviceId相同，若存在则将该命令转换来查询对应的子设备
                }
                break;

            case 2013:	//子设备ID鉴权响应
                if (!strcmp(device.deviceId, json_object_get_string(json_object_object_get(root_node, "deviceId")))) {
                    //看对应子设备ID authResult 为0 则鉴权成功
                }

                break;

            case 2015:	//子设备解绑请求 #
                if (!strcmp(device.deviceId, json_object_get_string(json_object_object_get(root_node, "deviceId")))) {
                    //看对应子设备ID 进行下线解绑操作
                }
                break;

            case 2017:	//子设备绑定上报响应
                if (!strcmp(device.deviceId, json_object_get_string(json_object_object_get(root_node, "deviceId")))) {
                    //看对应子设备ID
                }

                break;

            case 2019:	//子设备解绑上报响应
                if (!strcmp(device.deviceId, json_object_get_string(json_object_object_get(root_node, "deviceId")))) {
                    //看对应子设备ID
                }

                break;

            case 2021:	//子设备在线状态上报响应
                if (!strcmp(device.deviceId, json_object_get_string(json_object_object_get(root_node, "deviceId")))) {
                    //看对应子设备ID
                }

                break;

            case 2023:	//设备版本上报响应
                if (!strcmp(device.deviceId, json_object_get_string(json_object_object_get(root_node, "deviceId")))) {
                    //
                }

                break;

            case 2025:	//设备版本升级通知请求 #
                if (!strcmp(device.deviceId, json_object_get_string(json_object_object_get(root_node, "deviceId")))) {
                    //
                }

                break;

            case 2027:	//设备版本升级结果上报响应
                if (!strcmp(device.deviceId, json_object_get_string(json_object_object_get(root_node, "deviceId")))) {
                    //
                }

                break;

            case 3003:	//语音控制响应
                if ((400002 || 0) != atoi(json_object_get_string(json_object_object_get(root_node, "result")))) {
                    log_info("%d result not 0", __LINE__);
                    CT_error_fun(atoi(json_object_get_string(json_object_object_get(root_node, "result"))));
                    break;
                }

                if (!strcmp(device.deviceId, json_object_get_string(json_object_object_get(root_node, "deviceId")))) {
                    //
                }
                if (json_object_object_get_ex(root_node, "ctrlResult", &first_node)) {
                    char ctrl_buf[512] = {0};
                    strcpy(ctrl_buf, json_object_get_string(first_node));
                    //进行后续操作
                }

                break;

            case 3001:	//语音合成响应
                if (!strcmp(device.deviceId, json_object_get_string(json_object_object_get(root_node, "deviceId")))) {
                    //
                }
                if (json_object_object_get_ex(root_node, "ttsResult", &first_node)) {
                    char tts_buf[512] = {0};
                    strcpy(tts_buf, json_object_get_string(first_node));
                    //进行后续操作
                }

                break;

            case 2113:	//子设备ctei鉴权
                if (!strcmp(device.deviceId, json_object_get_string(json_object_object_get(root_node, "deviceId")))) {
                    //看对应子设备ID authResult 为0 则鉴权成功
                }

                break;

            case 9001:	//设备数据下行请求 #
                if (!strcmp(device.deviceId, json_object_get_string(json_object_object_get(root_node, "deviceId")))) {

                }

                //downData

                break;

            case 9003:	//设备数据上行响应
                if (!strcmp(device.deviceId, json_object_get_string(json_object_object_get(root_node, "deviceId")))) {

                }

                //upData

                break;
#endif

deault:
                break;

            }
exit:
            json_object_put(second_node);
            second_node = NULL;
            json_object_put(first_node);
            first_node = NULL;
            json_object_put(root_node);
            root_node = NULL;
            if (decode_buf) {
                free(decode_buf);
                decode_buf = NULL;
            }
            if (decrypt_buf) {
                free(decrypt_buf);
                decrypt_buf = NULL;
            }
            break;

        case CONNECT_CLOSE:
            connecting = 0;
            break;

        default:
            break;
        }
    }
}

//recv
static void tcp_recv_handler(void *priv)
{
    int recv_len = 0;
    char *ptr;
    char recv_buf[1024] = {0};

    for (;;) {
        recv_len = sock_recvfrom(device.tcp_sock, recv_buf + recv_len, sizeof(recv_buf), 0, NULL, NULL);
        if (recv_len > 0) {
            ptr = strstr(recv_buf, "\r\n");

            if (ptr != NULL) {
                printf("recv completed!\r\n");
                char *tmp_buf = (char *)calloc(1, strlen(recv_buf) + 1);
                strcpy(tmp_buf, recv_buf);
                os_taskq_post("server_to_device", 2, RECV_MSG, (int *)tmp_buf);
                recv_len = 0;
                memset(recv_buf, 0, sizeof(recv_buf));
            }
        } else {
            printf("tcp recv len = %d\r\n", recv_len);
            break;
        }
    }
}

static int tcp_device_init(char *tcp_host)
{
    char ipaddr[20];
    if (device.tcp_sock) {
        sock_unreg(device.tcp_sock);
        device.tcp_sock = NULL;
    }

    if (tcp_host != NULL) {
        memset(&device.dest, 0, sizeof(device.dest));
        struct hostent *hent = NULL;
        int cnt = 0;
        char tmp_host[50] = {0}, *pA = NULL;
        strcpy(tmp_host, tcp_host);
        pA = strpbrk(tmp_host, ":/");
        if (NULL == pA) {
            cnt = strlen(tmp_host);
        } else {
            cnt = pA - tmp_host;
        }
        tmp_host[cnt] = '\0';
        pA = strstr(tcp_host, tmp_host) + strlen(tmp_host);
        if (':' == *pA) {
            pA++;
            device.dest.sin_port = htons(atoi(pA));
        } else {
            device.dest.sin_port = htons(80);
        }
        hent = gethostbyname(tmp_host);
        if (hent == NULL) {
            printf("hent NULL\r\n");
            return -1;
        }
        memcpy(&device.dest.sin_addr, hent->h_addr, sizeof(struct in_addr));
    } else {
        memset(&device.dest, 0, sizeof(device.dest));
        device.dest.sin_addr.s_addr = inet_addr(device.tcpHost);
        device.dest.sin_port = htons(atoi(device.tcp_port));
    }

    device.tcp_sock = sock_reg(AF_INET, SOCK_STREAM, 0, NULL, NULL);
    if (device.tcp_sock == NULL) {
        log_info("%s tcp: sock_reg fail\n",  __func__);
        return -1;
    }
    if (0 != sock_set_reuseaddr(device.tcp_sock)) {
        sock_unreg(device.tcp_sock);
        return -1;
    }
    sock_set_connect_to(device.tcp_sock, 10000);
    /* sock_set_recv_timeout(device.tcp_sock, 40000); */
    sock_set_send_timeout(device.tcp_sock, 10000);

    Get_IPAddress(1, device.ipaddr);

    device.local.sin_addr.s_addr = inet_addr(device.ipaddr);
    device.local.sin_port = htons(0);
    device.local.sin_family = AF_INET;
    if (0 != sock_bind(device.tcp_sock, (struct sockaddr *)&device.local, sizeof(struct sockaddr_in))) {
        sock_unreg(device.tcp_sock);
        log_info("%s sock_bind fail\n",  __func__);
        return -1;
    }


    device.dest.sin_family = AF_INET;
    if (0 != sock_connect(device.tcp_sock, (struct sockaddr *)&device.dest, sizeof(struct sockaddr_in))) {
        log_info("sock_connect fail.\n");
        printf("ip addr = 0x%x,port = 0x%x\r\n", device.dest.sin_addr.s_addr, device.dest.sin_port);
        sock_unreg(device.tcp_sock);
        return -1;
    }


    return 0;
}

static void device_login_func(void *priv)
{
    u8 i = 0;
    while (1) {
        if (!CT_connect_flag) {
            printf("-------%s-------%d\r\n", __func__, __LINE__);
            return;
        }
        for (int cnt = 0; cnt < 3; cnt++) {
            if (!tcp_device_init(CT_LOGIN_HOST)) {
                goto _login;
            }
        }
        if (device.authInterval) {
            os_time_dly(device.authInterval * 100);
        } else {
            if (i < 3) {
                os_time_dly(60 * 100);
                i++;
            } else {
                os_time_dly(600 * 100);
            }
        }
    }
_login:
    //创建线程，用于接收tcp_server的数据
    thread_fork("login_recv_handler", 8, 2048, 20, NULL, tcp_recv_handler, NULL);

    if (!server_func_id) {
        thread_fork("server_to_device", 8, 8192, 20, &server_func_id, CT_server_to_device_fun, NULL);
    }

    tcp_send_function(1102);

    if (!heartbeat_timeout_id) {
        heartbeat_timeout_id = sys_timeout_add_to_task("app_core", NULL, timeout_func, 20 * 1000);
    }
    return ;

}

static void timeout_func(void *priv)
{
    printf("into timeout\r\n");
    device.login_flag = 0;
    report_all = 0;
    heart_send = 0;
    if (device.tcp_sock) {
        sock_set_quit(device.tcp_sock);
        sock_unreg(device.tcp_sock);	//断开服务器socket
        device.tcp_sock = NULL;
    }

    if (heartbeat_id) {
        sys_timer_del(heartbeat_id);
        heartbeat_id = 0;
    }

    if (heartbeat_timeout_id) {
        sys_timer_del(heartbeat_timeout_id);
        heartbeat_timeout_id = 0;
    }

    if (!dev_login_pid) {
        thread_fork("device_login", 8, 4096, 0, &dev_login_pid, device_login_func, NULL);
    }
}

static void device_status_func(void *priv)
{
    tcp_send_function(2006);
}
static void heartbeat_ctl_func(void *priv)
{
    tcp_send_function(1000);
}

static void status_set_func(int serial, char *name, char *param)
{
    int type  = 0, power_status = 0;
    struct dev_status *status_info = NULL;
    struct list_head *pos = NULL;
    if (!strcmp(name, "SET_POWER")) {
        type = 1;
    } else if (!strcmp(name, "SET_LIGHT_TEMP")) {
        type = 2;
    } else if (!strcmp(name, "SET_LIGHTNESS")) {
        type = 3;
    } else if (!strcmp(name, "SET_SOUND")) {
        type = 4;
    }

    list_for_each(pos, &status_head.entry) {
        status_info = list_entry(pos, struct dev_status, entry);
        if (type == 1) {
            if (!strcmp(status_info->statusName, "POWER") && status_info->id_num == serial) {
                printf("into power\r\n");
                power_status = atoi(param);
                if (power_status == 2) {
                    power_status = status_info->statusValue ? 0 : 1;
                }
                status_info->statusValue = power_status;
                status_info->upload_flag = 1;
                break;
            }
        } else if (type == 2) {
            if (!strcmp(status_info->statusName, "LIGHT_TEMP") && status_info->id_num == serial) {
                printf("into light temp\r\n");
                status_info->statusValue = atoi(param);
                status_info->upload_flag = 1;
                break;
            }
        } else if (type == 3) {
            if (!strcmp(status_info->statusName, "LIGHTNESS") && status_info->id_num == serial) {
                printf("into lightness\r\n");
                status_info->statusValue = atoi(param);
                status_info->upload_flag = 1;
                break;
            }
        } else if (type == 4) {
            if (!strcmp(status_info->statusName, "SOUND") && status_info->id_num == serial) {
                printf("into sound\r\n");
                status_info->statusValue = 0; 		//待解决
                status_info->upload_flag = 1;
                printf("sound = %s\r\n", param);
                break;
            }
        }
    }
}

static void tcp_send_function(int code)
{
    int len = 0, s_len = 0;
    char *data = NULL;
    printf("code = %d\r\n", code);
    switch (code) {
    case 1004:
        data = CT_dev_connect_request();
        break;
    case 1102:
        data = CT_dev_login_CTEI_request();
        break;
    case 2006:
        data = CT_dev_report_request();
        break;
    case 1000:
        data = CT_dev_heartbeat_request();
        break;
    case 2004:
        data = CT_dev_control_reply();
        break;
    default:
        break;

    }

    int ret = sock_send(device.tcp_sock, data, strlen(data), 0);

    if (data) {
        free(data);
    }
}

static int CT_state_check(void)
{
    if (device.login_flag) {
        return AI_STAT_CONNECTED;
    }
    return AI_STAT_DISCONNECTED;

}

static int CT_connect(void)
{
    CT_connect_flag = 1;
    if (!status_head.entry.next) {
        info_init();
    }
    if (list_empty(&status_head.entry)) {
        if (device_read_flash()) {
            printf("no flash save,goto init------%s-----%d\r\n", __func__, __LINE__);
            device_status_init();
        }
    }
    device.login_flag = 0;
    if (!dev_login_pid) {
        thread_fork("device_login", 8, 2048, 0, &dev_login_pid, device_login_func, NULL);
    }
    return 0;
}

static int CT_do_event(int event, int arg)
{
    log_info("do_event : %d", event);
    return 0;
}

static int CT_disconnect(void)
{
    CT_connect_flag = 0;
    report_all = 0;
    device.login_flag = 0;
    sock_unreg(device.tcp_sock);
    device.tcp_sock = NULL;
    if (heartbeat_id) {
        sys_timer_del(heartbeat_id);
        heartbeat_id = 0;
    }
    if (heartbeat_timeout_id) {
        sys_timeout_del(heartbeat_timeout_id);
        heartbeat_timeout_id = 0;
    }
    os_taskq_post("server_to_device", 1, CONNECT_CLOSE);
    return 0;
}

static void info_init()
{
    INIT_LIST_HEAD(&status_head.entry);	//初始化状态节点头
    INIT_LIST_HEAD(&res_head.entry);	//初始化资源节点头
    /* INIT_LIST_HEAD(&event_head.entry);	//初始化事件节点头 */

    //设备信息请向电信方获取
    char pin_iv[] = "G8NT7SQWC5JT67IJ4LW62S0YUCVD6YR3";
    strcpy(device.model, "ZD18");
    strncpy(device.pin, pin_iv, 16);
    strncpy(device.iv, pin_iv + 16, 16);
    printf("pin = %s \niv = %s\r\n", device.pin, device.iv);
    strcpy(device.mac, "38C3DEFE1AA8");		//12
    strcpy(device.ctei, "184579850000003");	//15
    strcpy(device.deviceId, "00A1140101080210037376982606879673");	//34

}

REGISTER_AI_SDK(CT_MC_api) = {
    .name			= "China_Telecom_Mc",
    .connect		= CT_connect,
    .state_check	= CT_state_check,
    .do_event		= CT_do_event,
    .disconnect		= CT_disconnect,
};
