#include "system/timer.h"
#include "json_c/json.h"
#include "json_c/json_tokener.h"
#include "dui.h"
#ifdef DUI_MEMHEAP_DEBUG
#include "mem_leak_test.h"
#endif

struct list_head alarm_list_head = LIST_HEAD_INIT(alarm_list_head);
static OS_MUTEX alarm_list_op_mutex;


typedef struct  {
    struct list_head entry;
    u32 recent_tsp;
    char vid[64]; //默认128位的vid
    char object[34];//默认允许11个字符
    char object_type;//用来判断提醒类型，避免在中断中频繁比较
} ALARM_LIST_INFO;

//汉字编码的utf-8编码
static const char richeng[] = {0xE6, 0x97, 0xA5, 0xE7, 0xA8, 0x8B, 0x00};
static const char naozhong[] = {0xE9, 0x97, 0xB9, 0xE9, 0x92, 0x9F, 0x00};
static const char caozuo[] = {0xE5, 0xAF, 0xB9, 0xE8, 0xB1, 0xA1, 0x00};
static int alarm_notify_flag;

void set_alarm_notify_flag(u8 value)
{
    alarm_notify_flag = value;
}

void dui_alarm_callback(void *priv)
{
    ALARM_LIST_INFO *pos, *n;
    char *vid = NULL;
    char  object[64];
    int vid_size = 512;
    u8 find_alarm = 0;
    u8 alarm_object_type = 0;
    u32 len = 0;
    time_t t;
    t = time(NULL);
    os_mutex_pend(&alarm_list_op_mutex, 0);
    list_for_each_entry_safe(pos, n, &alarm_list_head, entry) {

        if (t <  pos->recent_tsp) {
            break;
        }

        if (!vid) {
            vid = calloc(1, vid_size);
            if (!vid) {
                printf("\n%s %d vid calloc fail\n", __func__, __LINE__);
                break;
            }
        }
        if (find_alarm) {
            if (alarm_object_type == SCHEDULE_TYPE && pos->object_type == SCHEDULE_TYPE) {
                if ((len + 1 + strlen(pos->vid) > 512)) {
                    vid_size += 512;
                    char *old_vid = vid;
                    vid = realloc(old_vid, vid_size);
                    if (!vid) {
                        printf("\n%s %d vid calloc fail\n", __func__, __LINE__);
                        vid = old_vid;
                        break;
                    }
                }
                strcat(vid, ",");
                strcat(vid, pos->vid);
                len += 1 + strlen(pos->vid);
            } else if (alarm_object_type == ALARM_CLOCK_TYPE && pos->object_type == SCHEDULE_TYPE) {
                memset(vid, 0, len);
                len = 0;
                strcpy(vid, pos->vid);
                strcpy(object, pos->object);
                len += strlen(pos->vid);
                alarm_object_type = pos->object_type;
            }
        } else {
            find_alarm = 1;
            strcpy(vid, pos->vid);
            strcpy(object, pos->object);
            len += strlen(pos->vid);
            alarm_object_type = pos->object_type;
        }
        list_del(&pos->entry);
        free(pos);
    }
    os_mutex_post(&alarm_list_op_mutex);

    if (find_alarm) {

        find_alarm = 0;
        //有网络的情况先发消息给这个任务
        extern int lwip_dhcp_bound(void);
        if (lwip_dhcp_bound()) {
            char *slots = malloc(strlen(vid) + strlen(object) + 64);
            ASSERT(slots != NULL, "\n malloc fail\n");
            sprintf(slots, "{\"vid\":\"%s\",\"%s\":\"%s\"}", vid, caozuo, object);
            DUI_OS_TASKQ_POST("dui_app_task", 4, DUI_RECORD_BREAK, DUI_ALARM_RING, slots, alarm_object_type);
        } else {
            //没有网络的情况直接播提示音
            char *name = NULL;
            if (alarm_object_type == ALARM_CLOCK_TYPE) {
                name = "reminder.mp3";
            } else {
                name = "schedule.mp3";
            }
            extern void play_voice_prompt_for_app(char *name);
            play_voice_prompt_for_app(name);
        }
    }

    if (vid) {
        free(vid);
    }
}

int dui_alarm_init(void)
{
    if (!os_mutex_valid(&alarm_list_op_mutex)) {
        os_mutex_create(&alarm_list_op_mutex);
        //可以设置一个定时器，进入中断就读取rtc,或者获取网络时间，判断闹钟是否到时
        sys_timer_add_to_task("sys_timer", NULL, dui_alarm_callback, 100);
    }
    set_alarm_notify_flag(1);
    return 0;
}

