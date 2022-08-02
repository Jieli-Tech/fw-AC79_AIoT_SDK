#include <string.h>
#include "cJSON_common/cJSON.h"
#include "iot.h"
#include "dui.h"
#include "system/spinlock.h"
#ifdef DUI_MEMHEAP_DEBUG
#include "mem_leak_test.h"
#endif

static DEFINE_SPINLOCK(lock);
static media_item_t *g_media_item = NULL;

static void free_media_item(media_item_t *item)
{
    if (item) {
        if (item->linkUrl) {
            free(item->linkUrl);
        }
        if (item->imageUrl) {
            free(item->imageUrl);
        }
        if (item->title) {
            free(item->title);
        }
        if (item->album) {
            free(item->album);
        }
        if (item->subTitle) {
            free(item->subTitle);
        }
        free(item);
    }
}

int dui_media_music_item(media_item_t *item)
{
    if (!g_media_item) {
        return -1;
    }

    spin_lock(&lock);
    memcpy(item, g_media_item, sizeof(media_item_t));
    spin_unlock(&lock);

    return 0;
}

int dui_iot_music_play(const char *url, media_item_t *media)
{
    spin_lock(&lock);
    if (g_media_item) {
        free_media_item(g_media_item);
        g_media_item = media;
    }
    spin_unlock(&lock);

    dui_iot_media_audio_play(url);

    return 0;
}

void dui_free_media_item(void)
{
    free_media_item(g_media_item);
    g_media_item = NULL;
}

