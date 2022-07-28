
#include "net_video_rec.h"
#include "video_rec.h"
#include "server/ctp_server.h"
#include "server/net_server.h"
#include "json_c/json.h"
#include "json_c/json_tokener.h"
#include "server/server_core.h"
#include "system/app_core.h"
#include "action.h"
#include "storage_device.h"
#include "app_config.h"
#include "fs/fs.h"
#include "app_database.h"
#include "http/http_server.h"
#include "server/net_server.h"
#include "ctp.h"
#include "cdp.h"
#include "os/os_api.h"
#include "action.h"
#include "wifi/wifi_connect.h"
#include "time.h"
#include "event/key_event.h"

#define CTP_CMD_HEADER "{\"errno\":%d,\"op\":\"%s\",\"param\":{"
#define CTP_CMD_HEADER_WITHOUT_ERR "{\"op\":\"%s\",\"param\":{"
#define CTP_CMD_END "}}"
#define CTP_ERR_MESSAGE "{\"errno\":%d}"
#define WIFI_RT_STREAM 1


static char file_name[64];
static int app_online_timer;
extern void net_video_rec_post_msg(const char *msg, ...);//更新UI
/************************************/
/*tiny code*/
extern int db_select(const char *name);
extern int db_update(const char *name, u32 value);
/************************************/

static struct ctp_arg info;

int app_rtsp_use_ffmpeg(void)
{
    return (info.num > 0 ? 1 : 0);
}
int send_ctp_string(int cmd_type, char *buf, const char *_req, void *priv)
{
    struct ctp_req req;
    struct server *ctp = NULL;

    ctp = get_ctp_server_hander();
    if (ctp == NULL) {
        return -1;
    }

    req.parm = buf;
    req.topic = _req;
    req.cli = priv;
    if (!priv) {
        req.cli = info.cli;
    }

    if (server_request(ctp, cmd_type, (void *)&req)) {
        return -1;
    }

    return 0;
}

//param:
//字符串集合类似"res:720p,mic:on,par:off"中的以逗号分隔
static inline int _CTP_CMD_COMBINED(int cmd_type, void *priv, u32 err, const char *_req, const char *mothod, char *str)
{
    char *buf = NULL;
    int ret;
    int id = 0;
    char tmp[64];
    char *tmp1 = NULL;
    buf = (char *)malloc(512);

    if (buf == NULL) {
        printf("%s %d mem is fail \n", __func__, __LINE__);
        return -1;
    }

    if (err) {
        ret = snprintf(buf, 512, CTP_CMD_HEADER, err, mothod);
    } else {
        ret = snprintf(buf, 512, CTP_CMD_HEADER_WITHOUT_ERR, mothod);
    }

    char *key;
    char *value;

    if (!err && str != NULL) {
        while (1) {
            key = strtok_r(str, ":", &tmp1); //这函数会改数组，不能用const
            value = strtok_r(NULL, ",", &tmp1);

            if (key == NULL || value == NULL) {
                break;
            }

            ret = sprintf(tmp, "\"%s\":\"%s\",", key, value);
            tmp[ret] = '\0';
            ret = snprintf(buf, 512, "%s%s", buf, tmp);
            str = NULL;
        }

        buf[ret - 1] = '\0';//主要是去掉最后的逗号
        ret = snprintf(buf, 512, "%s%s", buf, CTP_CMD_END);
    } else {
        snprintf(buf, 512, "{\"op\":\"NOTIFY\",\"errno\":%d,\"param\":{%s}}", err, str);
    }

//打开ctp_server
//
    void *cli = NULL;
    if (!strcmp(_req, info.topic) || !priv) {
        cli = info.cli;
    } else {
        cli = priv;
    }


    if (send_ctp_string(cmd_type, buf, _req, cli) < 0) {
        free(buf);
        return -1;
    }
    return 0;
}

int CTP_CMD_COMBINED(void *priv, u32 err, const char *_req, const char *mothod, char *str)
{
    return _CTP_CMD_COMBINED(CTP_NOTIFY_COMMAND, priv, err, _req, mothod, str);
}
static void __all_get_cmd_run(void *priv, char *content)
{
    const struct ctp_map_entry *map = NULL;
    list_for_ctp_mapping_tab(map) {
        if (map->get != NULL) {
            map->get(priv, content);    //执行map中所有get命令
        }
    }
}
/*CTP命令解析*/
/*耗时CTP命令，尽量创建线程处理，防止app_core线程taskq full*/


int ctp_cmd_analysis(const char *topic, char *content, void *priv)
{
    struct ctp_map_entry *map = NULL;
    struct application *app = NULL;
    char buf[128];
    struct intent it;
    int ret = -1;
    if (strlen(topic) <= 0 || strlen(content) <= 0) {
        printf("%s  %d err....\n", __func__, __LINE__);
        return -1;
    }


    strcpy(info.topic, topic);
    info.content = NULL;
    info.cli = priv;

    list_for_ctp_mapping_tab(map) {
        if (!strcmp(topic, map->ctp_command)) {
            if (strstr(content, "PUT") && map->put != NULL) {
                if (map->sync != true) {
                    map->sync = true;  //防止APP多次发送重发命令

                    //保存priv

                    ret =  map->put(priv, content);

                    if (map->dev_cmd != NULL) {
                        int data = db_select(map->dev_cmd);
                        data = data > 0 ? data : 0;
                        snprintf(buf, sizeof(buf), "%s:%d", map->dev_cmd, data);
                        CTP_CMD_COMBINED(NULL, CTP_NO_ERR, map->ctp_command, "NOTIFY", buf);
                        /* db_flush(); */
                    }
                } else {
                    printf("Warnning CTP<%s> is doing now\n", map->ctp_command);
                    ret = 0;
                }
            } else if (strstr(content, "GET") && map->get != NULL) {
                if (map->sync != true) {
                    map->sync = true;
                    ret = map->get(priv, content);
                } else {
                    printf("Warnning CTP<%s> is doing now\n", map->ctp_command);
                    ret = 0;
                }
            } else {
                puts("content is error \n\n");
            }

        }

        if (ret != -1) {
            return ret;
        }
    }

    CTP_CMD_COMBINED(priv, CTP_NO_TOPIC, topic, "NOTIFY", CTP_NO_TOPIC_MSG);
    printf("%s %d not find it or cb is NULL\n", __func__, __LINE__);
    return -1;

}

/*小机主界面UI切换*/
static void net_switch_ui(const char *app_name)
{
    struct intent it;
    struct application *app = NULL;

    init_intent(&it);
    app = get_current_app();
    if (app) {
        if (!strcmp(app->name, "usb_app")) {
            puts("IN USB MODE\n");
            return;
        }
        printf("set app:%s    get app:%s\n", app_name, app->name);

        /*if (strcmp(app_name, app->name)) {*/
        if (!strstr(app->name, app_name)) {

            it.action = ACTION_BACK;
            start_app(&it);
            it.name = app_name;

            if (!strcmp("video_rec", app_name)) {
                it.action = ACTION_VIDEO_REC_MAIN;
            }

            if (!strcmp("video_photo", app_name)) {
                it.action = ACTION_PHOTO_TAKE_MAIN;
            }
            /* sys_key_event_takeover(false, false); */
            start_app(&it);

        }

    }

}
static int sys_key_touch_disable_scan(void *p)
{
    /* int ret; */
    /* ret = sys_key_event_disable_get(); */
    /* if (!ret) { //使能需要禁用 */
    /* sys_key_event_disable(); */
    /* } */
    key_event_disable();

    /* ret = sys_touch_event_disable_get(); */
    /* if (!ret) { //使能需要禁用 */
    /* sys_touch_event_disable(); */
    /* } */
    return 0;
}
void ctp_cmd_socket_unregister(void *priv)
{
    if (!priv) {
        priv = info.cli;
        if (!priv) {
            return;
        }
    }
    struct sockaddr_in *addr;
    addr = (struct sockaddr_in *)ctp_srv_get_cli_addr(priv);
    if (!addr) {
        addr = (struct sockaddr_in *)cdp_srv_get_cli_addr(priv);
    }
    if (addr) {
        extern int TCP_client_socket_quit(int addr);
        extern int UDP_client_socket_unreg(int addr);
        TCP_client_socket_quit(addr->sin_addr.s_addr);
        UDP_client_socket_unreg(addr->sin_addr.s_addr);
    }
}
//添加命令回调
int cmd_put_app_access(void *priv, char *content)
{
    json_object *new_obj = NULL; //
    json_object *parm = NULL;
    json_object *key = NULL;
    struct application *app;
    struct intent it = {0};
    const int access_num = 0;
    char buf[128];
    void *addr = NULL;
    addr = (void *)ctp_srv_get_cli_addr(priv);
    if (!addr) {
        addr = (void *)cdp_srv_get_cli_addr(priv);
    }
    if (addr != info.dest_addr) {
        info.num++;
    }
    //分解content字段
    puts("\n\n APP_ACCESS \n");
    printf("app_accept_num : %d \n", info.num);
    /*sys_key_event_disable();*/
    /*sys_touch_event_disable();*/
    /*app_online_timer = sys_timer_add(NULL, sys_key_touch_disable_scan, 2 * 1000);//添加检查按键和触屏使能*/

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");

    key =  json_object_object_get(parm, "type");

    const char *type = json_object_get_string(key);
    if (atoi(type)) {
        puts("phone : iOS\n");
    } else {
        puts("phone : Andriod\n");
    }
    //版本是否匹配
    key =  json_object_object_get(parm, "ver");
    const char *ver = json_object_get_string(key);
    printf("version : %s\n", ver);
    app = get_current_app();
    if (app && !strcmp(app->name, "usb_app") && dev_online("usb0")) {
        CTP_CMD_COMBINED(priv, CTP_IN_USB, "APP_ACCESS", "NOTIFY", CTP_IN_USB_MSG);
        ctp_srv_disconnect_cli(priv);
        cdp_srv_disconnect_cli(priv);
        goto err;
    } else if (!app || !app->name || !strstr(app->name, "video_rec")) {
        if (app && app->name) {
            init_intent(&it);
            it.name = app->name;
            it.action = ACTION_BACK;
            start_app(&it);
        }
        init_intent(&it);
        it.name = "net_video_rec";//不录卡net_video_rec，录卡video_rec
        it.action = ACTION_VIDEO_REC_MAIN;
        start_app(&it);
    }
    printf("access_num : ctp %d , cdp %d \n\n", ctp_srv_get_cli_cnt(), cdp_srv_get_cli_cnt());
    if ((ctp_srv_get_cli_cnt() > ACCESS_NUM || cdp_srv_get_cli_cnt() > ACCESS_NUM) ||
        (ctp_srv_get_cli_cnt() == ACCESS_NUM && cdp_srv_get_cli_cnt() == ACCESS_NUM)) {
        CTP_CMD_COMBINED(priv, CTP_ACCESS_RFU, "APP_ACCESS", "NOTIFY", CTP_ACCESS_RFU_MSG);
        ctp_srv_disconnect_cli(priv);
        cdp_srv_disconnect_cli(priv);
        goto err;
    }

    snprintf(buf, sizeof(buf), "type:%s,ver:%s", type, ver);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "APP_ACCESS", "NOTIFY", buf);
    //app_access命令完成后，随后发送所有get命令
    __all_get_cmd_run(priv, content);
    info.dest_addr = addr;
    info.cli = priv;

err:
    json_object_put(new_obj);
    return 0;
}
int cmd_get_sd_status(void *priv, char *content)
{
    char buf[16];

    snprintf(buf, sizeof(buf), "online:%d", storage_device_ready());
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "SD_STATUS", "NOTIFY", buf);

    return 0;

}
int cmd_get_keep_alive_interval(void *priv, char *content)
{
    int timeout;
    char buf[16];
    //分解content字段
    timeout = ctp_srv_get_keep_alive_timeout();
    snprintf(buf, sizeof(buf), "timeout:%d", timeout);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "KEEP_ALIVE_INTERVAL", "NOTIFY", buf);


    return 0;

}
int cmd_get_bat_status(void *priv, char *content)
{
    char buf[32];
    //分解content字段

    snprintf(buf, sizeof(buf), "level:%d", 4);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "BAT_STATUS", "NOTIFY", buf); //目前没有电池状态，先写充电中 4

    return 0;

}
int cmd_get_uuid(void *priv, char *content)
{
    u8 temp;
    int i, j;
    char buf[128] = {0};
    u8 mac[6];
    wifi_get_mac(mac);
    snprintf(buf, sizeof(buf), "uuid:%s%02x%02x%02x%02x%02x%02x", "f2dd3cd7-b026-40aa-aaf4-", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    /*printf("\n\nUUID : %s \n\n",buf);*/
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "UUID", "NOTIFY", buf);
    return 0;
}

