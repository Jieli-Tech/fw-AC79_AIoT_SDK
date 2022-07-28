#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tvs_api/tvs_api.h"
#include "tvs_platform_impl.h"

#include "tvs_api_config.h"
#include "tvs_audio_provider.h"
//添加头文件
#include "server/audio_server.h"
#include "server/ai_server.h"
#include "server/server_core.h"
#include "os/os_api.h"
#include "asm/crc16.h"
#include "asm/sfc_norflash_api.h"
#include "tvs_api_config.h"

// 设备最小音量
#define MIN_VOLUME  0

// 设备最大音量
#define MAX_VOLUME  100

extern const struct ai_sdk_api tc_tvs_api;

/***************************录音*************************/

static struct {
    struct server *enc_server;  //编码server
    const char *sample_source;
    u8 rec_state;               //收到数据标志位
    u8 channel_bit_map;
} record_hdl;

enum {
    RECORDER_START,
    RECORDER_STOP,
};


//功  能：MIC驱动底层将调用该函数，将录音数据写进指定的文件中
//参  数：void *file：文件描述符
//        void *data：data指针
//        u32 len   : data大小
//返回值：写入字节数
static int record_vfs_fwrite(void *file, void *data, u32 len)
{
    ((u8 *)data)[0] = len - 1;
    return tvs_audio_provider_write((char *)data, len);
}


static int record_vfs_fclose(void *file)
{
    return 0;
}

//录音文件操作方法集。传递给MIC底层驱动调用
static const struct audio_vfs_ops record_vfs_ops = {
    .fwrite = record_vfs_fwrite,
    .fclose = record_vfs_fclose,
};

//功能：底层驱动回调函数
static void enc_server_event_handler(void *priv, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_SERVER_EVENT_ERR:
        printf("record : AUDIO_SERVER_EVENT_ERR\n");
        tvs_api_stop_recognize();
        break;
    case AUDIO_SERVER_EVENT_END:
        printf("record : AUDIO_SERVER_EVENT_END\n");
        tvs_api_stop_recognize();
        break;
    case AUDIO_SERVER_EVENT_SPEAK_START:
        printf("-------------speak start!!\n");
        break;
    case AUDIO_SERVER_EVENT_SPEAK_STOP:
        printf("-------------speak stop!!\n");
        tvs_api_stop_recognize();
        break;
    default:
        break;
    }
}

static int mic_record_start(int sample_rate, int channel)
{
    int err = -1;
    union audio_req req = {0};
    OS_SEM mic_sem;

    if (record_hdl.enc_server) {
        return err;
    }

    //打开enc_server
    record_hdl.enc_server = server_open("audio_server", "enc");
    if (!record_hdl.enc_server) {
        return err;
    }
    server_register_event_handler_to_task(record_hdl.enc_server, NULL, enc_server_event_handler, "tc_tvs_task");

    //mic数据采集配置
    req.enc.cmd				=	AUDIO_ENC_OPEN;	//命令
    req.enc.channel			=	channel;		//采样通道数目
    req.enc.volume			=	100;			//录音音量
    req.enc.output_buf		=	NULL;			//缓存buf
    req.enc.output_buf_len	=	2 * 1024;		//缓存buf大小
    req.enc.sample_rate		=	sample_rate;	//采样率
    req.enc.format			=	"speex";			//录音数据格式
    req.enc.sample_source 	=   record_hdl.sample_source ? record_hdl.sample_source : "mic";	//采样源
    req.enc.channel_bit_map	=   record_hdl.channel_bit_map;
    req.enc.vfs_ops			=	&record_vfs_ops;//文件操作方法集
    req.enc.msec			=	15 * 1000;				//采样时间
    req.enc.frame_head_reserve_len = 1;
    //req.enc.use_vad			=	1;
#if 1
    os_sem_create(&mic_sem, 0);
    ai_server_event_notify(&tc_tvs_api, &mic_sem, AI_SERVER_EVENT_MIC_OPEN);
    os_sem_pend(&mic_sem, 0);
    /* os_sem_del(&mic_sem, 1); */
#endif
    record_hdl.rec_state = RECORDER_START;
    //发送录音请求
    err = server_request(record_hdl.enc_server, AUDIO_REQ_ENC, &req);
    if (err) {
        goto mic_err;
    }
    return 0;

mic_err:
    if (record_hdl.enc_server) {
        server_close(record_hdl.enc_server);
        record_hdl.enc_server = NULL;
    }

    return -1;

}

