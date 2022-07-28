#include "os/os_api.h"
#include "device/device.h"
#include "wifi/wifi_connect.h"
#include "json_c/json.h"
#include "json_c/json_tokener.h"
#include "lwip.h"
#include "http/http_cli.h"
#include "app_config.h"
#include "system/database.h"
#include "system/init.h"
#include "boot.h"
#include "asm/sfc_norflash_api.h"
#include "asm/crc16.h"
#include "syscfg/syscfg_id.h"

extern int bytecmp(unsigned char *p, unsigned char ch, unsigned int num);

struct get_mac_oauth_ctx {
    void (*callback)(void);
    char auth_key[33];
    char code[33];
    char uuid[50];
    httpcli_ctx ctx;
};
static struct get_mac_oauth_ctx *get_mac_oauth_ctx;
static int get_macaddr_task_pid;
static char server_assign_macaddr_ok, get_macaddr_task_exit_flag;
static char s_mac_addr[6];
static OS_MUTEX mutex;

#define PROFILE_HOST 				"profile.jieliapp.com"
#define GET_GET_MACADDR 			"https://"PROFILE_HOST"/license/v1/device/macaddress/auth?auth_key=%s&code=%s"
#define GET_GET_MACADDR_SUCC 		"https://"PROFILE_HOST"/license/v1/device/macaddress/check?uuid=%s"

extern char *get_macaddr_auth_key(void);
extern char *get_macaddr_code(void);

static int __https_get_mothed(const char *url, int (*cb)(char *, void *), void *priv)
{
    int error = 0;
    http_body_obj http_body_buf;

    memset(&http_body_buf, 0x0, sizeof(http_body_obj));
    memset(&get_mac_oauth_ctx->ctx, 0x0, sizeof(httpcli_ctx));

    http_body_buf.recv_len = 0;
    http_body_buf.buf_len = 5 * 1024;
    http_body_buf.buf_count = 1;
    http_body_buf.p = (char *)malloc(http_body_buf.buf_len * sizeof(char));

    get_mac_oauth_ctx->ctx.url = url;
    get_mac_oauth_ctx->ctx.priv = &http_body_buf;
    get_mac_oauth_ctx->ctx.connection = "close";
    get_mac_oauth_ctx->ctx.timeout_millsec = 10000;

    error = httpcli_get(&get_mac_oauth_ctx->ctx);

    if (error == HERROR_OK) {
        if (cb) {
            error = cb(http_body_buf.p, priv);
        } else {
            error = 0;
        }
    } else {
        error = -1;
    }

    if (http_body_buf.p) {
        free(http_body_buf.p);
    }

    return error;
}

static int set_flash_wifi_mac(char *mac)
{
#if 1 //写到BT_IF
    syscfg_write(CFG_BT_MAC_ADDR, mac, 6);
#else //写到最后4K区域
    u32 flash_info_capacity;
    u8 *reserve_4k_area = malloc(4096);
    if (reserve_4k_area == NULL) {
        return -1;
    }
    norflash_ioctl(NULL, IOCTL_GET_CAPACITY, (u32)&flash_info_capacity);
    norflash_read(NULL, reserve_4k_area, 4096, flash_info_capacity - 4096);
    norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, flash_info_capacity - 4096);
    memcpy(reserve_4k_area + 4096 - 256 + 0x10 * 4, mac, 6);
    u16 mac_value_crc = CRC16((const void *)mac, 6);
    reserve_4k_area[4096 - 256 + 0x10 * 4 + 6] = (mac_value_crc >> 8) & 0xFF;
    reserve_4k_area[4096 - 256 + 0x10 * 4 + 6 + 1] = (mac_value_crc & 0xFF);
    norflash_write(NULL, reserve_4k_area, 4096, flash_info_capacity - 4096);
    free(reserve_4k_area);

    memcpy(boot_info.mac.value, mac, 6);
    boot_info.mac.value_crc = ((mac_value_crc & 0xFF) << 8) | ((mac_value_crc >> 8) & 0xFF);
#endif
    wifi_set_mac(mac);
    set_wireless_netif_macaddr(mac);

    printf("set_flash_mac addr valid,[%02x:%02x:%02x:%02x:%02x:%02x] \n", (u8)mac[0], (u8)mac[1], (u8)mac[2], (u8)mac[3], (u8)mac[4], (u8)mac[5]);

    return 0;
}