int cmd_get_video_param(void *priv, char *content)
{
    char buf[128];

    int res = db_select("res");
    switch (res) {
    case VIDEO_RES_1080P:
        snprintf(buf, sizeof(buf), "w:%d,h:%d,format:0,fps:%d,rate:%d", 1280, 720, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
        break;

    case VIDEO_RES_720P:
        snprintf(buf, sizeof(buf), "w:%d,h:%d,format:0,fps:%d,rate:%d", 640, 480, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
        break;

    case VIDEO_RES_VGA:
        snprintf(buf, sizeof(buf), "w:%d,h:%d,format:0,fps:%d,rate:%d", 640, 480, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
        break;

    default:
        snprintf(buf, sizeof(buf), "w:%d,h:%d,format:0,fps:%d,rate:%d", 1280, 720, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
        break;
    }
    printf("buf -> %s\n", buf);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_PARAM", "NOTIFY", buf);
    return 0;

}

int cmd_put_video_param(void *priv,  char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *h = NULL;
    struct intent it;
    char buf[128];
    const char *height, *width, *format;
    //分解content字段
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");

    h =  json_object_object_get(parm, "h");
    height = json_object_get_string(h);

    printf("height : %s\n", height);

    if (strstr(height, "480")) {
        db_update("res", VIDEO_RES_720P);
    } else if (strstr(height, "720")) {
        db_update("res", VIDEO_RES_1080P);
    } else {
        db_update("res", VIDEO_RES_1080P);
    }

    int res = db_select("res");
#ifdef CONFIG_UI_ENABLE
    net_video_rec_post_msg("changeRES");
#endif
    switch (res) {
    case VIDEO_RES_1080P:
        snprintf(buf, sizeof(buf), "w:%d,h:%d,format:%d,fps:%d,rate:%d", 1280, 720, 0, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
        break;

    case VIDEO_RES_720P:
        snprintf(buf, sizeof(buf), "w:%d,h:%d,format:%d,fps:%d,rate:%d", 640, 480, 0, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
        break;

    case VIDEO_RES_VGA:
        snprintf(buf, sizeof(buf), "w:%d,h:%d,format:%d,fps:%d,rate:%d", 640, 480, 0, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
        break;

    default:
        snprintf(buf, sizeof(buf), "w:%d,h:%d,format:%d,fps:%d,rate:%d", 1280, 720, 0, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
        break;
    }
    printf("buf -> %s\n", buf);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_PARAM", "NOTIFY", buf);

    json_object_put(new_obj);
    return 0;

}


int cmd_get_pull_video_param(void *priv,  char *content)
{
    struct intent it;
    char buf[128];
    int res = db_select("res2");
    res = res > 0 ? res : 0;
    switch (res) {
    case VIDEO_RES_1080P:
        snprintf(buf, sizeof(buf), "w:%d,h:%d,format:%d,fps:%d,rate:%d", 1280, 720, 0, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
        break;

    case VIDEO_RES_720P:
        snprintf(buf, sizeof(buf), "w:%d,h:%d,format:%d,fps:%d,rate:%d", 640, 480, 0, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
        break;

    case VIDEO_RES_VGA:
        snprintf(buf, sizeof(buf), "w:%d,h:%d,format:%d,fps:%d,rate:%d", 640, 480, 0, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
        break;
    default:
        snprintf(buf, sizeof(buf), "w:%d,h:%d,format:%d,fps:%d,rate:%d", 640, 480, 0, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
        break;
    }

    printf("buf -> %s\n", buf);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "PULL_VIDEO_PARAM", "NOTIFY", buf);
    return 0;

}

int cmd_put_pull_video_param(void *priv,  char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *h = NULL;
    struct intent it;
    char buf[128];
    const char *height, *width, *format;
    //分解content字段
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");

    h =  json_object_object_get(parm, "h");
    height = json_object_get_string(h);

    printf("height : %s\n", height);
    if (strstr(height, "480")) {
        db_update("res2", VIDEO_RES_VGA);
    } else if (strstr(height, "720")) {
        db_update("res2", VIDEO_RES_720P);
    } else {
        db_update("res2", VIDEO_RES_1080P);
    }
    int res = db_select("res2");
    res = res > 0 ? res : 0;
    printf("res->%d \n", res);
    /* #if defined CONFIG_UI_STYLE_LY_ENABLE */
    /* ui_text_show_index_by_id(TEXT_RES_REC, res); */
    /* #endif */
    switch (res) {
    case VIDEO_RES_1080P:
        snprintf(buf, sizeof(buf), "w:%d,h:%d,format:%d,fps:%d,rate:%d", 1280, 720, 0, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
        break;

    case VIDEO_RES_720P:
        snprintf(buf, sizeof(buf), "w:%d,h:%d,format:%d,fps:%d,rate:%d", 640, 480, 0, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
        break;

    case VIDEO_RES_VGA:
        snprintf(buf, sizeof(buf), "w:%d,h:%d,format:%d,fps:%d,rate:%d", 640, 480, 0, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
        break;
    default:
        snprintf(buf, sizeof(buf), "w:%d,h:%d,format:%d,fps:%d,rate:%d", 640, 480, 0, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
        break;
    }

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "PULL_VIDEO_PARAM", "NOTIFY", buf);

    json_object_put(new_obj);
    return 0;

}

int cmd_put_video_cyc_savefile(void *priv,  char *content)
{
    char buf[128];
    struct intent it;
    struct application *app = NULL;
    if (storage_device_ready() == 0) {
        CTP_CMD_COMBINED(priv, CTP_SD_OFFLINE, "VIDEO_CYC_SAVEFILE", "NOTIFY", CTP_SD_OFFLINE_MSG);
    } else {
        app = get_current_app();
        init_intent(&it);
        /*if (!strcmp(app->name, "video_rec")) {*/
        if (strstr(app->name, "video_rec")) {
            it.name = "net_video_rec";
            it.action = ACTION_VIDEO_CYC_SAVEFILE;
            start_app(&it);
        } else {
            /* CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_CYC_SAVEFILE", "NOTIFY", CTP_NO_ERR_MSG); */
            CTP_CMD_COMBINED(priv, CTP_REQUEST, "VIDEO_CYC_SAVEFILE", "NOTIFY", CTP_REQUEST_MSG);
        }
    }
    return 0;
}

#ifdef CONFIG_NET_SCR
static int cmd_get_net_scr(void *priv, char *content)
{
    char buf[128];
    u8 status;
    status = get_net_scr_status();
    snprintf(buf, sizeof(buf), "status:%d", status);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "NET_SCR", "NOTIFY", buf);
    return 0;
}

static int cmd_put_net_scr(void *priv, char *content)
{
    struct __NET_SCR_CFG cfg = {0};

    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];
    char *s_str;
    u8 status;
    new_obj = json_tokener_parse(content);
    parm    =  json_object_object_get(new_obj, "param");
    tmp  = json_object_object_get(parm, "status");
    s_str = json_object_get_string(tmp);
    status = atoi(s_str);
    snprintf(buf, sizeof(buf), "status:%d", status);
    if (1 == status) {
        tmp  = json_object_object_get(parm, "w");
        s_str = json_object_get_string(tmp);
        cfg.src_w = atoi(s_str);
        printf("\n [MSG] cfg.src_w = %d \n", cfg.src_w);
        tmp  = json_object_object_get(parm, "h");
        s_str = json_object_get_string(tmp);
        cfg.src_h = atoi(s_str);
        printf("\n [MSG] cfg.src_h = %d \n", cfg.src_h);
        tmp  = json_object_object_get(parm, "fps");
        s_str = json_object_get_string(tmp);
        cfg.fps = atoi(s_str);
        if (ctp_srv_get_cli_addr(priv)) {
            memcpy(&cfg.cli_addr, ctp_srv_get_cli_addr(priv), sizeof(struct sockaddr_in));
        } else {
            memcpy(&cfg.cli_addr, cdp_srv_get_cli_addr(priv), sizeof(struct sockaddr_in));
        }
        net_scr_init(&cfg);
    } else if (0 == status) {
        if (ctp_srv_get_cli_addr(priv)) {
            memcpy(&cfg.cli_addr, ctp_srv_get_cli_addr(priv), sizeof(struct sockaddr_in));
        } else {
            memcpy(&cfg.cli_addr, cdp_srv_get_cli_addr(priv), sizeof(struct sockaddr_in));
        }
        net_scr_uninit(&cfg);
    }
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "NET_SCR", "NOTIFY", buf);

    json_object_put(new_obj);
    return 0;
}
#endif


int cmd_get_video_ctrl(void *priv, char *content)
{
    struct intent it;
    char buf[128];
    init_intent(&it);
    u32 status;
    const char *path = NULL;


    net_switch_ui("video_rec");
    it.name = "net_video_rec";
    it.action = ACTION_VIDEO_REC_GET_APP_STATUS;
    start_app(&it);
    struct video_rec_hdl *rec_handler = (struct video_rec_hdl *)it.data;
    // status = *((u32 *)it.data);
    if (rec_handler) {
        status = rec_handler->state;
    } else {
        status = 0;
    }

    if (status == VIDREC_STA_START) {
        snprintf(buf, sizeof(buf), "status:%d", 1);
        CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_CTRL", "NOTIFY", buf);

    } else {
        snprintf(buf, sizeof(buf), "status:%d", 0);
        CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_CTRL", "NOTIFY", buf);
    }

    return 0;

}
int cmd_put_video_ctrl(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    struct json_object *sta = NULL;
    struct intent it;
    char buf[128];
    const char *status;
    u32 status1 = 0;
    u32 status2 = 0;
    char *path = NULL;
    init_intent(&it);
    //分解content字段
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    sta =  json_object_object_get(parm, "status");

    /*printf("cmd_put_video_ctrl : %s \n",content);*/

    status = json_object_get_string(sta);
    status2 = atoi(status);

    net_switch_ui("video_rec");

    it.name = "net_video_rec";
    it.action = ACTION_VIDEO_REC_GET_APP_STATUS;
    start_app(&it);

    struct video_rec_hdl *rec_handler = (struct video_rec_hdl *)it.data;
    if (rec_handler) {
        status1 = rec_handler->state;
    } else {
        status1 = 0;
    }

    if (status1 == VIDREC_STA_START || status1 == VIDREC_STA_STARTING) {
        if (status2) {
            snprintf(buf, sizeof(buf), "status:%d", 1);
            CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_CTRL", "NOTIFY", buf);
        } else {
            printf("ctp open video \n\n");
            it.action = ACTION_VIDEO_REC_CONCTRL;
            start_app(&it);

        }
    } else {
        if (status2) {
            printf("ctp open video \n\n");
            it.action = ACTION_VIDEO_REC_CONCTRL;
            start_app(&it);

        } else {
            snprintf(buf, sizeof(buf), "status:%d", 0);
            CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_CTRL", "NOTIFY", buf);

        }
    }

    json_object_put(new_obj);
    return 0;
}

int cmd_get_video_finish(void *priv, char *content)
{
    struct intent it;
    char buf[128];
    init_intent(&it);

    it.name = "net_video_rec";
    it.action = ACTION_VIDEO_REC_GET_APP_STATUS;
    start_app(&it);

    struct video_rec_hdl *rec_handler = (struct video_rec_hdl *)it.data;

    if (rec_handler && rec_handler->state == VIDREC_STA_START) {
        snprintf(buf, sizeof(buf), "status:%d", 1);
    } else {
        snprintf(buf, sizeof(buf), "status:%d", 0);
    }

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_FINISH", "NOTIFY", buf);
    return 0;
}


int cmd_get_photo_reso(void *priv, char *content)
{
    char buf[32];
    struct intent it;
    init_intent(&it);
    /*     it.action = ACTION_BACK; */
    /* start_app(&it); */
    int pres = db_select("pres");
    pres = pres > 0 ? pres : 0;

    switch (pres) {
    case PHOTO_RES_VGA:
        snprintf(buf, sizeof(buf), "res:0");
        break;

    case PHOTO_RES_1M:
        snprintf(buf, sizeof(buf), "res:1");
        break;

    case PHOTO_RES_2M:
        snprintf(buf, sizeof(buf), "res:2");
        break;

    case PHOTO_RES_3M:
        snprintf(buf, sizeof(buf), "res:3");
        break;

    case PHOTO_RES_5M:
        snprintf(buf, sizeof(buf), "res:4");
        break;

    case PHOTO_RES_8M:
        snprintf(buf, sizeof(buf), "res:5");
        break;

    case PHOTO_RES_10M:
        snprintf(buf, sizeof(buf), "res:6");
        break;

    case PHOTO_RES_12M:
        snprintf(buf, sizeof(buf), "res:7");
        break;

    default:
        break;
    }

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "PHOTO_RESO", "NOTIFY", buf);
    return 0;
}

int cmd_put_photo_reso(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *res = NULL;
    u32 pres = 0;
    char buf[128];

//设置参数
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    res =  json_object_object_get(parm, "res");

    const char *photo_res_value = json_object_get_string(res);
    printf("ctp photo reso %s \n", photo_res_value);

    switch (atoi(photo_res_value)) {
    case 0:
        pres = PHOTO_RES_VGA;
        break;

    case 1:
        pres = PHOTO_RES_1M;
        break;

    case 2:
        pres = PHOTO_RES_2M;
        break;

    case 3:
        pres = PHOTO_RES_3M;
        break;

    case 4:
        pres = PHOTO_RES_5M;
        break;

    case 5:
        pres = PHOTO_RES_8M;
        break;

    case 6:
        pres = PHOTO_RES_10M;
        break;

    case 7:
        pres = PHOTO_RES_12M;
        break;

    default:
        break;
    }

    if (pres > PHOTO_RES_2M) { //DV15 , 拍照不能大于3M，分辨率<=1280*720
        pres = PHOTO_RES_2M;
    }
    db_update("pres", pres);

    snprintf(buf, sizeof(buf), "res:%d", atoi(photo_res_value));

    printf("buf->%s\n", buf);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "PHOTO_RESO", "NOTIFY", buf);
    json_object_put(new_obj);
    return 0;

}
static void take_photo_thread(void *arg)
{
    struct intent it;
    init_intent(&it);
    it.name = "video_photo";
    it.action = ACTION_PHOTO_TAKE_CONTROL;
    os_time_dly(50);
    start_app(&it);

}
int cmd_put_photo_ctrl(void *priv, char *content)
{
    char buf[128];
    struct intent it;
    struct application *app = NULL;
    if (storage_device_ready() == 0) {
        //CTP_ERR(CTP_SD_OFFLINE);
        CTP_CMD_COMBINED(priv, CTP_SD_OFFLINE, "PHOTO_CTRL", "NOTIFY", CTP_SD_OFFLINE_MSG);
    } else {

        app = get_current_app();
        init_intent(&it);
        /*if (!strcmp(app->name, "video_rec")) {*/
        if (strstr(app->name, "video_rec")) {
            it.name = "net_video_rec";
            it.action = ACTION_VIDEO_REC_GET_APP_STATUS;
            start_app(&it);

            struct net_video_hdl *net_handler = (struct net_video_hdl *)it.exdata;
            struct video_rec_hdl *rec_handler = (struct video_rec_hdl *)it.data;

            /*if (net_handler->net_state == VIDREC_STA_START*/
            /*|| net_handler->net_state1 == VIDREC_STA_START*/
            /*|| rec_handler->state == VIDREC_STA_START) {*/
            /*printf("rec_handler->state:%d\n", rec_handler->state);*/
            it.action = ACTION_VIDEO_TAKE_PHOTO;
            start_app(&it);
            /*}*/
            /*else {*/
            /*net_switch_ui("video_photo");*/
            /*thread_fork("take_photo_thread", 10, 0x1000, 0, 0, take_photo_thread, NULL);*/
            /*}*/
        }
        /*else if (!strcmp(app->name, "video_dec")) {*/
        /*net_switch_ui("video_photo");*/
        /*thread_fork("take_photo_thread", 10, 0x1000, 0, 0, take_photo_thread, NULL);*/
        /*} else {*/
        /*it.name = "video_photo";*/
        /*it.action = ACTION_PHOTO_TAKE_CONTROL;*/
        /*start_app(&it);*/
        /*}*/
    }
    return 0;
}
int cmd_get_self_timer(void *priv, char *content)
{
    char buf[32];
    int phm = db_select("phm");
    snprintf(buf, sizeof(buf), "phm:%d", phm > 0 ? phm : 0);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "SELF_TIMER", "NOTIFY", buf);
    return 0;
}

int cmd_put_self_timer(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *phm = NULL;
    char buf[128];

//设置参数
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    phm =  json_object_object_get(parm, "phm");

    const char *photo_phm_value = json_object_get_string(phm);
    printf("ctp self timer  %s \n", photo_phm_value);
    db_update("phm", atoi(photo_phm_value));
    json_object_put(new_obj);
    return 0;

}


int cmd_get_burst_shot(void *priv, char *content)
{
    char buf[32];

    int cyt = db_select("cyt");
    snprintf(buf, sizeof(buf), "cyt:%d", cyt > 0 ? cyt : 0);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "BURST_SHOT", "NOTIFY", buf);
    return 0;
}

int cmd_put_burst_shot(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *cyt = NULL;
    char buf[128];

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    cyt =  json_object_object_get(parm, "cyt");

    const char *photo_cyt_value = json_object_get_string(cyt);
    printf("ctp burst_shot  %s \n", photo_cyt_value);
    db_update("cyt", atoi(photo_cyt_value));
    json_object_put(new_obj);
    return 0;

}

int cmd_get_key_voice(void *priv, char *content)
{
    char buf[32];
    int kvo = db_select("kvo");
    snprintf(buf, sizeof(buf), "kvo:%d", kvo > 0 ? kvo : 0);

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "KEY_VOICE", "NOTIFY", buf);
    return 0;
}

int cmd_put_key_voice(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *kvo = NULL;
    char buf[128];

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    kvo =  json_object_object_get(parm, "kvo");
    const char *photo_kvo_value = json_object_get_string(kvo);
    db_update("kvo", atoi(photo_kvo_value));
    json_object_put(new_obj);
    return 0;
}

int cmd_get_board_voice(void *priv, char *content)
{
    char buf[32];
    int bvo = db_select("bvo");
    snprintf(buf, sizeof(buf), "bvo:%d", bvo > 0 ? bvo : 0);

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "BOARD_VOICE", "NOTIFY", buf);
    return 0;
}

int cmd_put_board_voice(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *bvo = NULL;
    char buf[128];

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    bvo =  json_object_object_get(parm, "bvo");
    const char *bvo_value = json_object_get_string(bvo);
    db_update("bvo", atoi(bvo_value));
    json_object_put(new_obj);
    return 0;
}




int cmd_get_light_fre(void *priv, char *content)
{
    char buf[32];
    int fre = db_select("fre");
    snprintf(buf, sizeof(buf), "fre:%d", fre > 0 ? fre : 0);

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "LIGHT_FRE", "NOTIFY", buf);
    return 0;
}

int cmd_put_light_fre(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *fre = NULL;
    char buf[128];

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    fre =  json_object_object_get(parm, "fre");

    const char *light_fre_value = json_object_get_string(fre);
    db_update("fre", atoi(light_fre_value));
    json_object_put(new_obj);
    return 0;

}

int cmd_get_auto_stutdown(void *priv, char *content)
{
    char buf[32];

    int aff = db_select("aff");
    snprintf(buf, sizeof(buf), "aff:%d", aff > 0 ? aff : 0);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "AUTO_STUTDOWN", "NOTIFY", buf);
    return 0;
}


int cmd_put_auto_stutdown(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *aff = NULL;
    char buf[128];

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    aff =  json_object_object_get(parm, "aff");

    const char *aff_value = json_object_get_string(aff);
    printf("ctp aff  %s \n", aff_value);
    db_update("aff", atoi(aff_value));
    json_object_put(new_obj);
    return 0;

}

int cmd_get_screen_pro(void *priv, char *content)
{
    char buf[32];
    int pro = db_select("pro");

    snprintf(buf, sizeof(buf), "pro:%d", pro > 0 ? pro : 0);

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "SCREEN_PRO", "NOTIFY", buf);
    return 0;
}


int cmd_put_screen_pro(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *pro = NULL;
    char buf[128];
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    pro =  json_object_object_get(parm, "pro");

    const char *pro_value = json_object_get_string(pro);
    printf("ctp pro  %s \n", pro_value);

    db_update("pro", atoi(pro_value));
    json_object_put(new_obj);
    return 0;
}

int cmd_get_tv_mode(void *priv, char *content)
{
    char buf[32];
    int tvm = db_select("tvm");
    snprintf(buf, sizeof(buf), "tvm:%d", tvm > 0 ? tvm : 0);

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "TV_MODE", "NOTIFY", buf);
    return 0;
}


int cmd_put_tv_mode(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tvm = NULL;
    char buf[128];

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tvm =  json_object_object_get(parm, "tvm");

    const char *tvm_value = json_object_get_string(tvm);
    printf("ctp pro  %s \n", tvm_value);
    db_update("tvm", atoi(tvm_value));
    json_object_put(new_obj);
    return 0;
}

int cmd_get_sd_size(void *priv, char *content)
{
    char buf[32];
    u32 space;
    struct vfs_partition *part;

    if (storage_device_ready() == 0) {
        CTP_CMD_COMBINED(priv, CTP_SD_OFFLINE, "TF_CAP", "NOTIFY", CTP_SD_OFFLINE_MSG);
    } else {
        part = fget_partition(CONFIG_ROOT_PATH);
        fget_free_space(CONFIG_ROOT_PATH, &space);
        snprintf(buf, sizeof(buf), "left:%d,total:%d", space / 1024, part->total_size / 1024);
        CTP_CMD_COMBINED(priv, CTP_NO_ERR, "TF_CAP", "NOTIFY", buf);

    }

    return 0;
}


int cmd_put_format(void *priv, char *content)
{
    char buf[64];
    int err;

    if (storage_device_ready()) {
        err = f_format(CONFIG_ROOT_PATH, "fat", 32 * 1024);

        if (err != 0) {
            CTP_CMD_COMBINED(priv, CTP_SDCARD, "FORMAT", "NOTIFY", CTP_SDCARD_MSG);
            return -EFAULT;
        }
    } else {
        err = f_format(CONFIG_STORAGE_PATH, "fat", 32 * 1024);

        if (err != 0) {
            CTP_CMD_COMBINED(priv, CTP_SDCARD, "FORMAT", "NOTIFY", CTP_SDCARD_MSG);
            return -EFAULT;
        }
    }

#if defined CONFIG_ENABLE_VLIST
    FILE_DELETE(NULL, 1);
#endif
    snprintf(buf, sizeof(buf), "frm:1");
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "FORMAT", "NOTIFY", buf);

    return 0;
}
int cmd_get_system_default(void *priv, char *content)
{
    char buf[32];


    return 0;
}