//功  能：关闭MIC数据采样
//参  数：无
//返回值：无
static void mic_record_stop(void)
{
    union audio_req req = {0};

    record_hdl.rec_state = RECORDER_STOP;

    //关闭enc_server
    if (record_hdl.enc_server) {
        req.enc.cmd = AUDIO_ENC_CLOSE;
        server_request(record_hdl.enc_server, AUDIO_REQ_ENC, &req);
        ai_server_event_notify(&tc_tvs_api, NULL, AI_SERVER_EVENT_MIC_CLOSE);
        server_close(record_hdl.enc_server);
        record_hdl.enc_server = NULL;
    }
}

void set_tvs_rec_sample_source(const char *sample_source)
{
    record_hdl.sample_source = sample_source;
}

void set_tvs_rec_channel_bit_map(u8 channel_bit_map)
{
    record_hdl.channel_bit_map = channel_bit_map;
}

int tvs_media_recorder_impl_open(int bitrate, int channel)
{
    //打开声卡，准备录制，用于智能语音对话阶段
    return mic_record_start(bitrate, channel);
}

void tvs_media_recorder_impl_close()
{
    //录制结束，关闭声卡，用于智能语音对话阶段
    mic_record_stop();
}

static int tvs_media_recorder_impl_read(void *buffer, unsigned int buffer_len)
{
    //录制PCM，填充buffer，用于智能语音对话阶段
    return 0;
}

/*********************************播放PCM格式音频****************************/

static int tvs_soundcard_player_open(int bitrate, int channel)
{
    //TODO 打开声卡，准备播放PCM，用于TTS播放阶段

    TVS_ADAPTER_PRINTF("*******start playing tts, bitrate %d, channel %d*******\n", bitrate, channel);

    return 0;
}

static void tvs_soundcard_player_close()
{
    //TODO 播放PCM结束，关闭声卡，用于TTS播放阶段

    TVS_ADAPTER_PRINTF("*******stop playing tts*******\n");
}

static int tvs_soundcard_player_write(void *data, unsigned int data_bytes)
{
    //TODO 播放PCM，用于TTS播放阶段

    //TVS_ADAPTER_PRINTF("*******playing, data size %d*******\n", data_bytes);
    return data_bytes;
}

static void set_device_volume(int volume)
{
    //设置系统音量
    ai_server_event_notify(&tc_tvs_api, (void *)volume, AI_SERVER_EVENT_VOLUME_CHANGE);
    TVS_ADAPTER_PRINTF("*******set device volume %d*******\n", volume);
}

static int tvs_platform_impl_soundcard_control(bool open, bool recorder, tvs_soundcard_config *sc_config)
{
    if (!recorder) {
        if (!open) {
            tvs_soundcard_player_close();
            return 0;
        } else {
            int bitrate = sc_config == NULL ? 16000 : sc_config->bitrate;
            int channel = sc_config == NULL ? 1 : sc_config->channel;
            return tvs_soundcard_player_open(bitrate, channel);
        }
    } else {
        if (open) {
            int bitrate = sc_config == NULL ? 16000 : sc_config->bitrate;
            int channel = sc_config == NULL ? 1 : sc_config->channel;
            return tvs_media_recorder_impl_open(bitrate, channel);
        } else {
            tvs_media_recorder_impl_close();
            return 0;
        }
    }
}

int tvs_platform_impl_set_current_cloud_volume(int cloud_volume, int cloud_max_value, bool do_init)
{
    if (cloud_volume > cloud_max_value) {
        cloud_volume = cloud_max_value;
    }

    int device_volume = ((MAX_VOLUME - MIN_VOLUME) * cloud_volume / cloud_max_value) + MIN_VOLUME;
    set_device_volume(device_volume);

    /* tvs_platform_adapter_on_volume_changed(cloud_volume); */

    return 0;
}