static int get_flash_mac_addr(char mac[6])
{
    if (server_assign_macaddr_ok) {
        memcpy(mac, s_mac_addr, 6);
        return 0;
    }

    if (syscfg_read(CFG_BT_MAC_ADDR, mac, 6) <= 0) {
        puts("get_flash_mac_addr syscfg_read fail!\r\n");
        return -1;
    }

    if ((mac[0] & ((1 << 0) | (1 << 1))) || !bytecmp((u8 *)mac, 0, 6) || !bytecmp((u8 *)mac, 0xff, 6)) {
        printf("get_flash_mac_addr format error [%02x:%02x:%02x:%02x:%02x:%02x] \r\n", (unsigned char)mac[0], (unsigned char)mac[1], (unsigned char)mac[2], (unsigned char)mac[3], (unsigned char)mac[4], (unsigned char)mac[5]);
        return -1;
    }

    memcpy(s_mac_addr, mac, 6);
    server_assign_macaddr_ok = 1;

    printf("get_flash_mac_addr valid [%02x:%02x:%02x:%02x:%02x:%02x] \r\n", (unsigned char)mac[0], (unsigned char)mac[1], (unsigned char)mac[2], (unsigned char)mac[3], (unsigned char)mac[4], (unsigned char)mac[5]);

    return 0;
}

static int oauth_get_macaddr(char *resp_buf, void *priv)
{
    json_object *new_obj;
    const char *json_str, *mac, *uuid;

    json_str = strstr(resp_buf, "{\"");
    if (!json_str) {
        return -1;
    }

    new_obj = json_tokener_parse(json_str);
    if (!new_obj) {
        return -1;
    }

    mac = json_object_get_string(json_object_object_get(json_object_object_get(new_obj, "data"), "mac"));
    uuid = json_object_get_string(json_object_object_get(json_object_object_get(new_obj, "data"), "uuid"));
    if (!mac || mac[0] == '\0' || !uuid || uuid[0] == '\0' || !strcmp(mac, "000000000000")) {
        printf("get server macaddr  error : %s \n", resp_buf);
        goto EXIT;
    }
    strcpy(get_mac_oauth_ctx->uuid, uuid);

    u8 mac_data[6];
    char __mac[16] = {0};
    u32 tmp;

    strncpy(__mac, mac, sizeof(__mac) - 1);
    //bug fix
    sscanf(&__mac[10], "%x", &tmp);
    mac_data[5] = tmp;
    __mac[10] = '\0';
    sscanf(&__mac[8], "%x", &tmp);
    mac_data[4] = tmp;
    __mac[8] = '\0';
    sscanf(&__mac[6], "%x", &tmp);
    mac_data[3] = tmp;
    __mac[6] = '\0';
    sscanf(&__mac[4], "%x", &tmp);
    mac_data[2] = tmp;
    __mac[4] = '\0';
    sscanf(&__mac[2], "%x", &tmp);
    mac_data[1] = tmp;
    __mac[2] = '\0';
    sscanf(&__mac[0], "%x", &tmp);
    mac_data[0] = tmp;

    set_flash_wifi_mac((char *)mac_data);

    json_object_put(new_obj);

    return 0;

EXIT:
    json_object_put(new_obj);
    return -1;
}

static void __get_macaddr_task(void *priv)
{
    int ret;
    char url[256];

    sprintf(url, GET_GET_MACADDR, get_mac_oauth_ctx->auth_key, get_mac_oauth_ctx->code);

    while (!get_macaddr_task_exit_flag) {
        ret = __https_get_mothed(url, oauth_get_macaddr, NULL);

        if (!ret) {
            sprintf(url, GET_GET_MACADDR_SUCC, get_mac_oauth_ctx->uuid);
            __https_get_mothed(url, NULL, NULL);
            get_mac_oauth_ctx->callback();
            break;
        }
        os_time_dly(200);
    }

    os_mutex_pend(&mutex, 0);
    free(get_mac_oauth_ctx);
    get_mac_oauth_ctx = NULL;
    os_mutex_post(&mutex);
}

char is_server_assign_macaddr_ok(void)
{
    return server_assign_macaddr_ok;
}