int cmd_put_system_default(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *def = NULL;
    char buf[128];
    char ssid[32];
    char pwd[64];
    u8 mac_addr[6];
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    def =  json_object_object_get(parm, "def");

    const char *def_value = json_object_get_string(def);
    printf("ctp pro  %s \n", def_value);

    if (atoi(def_value)) {
#if defined (WIFI_CAM_SUFFIX)
        sprintf(ssid, AP_WIFI_CAM_PREFIX WIFI_CAM_SUFFIX);
#else
        wifi_get_mac(mac_addr);
        sprintf(ssid, AP_WIFI_CAM_PREFIX"%02x%02x%02x%02x%02x%02x"
                , mac_addr[0]
                , mac_addr[1]
                , mac_addr[2]
                , mac_addr[3]
                , mac_addr[4]
                , mac_addr[5]);
#endif
        wifi_store_mode_info(AP_MODE, ssid, AP_WIFI_CAM_WIFI_PWD);
        snprintf(buf, sizeof(buf), "def:1");
        CTP_CMD_COMBINED(priv, CTP_NO_ERR, "SYSTEM_DEFAULT", "NOTIFY", buf);

        os_time_dly(200);
        cpu_reset();
    }

    json_object_put(new_obj);
    return 0;
}
int cmd_get_language(void *priv, char *content)
{
    char buf[32];
    int lag = db_select("lag");
    snprintf(buf, sizeof(buf), "lag:%d", lag > 0 ? lag : 0);

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "LANGUAGE", "NOTIFY", buf);
    return 0;
}

int cmd_put_language(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *lag = NULL;
    char buf[128];

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    lag =  json_object_object_get(parm, "lag");

    const char *lag_value = json_object_get_string(lag);
    printf("cmd_put_language :ctp pro  %s \n", lag_value);
    db_update("lag", atoi(lag_value));
    /*lag_set_function(atoi(lag_value));*/

    json_object_put(new_obj);
    return 0;
}

int cmd_get_double_video(void *priv, char *content)
{
    char buf[32];
    int two = db_select("two");
    snprintf(buf, sizeof(buf), "two:%d", two > 0 ? two : 0);

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "DOUBLE_VIDEO", "NOTIFY", buf);
    return 0;
}


int cmd_put_double_video(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "two");

    const char *tmp_value = json_object_get_string(tmp);
    printf("ctp pro  %s \n", tmp_value);
    db_update("two", atoi(tmp_value));
    json_object_put(new_obj);
    return 0;
}

int cmd_get_video_loop(void *priv, char *content)
{
    char buf[32];
    int cyc = db_select("cyc");
    snprintf(buf, sizeof(buf), "cyc:%d", cyc > 0 ? cyc : 0);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_LOOP", "NOTIFY", buf);
    return 0;
}


int cmd_put_video_loop(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];
    const char *tmp_value = json_object_get_string(tmp);
    printf("ctp pro  %s \n", tmp_value);
    db_update("cyc", atoi(tmp_value));


    json_object_put(new_obj);
    return 0;
}
int cmd_get_video_wdr(void *priv, char *content)
{
    char buf[32];
    int wdr = db_select("wdr");
    snprintf(buf, sizeof(buf), "wdr:%d", wdr > 0 ? wdr : 0);

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_WDR", "NOTIFY", buf);
    return 0;
}


int cmd_put_video_wdr(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];


    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "wdr");

    const char *tmp_value = json_object_get_string(tmp);
    printf("ctp pro  %s \n", tmp_value);
    u32 wdr = db_update("wdr", atoi(tmp_value));
    json_object_put(new_obj);
    return 0;
}



int cmd_get_video_exp(void *priv, char *content)
{
    char buf[32];

    int exp = db_select("exp");
    snprintf(buf, sizeof(buf), "exp:%d", exp > 0 ? exp : 0);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_EXP", "NOTIFY", buf);
    return 0;
}


int cmd_put_video_exp(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "exp");

    const char *tmp_value = json_object_get_string(tmp);
    printf("ctp pro  %s \n", tmp_value);
    db_update("exp", atoi(tmp_value));
    json_object_put(new_obj);
    return 0;
}

int cmd_get_video_move_check(void *priv, char *content)
{
    char buf[32];

    int exp = db_select("mot");
    snprintf(buf, sizeof(buf), "mot:%d", exp > 0 ? exp : 0);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "MOVE_CHECK", "NOTIFY", buf);
    return 0;

}


int cmd_put_video_move_check(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "mot");

    const char *tmp_value = json_object_get_string(tmp);
    printf("ctp pro  %s \n", tmp_value);
    db_update("mot", atoi(tmp_value));

#ifdef CONFIG_UI_ENABLE
    /*net_video_rec_post_msg("changeMOT");*/
#endif

    json_object_put(new_obj);
    return 0;
}

int cmd_get_video_mic(void *priv, char *content)
{
    char buf[32];

    int mic = db_select("mic");
    snprintf(buf, sizeof(buf), "mic:%d", mic > 0 ? mic : 0);

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_MIC", "NOTIFY", buf);
    return 0;
}


int cmd_put_video_mic(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "mic");

    const char *tmp_value = json_object_get_string(tmp);
    printf("ctp pro  %s \n", tmp_value);
    db_update("mic", atoi(tmp_value));
    json_object_put(new_obj);
#ifdef CONFIG_UI_ENABLE
    if (atoi(tmp_value)) {
        net_video_rec_post_msg("onMIC");
    } else {
        net_video_rec_post_msg("offMIC");
    }
#endif
    return 0;
}

int cmd_get_video_date(void *priv, char *content)
{
    char buf[32];
    int dat = db_select("dat");
    snprintf(buf, sizeof(buf), "dat:%d", dat > 0 ? dat : 0);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_DATE", "NOTIFY", buf);
    return 0;
}

int cmd_get_rt_stream0_res(void *priv, char *content)
{
    char buf[32];
    int dat = db_select("rtf");
    snprintf(buf, sizeof(buf), "rtf:%d", dat > 0 ? dat : 0);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "RTF_RES", "NOTIFY", buf);
    return 0;

}


int cmd_get_rt_stream1_res(void *priv, char *content)
{
    char buf[32];
    int dat = db_select("rtb");
    snprintf(buf, sizeof(buf), "rtb:%d", dat > 0 ? dat : 0);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "RTB_RES", "NOTIFY", buf);
    return 0;

}


/*extern int lab_set_function(u32 parm);*/
int cmd_put_video_date(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "dat");

    const char *tmp_value = json_object_get_string(tmp);
    printf("ctp pro  %s \n", tmp_value);
    db_update("dat", atoi(tmp_value));

    /*lab_set_function(atoi(tmp_value));*/

    json_object_put(new_obj);
    return 0;
}



int cmd_get_video_car_num(void *priv, char *content)
{
    char buf[32];
    int num = db_select("num");
    snprintf(buf, sizeof(buf), "num:%d", num > 0 ? num : 0);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_CAR_NUM", "NOTIFY", buf);
    return 0;
}


int cmd_put_video_car_num(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "num");

    const char *tmp_value = json_object_get_string(tmp);
    printf("ctp pro  %s \n", tmp_value);
    db_update("num", atoi(tmp_value));
    json_object_put(new_obj);
    return 0;
}

int cmd_get_gra_sen(void *priv, char *content)
{
    char buf[32];
    int gra = db_select("gra");
    snprintf(buf, sizeof(buf), "gra:%d", gra > 0 ? gra : 0);

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "GRA_SEN", "NOTIFY", buf);
    return 0;
}

int cmd_put_gra_sen(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "gra");

    const char *tmp_value = json_object_get_string(tmp);
    printf("ctp pro  %s \n", tmp_value);
    db_update("gra", atoi(tmp_value));
    json_object_put(new_obj);
    return 0;
}
int cmd_get_video_par_car(void *priv, char *content)
{
    char buf[32];
    int par = db_select("par");
    snprintf(buf, sizeof(buf), "par:%d", par > 0 ? par : 0);

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_PAR_CAR", "NOTIFY", buf);
    return 0;
}