const char *tvs_platform_impl_load_preference(int *preference_size)
{
    //将保存的preference加载出来，需要通过malloc申请内存，SDK在回调此函数后，会调用free来执行清除操作
    printf("%s", __func__);

    u32 addr;
    char *preference_buffer = NULL;
    *preference_size = 0;
    u8 *preference_data = (u8 *)calloc(8, 1024);
    if (!preference_data) {
        return NULL;
    }

    FILE *profile_fp = fopen(TENCENT_PATH, "r");
    if (profile_fp == NULL) {
        goto exit;
    }
    struct vfs_attr file_attr;
    fget_attrs(profile_fp, &file_attr);
    addr = sdfile_cpu_addr2flash_addr(file_attr.sclust);
    fclose(profile_fp);

    norflash_read(NULL, preference_data, 8 * 1024, addr);

    u16 *preference_crc = (u16 *)&preference_data[0];
    u16 *preference_len = (u16 *)&preference_data[2];
    if (*preference_len > 8 * 1024) {
        printf("-------------%s-----------------%d  no data , preference_len = %d \n", __func__, __LINE__, *preference_len);
        goto exit;
    }
    //yii:CRC校验比较
    if (*preference_crc != CRC16((const void *)&preference_data[4], (u32)*preference_len)) {
        printf("----------%s--------%d-------CRC16 failed", __func__, __LINE__);
        goto exit;
    }
    preference_buffer = (char *)calloc(1, (u32) * preference_len);
    if (preference_buffer) {
        *preference_size = *preference_len;
        memcpy(preference_buffer, &preference_data[4], *preference_size);
    }

exit:
    if (preference_data) {
        free(preference_data);
    }

    return preference_buffer;
}

int tvs_platform_impl_save_preference(const char *preference_buffer, int preference_size)
{
    //保存preference，preference_buffer为一个json字符串
    printf("%s", __func__);

    u32 addr = 0, erase_addr;
    u32 i = 0, erase_block = 0;
    int total_len = preference_size + 4;

    u8 *preference_data = (u8 *)calloc(8, 1024);
    if (!preference_data) {
        return -1;
    }

    FILE *profile_fp = fopen(TENCENT_PATH, "r");
    if (profile_fp == NULL) {
        free(preference_data);
        return -1;
    }
    struct vfs_attr file_attr;
    fget_attrs(profile_fp, &file_attr);
    //preference 8k | (account_info  1k  psk_info 1k) |
    addr = sdfile_cpu_addr2flash_addr(file_attr.sclust);
    fclose(profile_fp);

    //擦除flash
    if (total_len % (4 * 1024)) {
        erase_block = (total_len / (4 * 1024)) + 1;
    } else {
        erase_block = (total_len / (4 * 1024));
    }

    for (i = 1, erase_addr = addr; i <= erase_block; i++, erase_addr += 4 * 1024) {
        norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, erase_addr);
    }

    u16 *preference_crc = (u16 *)&preference_data[0];
    u16 *preference_len = (u16 *)&preference_data[2];
    memcpy(&preference_data[4], preference_buffer, preference_size);

    *preference_len = (u16)preference_size;
    *preference_crc = CRC16((const void *)preference_buffer, preference_size);
    //写flash
    norflash_write(NULL, preference_data, total_len, addr);
    free(preference_data);

    return 0;
}

void tvs_init_platform_adapter_impl(tvs_platform_adaptor *adapter)
{
    // adapter实现
    adapter->soundcard_control = tvs_platform_impl_soundcard_control;
    adapter->soundcard_pcm_read = tvs_media_recorder_impl_read;
    adapter->soundcard_pcm_write = tvs_soundcard_player_write;
    adapter->set_volume = tvs_platform_impl_set_current_cloud_volume;
    adapter->load_preference = tvs_platform_impl_load_preference;
    adapter->save_preference = tvs_platform_impl_save_preference;
}