int dui_alarm_sync(const char *extra)
{
    ALARM_LIST_INFO *info, *pos, *n;
    json_object *root_node = json_tokener_parse(extra);
    json_object *first_node, *second_node, *third_node;
    json_object *array  = NULL;
    json_object *node  = NULL;
    int ret = 0;
    if (!root_node) {
        printf("\n %s %d\n", __func__, __LINE__);
        ret =  -1;
        goto exit;
    }
    os_mutex_pend(&alarm_list_op_mutex, 0);
    list_for_each_entry_safe(pos, n, &alarm_list_head, entry) {
        list_del(&pos->entry);
        free(pos);
    }
    os_mutex_post(&alarm_list_op_mutex);
    if (json_object_object_get_ex(root_node, "content", &first_node)) {
        for (int i = 0; i < json_object_array_length(first_node) && i < 10; i++) {
            second_node = json_object_array_get_idx(first_node, i);
            info = calloc(1, sizeof(ALARM_LIST_INFO));
            if (!info) {
                printf("\n %s  %d info calloc fail\n", __func__, __LINE__);
                ret =  -1;
                goto exit;
            }
            array = json_object_new_array();
            node = json_object_new_object();
            if (json_object_object_get_ex(second_node, "operation", &third_node)) {
                json_object_object_add(node, "operation", json_object_new_string(json_object_get_string(third_node)));
            }

            if (json_object_object_get_ex(second_node, "event", &third_node)) {
                json_object_object_add(node, "event", json_object_new_string(json_object_get_string(third_node)));
            }

            if (json_object_object_get_ex(second_node, "date", &third_node)) {
                json_object_object_add(node, "date", json_object_new_string(json_object_get_string(third_node)));
            }

            if (json_object_object_get_ex(second_node, "time", &third_node)) {
                json_object_object_add(node, "time", json_object_new_string(json_object_get_string(third_node)));
            }

            if (json_object_object_get_ex(second_node, "timestamp", &third_node)) {
                json_object_object_add(node, "timestamp", json_object_new_string(json_object_get_string(third_node)));
            }

            if (json_object_object_get_ex(second_node, "recent_tsp", &third_node)) {
                /* printf("\n recent_tsp = %s\n",json_object_get_string(third_node)); */
                info->recent_tsp = atoi(json_object_get_string(third_node));
                json_object_object_add(node, "recent_tsp", json_object_new_string(json_object_get_string(third_node)));
            }
            if (json_object_object_get_ex(second_node, "vid", &third_node)) {
                /* printf("\n vid = %s\n",json_object_get_string(third_node)); */
                strcpy(info->vid, json_object_get_string(third_node));
                json_object_object_add(node, "vid", json_object_new_string(json_object_get_string(third_node)));
            }
            if (json_object_object_get_ex(second_node, "object", &third_node)) { //闹钟 日程
                json_object_object_add(node, "object", json_object_new_string(json_object_get_string(third_node)));
                /* put_buf(json_object_get_string(third_node),strlen(json_object_get_string(third_node))); */
                if (!strcmp(richeng, json_object_get_string(third_node))) {
                    info->object_type = SCHEDULE_TYPE;
                    strcpy(info->object, json_object_get_string(third_node));
                } else if (!strcmp(naozhong, json_object_get_string(third_node))) {
                    info->object_type = ALARM_CLOCK_TYPE;
                    strcpy(info->object, json_object_get_string(third_node));
                }
            }
            if (json_object_object_get_ex(second_node, "period", &third_node)) {
                json_object_object_add(node, "period", json_object_new_string(json_object_get_string(third_node)));
                /* put_buf(json_object_get_string(third_node),strlen(json_object_get_string(third_node)));//上午，中午，下午 */
            }
            os_mutex_pend(&alarm_list_op_mutex, 0);
            list_add_tail(&info->entry, &alarm_list_head);
            os_mutex_post(&alarm_list_op_mutex);
            json_object_array_add(array, node);
            if (alarm_notify_flag) {
                iot_add_dui_remind(json_object_get_string(array));
            }
            json_object_put(array);
        }
    }
exit:
    alarm_notify_flag = 0;
    json_object_put(root_node);
    return ret;
}