int cmd_put_video_par_car(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "par");

    const char *tmp_value = json_object_get_string(tmp);
    printf("ctp pro  %s \n", tmp_value);
    db_update("par", atoi(tmp_value));
    json_object_put(new_obj);

#ifdef CONFIG_UI_ENABLE
    net_video_rec_post_msg("changePAR");
#endif
    return 0;
}
int cmd_get_video_inv(void *priv, char *content)
{
    char buf[32];
    int gap = db_select("gap");
    snprintf(buf, sizeof(buf), "gap:%d", gap > 0 ? gap : 0);

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_INV", "NOTIFY", buf);
    return 0;
}

int cmd_put_video_inv(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "gap");

    const char *tmp_value = json_object_get_string(tmp);
    printf("ctp pro  %s \n", tmp_value);
    db_update("gap", atoi(tmp_value));
    json_object_put(new_obj);
    return 0;
}


int cmd_get_photo_quality(void *priv, char *content)
{
    char buf[32];
    int qua = db_select("qua");
    snprintf(buf, sizeof(buf), "qua:%d", qua > 0 ? qua : 0);

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "PHOTO_QUALITY", "NOTIFY", buf);
    return 0;
}

int cmd_put_photo_quality(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "qua");

    const char *tmp_value = json_object_get_string(tmp);
    printf("ctp pro  %s \n", tmp_value);
    db_update("qua", atoi(tmp_value));
    json_object_put(new_obj);
    return 0;
}
int cmd_get_photo_sharpness(void *priv, char *content)
{
    char buf[32];
    int acu = db_select("acu");
    snprintf(buf, sizeof(buf), "acu:%d", acu > 0 ? acu : 0);

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "PHOTO_SHARPNESS", "NOTIFY", buf);
    return 0;
}

int cmd_put_photo_sharpness(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "acu");

    const char *tmp_value = json_object_get_string(tmp);
    printf("ctp pro  %s \n", tmp_value);
    db_update("acu", atoi(tmp_value));
    json_object_put(new_obj);
    return 0;
}

int cmd_get_white_balance(void *priv, char *content)
{
    char buf[32];
    int wbl = db_select("wbl");
    snprintf(buf, sizeof(buf), "wbl:%d", wbl > 0 ? wbl : 0);

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "WHITE_BALANCE", "NOTIFY", buf);
    return 0;
}

int cmd_put_white_balance(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "wbl");

    const char *tmp_value = json_object_get_string(tmp);
    printf("ctp pro  %s \n", tmp_value);
    db_update("wbl", atoi(tmp_value));
    json_object_put(new_obj);
    return 0;
}

int cmd_get_photo_iso(void *priv, char *content)
{
    char buf[32];
    int iso = db_select("iso");
    snprintf(buf, sizeof(buf), "iso:%d", iso > 0 ? iso : 0);

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "PHOTO_ISO", "NOTIFY", buf);
    return 0;
}

int cmd_put_photo_iso(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];

    tmp =  json_object_object_get(parm, "iso");

    const char *tmp_value = json_object_get_string(tmp);
    printf("ctp pro  %s \n", tmp_value);
    db_update("iso", atoi(tmp_value));
    json_object_put(new_obj);
    return 0;
}


int cmd_get_photo_exp(void *priv, char *content)
{
    char buf[32];
    int pexp = db_select("pexp");
    snprintf(buf, sizeof(buf), "exp:%d", pexp > 0 ? pexp : 0);

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_EXP", "NOTIFY", buf);
    return 0;
}


int cmd_put_photo_exp(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "exp");

    const char *tmp_value = json_object_get_string(tmp);
    printf("ctp pro  %s \n", tmp_value);
    db_update("pexp", atoi(tmp_value));
    json_object_put(new_obj);
    return 0;
}

int cmd_get_anti_tremor(void *priv, char *content)
{
    char buf[32];
    int sok = db_select("sok");
    snprintf(buf, sizeof(buf), "sok:%d", sok > 0 ? sok : 0);

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "ANTI_TREMOR", "NOTIFY", buf);
    return 0;
}

int cmd_put_anti_tremor(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "sok");

    const char *tmp_value = json_object_get_string(tmp);
    printf("ctp pro  %s \n", tmp_value);
    db_update("sok", atoi(tmp_value));
    json_object_put(new_obj);
    return 0;
}

int cmd_get_photo_date(void *priv, char *content)
{
    char buf[32];

    int dat = db_select("dat");
    snprintf(buf, sizeof(buf), "dat:%d", dat > 0 ? dat : 0);

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "PHOTO_DATE", "NOTIFY", buf);
    return 0;
}

int cmd_put_photo_date(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "dat");

    const char *tmp_value = json_object_get_string(tmp);
    printf("ctp pro  %s \n", tmp_value);
    db_update("pdat", atoi(tmp_value));
    json_object_put(new_obj);
    return 0;
}

int cmd_get_fast_sca(void *priv, char *content)
{
    char buf[32];
    int sca = db_select("sca");
    snprintf(buf, sizeof(buf), "sca:%d", sca > 0 ? sca : 0);

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "FAST_SCA", "NOTIFY", buf);
    return 0;
}

int cmd_put_fast_sca(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "sca");

    const char *tmp_value = json_object_get_string(tmp);
    printf("ctp pro  %s \n", tmp_value);
    db_update("sca", atoi(tmp_value));
    json_object_put(new_obj);
    return 0;
}


int cmd_get_photo_color(void *priv, char *content)
{
    char buf[32];
    int col = db_select("col");
    snprintf(buf, sizeof(buf), "col:%d", col > 0 ? col : 0);

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "PHOTO_COLOR", "NOTIFY", buf);
    return 0;
}

int cmd_put_photo_color(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "col");

    const char *tmp_value = json_object_get_string(tmp);
    printf("ctp pro  %s \n", tmp_value);
    db_update("col", atoi(tmp_value));
    json_object_put(new_obj);
    return 0;
}

#if WIFI_RT_STREAM
int cmd_put_open_rt_stream(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    char buf[128];
    u8 mark;
    const char *h, *w, *format, *fps;
    //切换UI，禁止key
    /* sys_key_event_disable(); */
    net_switch_ui("video_rec");
    struct intent it;
    init_intent(&it);
    it.name = "net_video_rec";
//设置参数
    it.action = ACTION_VIDEO0_OPEN_RT_STREAM;
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    h = json_object_get_string(json_object_object_get(parm, "h"));
    w = json_object_get_string(json_object_object_get(parm, "w"));
    format = json_object_get_string(json_object_object_get(parm, "format"));
    fps = json_object_get_string(json_object_object_get(parm, "fps"));

    mark = 2;
    struct rt_stream_app_info info;
    if (w && h) {
        info.width = atoi(w);
        info.height = atoi(h);
    } else {
        info.width = 1920;
        info.height = 1080;
    }
    if (info.width == 1920) {
        db_update("rtf", VIDEO_RES_1080P);
    } else if (info.width == 1280) {
        db_update("rtf", VIDEO_RES_720P);
    } else if (info.width == 640) {
        db_update("rtf", VIDEO_RES_VGA);
    }
    /* db_flush(); */


    info.fps    = atoi(fps);
    if (atoi(format) == 1) {
        info.type   = NET_VIDEO_FMT_MOV;
    } else if (atoi(format) == 0) {

        info.type = NET_VIDEO_FMT_AVI;
    } else {

        info.type   = NET_VIDEO_FMT_MOV;
        printf("undefined info.type ,default use H264\n");
    }
    info.priv = priv;

    it.data = (const char *)&mark;//打开视频
    it.exdata = (u32) &info; //视频参数

    start_app(&it);

    json_object_put(new_obj);
    return 0;

}
int cmd_put_open_audio_rt_stream(void *priv, char *content)
{
    printf("no set %s function\n", __func__);
    /*
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    char buf[128];
    u8 mark;
    const char *format, *rate, *width, *channel;
    struct intent it;
    init_intent(&it);

    it.name = "net_video_rec";
    //设置参数
    it.action = ACTION_VIDEO0_OPEN_RT_STREAM;
    new_obj = json_tokener_parse(content);parm =  json_object_object_get(new_obj, "param");
    rate = json_object_get_string(json_object_object_get(parm, "rate"));
    width = json_object_get_string(json_object_object_get(parm, "width"));
    format = json_object_get_string(json_object_object_get(parm, "format"));
    channel = json_object_get_string(json_object_object_get(parm, "channel"));

    snprintf(buf, sizeof(buf), "width:%s,rate:%s,format:%s,channel:%s\n", width, rate, format, channel);
    mark = 0;
    it.data = (const char *)&mark;//打开音频
    it.exdata = buf; //音频参数
    start_app(&it);
    json_object_put(new_obj);
    */
    return 0;


}

int cmd_put_open_pull_rt_stream(void *priv, char *content)
{
#if defined (CONFIG_VIDEO1_ENABLE) || defined (CONFIG_VIDEO2_ENABLE)
#ifdef CONFIG_VIDEO1_ENABLE

#ifndef CONFIG_SPI_VIDEO_ENABLE
    if (!dev_online("video1.*")) {
        CTP_CMD_COMBINED(NULL, CTP_PULL_OFFLINE, "OPEN_PULL_RT_STREAM", "NOTIFY", CTP_PULL_OFFLINE_MSG);
        return 0;
    }
#endif
#endif
#ifdef CONFIG_VIDEO2_ENABLE

    if (!dev_online("uvc")) {
        CTP_CMD_COMBINED(NULL, CTP_PULL_OFFLINE, "OPEN_PULL_RT_STREAM", "NOTIFY", CTP_PULL_OFFLINE_MSG);
        return 0;
    }

#endif


    json_object *new_obj = NULL;
    json_object *parm = NULL;
    char buf[128];
    u8 mark;
    const char *h, *w, *format, *fps;
    struct intent it;
    init_intent(&it);
    net_switch_ui("video_rec");

    it.name = "net_video_rec";
//设置参数
    it.action = ACTION_VIDEO1_OPEN_RT_STREAM;
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    h = json_object_get_string(json_object_object_get(parm, "h"));
    w = json_object_get_string(json_object_object_get(parm, "w"));
    format = json_object_get_string(json_object_object_get(parm, "format"));
    fps = json_object_get_string(json_object_object_get(parm, "fps"));
    struct rt_stream_app_info info;
    info.width = atoi(w);
    info.height = atoi(h);
    info.fps    = atoi(fps);


    if (atoi(format) == 1) {
        info.type   = NET_VIDEO_FMT_MOV;
    } else if (atoi(format) == 0) {

        info.type = NET_VIDEO_FMT_AVI;
    } else {

        info.type   = NET_VIDEO_FMT_MOV;
        printf("undefined info.type ,default use H264\n");
    }



    info.priv = priv;
    if (info.width == 1920) {
        db_update("rtb", VIDEO_RES_1080P);
    } else if (info.width == 1280) {
        db_update("rtb", VIDEO_RES_720P);
    } else if (info.width == 640) {
        db_update("rtb", VIDEO_RES_VGA);
    }
    /* db_flush(); */


    mark = 2;
    it.data = (const char *)&mark;//打开视频
    it.exdata = (u32) &info; //视频参数

    start_app(&it);

    json_object_put(new_obj);
#else
    CTP_CMD_COMBINED(priv, CTP_PULL_NOSUPPORT, "OPEN_PULL_RT_STREAM", "NOTIFY", CTP_PULL_NOSUPPORT_MSG);
#endif
    return 0;

}
int cmd_put_open_pull_audio_rt_stream(void *priv, char *content)
{
    printf("no set %s function\n", __func__);
    /*
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    char buf[128];
    u8 mark;
    const char *format, *rate, *width, *channel;
    struct intent it;
    init_intent(&it);

    it.name = "net_video_rec";
    //设置参数
    it.action = ACTION_VIDEO1_OPEN_RT_STREAM;
    new_obj = json_tokener_parse(content);parm =  json_object_object_get(new_obj, "param");
    rate = json_object_get_string(json_object_object_get(parm, "rate"));
    width = json_object_get_string(json_object_object_get(parm, "width"));
    format = json_object_get_string(json_object_object_get(parm, "format"));
    channel = json_object_get_string(json_object_object_get(parm, "channel"));
    //printf("ctp pro  %s \n", h);

    snprintf(buf, sizeof(buf), "width:%s,rate:%s,format:%s,channel:%s\n", width, rate, format, channel);
    mark = 0;
    it.data = (const char *)&mark;//打开音频
    it.exdata = buf; //音频参数
    start_app(&it);
    json_object_put(new_obj);
    */
    return 0;


}
void close_rt_stream(struct sockaddr_in *dest)
{
    struct intent it;
    u8 mark = 2;

    if (!ctp_srv_get_cli_cnt()) {
        //打开key ，切换UI(录像模式)
        net_switch_ui("video_rec");
        it.name = "net_video_rec";
        it.action = ACTION_VIDEO_REC_GET_APP_STATUS;
        start_app(&it);
        struct net_video_hdl *net_rec_handler = (struct net_video_hdl *)it.exdata;

        if (net_rec_handler && net_rec_handler->net_video0_vrt_on) {
            puts("all rt0 stream close\n");
            it.action = ACTION_VIDEO0_CLOSE_RT_STREAM;
            it.data = (char *)&mark;
            start_app(&it);

        }

        if (net_rec_handler && net_rec_handler->net_video1_vrt_on) {
            puts("all rt1 stream close\n");
            it.action = ACTION_VIDEO1_CLOSE_RT_STREAM;
            it.data = (char *)&mark;
            start_app(&it);

        }

    }
}


