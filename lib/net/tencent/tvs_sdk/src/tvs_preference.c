
#define TVS_LOG_DEBUG   1
#define TVS_LOG_DEBUG_MODULE  "PREF"
#include "tvs_log.h"

#include "tvs_jsons.h"
#include "tvs_preference.h"
#include "os_wrapper.h"
#include "tvs_preference_interface.h"
#include "tvs_threads.h"
#include "syscfg/syscfg_id.h"

cJSON *g_tvs_preference_root = NULL;

TVS_LOCKER_DEFINE

void tvs_preference_lock()
{
    do_lock();
}

void tvs_preference_unlock()
{
    do_unlock();
}

int tvs_preference_module_init()
{
    TVS_LOCKER_INIT

    return 0;
}

int tvs_preference_init()
{

    if (g_tvs_preference_root == NULL) {
        int size = 0;
        const char *json = tvs_preference_load_data(&size);
        TVS_LOG_PRINTF("load preference: %.*s\n", size, json);
        g_tvs_preference_root = cJSON_Parse(json);
        free((void *)json);		//yii:释放perference字符串
    }

    if (g_tvs_preference_root == NULL) {
        g_tvs_preference_root = cJSON_CreateObject();
        TVS_LOG_PRINTF("create default\n");
    }

    return 0;
}

int tvs_preference_save()
{
    if (g_tvs_preference_root == NULL) {
        TVS_LOG_PRINTF("invalid json root\n");
        return -1;
    }

    const char *json = cJSON_PrintUnformatted(g_tvs_preference_root);

    if (NULL == json) {
        TVS_LOG_PRINTF("format json error\n");
        return -1;
    }

    int ret = tvs_preference_save_data(json, strlen(json) + 1);

    TVS_LOG_PRINTF("save preference: %s, ret %d\n", json, ret);

    if (json != NULL) {
        TVS_FREE((char *)json);
    }
    return ret;
}


int tvs_preference_get_number_value(const char *name, int *value, int default_value)
{
    *value = default_value;

    if (!strcmp(name, TVS_PREFERENCE_VOLUME)) {
        syscfg_read(CFG_MUSIC_VOL, value, sizeof(*value));
        printf("-------%s-------%d---------vol = %d\r\n", __func__, __LINE__, *value);
    } else {
        do_lock();
        tvs_preference_init();

        if (g_tvs_preference_root == NULL || name == NULL || value == NULL) {
            TVS_LOG_PRINTF("invalid param\n");
            do_unlock();
            return -1;
        }

        cJSON *node = cJSON_GetObjectItem(g_tvs_preference_root, name);

        if (node != NULL) {
            *value = node->valueint;
            TVS_LOG_PRINTF("%s - %d\n", name, *value);
        } else {
            *value = default_value;
            TVS_LOG_PRINTF("%s - default %d\n", name, *value);
        }

        do_unlock();
    }

    return 0;
}

const char *tvs_preference_get_string_value(const char *name)
{
    do_lock();

    tvs_preference_init();

    if (g_tvs_preference_root == NULL || name == NULL) {
        do_unlock();
        return NULL;
    }

    cJSON *node = cJSON_GetObjectItem(g_tvs_preference_root, name);

    if (node != NULL) {
        do_unlock();
        return node->valuestring;
    }

    do_unlock();
    return NULL;
}

int tvs_preference_set_string_value(const char *name, const char *value)
{
    do_lock();

    tvs_preference_init();

    if (g_tvs_preference_root == NULL || name == NULL) {
        do_unlock();
        return -1;
    }

    TVS_LOG_PRINTF("%s - %s\n", name, value);

    cJSON *target = cJSON_GetObjectItem(g_tvs_preference_root, name);
    value = (value == NULL ? "" : value);

    if (target != NULL) {
        if (strcmp(value, target->valuestring) == 0) {
            TVS_LOG_PRINTF("the same\n");
            do_unlock();
            return 0;
        }

        cJSON_ReplaceItemInObject(g_tvs_preference_root, name, cJSON_CreateString(value));
    } else {
        cJSON_AddItemToObject(g_tvs_preference_root, name, cJSON_CreateString(value));
    }

    int ret = tvs_preference_save();
    do_unlock();
    return ret;
}

int tvs_preference_set_number_value(const char *name, int value)
{
    do_lock();

    tvs_preference_init();

    if (g_tvs_preference_root == NULL || name == NULL) {
        do_unlock();
        return -1;
    }

    TVS_LOG_PRINTF("%s - %d\n", name, value);
    cJSON *target = cJSON_GetObjectItem(g_tvs_preference_root, name);

    if (target != NULL) {
        if (target->valueint == value) {
            TVS_LOG_PRINTF("the same\n");
            do_unlock();
            return 1;
        }

        cJSON_ReplaceItemInObject(g_tvs_preference_root, name, cJSON_CreateNumber(value));
    } else {
        cJSON_AddItemToObject(g_tvs_preference_root, name, cJSON_CreateNumber(value));
    }

    int ret = tvs_preference_save();
    do_unlock();
    return ret;
}

int tvs_preference_set_int_array_value(const char *name, const int *numbers, int count)
{
    do_lock();

    tvs_preference_init();

    if (g_tvs_preference_root == NULL || name == NULL) {
        do_unlock();
        return -1;
    }

    cJSON *target = cJSON_GetObjectItem(g_tvs_preference_root, name);

    if (target != NULL) {
        cJSON_ReplaceItemInObject(g_tvs_preference_root, name, cJSON_CreateIntArray(numbers, count));
    } else {
        cJSON_AddItemToObject(g_tvs_preference_root, name, cJSON_CreateIntArray(numbers, count));
    }

    int ret = tvs_preference_save();
    do_unlock();
    return ret;
}

int tvs_preference_get_int_array_value(const char *name, int **array)
{
    do_lock();

    tvs_preference_init();

    if (g_tvs_preference_root == NULL || array == NULL) {
        do_unlock();
        return 0;
    }

    cJSON *int_arr = cJSON_GetObjectItem(g_tvs_preference_root, name);
    if (NULL != int_arr) {
        int size = cJSON_GetArraySize(int_arr);

        if (size > 0) {
            int *data = TVS_MALLOC(sizeof(int) * size);
            for (int i = 0; i < size; i++) {
                cJSON *child = cJSON_GetArrayItem(int_arr, i);

                data[i] = child->valueint;
            }

            *array = data;
        }
        do_unlock();
        return size;
    }
    do_unlock();
    return 0;
}

cJSON *tvs_preference_get_node(const char *name)
{
    tvs_preference_init();

    if (g_tvs_preference_root == NULL || name == NULL) {
        return NULL;
    }

    return cJSON_GetObjectItem(g_tvs_preference_root, name);
}

