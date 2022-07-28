#include "mssdp/mssdp.h"
#include "device/device.h"
#include "camera.h"
#include "fs/fs.h"
#include "app_config.h"
#include "dev_desc.h"
#include "gpio.h"
#include "json_c/json.h"
#include "json_c/json_tokener.h"
#include "event/event.h"


static char json_buffer[1024];
static char MassProduction_mode = 0;
const char *__attribute__((weak))get_dev_desc_doc()
{
    int ret;

    memset(json_buffer, 0, 1024);
    memcpy(json_buffer, DEV_DESC_CONTENT, strlen(DEV_DESC_CONTENT));

    return json_buffer;
}

char get_MassProduction(void)
{
#ifdef CONFIG_MASS_PRODUCTION_ENABLE
#ifdef CONFIG_PRODUCTION_IO_PORT
    char ret  = MassProduction_mode;

    if (!ret) {
        gpio_set_die(CONFIG_PRODUCTION_IO_PORT, 1);
        gpio_set_pull_up(CONFIG_PRODUCTION_IO_PORT, !CONFIG_PRODUCTION_IO_STATE);
        gpio_set_pull_down(CONFIG_PRODUCTION_IO_PORT, CONFIG_PRODUCTION_IO_STATE);
        gpio_direction_input(CONFIG_PRODUCTION_IO_PORT);
        delay(1000);
        ret = gpio_read(CONFIG_PRODUCTION_IO_PORT);
        MassProduction_mode = (ret == CONFIG_PRODUCTION_IO_STATE) ? true : false;
        ret = MassProduction_mode;
    }
    return ret;
#else
    return 1;//1量产模式
#endif
#else
    return 0;
#endif
}