static int cmd_put_close_rt_stream(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    char buf[128];
    u8 mark;
    const char *status = NULL;
    struct intent it;

    init_intent(&it);
    net_switch_ui("video_rec");
    it.name = "net_video_rec";
//设置参数
    it.action = ACTION_VIDEO0_CLOSE_RT_STREAM;
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    status = json_object_get_string(json_object_object_get(parm, "status"));

    if (atoi(status)) {
        mark = 2;
        it.data = (const char *)&mark; //close video param
        start_app(&it);

    }

    json_object_put(new_obj);
    return 0;
}
int cmd_put_close_audio_rt_stream(void *priv, char *content)
{
    printf("no set %s function\n", __func__);
    return 0;

}
int cmd_put_close_pull_audio_rt_stream(void *priv, char *content)
{
    printf("no set %s function\n", __func__);
    return 0;

}
int cmd_put_close_pull_rt_stream(void *priv, char *content)
{

#if defined (CONFIG_VIDEO1_ENABLE) || defined (CONFIG_VIDEO2_ENABLE)

#ifdef CONFIG_VIDEO1_ENABLE
#ifndef CONFIG_SPI_VIDEO_ENABLE
    if (!dev_online("video1.*")) {
        CTP_CMD_COMBINED(NULL, CTP_PULL_OFFLINE, "CLOSE_PULL_RT_STREAM", "NOTIFY", CTP_PULL_OFFLINE_MSG);
        return 0;
    }
#endif
#endif
#ifdef CONFIG_VIDEO2_ENABLE

    if (!dev_online("uvc")) {
        CTP_CMD_COMBINED(NULL, CTP_PULL_OFFLINE, "CLOSE_PULL_RT_STREAM", "NOTIFY", CTP_PULL_OFFLINE_MSG);
        return 0;
    }

#endif


    json_object *new_obj = NULL;
    json_object *parm = NULL;
    char buf[128];
    u8 mark;
    const char *status = NULL;
    struct intent it;
    /* sys_key_event_enable(); */
    init_intent(&it);
    net_switch_ui("video_rec");
    it.name = "net_video_rec";
//设置参数
    it.action = ACTION_VIDEO1_CLOSE_RT_STREAM;
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    status = json_object_get_string(json_object_object_get(parm, "status"));

    if (atoi(status)) {
        mark = 2;
        it.data = (const char *)&mark; //close video param
        start_app(&it);

    }

    json_object_put(new_obj);
#else

    CTP_CMD_COMBINED(priv, CTP_PULL_NOSUPPORT, "CLOSE_PULL_RT_STREAM", "NOTIFY", CTP_PULL_NOSUPPORT_MSG);
#endif
    return 0;

}

int cmd_get_close_rt_stream(void *priv, char *content)
{
    struct intent it;
    char buf[128];
    init_intent(&it);
    it.name = "net_video_rec";
    it.action = ACTION_VIDEO_REC_GET_APP_STATUS;
    start_app(&it);
    /* struct video_rec_hdl *rec_handler = (struct video_rec_hdl *)it.data; */
    struct net_video_hdl *rec_handler = (struct net_video_hdl *)it.exdata;

    if (rec_handler && rec_handler->net_video0_vrt_on == 1) {
        strcpy(buf, "status:1");
    } else {
        strcpy(buf, "status:0");
    }

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "CLOSE_RT_STREAM", "NOTIFY", buf);
    return 0;
}
int cmd_get_close_audio_rt_stream(void *priv, char *content)
{
    printf("no set %s function\n", __func__);
    /*
    struct intent it;
    char buf[128];
    init_intent(&it);
    it.name = "net_video_rec";
    it.action = ACTION_VIDEO_REC_GET_APP_STATUS;

    struct video_rec_hdl *rec_handler = (struct video_rec_hdl *)it.data;

    if(rec_handler && rec_handler->net_video0_art_on == 1) {
        strcpy(buf, "status:1");
    } else {
        strcpy(buf, "status:0");
    }

    CTP_CMD_COMBINED(priv ,CTP_NO_ERR,"CLOSE_AUDIO_RT_STREAM", "NOTIFY", buf);
    */
    return 0;
}
int cmd_get_close_pull_audio_rt_stream(void *priv, char *content)
{
    printf("no set %s function\n", __func__);
    /*
    struct intent it;
    char buf[128];
    init_intent(&it);
    it.name = "net_video_rec";
    it.action = ACTION_VIDEO_REC_GET_APP_STATUS;

    struct video_rec_hdl *rec_handler = (struct video_rec_hdl *)it.data;

    if(rec_handler && rec_handler->net_video1_art_on == 1) {
        strcpy(buf, "status:1");
    } else {
        strcpy(buf, "status:0");
    }

    CTP_CMD_COMBINED(priv ,CTP_NO_ERR,"CLOSE_PULL_AUDIO_RT_STREAM", "NOTIFY", buf);
    */
    return 0;
}
int cmd_get_close_pull_rt_stream(void *priv, char *content)
{
#if defined (CONFIG_VIDEO1_ENABLE) || defined (CONFIG_VIDEO2_ENABLE)
    struct intent it;
    char buf[128];
    init_intent(&it);
    it.name = "net_video_rec";
    it.action = ACTION_VIDEO_REC_GET_APP_STATUS;

    start_app(&it);
    /* struct video_rec_hdl *rec_handler = (struct video_rec_hdl *)it.data; */

    struct net_video_hdl *rec_handler = (struct net_video_hdl *)it.data;

    if (rec_handler && rec_handler->net_video1_vrt_on == 1) {
        strcpy(buf, "status:1");
    } else {
        strcpy(buf, "status:0");
    }

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "CLOSE_PULL_RT_STREAM", "NOTIFY", buf);
#else
    CTP_CMD_COMBINED(priv, CTP_PULL_NOSUPPORT, "CLOSE_PULL_RT_STREAM", "NOTIFY", CTP_PULL_NOSUPPORT_MSG);
#endif
    return 0;
}
#endif

#if 1
static int cmd_put_make_forward_files_list(void *priv, char *content)
{

    char buf[128];
    char path[64];
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char type = 0;
    u32 file_num = 0;
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "type");

    const char *tmp_value = json_object_get_string(tmp);

    tmp =  json_object_object_get(parm, "num");
    if (tmp != NULL) {
        const char *num = json_object_get_string(tmp);
        if (num != NULL && atoi(num) != 0) {
            file_num = atoi(num);
        }
    }
    if (tmp_value == NULL) {
        type = VID_JPG;
    } else {
        type = atoi(tmp_value);
    }
    /*printf("11-----ctp type  %s  , file_num = %d \n\n", tmp_value, file_num);*/
    switch (type) {
    case -1 :
        CTP_CMD_COMBINED(priv, CTP_SD_OFFLINE, "FORWARD_MEDIA_FILES_LIST", "NOTIFY", CTP_SD_OFFLINE_MSG);
        break;
    case NONE:
        /*snprintf(buf, sizeof(buf), "type:0,path:%s", CONFIG_REC_PATH_1"vf_list.txt");*/
        snprintf(buf, sizeof(buf), "type:0,path:%s", CONFIG_REC_PATH_0"vf_list.txt");
        CTP_CMD_COMBINED(priv, CTP_NO_ERR, "FORWARD_MEDIA_FILES_LIST", "NOTIFY", buf);
        break;
    case VID_JPG:

        if (!file_num) {

#if defined CONFIG_ENABLE_VLIST
            if (!FILE_INITIND_CHECK()) {
                FILE_GEN();
                /*snprintf(buf, sizeof(buf), "type:1,path:%s", CONFIG_REC_PATH_1"vf_list.txt");*/
                snprintf(buf, sizeof(buf), "type:1,path:%s", CONFIG_REC_PATH_0"vf_list.txt");
                CTP_CMD_COMBINED(priv, CTP_NO_ERR, "FORWARD_MEDIA_FILES_LIST", "NOTIFY", buf);

            } else {
                CTP_CMD_COMBINED(priv, CTP_REQUEST, "FORWARD_MEDIA_FILES_LIST", "NOTIFY", CTP_REQUEST_MSG);
            }
        } else {
            FILE_LIST_INIT_SMALL(file_num);
            /*snprintf(buf, sizeof(buf), "type:1,path:%s", CONFIG_REC_PATH_1"vf_list_small.txt");*/
            snprintf(buf, sizeof(buf), "type:1,path:%s", CONFIG_REC_PATH_0"vf_list_small.txt");
            CTP_CMD_COMBINED(priv, CTP_NO_ERR, "FORWARD_MEDIA_FILES_LIST", "NOTIFY", buf);
#endif
        }
        break;
    case VIDEO:
        vf_list(type, 1, path);
        snprintf(buf, sizeof(buf), "type:2,path:%s", path);
        CTP_CMD_COMBINED(priv, CTP_NO_ERR, "FORWARD_MEDIA_FILES_LIST", "NOTIFY", buf);
        break;
    case JPG:
        vf_list(type, 1, path);
        snprintf(buf, sizeof(buf), "type:3,path:%s", path);
        CTP_CMD_COMBINED(priv, CTP_NO_ERR, "FORWARD_MEDIA_FILES_LIST", "NOTIFY", buf);
        break;
    default:
        break;
    }


    json_object_put(new_obj);
    return 0;
}

static int cmd_put_make_behind_files_list(void *priv, char *content)
{
    char buf[128];
    char path[64];
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char type = 0;
    u32 file_num = 0;
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "type");

    const char *tmp_value = json_object_get_string(tmp);
    printf("ctp type  %s \n", tmp_value);


    tmp =  json_object_object_get(parm, "num");
    if (tmp != NULL) {
        const char *num = json_object_get_string(tmp);
        if (num != NULL && atoi(num) != 0) {
            file_num = atoi(num);
        }
    }
    if (tmp_value == NULL) {
        type = VID_JPG;
    } else {
        type = atoi(tmp_value);
    }
    switch (type) {
    case -1 :
        CTP_CMD_COMBINED(priv, CTP_SD_OFFLINE, "BEHIND_MEDIA_FILES_LIST", "NOTIFY", CTP_SD_OFFLINE_MSG);
        break;
    case NONE:
        snprintf(buf, sizeof(buf), "type:0,path:%s", CONFIG_REC_PATH_2"vf_list.txt");
        CTP_CMD_COMBINED(priv, CTP_NO_ERR, "BEHIND_MEDIA_FILES_LIST", "NOTIFY", buf);
        break;
    case VID_JPG:
        if (!file_num) {

#if defined CONFIG_ENABLE_VLIST
            if (!FILE_INITIND_CHECK()) {
                FILE_GEN();
                snprintf(buf, sizeof(buf), "type:1,path:%s", CONFIG_REC_PATH_2"vf_list.txt");
                CTP_CMD_COMBINED(priv, CTP_NO_ERR, "BEHIND_MEDIA_FILES_LIST", "NOTIFY", buf);


            } else {

                CTP_CMD_COMBINED(priv, CTP_REQUEST, "BEHIND_MEDIA_FILES_LIST", "NOTIFY", CTP_REQUEST_MSG);
            }

        } else {
            FILE_LIST_INIT_SMALL(file_num);
            snprintf(buf, sizeof(buf), "type:1,path:%s", CONFIG_REC_PATH_2"vf_list_small.txt");
            CTP_CMD_COMBINED(priv, CTP_NO_ERR, "BEHIND_MEDIA_FILES_LIST", "NOTIFY", buf);
#endif
        }
        break;
    case VIDEO:
        vf_list(type, 0, path);
        snprintf(buf, sizeof(buf), "type:2,path:%s", path);
        CTP_CMD_COMBINED(priv, CTP_NO_ERR, "BEHIND_MEDIA_FILES_LIST", "NOTIFY", buf);
        break;
    case JPG:
        vf_list(type, 0, path);
        snprintf(buf, sizeof(buf), "type:3,path:%s", path);
        CTP_CMD_COMBINED(priv, CTP_NO_ERR, "BEHIND_MEDIA_FILES_LIST", "NOTIFY", buf);
        break;
    default:
        break;
    }

    json_object_put(new_obj);
    return 0;

}

#endif

static int cmd_get_firmware_upgrade_ready(void *priv, char *content)
{
    char buf[128];
    u8 status;
    status = 1;
    snprintf(buf, sizeof(buf), "status:%d", status);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "FIRMWARE_UPGRADE_READY", "status", buf);

    return 0;
}

static int cmd_put_firmware_file_send_end(void *priv, char *content)
{
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "FIRMWARE_FILE_SEND_END", "NOTIFY", NULL);
    return 0;
}

static int cmd_get_rt_talk_ctl(void *priv, char *content)
{
    char buf[128];

    /*extern u8 get_rt_talk_status(void);*/

    /*snprintf(buf, sizeof(buf), "status:%d", get_rt_talk_status());*/

    /*CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "RT_TALK_CTL", "NOTIFY", buf);*/

    return 0;
}

static int cmd_put_rt_talk_ctl(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];
    const char *s_str;
    u8 status;


    new_obj = json_tokener_parse(content);
    parm    =  json_object_object_get(new_obj, "param");
    tmp  = json_object_object_get(parm, "status");
    s_str = json_object_get_string(tmp);
    printf("\n------%s------\n", s_str);
    status = atoi(s_str);
    printf("\n-----status is %d-------\n", status);
    snprintf(buf, sizeof(buf), "status:%d", status);
    /*if (status == 1) {*/
    /*extern int rt_talk_net_init(void);*/
    /*rt_talk_net_init();*/
    /*} else if (status == 0) {*/
    /*extern int rt_talk_net_uninit(void);*/
    /*rt_talk_net_uninit();*/
    /*}*/
    snprintf(buf, sizeof(buf), "status:%d", status);

    CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "RT_TALK_CTL", "NOTIFY", buf);

    return 0;
}

static int cmd_get_voice_talk_ctl(void *priv, char *content)
{
    char buf[256 ] = {0};

    snprintf(buf, sizeof(buf), CONFIG_ROOT_PATH);
    CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "VOICE_TALK_CTL", "NOTIFY", buf);

    return 0;
}

static int cmd_put_voice_talk_ctl(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[256 ] = {0};
    const char *s_str;
    u8 status;

    new_obj = json_tokener_parse(content);
    parm    =  json_object_object_get(new_obj, "param");
    tmp  = json_object_object_get(parm, "status");
    s_str = json_object_get_string(tmp);
    status = atoi(s_str);
    printf("\n-----status is %d-------\n", status);
    snprintf(buf, sizeof(buf), "status:%d", status);
    if (status == 1) {
        tmp  = json_object_object_get(parm, "PATH");
        s_str = json_object_get_string(tmp);
        printf("\n %s \n", s_str);
        /*extern void play_voice_file(const char *file_name);*/
        /*play_voice_file(s_str);*/

    } else {

    }

    CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "VOICE_TALK_CTL", "NOTIFY", buf);

    return 0;
}