int dui_alarm_add(const char *extra)
{
    json_object *root_node = json_tokener_parse(extra);
    json_object *first_node, *second_node, *third_node;
    json_object *array  = NULL;
    json_object *node  = NULL;
    int ret = 0;
    if (!root_node) {
        printf("\n %s %d\n", __func__, __LINE__);
        ret =  -1;
        goto exit;
    }

    if (json_object_object_get_ex(root_node, "content", &first_node)) {
        for (int i = 0; i < json_object_array_length(first_node) && i < 10; i++) {
            second_node = json_object_array_get_idx(first_node, i);
            array = json_object_new_array();
            node = json_object_new_object();
            if (json_object_object_get_ex(second_node, "operation", &third_node)) {
                json_object_object_add(node, "operation", json_object_new_string(json_object_get_string(third_node)));
            }

            if (json_object_object_get_ex(second_node, "event", &third_node)) {
                json_object_object_add(node, "event", json_object_new_string(json_object_get_string(third_node)));
            }
            if (json_object_object_get_ex(second_node, "date", &third_node)) {
                json_object_object_add(node, "date", json_object_new_string(json_object_get_string(third_node)));
            }

            if (json_object_object_get_ex(second_node, "time", &third_node)) {
                json_object_object_add(node, "time", json_object_new_string(json_object_get_string(third_node)));
            }

            if (json_object_object_get_ex(second_node, "timestamp", &third_node)) {
                json_object_object_add(node, "timestamp", json_object_new_string(json_object_get_string(third_node)));
            }

            if (json_object_object_get_ex(second_node, "recent_tsp", &third_node)) {
                /* printf("\n recent_tsp = %s\n",json_object_get_string(third_node)); */
                json_object_object_add(node, "recent_tsp", json_object_new_string(json_object_get_string(third_node)));
            }
            if (json_object_object_get_ex(second_node, "vid", &third_node)) {
                /* printf("\n vid = %s\n",json_object_get_string(third_node)); */
                json_object_object_add(node, "vid", json_object_new_string(json_object_get_string(third_node)));
            }
            if (json_object_object_get_ex(second_node, "object", &third_node)) { //闹钟 日程
                json_object_object_add(node, "object", json_object_new_string(json_object_get_string(third_node)));
                /* put_buf(json_object_get_string(third_node),strlen(json_object_get_string(third_node))); */
            }
            if (json_object_object_get_ex(second_node, "period", &third_node)) {
                json_object_object_add(node, "period", json_object_new_string(json_object_get_string(third_node)));
                /* put_buf(json_object_get_string(third_node),strlen(json_object_get_string(third_node)));//上午，中午，下午 */
            }
            json_object_array_add(array, node);
            iot_add_dui_remind(json_object_get_string(array));
            json_object_put(array);
        }
    }

exit:
    json_object_put(root_node);
    return ret;

}

int dui_alarm_del(const char *extra)
{
    printf("\n extera = %s\n", extra);
    json_object *root_node = json_tokener_parse(extra);
    json_object *first_node, *second_node, *third_node;
    json_object *array  = NULL;
    json_object *node  = NULL;
    int ret = 0;
    if (!root_node) {
        printf("\n %s %d\n", __func__, __LINE__);
        ret =  -1;
        goto exit;
    }

    if (json_object_object_get_ex(root_node, "content", &first_node)) {
        for (int i = 0; i < json_object_array_length(first_node) && i < 10; i++) {
            second_node = json_object_array_get_idx(first_node, i);
            array = json_object_new_array();
            node = json_object_new_object();
            if (json_object_object_get_ex(second_node, "operation", &third_node)) {
                json_object_object_add(node, "operation", json_object_new_string(json_object_get_string(third_node)));
            }

            if (json_object_object_get_ex(second_node, "event", &third_node)) {
                json_object_object_add(node, "event", json_object_new_string(json_object_get_string(third_node)));
            }

            if (json_object_object_get_ex(second_node, "date", &third_node)) {
                json_object_object_add(node, "date", json_object_new_string(json_object_get_string(third_node)));
            }

            if (json_object_object_get_ex(second_node, "time", &third_node)) {
                json_object_object_add(node, "time", json_object_new_string(json_object_get_string(third_node)));
            }

            if (json_object_object_get_ex(second_node, "timestamp", &third_node)) {
                json_object_object_add(node, "timestamp", json_object_new_string(json_object_get_string(third_node)));
            }

            if (json_object_object_get_ex(second_node, "recent_tsp", &third_node)) {
                /* printf("\n recent_tsp = %s\n",json_object_get_string(third_node)); */
                json_object_object_add(node, "recent_tsp", json_object_new_string(json_object_get_string(third_node)));
            }

            if (json_object_object_get_ex(second_node, "vid", &third_node)) {
                /* printf("\n vid = %s\n",json_object_get_string(third_node)); */
                json_object_object_add(node, "vid", json_object_new_string(json_object_get_string(third_node)));
            }

            if (json_object_object_get_ex(second_node, "object", &third_node)) { //闹钟 日程
                json_object_object_add(node, "object", json_object_new_string(json_object_get_string(third_node)));
                /* put_buf(json_object_get_string(third_node),strlen(json_object_get_string(third_node))); */
            }

            if (json_object_object_get_ex(second_node, "period", &third_node)) {
                json_object_object_add(node, "period", json_object_new_string(json_object_get_string(third_node)));
                /* put_buf(json_object_get_string(third_node),strlen(json_object_get_string(third_node)));//上午，中午，下午 */
            }

            json_object_array_add(array, node);
            iot_del_dui_remind(json_object_get_string(array));
            json_object_put(array);
        }
    }

exit:
    json_object_put(root_node);
    return 0;
}

int dui_alarm_query(void)
{
    int ret;
    os_mutex_pend(&alarm_list_op_mutex, 0);
    ret = list_empty(&alarm_list_head);
    os_mutex_post(&alarm_list_op_mutex);
    return ret;
}