int server_assign_macaddr(void (*callback)(void))
{
    const char *auth_key, *code;

    if (get_macaddr_task_pid) {
        return 0;
    }

    auth_key = get_macaddr_auth_key();
    code = get_macaddr_code();

    if (!auth_key || !code) {
        printf("server_assign_macaddr fail\n");
        return -1;
    }

    os_mutex_pend(&mutex, 0);
    get_mac_oauth_ctx = calloc(1, sizeof(struct get_mac_oauth_ctx));
    if (!get_mac_oauth_ctx) {
        printf("server_assign_macaddr malloc fail\n");
        os_mutex_post(&mutex);
        return -1;
    }
    get_mac_oauth_ctx->callback = callback;
    os_mutex_post(&mutex);

    snprintf(get_mac_oauth_ctx->auth_key, sizeof(get_mac_oauth_ctx->auth_key), "%s", auth_key);
    snprintf(get_mac_oauth_ctx->code, sizeof(get_mac_oauth_ctx->code), "%s", code);

    return thread_fork("get_macaddr_task", 22, 521 * 3, 0, &get_macaddr_task_pid, __get_macaddr_task, NULL);
}

void cancel_server_assign_macaddr(void)
{
    os_mutex_pend(&mutex, 0);
    if (get_mac_oauth_ctx) {
        get_macaddr_task_exit_flag = 1;
        http_cancel_dns(&get_mac_oauth_ctx->ctx);
        os_mutex_post(&mutex);
        thread_kill(&get_macaddr_task_pid, KILL_WAIT);
        get_macaddr_task_exit_flag = 0;
    } else {
        os_mutex_post(&mutex);
    }
}

#ifdef CONFIG_ASSIGN_MACADDR_ENABLE

int init_net_device_mac_addr(char *macaddr, char ap_mode)
{
    static u8 first = 1;
    if (-1 == get_flash_mac_addr(macaddr)) {
        if (first) {

            u8 flash_uid[16];
            memcpy(flash_uid, get_norflash_uuid(), 16);
            do {
                u32 crc32 = JL_RAND->R64H ^ CRC32(flash_uid, sizeof(flash_uid));
                u16 crc16 = JL_RAND->R64L ^ CRC16(flash_uid, sizeof(flash_uid));
                memcpy(macaddr, &crc32, sizeof(crc32));
                memcpy(&macaddr[4], &crc16, sizeof(crc16));
            } while (!bytecmp((u8 *)macaddr, 0, 6));
            //此处用户可自行修改为本地生成mac地址的算法
            macaddr[0] &= ~((1 << 0) | (1 << 1));
            memcpy(s_mac_addr, macaddr, 6);
            first = 0;
        } else {
            memcpy(macaddr, s_mac_addr, 6);
        }
        if (ap_mode) {
            set_flash_wifi_mac(macaddr); //AP模式需要保存mac地址,否则改密码 ios手机 重连有问题
        }
        printf("wifi use flash_uid+random mac[%x:%x:%x:%x:%x:%x] \r\n", (unsigned char)macaddr[0], (unsigned char)macaddr[1], (unsigned char)macaddr[2], (unsigned char)macaddr[3], (unsigned char)macaddr[4], (unsigned char)macaddr[5]);
    }

    return 0;
}

static int mutex_init(void)
{
    return os_mutex_create(&mutex);
}
early_initcall(mutex_init);

#else

int init_net_device_mac_addr(char *macaddr, char ap_mode)
{
    if (-1 == get_flash_mac_addr(macaddr)) {

        u8 flash_uid[16];
        memcpy(flash_uid, get_norflash_uuid(), 16);
        do {
            u32 crc32 = JL_RAND->R64H ^ CRC32(flash_uid, sizeof(flash_uid));
            u16 crc16 = JL_RAND->R64L ^ CRC16(flash_uid, sizeof(flash_uid));
            memcpy(macaddr, &crc32, sizeof(crc32));
            memcpy(&macaddr[4], &crc16, sizeof(crc16));
        } while (!bytecmp((u8 *)macaddr, 0, 6));
        //此处用户可自行修改为本地生成mac地址的算法
        macaddr[0] &= ~((1 << 0) | (1 << 1));
        memcpy(s_mac_addr, macaddr, 6);
        set_flash_wifi_mac(macaddr);
        server_assign_macaddr_ok = 1;
    }

    printf("wifi use flash_uid+random mac[%x:%x:%x:%x:%x:%x] \r\n", (unsigned char)macaddr[0], (unsigned char)macaddr[1], (unsigned char)macaddr[2], (unsigned char)macaddr[3], (unsigned char)macaddr[4], (unsigned char)macaddr[5]);

    return 0;
}

#endif