static int cmd_put_files_delete(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[256 ] = {0};
    u32 i = 0;
    char filename[8];
    u32 ret = 0;
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");

    while (1) {
        sprintf(filename, "path_%d", i);
        tmp =  json_object_object_get(parm, filename);

        if (tmp == NULL) {
            break;
        }

        const char *tmp_value = json_object_get_string(tmp);
        printf("filename %s \n", tmp_value);

        if (fdelete_by_name(tmp_value)) {
            printf("fdelete by name\n");
            ret = snprintf(buf, sizeof(buf), "status:%d,path:%s", 0, tmp_value);
            CTP_CMD_COMBINED(priv, CTP_OPEN_FILE, "FILES_DELETE", "NOTIFY", buf);
        } else {

#if defined CONFIG_ENABLE_VLIST
            FILE_DELETE(tmp_value, 1);
#endif
            snprintf(buf, sizeof(buf), "status:%d,path:%s", 1, tmp_value);
            CTP_CMD_COMBINED(priv, CTP_NO_ERR, "FILES_DELETE", "NOTIFY", buf);

        }

        i++;
    }

    json_object_put(new_obj);
    return 0;

}
static int cmd_put_multi_cover_figure(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];
    u32 i = 0;
    char filename[8];
    u32 ret = 0;
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    char (*file_name_array)[64] = calloc(1, 51 * 64);
    if (file_name_array == NULL) {
        CTP_CMD_COMBINED(priv, CTP_REQUEST, "MULTI_COVER_FIGURE", "NOTIFY", CTP_REQUEST_MSG);
        return 0;
    }

    while (1) {
        sprintf(filename, "path_%d", i);
        tmp =  json_object_object_get(parm, filename);

        if (tmp == NULL) {
            break;
        }

        if (i > 50) {
            printf("path is too many\n");
            CTP_CMD_COMBINED(priv, CTP_REQUEST, "MULTI_COVER_FIGURE", "NOTIFY", CTP_REQUEST_MSG);
            break;
        }

        const char *tmp_value = json_object_get_string(tmp);
        strcpy(file_name_array[i], tmp_value);
        /* printf("%d filename %s \n", i, file_name_array[i]); */
        i++;
    }

    if (!i) {
        free(file_name_array);
        file_name_array = NULL;
        CTP_CMD_COMBINED(priv, CTP_REQUEST, "MULTI_COVER_FIGURE", "NOTIFY", CTP_REQUEST_MSG);
        json_object_put(new_obj);
        return 0;
    }

    struct net_req req;
    memset(&req, 0, sizeof(struct net_req));
    req.pre.type = PREVIEW;
    req.pre.filename = file_name_array;

    /*if (video_preview_post_msg(&req)) {*/
    /*CTP_CMD_COMBINED(priv, CTP_REQUEST, "MULTI_COVER_FIGURE", "NOTIFY", CTP_REQUEST_MSG);*/
    /*json_object_put(new_obj);*/
    /*return 0;*/
    /*}*/

    json_object_put(new_obj);

    return 0;

}

static int cmd_put_thunbails(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    u32 i = 0;
    char buf[128];
    u32 ret = 0;
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "path");
    const char *path = json_object_get_string(tmp);
    tmp =  json_object_object_get(parm, "offset");
    const char *offset = json_object_get_string(tmp);
    tmp =  json_object_object_get(parm, "num");
    const char *num = json_object_get_string(tmp);
    tmp =  json_object_object_get(parm, "timeinv");
    const char *timeinv = json_object_get_string(tmp);

    printf("path -> %s    offset-> %s   num->%s   timeinv->%s\n"
           , path
           , offset
           , num
           , timeinv);
    struct net_req req;
    memset(&req, 0, sizeof(struct net_req));

    strcpy(file_name, path);
    req.pre.type = THUS;

    char (*filename)[64];
    req.pre.filename = (char (*)[64])file_name;

    if (num == NULL) {
        req.pre.num = 0;
    } else {
        req.pre.num     = atoi(num);
    }

    req.pre.offset  = atoi(offset);
    req.pre.timeinv = atoi(timeinv);
    /*if (video_preview_post_msg(&req)) {*/
    /*CTP_CMD_COMBINED(priv, CTP_REQUEST, "THUMBNAILS", "NOTIFY", CTP_REQUEST_MSG);*/
    /*json_object_put(new_obj);*/
    /*return 0;*/
    /*}*/

    json_object_put(new_obj);
    return 0;


}
static int cmd_put_thunbails_ctrl(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];
    u32 i = 0;
    char filename[8];
    u32 ret = 0;
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "status");
    const char *status = json_object_get_string(tmp);
    printf("status -> %s\n", status);

    if (ctp_srv_get_cli_addr(priv)) {
        /*video_cli_slide(ctp_srv_get_cli_addr(priv), atoi(status));*/
    } else {
        /*video_cli_slide(cdp_srv_get_cli_addr(priv), atoi(status));*/
    }
    snprintf(buf, sizeof(buf), "status:%s", status);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "THUMBNAILS_CTRL", "NOTIFY", buf);
    json_object_put(new_obj);
    return 0;


}
static int cmd_put_time_axis_play(void *priv, char *content)
{
    struct server *net = NULL;
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    u32 i = 0;
    char filename[8];
    u32 ret = 0;
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "path");
    const char *file_name = json_object_get_string(tmp);

    tmp =  json_object_object_get(parm, "offset");
    const char *msec = json_object_get_string(tmp);

    printf("filename :%s   msec:%s\n", file_name, msec);
    struct net_req req;
    memset(&req, 0, sizeof(struct net_req));
    strcpy(req.playback.file_name, file_name);
    req.playback.msec = atoi(msec);
    /*if (video_playback_post_msg(&req)) {*/
    /*CTP_CMD_COMBINED(priv, CTP_REQUEST, "TIME_AXIS_PLAY", "NOTIFY", CTP_REQUEST_MSG);*/
    /*json_object_put(new_obj);*/
    /*return 0;*/
    /*}*/
    json_object_put(new_obj);

    return 0;

}

#define PLAY_VIDEO_CONTINUE 0
#define PLAY_VIDEO_PAUSE    1
#define PLAY_VIDEO_STOP     2
static int cmd_put_time_axis_play_ctrl(void *priv, char *content)
{
    char buf[128];
    struct server *net = NULL;
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    u32 i = 0;
    char filename[8];
    u32 ret = 0;
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "status");
    const char *status = json_object_get_string(tmp);

    struct sockaddr_in *dst_addr = ctp_srv_get_cli_addr(priv);
    if (!dst_addr) {
        dst_addr = cdp_srv_get_cli_addr(priv);
    }
    printf("status:%s\n", status);

    switch (atoi(status)) {
    case PLAY_VIDEO_CONTINUE:
        /*ret = playback_cli_continue(dst_addr);*/
        break;

    case PLAY_VIDEO_PAUSE:
        /*ret = playback_cli_pause(dst_addr);*/
        break;

    case PLAY_VIDEO_STOP:
        /*ret = playback_disconnect_cli(dst_addr);*/
        break;

    default:
        ret = -1;
        break;
    }

    if (!ret) {

        snprintf(buf, sizeof(buf), "status:%s", status);
        CTP_CMD_COMBINED(priv, CTP_NO_ERR, "TIME_AXIS_PLAY_CTRL", "NOTIFY", buf);
    } else {
        CTP_CMD_COMBINED(priv, CTP_REQUEST, "TIME_AXIS_PLAY_CTRL", "NOTIFY", CTP_REQUEST_MSG);
    }

    json_object_put(new_obj);

    return 0;

}

static int cmd_put_time_axis_fast_play(void *priv, char *content)
{
    char buf[128];
    struct server *net = NULL;
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    u32 i = 0;
    u32 speed = 0; // x2 x4 x8 x16 x32 x64
    char filename[8];
    u32 ret = 0;
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "level");
    const char *level = json_object_get_string(tmp);

    struct sockaddr_in *dst_addr = ctp_srv_get_cli_addr(priv);
    if (!dst_addr) {
        dst_addr = cdp_srv_get_cli_addr(priv);
    }
    printf("level:%s\n", level);
    if (atoi(level)) {
        speed = (1 << (atoi(level) - 1)) * 32;
    } else {
        speed = 0;
    }
    /*playback_cli_fast_play(dst_addr, speed);*/

    if (!ret) {
        snprintf(buf, sizeof(buf), "level:%s", level);
        CTP_CMD_COMBINED(priv, CTP_NO_ERR, "TIME_AXIS_FAST_PLAY", "NOTIFY", buf);
    } else {
        CTP_CMD_COMBINED(priv, CTP_REQUEST, "TIME_AXIS_FAST_PLAY", "NOTIFY", CTP_REQUEST_MSG);
    }

    json_object_put(new_obj);

    return 0;

}


static int cmd_get_wind_velocity(void *priv, char *content)   //cdp
{
    char buf[32];

    snprintf(buf, sizeof(buf), "wind_velocity:%d", 0);

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "WIND_VELOCITY", "NOTIFY", buf);
    return 0;
}
static int cmd_put_device_direction(void *priv, char *content)   //cdp
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    char buf[128];
    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");

    tmp =  json_object_object_get(parm, "device_direction");

    const char *tmp_value = json_object_get_string(tmp);
    printf(">>>device_direction:%s\n", tmp_value);

    snprintf(buf, sizeof(buf), "device_direction:%s", tmp_value);
    CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "DEVICE_DIRECTION_CONTROL", "NOTIFY", buf);

    json_object_put(new_obj);
    return 0;
}



static int cmd_get_date_time(void *priv, char *content)
{
    struct sys_time time;
    char buf[128];
    void *rtc_fd = NULL;
    rtc_fd = dev_open("rtc", NULL);
    if (!rtc_fd) {
        printf("rtc open err !!\n\n");
        return 0;
    }
    dev_ioctl(rtc_fd, IOCTL_GET_SYS_TIME, (u32)&time);
    dev_close(rtc_fd);

    snprintf(buf, sizeof(buf), "date:%04d%02d%02d%02d%02d%02d", time.year, time.month, time.day, time.hour, time.min, time.sec);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "DATE_TIME", "NOTIFY", buf);

    return 0;
}
static void date_time(const char *date, struct sys_time *tm)
{
    char *f = NULL;
    char *b = NULL;
    char buf[5] = {0};

    memcpy(buf, date, 4);
    tm->year = atoi(buf);
    memcpy(buf, date + 4, 2);
    buf[2] = '\0';
    tm->month = atoi(buf);
    memcpy(buf, date + 6, 2);
    buf[2] = '\0';
    tm->day = atoi(buf);
    memcpy(buf, date + 8, 2);
    buf[2] = '\0';
    tm->hour = atoi(buf);
    memcpy(buf, date + 10, 2);
    buf[2] = '\0';
    tm->min = atoi(buf);
    memcpy(buf, date + 12, 2);
    buf[2] = '\0';
    tm->sec = atoi(buf);

}
static int cmd_put_date_time(void *priv, char *content)
{
    char buf[128];
    struct server *net = NULL;
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    struct sys_time time;
    void *rtc_fd = NULL;


    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "date");
    const char *date = json_object_get_string(tmp);



    printf("date:%s\n", date);
    if (date != NULL) {
        date_time(date, &time);
    }
    printf("date->year:%04d month:%02d day:%02d hour:%02d min:%02d sec:%02d", time.year, time.month, time.day, time.hour, time.min, time.sec);
    rtc_fd = dev_open("rtc", NULL);
    if (!rtc_fd) {
        printf("open rtd err \n\n");
        json_object_put(new_obj);
        return 0;
    }
    dev_ioctl(rtc_fd, IOCTL_SET_SYS_TIME, (u32)&time);
    dev_close(rtc_fd);
    snprintf(buf, sizeof(buf), "date:%04d%02d%02d%02d%02d%02d", time.year, time.month, time.day, time.hour, time.min, time.sec);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "DATE_TIME", "NOTIFY", buf);
    json_object_put(new_obj);
    return 0;
}


static int cmd_get_ap_ssid_info(void *priv, char *content)
{
    char buf[128];
    struct wifi_mode_info info;
    info.mode = AP_MODE;
    wifi_get_mode_cur_info(&info);
    printf("ap get ssid:%s   pwd:%s  \n", info.ssid, info.pwd);
    snprintf(buf, sizeof(buf), "ssid:%s,pwd:%s", info.ssid, info.pwd);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "AP_SSID_INFO", "NOTIFY", buf);
    return 0;


}

static int cmd_put_ap_ssid_info(void *priv, char *content)
{
    char buf[128];
    struct server *net = NULL;
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    struct sys_time time;

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "ssid");
    const char *ssid = json_object_get_string(tmp);

    tmp =  json_object_object_get(parm, "pwd");

    const char *pwd = json_object_get_string(tmp);
    tmp =  json_object_object_get(parm, "status");
    const char *status = json_object_get_string(tmp);


    printf("cmd_put_ap_ssid_info : ssid:%s   pwd:%s \n%s\n", ssid, pwd, content);
    if (strlen(pwd) == 0) {

        snprintf(buf, sizeof(buf), "ssid:%s,status:%s", ssid,  status);
    } else {
        snprintf(buf, sizeof(buf), "ssid:%s,pwd:%s,status:%s", ssid, pwd, status);
    }
    if (ssid == NULL || (strlen(pwd) > 0 && strlen(pwd) < 8)
        || strlen(ssid) >= 32) {
        printf("ssid is null or pwd is less than 7\n");
        CTP_CMD_COMBINED(priv, CTP_REQUEST, "AP_SSID_INFO", "NOTIFY", buf);
        return 0;
    }

    struct wifi_mode_info info;
    if (!strcmp("", ssid)) {
        info.mode = AP_MODE;
        wifi_get_mode_cur_info(&info);
    } else {
        info.ssid = (char *)ssid;
        info.pwd = (char *)pwd;
    }
    wifi_store_mode_info(AP_MODE, info.ssid, info.pwd);

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "AP_SSID_INFO", "NOTIFY", buf);

    if (atoi(status)) {
        os_time_dly(300);//重启太快，没回复
        cpu_reset();
    }

    json_object_put(new_obj);

    return 0;
}
static int cmd_get_sta_ssid_info(void *priv, char *content)
{
    char buf[128];
    struct wifi_mode_info info;
    info.mode = STA_MODE;
    wifi_get_mode_cur_info(&info);
    printf("sta get ssid:%s   pwd:%s  \n", info.ssid, info.pwd);
    snprintf(buf, sizeof(buf), "ssid:%s,pwd:%s", info.ssid, info.pwd);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "STA_SSID_INFO", "NOTIFY", buf);
    return 0;
}