const char *create_mssdp_notify_msg(char *msg, u16_t dev_online_flag)
{
    char ip_addr[64];
    extern char wifi_get_rssi(void);
    extern void Get_IPAddress(char is_wireless, char *ipaddr);
    extern int storage_device_ready();
    Get_IPAddress(1, ip_addr);

#ifdef CONFIG_VIDEO_720P
    sprintf(msg, "{\"RTSP\":\"rtsp://%s/avi_rt/front.hd\",\
\"SIGNAL\":\"%d\",\
\"SD_STATUS\":\"%d\",\
\"DEV_ONLINE_FLAG\":\"%d\"\
}", \
            ip_addr, \
            wifi_get_rssi(), \
            storage_device_ready(), \
            dev_online_flag);
#else
    sprintf(msg, "{\"RTSP\":\"rtsp://%s/avi_rt/front.sd\",\
\"SIGNAL\":\"%d\",\
\"SD_STATUS\":\"%d\",\
\"DEV_ONLINE_FLAG\":\"%d\"\
}", \
            ip_addr, \
            wifi_get_rssi(), \
            storage_device_ready(), \
            dev_online_flag);
#endif

    return msg;

}
u8 video0_dev_wr_reg(u8 addr, u8 data, u8 is_write, u8 close, u8 *logo)
{
    int cmd;
    int wr_data = 0;
    static void *video_dev = NULL;
    if (!video_dev) {
        video_dev = dev_open("video0.0", NULL); //用来设置摄像头参数
        if (!video_dev) {
            printf("video open err \n");
            return -EINVAL;
        }
    }
    if (logo) {
        dev_ioctl(video_dev, CAMERA_GET_SENSOR_TYPE, (u32)logo);
    } else if (!close) {
        if (is_write) {
            cmd = CAMERA_WITE_BIT | addr;
            wr_data = data;
            dev_ioctl(video_dev, cmd, (u32)wr_data);
            wr_data = 0;
        } else {
            cmd = CAMERA_READ_BIT | addr;
            dev_ioctl(video_dev, cmd, (u32)&wr_data);
        }
    }
    if (close) {
        dev_close(video_dev);
        video_dev = NULL;
    }
    return (u8)wr_data;
}
void mssdp_camera_sensor_reg_save(u8 addr, u8 value)
{
    //addr,value拿到寄存器地址和寄存器值，用户自行保存到flash，这里只能保存1个寄存器地址和值
    //TODO...
}
int mssdp_recv_get_param(char *mssdp_buf)
{
    char *content = mssdp_buf;//MSSDP_TAB;
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *camera_obj = NULL;
    json_object *addr_opj = NULL;
    json_object *value_obj = NULL;
    json_object *tmp = NULL;
    char buf[32];
    char *wlog = "write_reg";
    char *rlog = "read_reg";
    char *slog = "save_reg";
    char logo[8];
    char *str;
    int str_len;
    int err;
    u8 addr;
    u8 value;
    u8 num = 0;
    u8 read_reg;
    u8 write_reg;
    u32 flash_addr = 0;
    extern int json_object_string_set(struct json_object * jso, const char *s, int len);

    //分解content字段
    new_obj = json_tokener_parse(content);
    camera_obj =  json_object_object_get(new_obj, "camera");
    if (!camera_obj) {
        camera_obj = json_object_object_get(new_obj, "CAMERA");
        if (!camera_obj) {
            printf("err no camera_table\n\n");
            goto exit;
        }
    }
    printf("--->recv cjson\n\n%s\n\n", content);
    video0_dev_wr_reg(0, 0, 0, 0, logo);
    for (char cnt = 0; cnt < 3; cnt++) {
        num = 0;
        do {
            switch (cnt) {
            case 0:
                sprintf(buf, "%s%d", rlog, num++);
                break;
            case 1:
                sprintf(buf, "%s%d", wlog, num++);
                break;
            case 2:
                sprintf(buf, "%s%d", slog, num++);
                break;
            default:
                goto exit;
            }
            parm =  json_object_object_get(new_obj, buf);
            if (parm) {
                addr_opj = json_object_object_get(parm, "addr");
                value_obj = json_object_object_get(parm, "value");
                if (addr_opj && value_obj) {
                    addr = value = read_reg = 0;
                    str = json_object_get_string(addr_opj);
                    str_len = json_object_get_string_len(addr_opj);
                    if (str && str_len) {
                        addr = (u8)strtoul(str, NULL, 16);
                    }
                    str = json_object_get_string(value_obj);
                    str_len = json_object_get_string_len(value_obj);
                    if (str && str_len) {
                        value = (u8)strtoul(str, NULL, 16);
                    }
                    if (cnt < 2) {//读写摄像头寄存器
                        read_reg = (u8)video0_dev_wr_reg(addr, value, cnt, 0, NULL);
                        if (!cnt) {
                            if (!str || !str_len || (strstr(str, "0x") || strstr(str, "0X")) || !value) {
                                sprintf(buf, "0x%02x", read_reg);
                            } else {
                                str = json_object_get_string(addr_opj);
                                str_len = json_object_get_string_len(addr_opj);
                                if (!str || (!value && !(strstr(str, "0x") || strstr(str, "0X")))) {
                                    sprintf(buf, "0x%02x", read_reg);
                                } else {
                                    sprintf(buf, "%02x", read_reg);
                                }
                            }
                            err = json_object_string_set(value_obj, buf, strlen(buf));
                            if (err) {
                                printf("err in json_object_string_set = %d \n", err);
                            }
                        } else {
                            read_reg = (u8)video0_dev_wr_reg(addr, value, !cnt, 0, NULL);//读回来校验是否写成功
                            if (read_reg != value) {
                                video0_dev_wr_reg(addr, value, cnt ? 0 : 1, 0, NULL);//重写
                            }
                        }
                    } else {//addr,value拿到寄存器地址和寄存器值，用户自行保存到flash，这里只能保存1个寄存器地址和值
                        mssdp_camera_sensor_reg_save(addr, value);
                    }
                }
            }
        } while (num < 32); //最多32项
    }
    sprintf(buf, "%s", logo);
    err = json_object_string_set(camera_obj, buf, strlen(buf));//修改camera
    if (err) {
        printf("err in json_object_string_set = %d \n", err);
    }

    str = json_object_get_string(new_obj);//获取整个cjson
    video0_dev_wr_reg(0, 0, 0, 1, NULL);

    mssdp_set_user_msg((const char *)str, 2);
    printf("--->send cjson\n\n%s\n\n", str);

exit:
    json_object_put(new_obj);
    return 0;

}


static void network_ssdp_cb(u32 dest_ipaddr, enum mssdp_recv_msg_type type, char *buf, void *priv)
{
    char msg[128];
    switch (type) {
    case MSSDP_SEARCH_MSG:
        printf("ssdp client[0x%x] search, %s\n", dest_ipaddr, buf);
        break;
    case MSSDP_BEFORE_SEND_NOTIFY_MSG:
        if (get_MassProduction()) {
            mssdp_set_notify_msg((const char *)create_mssdp_notify_msg(msg, 0), 2);
        }
        break;
    case MSSDP_USER_MSG:
        mssdp_recv_get_param(buf);
        break;
    default :
        break;
    }

}

void network_mssdp_init(void)
{
    char msg[128];
    puts("mssdp run \n");
    if (get_MassProduction()) {
        mssdp_init("MSSDP_SEARCH ", "MSSDP_NOTIFY ", "MSSDP_REGISTER ", 3333, network_ssdp_cb, NULL);
        mssdp_set_notify_msg((const char *)create_mssdp_notify_msg(msg, 0), 2);
    } else {
        mssdp_init("MSSDP_SEARCH ", "MSSDP_NOTIFY ", "MSSDP_REGISTER ", 3889, network_ssdp_cb, NULL);
        mssdp_set_notify_msg((const char *)get_dev_desc_doc(), 60);
    }
}

void network_mssdp_uninit(void)
{
    mssdp_uninit();
}



