#ifndef  __AUDIO_H__
#define  __AUDIO_H__

#include "system/task.h"
#include "device/device.h"
#include "usb_bulk_transfer.h"
#include "usb/host/usb_host.h"
#include "usb/device/uac_audio.h"


#define HEADPHONE_SUPPORTED                0x01
#define MICROPHONE_SUPPORTED               0x02
#define HEADSET_SUPPORTED                  0x03

struct audio_streaming_t {
    u8  bNumEndpoints;
    u8  bFormatType;		/** FORMAT_TYPE_1 */
    u8  bNrChannels;		/** physical channels in the stream */
    u8  bSubframeSize;
    u8  bBitResolution;
    u8  bSamFreqType;
    u32 tSamFreq[8];
    u8  host_ep; //主机传输端点
    u8  ep; //从机端点(由描述符中指定)
    u8  ep_Interval;
    u16 ep_max_packet_size;
};

struct audio_device_t {
    u8 interface_num; //接口号
    u8 subclass;
    u8 support;
    void *parent;
    struct audio_streaming_t as[8];
};

int usb_audio_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf);

// API
void usb_host_audio_init(const usb_dev usb_id, int (*put_buf)(const usb_dev usb_id, void *ptr, u32 len, u8 channel, u32 sample_rate), int (*get_buf)(const usb_dev usb_id, void *ptr, u32 len, u8 channel, u32 sample_rate));
void usb_host_audio_exit(const usb_dev usb_id);
void usb_audio_start_process(const usb_dev usb_id);
void usb_audio_stop_process(const usb_dev usb_id);

//headphone api
void set_usb_audio_play_volume(const usb_dev usb_id, u16 vol);
void usb_audio_start_play(const usb_dev usb_id, u8 channel, u8 bit_reso, u32 sample_rate); //指定播放数据的声道数,位数,采样率
void usb_audio_stop_play(const usb_dev usb_id);
void usb_audio_pause_play(const usb_dev usb_id);
void usb_audio_resume_play(const usb_dev usb_id);

//microphone api
void set_usb_audio_record_volume(const usb_dev usb_id, u16 vol);
void usb_audio_start_record(const usb_dev usb_id, u8 channel, u8 bit_reso, u32 sample_rate); //指定录制数据的位数,采样率
void usb_audio_stop_record(const usb_dev usb_id);
void usb_audio_pause_record(const usb_dev usb_id);
void usb_audio_resume_record(const usb_dev usb_id);

#endif