static int cmd_put_sta_ssid_info(void *priv, char *content)
{
    char buf[128];
    struct server *net = NULL;
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;
    struct sys_time time;

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "ssid");
    const char *ssid = json_object_get_string(tmp);

    tmp =  json_object_object_get(parm, "pwd");

    const char *pwd = json_object_get_string(tmp);
    tmp =  json_object_object_get(parm, "status");
    const char *status = json_object_get_string(tmp);



    printf("ssid:%s   pwd:%s \n", ssid, pwd);
    if (strlen(pwd) == 0) {

        snprintf(buf, sizeof(buf), "ssid:%s,status:%s", ssid, status);
    } else {
        snprintf(buf, sizeof(buf), "ssid:%s,pwd:%s,status:%s", ssid, pwd, status);
    }

    if (ssid == NULL || (strlen(pwd) > 0 && strlen(pwd) < 8)) {
        printf("ssid is null or pwd is less than 7\n");
        CTP_CMD_COMBINED(priv, CTP_REQUEST, "STA_SSID_INFO", "NOTIFY", CTP_REQUEST_MSG);
        return 0;
    }

    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "STA_SSID_INFO", "NOTIFY", buf);

    //断开所有客户端

    ctp_srv_disconnect_all_cli();
    cdp_srv_disconnect_all_cli();

    info.dest_addr = NULL;

    if (atoi(status)) {
        wifi_store_mode_info(STA_MODE, ssid, pwd);
    }

    //切换WIFI模式到STA模式,切换成功后设备自行连接上热点
    wifi_enter_sta_mode(ssid, pwd);
    //选择是否保存当前WIFI模式信息
    json_object_put(new_obj);

    return 0;
}
static int cmd_put_soft_reset(void *priv, char *content)
{
    int cnt = 0;
#if 1
    extern char net_update_request(void);
    while (net_update_request()) {
        cnt++;
        os_time_dly(20);//200ms
        if (cnt > 100) { //20s
            break;
        }
    }
#endif
    os_time_dly(200);
    cpu_reset();

    return 0;
}

static int cmd_put_file_lock(void *priv, char *content)
{
    char buf[32];
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *tmp = NULL;

    new_obj = json_tokener_parse(content);
    parm =  json_object_object_get(new_obj, "param");
    tmp =  json_object_object_get(parm, "path");
    const char *path = json_object_get_string(tmp);

    tmp =  json_object_object_get(parm, "status");
    const char *status = json_object_get_string(tmp);

    FILE *file = fopen(path, "r");
    if (file == NULL) {

        CTP_CMD_COMBINED(priv, CTP_OPEN_FILE, "FILE_LOCK", "NOTIFY", CTP_OPEN_FILE_MSG);
        return 0;
    }
    int attr;
    fget_attr(file, &attr);

    if (atoi(status)) {
        attr |= F_ATTR_RO;

#if defined CONFIG_ENABLE_VLIST
        FILE_CHANGE_ATTR(path, '2');
#endif
    } else {

#if defined CONFIG_ENABLE_VLIST
        FILE_CHANGE_ATTR(path, '1');
#endif
        attr &= ~F_ATTR_RO;
    }
    fset_attr(file, attr);
    fclose(file);
    snprintf(buf, sizeof(buf), "status:%d", atoi(status));
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "FILE_LOCK", "NOTIFY", buf);

    return 0;
}


static int cmd_put_generic_cmd(void *priv, char *content)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "status:%d", 1);

    printf("GENERIC_CMD  PUT\n");
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "GENERIC_CMD", "NOTIFY", buf);

    return 0;
}

static int cmd_get_generic_cmd(void *priv, char *content)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "status:%d", 1);
    printf("GENERIC_CMD  GET\n");
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "GENERIC_CMD", "NOTIFY", buf);
    return 0;


}




static int cmd_put_ctp_cli_connected(void *priv, char *content)
{
    return 0;
}

static int cmd_put_ctp_cli_disconnect(void *priv, char *content)
{
    char buf[32];
    struct sockaddr_in *dest_addr;
    struct ctp_map_entry *map;
    dest_addr = ctp_srv_get_cli_addr(priv);
    if (!dest_addr) {
        dest_addr = cdp_srv_get_cli_addr(priv);
    }
    printf("app_accept_num = %d \n", info.num);
    if (info.num > 0) {
        info.num--;
        if (info.num > 0) {
            list_for_ctp_mapping_tab(map) {
                map->sync = false;
            }
            ctp_srv_free_cli(priv);
            cdp_srv_free_cli(priv);
            return 0;
        }
    } else {
        list_for_ctp_mapping_tab(map) {
            map->sync = false;
        }
        return 0;
    }
    info.dest_addr = NULL;
    info.cli = NULL;

    /*extern void stream_media_server_ip_close(struct sockaddr_in * dest_addr);*/
    /*extern void stupid_ftpd_disconnect_cli(struct sockaddr_in * dest_addr);*/
    /*extern int video_rec_all_stop_notify(void);*/
    /*extern int rt_talk_net_uninit(void);*/

    ctp_cmd_socket_unregister(priv);

    if (app_online_timer) {
        sys_timer_del(app_online_timer);
        app_online_timer = 0;
    }
    close_rt_stream(dest_addr);
    /*stream_media_server_ip_close(dest_addr);*/
    /*stupid_ftpd_disconnect_cli(dest_addr);*/
    /*video_rec_all_stop_notify();*/
    printf("|CLI_DISCONNECT 0x%x, 0x%x\n", (u32)priv, (u32)dest_addr->sin_addr.s_addr);
    /*video_preview_and_thus_disconnect(dest_addr);*/
    /*playback_disconnect_cli(dest_addr);*/
    http_get_server_discpnnect_cli(dest_addr);
    /*rt_talk_net_uninit();*/
    strcpy(buf, "status:1");
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "CTP_CLI_DISCONNECT", "NOTIFY", buf);
    /*FILE_LIST_EXIT();*/
    /*wifi_rt_stream_uninit(dest_addr); */
    /*wifi_video_playback_uninit(dest_addr); */
    /*set_ftp_outscan_cnt(1, (u32)dest_addr->sin_addr.s_addr); */
    list_for_ctp_mapping_tab(map) {
        map->sync = false;
    }

#ifdef CONFIG_NET_SCR
    struct __NET_SCR_CFG cfg = {0};
    if (ctp_srv_get_cli_addr(priv)) {
        memcpy(&cfg.cli_addr, ctp_srv_get_cli_addr(priv), sizeof(struct sockaddr_in));
    } else {
        memcpy(&cfg.cli_addr, cdp_srv_get_cli_addr(priv), sizeof(struct sockaddr_in));
    }
    net_scr_uninit(&cfg);
#endif

    ctp_srv_free_cli(priv);
    cdp_srv_free_cli(priv);
    /* sys_key_event_enable(); */
    key_event_enable();
    /* sys_touch_event_enable(); */
    puts("|CLI_DISCONNECT OVER...\n\n\n\n");
#if TCFG_LCD_USB_SHOW_COLLEAGUE
    user_net_video_rec_open(1);
#endif
    return 0;
}


extern int net_critical_video_send(void);
static int cmd_put_video_bumping(void *priv, char *content)
{
#ifdef CRITICAL_VIDEO_EN
    char buf[128];

    snprintf(buf, sizeof(buf), "w:%d,h:%d,fps:%d,rate:%d,t:%d", 640, 480, net_video_rec_get_fps(), net_video_rec_get_audio_rate(), 10);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_BUMPING", "NOTIFY", buf);
    msleep(500);
    net_critical_video_send();
#endif
#ifdef CRITICAL_VIDEO_EN_DOUBLE
    char buf[128];

    snprintf(buf, sizeof(buf), "w:%d,h:%d,fps:%d,rate:%d,t:%d", 1280, 720, net_video_rec_get_fps(), net_video_rec_get_audio_rate(), 10);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_BUMPING", "NOTIFY", buf);
    msleep(500);
    net_critical_video_send();
#endif


#ifndef CRITICAL_VIDEO_EN
#ifndef CRITICAL_VIDEO_EN_DOUBLE
    CTP_CMD_COMBINED(priv, CTP_REQUEST, "VIDEO_BUMPING", "NOTIFY", CTP_REQUEST_MSG);
#endif
#endif

    return 0;
}
static int cmd_get_video_bumping(void *priv, char *content)
{
#ifdef CRITICAL_VIDEO_EN
    char buf[128];

    snprintf(buf, sizeof(buf), "w:%d,h:%d,fps:%d,rate:%d,t:%d", 640, 480, net_video_rec_get_fps(), net_video_rec_get_audio_rate(), 10);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_BUMPING", "NOTIFY", buf);
#endif
#ifdef CRITICAL_VIDEO_EN_DOUBLE
    char buf[128];

    snprintf(buf, sizeof(buf), "w:%d,h:%d,fps:%d,rate:%d,t:%d", 1280, 720, net_video_rec_get_fps(), net_video_rec_get_audio_rate(), 10);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "VIDEO_BUMPING", "NOTIFY", buf);
#endif
    return 0;
}

static int cmd_get_gen_chk(void *priv, char *content)
{
    return 0;
}

#include "video_rec.h"
static int cmd_get_pull_video_status(void *priv, char *content)
{
    char buf[128];

    int res = db_select("res2");
    res = res > 0 ? res : 0;

    switch (res) {
    case VIDEO_RES_1080P:
        snprintf(buf, sizeof(buf), "w:%d,h:%d,format:%d,fps:%d,rate:%d", 1280, 720, 1, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
        break;

    case VIDEO_RES_720P:
        snprintf(buf, sizeof(buf), "w:%d,h:%d,format:%d,fps:%d,rate:%d", 640, 480, 1, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
        break;

    case VIDEO_RES_VGA:
        snprintf(buf, sizeof(buf), "w:%d,h:%d,format:%d,fps:%d,rate:%d", 640, 480, 1, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
        break;

    default:
        snprintf(buf, sizeof(buf), "w:%d,h:%d,format:%d,fps:%d,rate:%d", 640, 480, 1, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
        break;
    }



#ifdef CONFIG_VIDEO1_ENABLE
#ifdef CONFIG_SPI_VIDEO_ENABLE
    extern int spi_camera_width_get(void);
    extern int spi_camera_height_get(void);
    snprintf(buf, sizeof(buf), "status:1,h:%d,w:%d,fps:%d,rate:%d,format:1", spi_camera_height_get(), spi_camera_width_get(), 15, net_video_rec_get_audio_rate());
    CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "PULL_VIDEO_STATUS", "NOTIFY", buf);
    return 0;
#endif
    if (dev_online("video1.*")) {
        switch (res) {
        case VIDEO_RES_1080P:
            snprintf(buf, sizeof(buf), "status:1,h:%d,w:%d,fps:%d,rate:%d,format:1", 720, 1280, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
            break;

        case VIDEO_RES_720P:
            snprintf(buf, sizeof(buf), "status:1,h:%d,w:%d,fps:%d,rate:%d,format:1", 480, 640, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
            break;

        case VIDEO_RES_VGA:
            snprintf(buf, sizeof(buf), "status:1,h:%d,w:%d,fps:%d,rate:%d,format:1", 480, 640, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
            break;

        default:
            snprintf(buf, sizeof(buf), "status:1,h:%d,w:%d,fps:%d,rate:%d,format:1", 720, 1280, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
//       CTP_ERR(CTP_REQUEST);
            break;
        }


        CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "PULL_VIDEO_STATUS", "NOTIFY", buf);
    } else {

        strcpy(buf, "status:0");
        CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "PULL_VIDEO_STATUS", "NOTIFY", buf);
    }
#endif
#ifdef CONFIG_VIDEO2_ENABLE

    if (dev_online("uvc")) {
        printf("UVC ON LINE \n\n");
        switch (res) {
        case VIDEO_RES_1080P:
            snprintf(buf, sizeof(buf), "status:1,h:%d,w:%d,fps:%d,rate:%d,format:1", 720, 1280, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
            break;

        case VIDEO_RES_720P:
            snprintf(buf, sizeof(buf), "status:1,h:%d,w:%d,fps:%d,rate:%d,format:1", 480, 640, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
            break;

        case VIDEO_RES_VGA:
            snprintf(buf, sizeof(buf), "status:1,h:%d,w:%d,fps:%d,rate:%d,format:1", 480, 640, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
            break;

        default:
            snprintf(buf, sizeof(buf), "status:1,h:%d,w:%d,fps:%d,rate:%d,format:1", 480, 640, net_video_rec_get_fps(), net_video_rec_get_audio_rate());
//       CTP_ERR(CTP_REQUEST);
            break;
        }

        CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "PULL_VIDEO_STATUS", "NOTIFY", buf);
    } else {
        strcpy(buf, "status:0");
        printf("UVC OFF LINE \n\n");
        CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "PULL_VIDEO_STATUS", "NOTIFY", buf);
    }

#endif



    return 0;
}
/***************************************************************************
功能：查看设备wifi支持模式、改变设备wifi运行模式
命令描述
Topic: "WIFI_MODE"
errno: 参考设备错误列表,
op: 操作类型("GET","PUT","NOTIFY")
status: wifi模式（0:2.4G, 1:5G)

GET ： "op":"GET"
2.4G回复说明   ：{"op":"NOTIFY", "param":{ "status":"0" }
支持5G回复说明 ：{"op":"NOTIFY", "param":{ "status":"1" }


设置2.4G模式："status":"0"
PUT ： "op":"PUT", "param":{ "status":"0"}
回复说明：{"op":"NOTIFY","param":{ "status":"0"}

设置5G模式："status":"1"
PUT ： "op":"PUT", "param":{ "status":"1"}
回复说明：{"op":"NOTIFY","param":{ "status":"1"}


wifi模块运行模式获取
Topic: "WIFI_RUN_MODE"
2.4G回复说明：{"op":"NOTIFY", "param":{ "status":"0" }
5G回复说明 ：{"op":"NOTIFY", "param":{ "status":"1" }

出错则APP收到CTP_REQUEST_MSG消息，即"errno":4, "op":"NOTIFY","param":{ "msg":"CTP_REQUEST_MSG"}
***************************************************************************/
int __attribute__((weak)) get_wifi_module_support_mode(void)
{
#ifdef USE_5G2G_TOW_CHANNEL
    return 1;
#else
    return 0;
#endif
}
static int cmd_put_wifi_change_mode(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *status = NULL;
    char *val_str;
    int val;
    char buf[64];
    int wifi_run_mode;
    /*extern int set_hostapd_config_file_normol_mode(void);*/
    /*extern int set_hostapd_config_file_5g_mode(void);*/

    //分解content字段
    new_obj = json_tokener_parse(content);
    parm = json_object_object_get(new_obj, "param");
    status = json_object_object_get(parm, "status");

    val_str = (char *)json_object_get_string(status);
    val = atoi(val_str) & 0x1;
    wifi_run_mode = db_select("wfmode");
    wifi_run_mode = wifi_run_mode > 0 ? wifi_run_mode : 0;
    if (wifi_run_mode == val) {
        snprintf(buf, sizeof(buf), "status:%d", wifi_run_mode);
        CTP_CMD_COMBINED(priv, CTP_NO_ERR, "WIFI_MODE", "NOTIFY", buf);
    } else {
        if (get_wifi_module_support_mode()) {
            db_update("wfmode", val);
            /* db_flush(); */
            wifi_run_mode = val;
            printf(" cmd_put_wifi_change_mode val : %d \n", val);
            if (val) {
                /*set_hostapd_config_file_5g_mode();*/
            } else {
                /*set_hostapd_config_file_normol_mode();*/
            }
            snprintf(buf, sizeof(buf), "status:%d", wifi_run_mode);
            CTP_CMD_COMBINED(priv, CTP_NO_ERR, "WIFI_MODE", "NOTIFY", buf);
            os_time_dly(50);
            wifi_off();
            os_time_dly(20);
            wifi_on();
        }
    }
    json_object_put(new_obj);
    return 0;
}
static int cmd_get_wifi_supprt_mode(void *priv, char *content)
{
    char buf[64];

    snprintf(buf, sizeof(buf), "status:%d", get_wifi_module_support_mode());
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "WIFI_MODE", "NOTIFY", buf);
    return 0;
}
static int cmd_get_wifi_run_mode(void *priv, char *content)
{
    char buf[64];
    int wfmode =  db_select("wfmode");

    snprintf(buf, sizeof(buf), "status:%d", wfmode > 0 ? wfmode : 0);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "WIFI_RUN_MODE", "NOTIFY", buf);
    return 0;
}
int cmd_take_photo_ctl(char start)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "status:%d", start);
    CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "APP_TAKE_PHOTO", "NOTIFY", buf);
    return 0;
}
static char app_video_rec = -1;
int cmd_video_rec_ctl(char start)
{
    char buf[64];

    start &= 0x1;
    if (start == app_video_rec) {
        return app_video_rec;
    }
    snprintf(buf, sizeof(buf), "status:%d", start);
    CTP_CMD_COMBINED(NULL, CTP_NO_ERR, "APP_VIDEO_REC", "NOTIFY", buf);//请求录像
    app_video_rec = start;

#if 0
    int to = 3000;//5s超时
    app_video_rec = !start;
    while (app_video_rec != start && to > 0) { //等待APP返回录像结果
        os_time_dly(1);
        to -= 10;
    }
    if (!to) {
        return -1;
    }
#else
    os_time_dly(10);
#endif // 0
    return app_video_rec;
}
static int cmd_put_app_video_rec(void *priv, char *content)
{
    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *status = NULL;
    char *val_str;
    char val;
    char buf[64];
    static char wifi_run_mode = 0;
    extern int set_hostapd_config_file_normol_mode(void);
    extern int set_hostapd_config_file_5g_mode(void);

    //分解content字段
    new_obj = json_tokener_parse(content);
    parm = json_object_object_get(new_obj, "param");
    status = json_object_object_get(parm, "status");

    val_str = (char *)json_object_get_string(status);
    val = atoi(val_str);
    app_video_rec = val & 0x1;
    json_object_put(new_obj);
    return 0;
}
static int cmd_get_app_video_rec(void *priv, char *content)
{
#if 0
    char buf[64];
    app_video_rec = 1;
    snprintf(buf, sizeof(buf), "status:%d", 1);
    CTP_CMD_COMBINED(priv, CTP_NO_ERR, "APP_VIDEO_REC", "NOTIFY", buf);//上电不请求APP录像
#endif // 0
    return 0;
}
//添加命令和相应的回调
//前GET后PUT
//
//
/*视频系列命令*/
const struct ctp_map_entry ctp_video_cmd_tab[] SEC_USED(.ctp_video_cmd) = {
    {NULL, "VIDEO_PARAM", cmd_get_video_param, cmd_put_video_param},
    {NULL, "VIDEO_CTRL", cmd_get_video_ctrl, cmd_put_video_ctrl},
    {NULL, "VIDEO_FINISH", NULL, NULL}, //特殊命令，只有回复，用于录像完成
    {NULL, "APP_VIDEO_REC", cmd_get_app_video_rec, cmd_put_app_video_rec},
    {"mic", "VIDEO_MIC", cmd_get_video_mic, cmd_put_video_mic},
    {"mot", "MOVE_CHECK", cmd_get_video_move_check, cmd_put_video_move_check},
    {"dat", "VIDEO_DATE", cmd_get_video_date, cmd_put_video_date},
    {"rtf", "RTF_RES", cmd_get_rt_stream0_res, NULL},
    {"rtb", "RTB_RES", cmd_get_rt_stream1_res, NULL},
#if 0
    {"two", "DOUBLE_VIDEO", cmd_get_double_video, cmd_put_double_video},
    {"cyc", "VIDEO_LOOP", cmd_get_video_loop, cmd_put_video_loop},
    {"wdr", "VIDEO_WDR", cmd_get_video_wdr, cmd_put_video_wdr},
    {"exp", "VIDEO_EXP", cmd_get_video_exp, cmd_put_video_exp},
    {"num", "VIDEO_CAR_NUM", cmd_get_video_car_num, cmd_put_video_car_num},
#endif
    {"gra", "GRA_SEN", cmd_get_gra_sen, cmd_put_gra_sen},
    {"par", "VIDEO_PAR_CAR", cmd_get_video_par_car, cmd_put_video_par_car},
    {"gap", "VIDEO_INV", cmd_get_video_inv, cmd_put_video_inv},
    {NULL, "THUMBNAILS", NULL, cmd_put_thunbails},
    {NULL, "THUMBNAILS_CTRL", NULL, cmd_put_thunbails_ctrl},
    {NULL,  "TIME_AXIS_PLAY", NULL, cmd_put_time_axis_play},
    {NULL,  "TIME_AXIS_PLAY_CTRL", NULL, cmd_put_time_axis_play_ctrl},
    {NULL,  "TIME_AXIS_FAST_PLAY", NULL, cmd_put_time_axis_fast_play},

#if WIFI_RT_STREAM
    {NULL, "OPEN_RT_STREAM", NULL, cmd_put_open_rt_stream},
    {NULL, "CLOSE_RT_STREAM", cmd_get_close_rt_stream, cmd_put_close_rt_stream},
    {NULL, "OPEN_AUDIO_RT_STREAM", NULL, cmd_put_open_audio_rt_stream},
    {NULL, "CLOSE_AUDIO_RT_STREAM", cmd_get_close_audio_rt_stream, cmd_put_close_audio_rt_stream},

    {NULL, "OPEN_PULL_RT_STREAM", NULL, cmd_put_open_pull_rt_stream},
    {NULL, "CLOSE_PULL_RT_STREAM", cmd_get_close_pull_rt_stream, cmd_put_close_pull_rt_stream},
    {NULL, "OPEN_PULL_AUDIO_RT_STREAM", NULL, cmd_put_open_pull_audio_rt_stream},
    {NULL, "CLOSE_PULL_AUDIO_RT_STREAM", cmd_get_close_pull_audio_rt_stream, cmd_put_close_pull_audio_rt_stream},
#endif
    {NULL, "VIDEO_BUMPING", cmd_get_video_bumping, cmd_put_video_bumping},

    {NULL, "PULL_VIDEO_STATUS", cmd_get_pull_video_status, NULL},
    {NULL, "PULL_VIDEO_PARAM", cmd_get_pull_video_param, cmd_put_pull_video_param},
    {NULL, "VIDEO_CYC_SAVEFILE", NULL, cmd_put_video_cyc_savefile},
#ifdef CONFIG_NET_SCR
    {NULL, "NET_SCR", cmd_get_net_scr, cmd_put_net_scr},
#endif
};


/*系统系列命令*/
const struct ctp_map_entry ctp_system_cmd_tab[] SEC_USED(.ctp_system_cmd) = {
    {"kep", "KEEP_ALIVE_INTERVAL", cmd_get_keep_alive_interval, NULL},
    {NULL, "APP_ACCESS", NULL, cmd_put_app_access},
    {"sd", "SD_STATUS", cmd_get_sd_status, NULL},
    {"bat", "BAT_STATUS", cmd_get_bat_status, NULL},
    {"uuid", "UUID", cmd_get_uuid, NULL},
    {"fp", "TF_CAP", cmd_get_sd_size, NULL},
#if 0
    {"bvo", "BOARD_VOICE", cmd_get_board_voice, cmd_put_board_voice},
    {"kvo", "KEY_VOICE", cmd_get_key_voice, cmd_put_key_voice},
    {"fre", "LIGHT_FRE", cmd_get_light_fre, cmd_put_light_fre},
    {"aff", "AUTO_SHUTDOWN", cmd_get_auto_stutdown, cmd_put_auto_stutdown},
    {"pro", "SCREEN_PRO", cmd_get_screen_pro, cmd_put_screen_pro},
    {"tvm", "TV_MODE", cmd_get_tv_mode, cmd_put_tv_mode},
    {"lag", "LANGUAGE", cmd_get_language, cmd_put_language},
#endif
    {NULL, "FORMAT", NULL, cmd_put_format},
    {"def", "SYSTEM_DEFAULT", cmd_get_system_default, cmd_put_system_default},

#if defined CONFIG_ENABLE_VLIST
    {NULL, "FORWARD_MEDIA_FILES_LIST", NULL, cmd_put_make_forward_files_list},
    {NULL, "BEHIND_MEDIA_FILES_LIST", NULL, cmd_put_make_behind_files_list},
#endif
    {NULL, "FILES_DELETE", NULL, cmd_put_files_delete},
    {NULL, "MULTI_COVER_FIGURE", NULL, cmd_put_multi_cover_figure},
    {NULL, "DATE_TIME", cmd_get_date_time, cmd_put_date_time},
    {NULL, "AP_SSID_INFO", cmd_get_ap_ssid_info, cmd_put_ap_ssid_info},
    {NULL, "STA_SSID_INFO", cmd_get_sta_ssid_info, cmd_put_sta_ssid_info},
    {NULL, "RESET", NULL, cmd_put_soft_reset},
    {NULL, "CTP_CLI_DISCONNECT", NULL, cmd_put_ctp_cli_disconnect},

    {NULL, "CTP_CLI_CONNECTED", NULL, cmd_put_ctp_cli_connected},
    {NULL, "GEN_CHK", cmd_get_gen_chk, NULL},
#if 1
    {NULL, "RT_TALK_CTL", cmd_get_rt_talk_ctl, cmd_put_rt_talk_ctl},
    {NULL, "VOICE_TALK_CTL", cmd_get_voice_talk_ctl, cmd_put_voice_talk_ctl},
#endif

    {NULL, "FILE_LOCK", NULL, cmd_put_file_lock},
    {NULL, "GENERIC_CMD", cmd_get_generic_cmd, cmd_put_generic_cmd},

    /**
       CDP控制命令,NULL类型的命令需要自己回复，系统级的命令就不管,已经做好了处理
    */
    {NULL, "WIND_VELOCITY", cmd_get_wind_velocity, NULL}, //获取风速等级
    {NULL, "DEVICE_DIRECTION_CONTROL", NULL, cmd_put_device_direction}, //控制设备飞行方向

    /*
     *  2.4G 和 5G切换
     */
    {NULL, "WIFI_MODE", cmd_get_wifi_supprt_mode, cmd_put_wifi_change_mode},
    {NULL, "WIFI_RUN_MODE", cmd_get_wifi_run_mode, NULL},

};


/*照片系列命令*/
const struct ctp_map_entry ctp_photo_cmd_tab[] SEC_USED(.ctp_photo_cmd) = {
    {NULL, "PHOTO_RESO", cmd_get_photo_reso, cmd_put_photo_reso},
    {"qua", "PHOTO_QUALITY", cmd_get_photo_quality, cmd_put_photo_quality},
    {NULL, "PHOTO_CTRL", NULL, cmd_put_photo_ctrl},
#if 0
    {"phm", "SELF_TIMER", cmd_get_self_timer, cmd_put_self_timer},
    {"cyt", "BURST_SHOT", cmd_get_burst_shot, cmd_put_burst_shot},
    {"acu", "PHOTO_SHARPNESS", cmd_get_photo_sharpness, cmd_put_photo_sharpness},
    {"wbl", "WHITE_BALANCE", cmd_get_white_balance, cmd_put_white_balance},
    {"iso", "PHOTO_ISO", cmd_get_photo_iso, cmd_put_photo_iso},
    {"pexp", "PHOTO_EXP", cmd_get_photo_exp, cmd_put_photo_exp},
    {"sok", "ANTI_TREMOR", cmd_get_anti_tremor, cmd_put_anti_tremor},
    {"pdat", "PHOTO_DATE", cmd_get_photo_date, cmd_put_photo_date},
    {"sca", "FAST_SCA", cmd_get_fast_sca, cmd_put_fast_sca },
    {"col", "PHOTO_COLOR", cmd_get_photo_color, cmd_put_photo_color},
#endif
};


int user_open_rt_stream(void)
{
    char buf[128];
    u8 mark;
    const char *h, *w, *format, *fps;
    //切换UI，禁止key
    /* sys_key_event_disable(); */
    net_switch_ui("video_rec");
    struct intent it;
    init_intent(&it);
    it.name = "net_video_rec";
//设置参数
    it.action = ACTION_VIDEO0_OPEN_RT_STREAM;

    mark = 2;
    struct rt_stream_app_info info;

    info.width = 640;
    info.height = 480;

    if (info.width == 1920) {
        db_update("rtf", VIDEO_RES_1080P);
    } else if (info.width == 1280) {
        db_update("rtf", VIDEO_RES_720P);
    } else if (info.width == 640) {
        db_update("rtf", VIDEO_RES_VGA);
    }
    /* db_flush(); */

    info.fps    = NET_VIDEO_REC_FPS0;//NET_VIDEO_REC_FPS1
    info.type = NET_VIDEO_FMT_AVI;
    info.priv = 0;

    it.data = (const char *)&mark;//打开视频
    it.exdata = (u32) &info; //视频参数

    start_app(&it);

    return 0;

}

int user_close_rt_stream(void)
{
    u8 mark;
    const char *status = NULL;
    struct intent it;

    init_intent(&it);
    net_switch_ui("video_rec");
    it.name = "net_video_rec";
//设置参数
    it.action = ACTION_VIDEO0_CLOSE_RT_STREAM;

    mark = 2;
    it.data = (const char *)&mark; //close video param
    start_app(&it);

    return 0;
}

