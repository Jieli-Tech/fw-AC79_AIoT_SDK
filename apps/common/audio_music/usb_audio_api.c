#include "system/includes.h"
#include "server/audio_server.h"
#include "server/server_core.h"
#include "server/usb_syn_api.h"
#include "video/video_ioctl.h"
#include "event/device_event.h"
#include "app_config.h"

extern void get_cfg_file_aec_config(struct aec_s_attr *aec_param);

#if TCFG_USB_SLAVE_AUDIO_ENABLE

#include "device/usb_stack.h"
#include "device/uac_stream.h"

struct usb_slave_mic_handle {
    struct video_buffer b;
    struct audio_format f;
    void *rec_dev;
    void *rec_priv;
    u8 *audio_rec_dma_buffer;
    u16 offset;
    u8 bindex;
};

struct usb_slave_spk_handle {
    OS_SEM rd_sem;
    void *play_dev;
    void *sample_sync_buf;
    cbuffer_t sample_sync_cbuf;
    u8 *dacsyn_ptr;
    dac_usb_syn_ops *dacUSBsyn_ops;
    u8 play_start;
};

static struct usb_slave_mic_handle *uac_slave_mic_handle[USB_MAX_HW_NUM];
static struct usb_slave_spk_handle *uac_slave_spk_handle[USB_MAX_HW_NUM];

static int usb_audio_mic_tx_handler(void *priv, void *data, int len)
{
    struct usb_slave_mic_handle *hdl = (struct usb_slave_mic_handle *)priv;
    int err = 0;
    struct video_buffer *b;

    if (!hdl->rec_dev) {
        return 0;
    }

    b = &hdl->b;
    b->noblock = 1;
    b->timeout = 0;
    b->index = hdl->bindex;

    if (hdl->offset == 0) {
        err = dev_ioctl(hdl->rec_dev, AUDIOC_DQBUF, (u32)b);
        if (err || !b->len) {
            return 0;
        }
    }

    memcpy(data, (u8 *)b->baddr + hdl->offset, len);
    hdl->offset += len;

    if (hdl->offset == b->len) {
        dev_ioctl(hdl->rec_dev, AUDIOC_QBUF, (u32)b);
        hdl->offset = 0;
    }

    return len;
}

static int usb_audio_mic_set_vol(struct usb_slave_mic_handle *hdl, u8 volume)
{
    if (hdl && hdl->rec_dev) {
#ifdef CONFIG_ALL_ADC_CHANNEL_OPEN_ENABLE
        return dev_ioctl(hdl->rec_dev, IOCTL_SET_VOLUME, (BIT(CONFIG_UAC_MIC_ADC_CHANNEL) << 8) | volume);
#else
        return dev_ioctl(hdl->rec_dev, IOCTL_SET_VOLUME, volume);
#endif
    }

    return 0;
}

static int usb_audio_mic_close(struct usb_slave_mic_handle *hdl, const usb_dev usb_id)
{
    if (!hdl) {
        return -1;
    }

    set_uac_mic_tx_handler(usb_id, NULL, NULL);

    if (hdl->rec_dev) {
        dev_ioctl(hdl->rec_dev, AUDIOC_STREAM_OFF, (u32)&hdl->bindex);
        if (hdl->offset) {
            dev_ioctl(hdl->rec_dev, AUDIOC_QBUF, (u32)&hdl->b);
            hdl->offset = 0;
        }
        dev_close(hdl->rec_dev);
        hdl->rec_dev = NULL;
    }
    if (hdl->audio_rec_dma_buffer) {
        free(hdl->audio_rec_dma_buffer);
        hdl->audio_rec_dma_buffer = NULL;
    }
    free(hdl);

    return 0;
}

static int usb_audio_mic_open(struct usb_slave_mic_handle *hdl,  const usb_dev usb_id, int value)
{
    int err = 0;
    void *mic_hdl = NULL;
    struct video_reqbufs breq = {0};

    if (!hdl || hdl->rec_dev) {
        return -1;
    }

    extern u16 uac_get_mic_vol(const usb_dev usb_id);
    extern u32 uac_get_mic_sameplerate(void *priv);
    u32 frame_len = ((uac_get_mic_sameplerate(NULL) * MIC_AUDIO_RES / 8 * MIC_CHANNEL) / 1000);
    frame_len += (uac_get_mic_sameplerate(NULL) % 1000 ? (MIC_AUDIO_RES / 8) * MIC_CHANNEL : 0);

    memset(&hdl->f, 0, sizeof(struct audio_format));
    memset(&hdl->b, 0, sizeof(struct video_buffer));

#if CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_PLNK0
    mic_hdl = dev_open("audio", (void *)AUDIO_TYPE_ENC_PLNK0);
#elif CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_PLNK1
    mic_hdl = dev_open("audio", (void *)AUDIO_TYPE_ENC_PLNK1);
#elif CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_IIS0
    mic_hdl = dev_open("audio", (void *)AUDIO_TYPE_ENC_IIS0);
#elif CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_IIS1
    mic_hdl = dev_open("audio", (void *)AUDIO_TYPE_ENC_IIS1);
#elif CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_LINEIN
    mic_hdl = dev_open("audio", (void *)AUDIO_TYPE_ENC_LINEIN);
#else
    mic_hdl = dev_open("audio", (void *)AUDIO_TYPE_ENC_MIC);
#endif

    if (!mic_hdl) {
        log_e("uac slave audio_open: err\n");
        return -EFAULT;
    }

    hdl->audio_rec_dma_buffer = malloc(frame_len * 3 * 4);
    if (!hdl->audio_rec_dma_buffer) {
        goto __err;
    }

    breq.buf  = hdl->audio_rec_dma_buffer;
    breq.size = frame_len * 3 * 4;

    err = dev_ioctl(mic_hdl, AUDIOC_REQBUFS, (unsigned int)&breq);
    if (err) {
        goto __err;
    }

    hdl->offset = 0;
#ifdef CONFIG_ALL_ADC_CHANNEL_OPEN_ENABLE
    hdl->f.channel_bit_map = BIT(CONFIG_UAC_MIC_ADC_CHANNEL);
#endif
#if CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_PLNK0
    hdl->f.sample_source = "plnk0";
#elif CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_PLNK1
    hdl->f.sample_source = "plnk1";
#elif CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_IIS0
    hdl->f.sample_source = "iis0";
#elif CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_IIS1
    hdl->f.sample_source = "iis1";
#else
    hdl->f.sample_source = "mic";
#endif
    hdl->f.format = "pcm";
    hdl->f.volume = uac_get_mic_vol(usb_id);
    hdl->f.channel = value >> 24;
    hdl->f.sample_rate = value & 0xffffff;
    hdl->f.frame_len = frame_len * 3;

    log_d("uac slave channel : %d\n", hdl->f.channel);
    log_d("uac slave sample_rate : %d\n", hdl->f.sample_rate);
    log_d("uac slave volume : %d\n", hdl->f.volume);
    log_d("uac slave frame_len : %d\n", hdl->f.frame_len);

#ifdef CONFIG_USB_AUDIO_AEC_ENABLE
    struct aec_s_attr aec_param = {0};

    if ((hdl->f.sample_rate == 8000 || hdl->f.sample_rate == 16000) && hdl->f.channel == 1) {
        get_cfg_file_aec_config(&aec_param);

        hdl->f.aec_enable = aec_param.EnableBit ? 1 : 0;

        if (hdl->f.aec_enable) {
            if (hdl->f.sample_rate == 16000) {
                aec_param.wideband = 1;
                aec_param.hw_delay_offset = 50;
            } else {
                aec_param.wideband = 0;
                aec_param.hw_delay_offset = 75;
            }

#if defined CONFIG_ALL_ADC_CHANNEL_OPEN_ENABLE && defined CONFIG_AISP_LINEIN_ADC_CHANNEL && defined CONFIG_AEC_LINEIN_CHANNEL_ENABLE
            aec_param.output_way = 1;
            hdl->f.channel_bit_map |= BIT(CONFIG_AISP_LINEIN_ADC_CHANNEL);
            if (CONFIG_AISP_LINEIN_ADC_CHANNEL < CONFIG_UAC_MIC_ADC_CHANNEL) {
                hdl->f.ch_data_exchange = 1;
            }
#endif
            hdl->f.ch_detached = 1;
            hdl->f.aec_attr = &aec_param;
        }
    }
#endif

    err = dev_ioctl(mic_hdl, AUDIOC_SET_FMT, (unsigned int)&hdl->f);
    if (err) {
        log_e("uac slave audio_set_fmt: err\n");
        goto __err;
    }

    err = dev_ioctl(mic_hdl, AUDIOC_STREAM_ON, (u32)&hdl->bindex);
    if (err) {
        log_e(" uac slave audio rec stream on err\n");
        goto __err;
    }

    hdl->rec_dev = mic_hdl;

    set_uac_mic_tx_handler(usb_id, hdl, usb_audio_mic_tx_handler);

    log_i("uac slave audio_enc_start: suss\n");

    return 0;
__err:
    if (mic_hdl) {
        dev_close(mic_hdl);
    }
    if (hdl->audio_rec_dma_buffer) {
        free(hdl->audio_rec_dma_buffer);
        hdl->audio_rec_dma_buffer = NULL;
    }

    return err;
}

static void speaker_rx_response(void *priv, void *buf, int len)
{
    struct usb_slave_spk_handle *hdl = (struct usb_slave_spk_handle *)priv;

    if (hdl && hdl->play_start && hdl->dacUSBsyn_ops) {
        hdl->dacUSBsyn_ops->run(hdl->dacsyn_ptr, buf, len);
    }
}

static int uac_slave_vfs_fread(void *file, void *data, u32 len)
{
    struct usb_slave_spk_handle *hdl = (struct usb_slave_spk_handle *)file;

    int rlen = 0;

    while (!rlen) {
        rlen = cbuf_get_data_size(&hdl->sample_sync_cbuf);
        if (!rlen) {
            if (!hdl->play_start) {
                return -2;
            }
            os_sem_pend(&hdl->rd_sem, 0);
        }
    }
    if (rlen > len) {
        rlen = len;
    }

    cbuf_read(&hdl->sample_sync_cbuf, data, rlen);

    return rlen;
}

static int uac_slave_vfs_fclose(void *file)
{
    return 0;
}

static int uac_slave_vfs_flen(void *file)
{
    return 0;
}

static const struct audio_vfs_ops uac_slave_vfs_ops = {
    .fread  = uac_slave_vfs_fread,
    .fclose = uac_slave_vfs_fclose,
    .flen   = uac_slave_vfs_flen,
};

static void dac_write_obuf(void *priv, u8 *outbuf, u32 len)
{
    struct usb_slave_spk_handle *hdl = (struct usb_slave_spk_handle *)priv;

    if (hdl->play_start) {
        cbuf_write(&hdl->sample_sync_cbuf, outbuf, len);
        os_sem_set(&hdl->rd_sem, 0);
        os_sem_post(&hdl->rd_sem);
    }
}

static u32 dac_get_obuf(void *priv)
{
    struct usb_slave_spk_handle *hdl = (struct usb_slave_spk_handle *)priv;

    return cbuf_get_data_size(&hdl->sample_sync_cbuf);
}

static int usb_audio_sample_sync_open(struct usb_slave_spk_handle *hdl, int sample_rate, u8 channel, const usb_dev usb_id)
{
    int sync_buff_size = sample_rate * 4 * channel / 1000 * 30;
    hdl->sample_sync_buf = zalloc(sync_buff_size);
    if (!hdl->sample_sync_buf) {
        return -ENOMEM;
    }

    os_sem_create(&hdl->rd_sem, 0);

    speaker_funapi sf;
    sf.output = dac_write_obuf;
    sf.getlen = dac_get_obuf;
    sf.priv = hdl;

    hdl->dacUSBsyn_ops = get_dac_usbsyn_ops();
    ASSERT(hdl->dacUSBsyn_ops != NULL);
    hdl->dacsyn_ptr = (u8 *)malloc(hdl->dacUSBsyn_ops->need_buf());
    if (!hdl->dacsyn_ptr) {
        goto __err;
    }
    cbuf_init(&hdl->sample_sync_cbuf, hdl->sample_sync_buf, sync_buff_size);
    hdl->dacUSBsyn_ops->open(hdl->dacsyn_ptr, sync_buff_size / 2, &sf);
    set_uac_speaker_rx_handler(usb_id, hdl, speaker_rx_response);

    return 0;

__err:
    if (hdl->sample_sync_buf) {
        free(hdl->sample_sync_buf);
        hdl->sample_sync_buf = NULL;
    }

    return -EINVAL;
}

static void usb_audio_sample_sync_close(struct usb_slave_spk_handle *hdl, const usb_dev usb_id)
{
    if (!hdl) {
        return;
    }

    set_uac_speaker_rx_handler(usb_id, NULL, NULL);

    if (hdl->dacsyn_ptr) {
        free(hdl->dacsyn_ptr);
        hdl->dacsyn_ptr = NULL;
    }
    hdl->dacUSBsyn_ops = NULL;

    if (hdl->sample_sync_buf) {
        free(hdl->sample_sync_buf);
        hdl->sample_sync_buf = NULL;
    }

    free(hdl);
}

static int usb_audio_speaker_open(struct usb_slave_spk_handle *hdl, const usb_dev usb_id, int value)
{
    int err = 0;
    union audio_req req = {0};
    u32 sample_rate;
    u8 channel;

    if (!hdl || hdl->play_dev) {
        return -1;
    }

    hdl->play_dev = server_open("audio_server", "dec");
    if (!hdl->play_dev) {
        return -1;
    }

    u16 vol = 0;
    extern void uac_get_cur_vol(const usb_dev usb_id, u16 * l_vol, u16 * r_vol);
    uac_get_cur_vol(usb_id, &vol, NULL);

    channel = value >> 24;
    sample_rate = value & 0xffffff;

    req.dec.cmd             = AUDIO_DEC_OPEN;
    req.dec.volume          = vol;
    req.dec.output_buf_len  = 640 * 3;
    req.dec.channel         = channel;
    req.dec.sample_rate     = sample_rate;
    req.dec.priority        = 1;
    req.dec.vfs_ops         = &uac_slave_vfs_ops;
    req.dec.dec_type        = "pcm";
    req.dec.sample_source   = "dac";
    /* req.dec.attr            = AUDIO_ATTR_REAL_TIME; */
    req.dec.file            = (FILE *)hdl;

    err = server_request(hdl->play_dev, AUDIO_REQ_DEC, &req);
    if (err) {
        goto __err1;
    }

    err = usb_audio_sample_sync_open(hdl, sample_rate, channel, usb_id);
    if (err) {
        goto __err1;
    }

    hdl->play_start = 1;

    req.dec.cmd = AUDIO_DEC_START;
    err = server_request(hdl->play_dev, AUDIO_REQ_DEC, &req);
    if (err) {
        goto __err1;
    }

    return 0;

__err1:
    hdl->play_start = 0;

    if (hdl->play_dev) {
        server_close(hdl->play_dev);
        hdl->play_dev = NULL;
    }

    return -1;
}

static int usb_audio_speaker_close(struct usb_slave_spk_handle *hdl, const usb_dev usb_id)
{
    union audio_req req = {0};

    if (!hdl) {
        return -1;
    }

    if (hdl->play_dev) {
        hdl->play_start = 0;
        os_sem_post(&hdl->rd_sem);
        req.dec.cmd = AUDIO_DEC_STOP;
        server_request(hdl->play_dev, AUDIO_REQ_DEC, &req);
        server_close(hdl->play_dev);
        hdl->play_dev = NULL;
    }

    usb_audio_sample_sync_close(hdl, usb_id);

    return 0;
}

static int usb_audio_speaker_set_vol(struct usb_slave_spk_handle *hdl, u8 volume)
{
    if (hdl && hdl->play_dev) {
        union audio_req req = {0};
        req.dec.cmd     = AUDIO_DEC_SET_VOLUME;
        req.dec.volume  = volume;
        return server_request(hdl->play_dev, AUDIO_REQ_DEC, &req);
    }

    return -1;
}

int usb_audio_event_handler(struct device_event *event)
{
    int value = event->value;
    const usb_dev usb_id = (const usb_dev)event->arg;
    struct usb_slave_mic_handle *mic_hdl = uac_slave_mic_handle[usb_id];
    struct usb_slave_spk_handle *spk_hdl = uac_slave_spk_handle[usb_id];

    switch (event->event) {
    case USB_AUDIO_MIC_OPEN:
        if (!mic_hdl) {
            mic_hdl = zalloc(sizeof(struct usb_slave_mic_handle));
            uac_slave_mic_handle[usb_id] = mic_hdl;
        }
        usb_audio_mic_open(mic_hdl, usb_id, value);
        break;
    case USB_AUDIO_MIC_CLOSE:
        usb_audio_mic_close(mic_hdl, usb_id);
        uac_slave_mic_handle[usb_id] = NULL;
        break;
    case USB_AUDIO_SET_MIC_VOL:
        log_i("USB_AUDIO_SET_MIC_VOL : %d\n", value);
        usb_audio_mic_set_vol(mic_hdl, value);
        break;
    case USB_AUDIO_SET_PLAY_VOL:
        log_i("USB_AUDIO_SET_PLAY_VOL : %d\n", value & 0xff);
        usb_audio_speaker_set_vol(spk_hdl, value & 0xff);
        break;
    case USB_AUDIO_PLAY_OPEN:
        log_i("USB_AUDIO_SET_PLAY_OPEN\n");
        if (!spk_hdl) {
            spk_hdl = zalloc(sizeof(struct usb_slave_spk_handle));
            uac_slave_spk_handle[usb_id] = spk_hdl;
        }
        usb_audio_speaker_open(spk_hdl, usb_id, value);
        break;
    case USB_AUDIO_PLAY_CLOSE:
        log_i("USB_AUDIO_SET_PLAY_CLOSE\n");
        usb_audio_speaker_close(spk_hdl, usb_id);
        uac_slave_spk_handle[usb_id] = NULL;
        break;
    }

    return true;
}

#endif


#if TCFG_HOST_AUDIO_ENABLE

#include "host/audio.h"

struct usb_host_mic_handle {
    struct video_buffer b;
    struct audio_format f;
    void *rec_dev;
    u8 *audio_rec_dma_buffer;
    u16 offset;
    u8 bindex;
};

struct usb_host_spk_handle {
    OS_SEM rd_sem;
    void *play_dev;
    cbuffer_t play_cbuf;
    void *play_buf;
    u8 play_start;
    u8 single_channel_to_double;
};

static struct usb_host_mic_handle *uac_host_mic_handle[USB_MAX_HW_NUM];
static struct usb_host_spk_handle *uac_host_spk_handle[USB_MAX_HW_NUM];

static int usb_host_audio_mic_tx_handler(struct usb_host_mic_handle *hdl, void *data, u32 len)
{
    int err = 0;
    struct video_buffer *b;

    if (!hdl || !hdl->rec_dev) {
        return 0;
    }

    b = &hdl->b;
    b->noblock = 1;
    b->timeout = 0;
    b->index = hdl->bindex;

    if (hdl->offset == 0) {
        err = dev_ioctl(hdl->rec_dev, AUDIOC_DQBUF, (u32)b);
        if (err || !b->len) {
            return 0;
        }
    }

    memcpy(data, (u8 *)b->baddr + hdl->offset, len);
    hdl->offset += len;

    if (hdl->offset == b->len) {
        dev_ioctl(hdl->rec_dev, AUDIOC_QBUF, (u32)b);
        hdl->offset = 0;
    }

    return len;
}

static int usb_host_audio_mic_close(struct usb_host_mic_handle *hdl)
{
    if (!hdl) {
        return -1;
    }

    if (hdl->rec_dev) {
        dev_ioctl(hdl->rec_dev, AUDIOC_STREAM_OFF, (u32)&hdl->bindex);
        if (hdl->offset) {
            dev_ioctl(hdl->rec_dev, AUDIOC_QBUF, (u32)&hdl->b);
            hdl->offset = 0;
        }
        dev_close(hdl->rec_dev);
        hdl->rec_dev = NULL;
    }
    if (hdl->audio_rec_dma_buffer) {
        free(hdl->audio_rec_dma_buffer);
        hdl->audio_rec_dma_buffer = NULL;
    }

    free(hdl);

    return 0;
}

static int usb_host_audio_mic_open(struct usb_host_mic_handle *hdl, u32 sample_rate, u32 frame_len, u8 channel)
{
    int err = 0;
    void *mic_hdl = NULL;
    struct video_reqbufs breq = {0};

    if (hdl->rec_dev) {
        return 0;
    }

    memset(&hdl->f, 0, sizeof(struct audio_format));
    memset(&hdl->b, 0, sizeof(struct video_buffer));

#if CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_PLNK0
    mic_hdl = dev_open("audio", (void *)AUDIO_TYPE_ENC_PLNK0);
#elif CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_PLNK1
    mic_hdl = dev_open("audio", (void *)AUDIO_TYPE_ENC_PLNK1);
#elif CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_IIS0
    mic_hdl = dev_open("audio", (void *)AUDIO_TYPE_ENC_IIS0);
#elif CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_IIS1
    mic_hdl = dev_open("audio", (void *)AUDIO_TYPE_ENC_IIS1);
#elif CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_LINEIN
    mic_hdl = dev_open("audio", (void *)AUDIO_TYPE_ENC_LINEIN);
#else
    mic_hdl = dev_open("audio", (void *)AUDIO_TYPE_ENC_MIC);
#endif

    if (!mic_hdl) {
        log_e("uac host audio_open: err\n");
        return -EFAULT;
    }

    hdl->audio_rec_dma_buffer = malloc(frame_len * 6);
    if (!hdl->audio_rec_dma_buffer) {
        goto __err;
    }

    breq.buf  = hdl->audio_rec_dma_buffer;
    breq.size = frame_len * 6;

    err = dev_ioctl(mic_hdl, AUDIOC_REQBUFS, (unsigned int)&breq);
    if (err) {
        goto __err;
    }

    hdl->offset = 0;
#ifdef CONFIG_ALL_ADC_CHANNEL_OPEN_ENABLE
    hdl->f.channel_bit_map = BIT(CONFIG_UAC_MIC_ADC_CHANNEL);
#endif
#if CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_PLNK0
    hdl->f.sample_source = "plnk0";
#elif CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_PLNK1
    hdl->f.sample_source = "plnk1";
#elif CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_IIS0
    hdl->f.sample_source = "iis0";
#elif CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_IIS1
    hdl->f.sample_source = "iis1";
#elif CONFIG_AUDIO_ENC_SAMPLE_SOURCE == AUDIO_ENC_SAMPLE_SOURCE_LINEIN
    hdl->f.sample_source = "linein";
#else
    hdl->f.sample_source = "mic";
#endif
    hdl->f.format = "pcm";
    hdl->f.volume = HOST_MIC_VOLUME;
    hdl->f.channel = channel;
    hdl->f.sample_rate = sample_rate;
    hdl->f.frame_len = frame_len;

    log_d("uac host channel : %d\n", hdl->f.channel);
    log_d("uac host sample_rate : %d\n", hdl->f.sample_rate);
    log_d("uac host volume : %d\n", hdl->f.volume);
    log_d("uac host frame_len : %d\n", hdl->f.frame_len);

#ifdef CONFIG_USB_AUDIO_AEC_ENABLE
    struct aec_s_attr aec_param = {0};

    if ((sample_rate == 8000 || sample_rate == 16000) && channel == 1) {
        get_cfg_file_aec_config(&aec_param);

        hdl->f.aec_enable = aec_param.EnableBit ? 1 : 0;

        if (hdl->f.aec_enable) {
            if (sample_rate == 16000) {
                aec_param.wideband = 1;
                aec_param.hw_delay_offset = 50;
            } else {
                aec_param.wideband = 0;
                aec_param.hw_delay_offset = 75;
            }

#if defined CONFIG_ALL_ADC_CHANNEL_OPEN_ENABLE && defined CONFIG_AISP_LINEIN_ADC_CHANNEL && defined CONFIG_AEC_LINEIN_CHANNEL_ENABLE
            aec_param.output_way = 1;
            hdl->f.channel_bit_map |= BIT(CONFIG_AISP_LINEIN_ADC_CHANNEL);
            if (CONFIG_AISP_LINEIN_ADC_CHANNEL < CONFIG_UAC_MIC_ADC_CHANNEL) {
                hdl->f.ch_data_exchange = 1;
            }
#endif
            hdl->f.ch_detached = 1;
            hdl->f.aec_attr = &aec_param;
        }
    }
#endif

    err = dev_ioctl(mic_hdl, AUDIOC_SET_FMT, (unsigned int)&hdl->f);
    if (err) {
        log_e("uac host audio_set_fmt: err\n");
        goto __err;
    }

    err = dev_ioctl(mic_hdl, AUDIOC_STREAM_ON, (u32)&hdl->bindex);
    if (err) {
        log_e(" uac host audio rec stream on err\n");
        goto __err;
    }

    hdl->rec_dev = mic_hdl;

    log_i("uac host audio_enc_start: suss\n");

    return 0;
__err:
    if (mic_hdl) {
        dev_close(mic_hdl);
    }
    if (hdl->audio_rec_dma_buffer) {
        free(hdl->audio_rec_dma_buffer);
        hdl->audio_rec_dma_buffer = NULL;
    }

    return err;
}

static int uac_host_vfs_fread(void *file, void *data, u32 len)
{
    struct usb_host_spk_handle *hdl = (struct usb_host_spk_handle *)file;

    int rlen = 0;

    while (!rlen) {
        rlen = cbuf_get_data_size(&hdl->play_cbuf);
        if (!rlen) {
            if (!hdl->play_start) {
                return -2;
            }
            os_sem_pend(&hdl->rd_sem, 0);
        }
    }

    if (hdl->single_channel_to_double) {
        if (rlen > len / 2) {
            rlen = len / 2;
        }
    } else {
        if (rlen > len) {
            rlen = len;
        }
    }

    cbuf_read(&hdl->play_cbuf, data, rlen);

    if (hdl->single_channel_to_double) {
        s16 *start = (s16 *)((u8 *)data + rlen);
        s16 *end = (s16 *)((u8 *)data + rlen * 2);

        for (u32 i = 0; i < rlen / 2; ++i) {
            *--end = *--start;
            *--end = *start;
        }

        rlen <<= 1;
    }

    return rlen;
}

static int uac_host_vfs_fclose(void *file)
{
    return 0;
}

static int uac_host_vfs_flen(void *file)
{
    return 0;
}

static const struct audio_vfs_ops uac_host_vfs_ops = {
    .fread  = uac_host_vfs_fread,
    .fclose = uac_host_vfs_fclose,
    .flen   = uac_host_vfs_flen,
};

static int usb_host_audio_speaker_open(struct usb_host_spk_handle *hdl, u32 sample_rate, u32 frame_len, u8 channel)
{
    int err = 0;
    union audio_req req = {0};

    if (hdl->play_dev) {
        return -1;
    }

    os_sem_create(&hdl->rd_sem, 0);

    int play_buf_size = sample_rate * 4 * channel / 1000 * 10;

    if (channel & BIT(7)) {
        channel &= ~BIT(7);
        hdl->single_channel_to_double = 1;
    }

    hdl->play_buf = malloc(play_buf_size);
    if (!hdl->play_buf) {
        return -1;
    }
    cbuf_init(&hdl->play_cbuf, hdl->play_buf, play_buf_size);

    hdl->play_dev = server_open("audio_server", "dec");
    if (!hdl->play_dev) {
        goto __err;
    }

    req.dec.cmd             = AUDIO_DEC_OPEN;
    req.dec.volume          = HOST_SPK_VOLUME;
    req.dec.output_buf_len  = 640 * 3;
    req.dec.channel         = channel;
    req.dec.sample_rate     = sample_rate;
    req.dec.priority        = 1;
    req.dec.vfs_ops         = &uac_host_vfs_ops;
    req.dec.dec_type        = "pcm";
    req.dec.sample_source   = "dac";
    /* req.dec.attr            = AUDIO_ATTR_REAL_TIME; */
    req.dec.file            = (FILE *)hdl;

    err = server_request(hdl->play_dev, AUDIO_REQ_DEC, &req);
    if (err) {
        goto __err;
    }

    hdl->play_start = 1;

    req.dec.cmd = AUDIO_DEC_START;
    err = server_request_async(hdl->play_dev, AUDIO_REQ_DEC, &req);
    if (err) {
        goto __err;
    }

    return 0;

__err:
    hdl->play_start = 0;

    if (hdl->play_buf) {
        free(hdl->play_buf);
        hdl->play_buf = NULL;
    }

    if (hdl->play_dev) {
        server_close(hdl->play_dev);
        hdl->play_dev = NULL;
    }

    return -1;
}

static int usb_host_audio_speaker_close(struct usb_host_spk_handle *hdl)
{
    union audio_req req = {0};

    if (!hdl) {
        return -1;
    }

    if (hdl->play_dev) {
        hdl->play_start = 0;
        os_sem_post(&hdl->rd_sem);
        req.dec.cmd = AUDIO_DEC_STOP;
        server_request(hdl->play_dev, AUDIO_REQ_DEC, &req);
        server_close(hdl->play_dev);
        free(hdl->play_buf);
        hdl->play_buf = NULL;
        hdl->play_dev = NULL;
    }

    free(hdl);

    return 0;
}

static int usb_host_audio_play_put_buf(const usb_dev usb_id, void *ptr, u32 len, u8 channel, u32 sample_rate)
{
    struct usb_host_mic_handle *hdl = uac_host_mic_handle[usb_id];

    if (channel == 0 && sample_rate == 0) {
        usb_host_audio_mic_close(hdl);
        uac_host_mic_handle[usb_id] = NULL;
        return 0;
    }
    if (!hdl) {
        hdl = zalloc(sizeof(struct usb_host_mic_handle));
        uac_host_mic_handle[usb_id] = hdl;
    }
    if (hdl && !hdl->rec_dev) {
        usb_host_audio_mic_open(hdl, sample_rate, len, channel);
    }
    if (hdl && hdl->rec_dev) {
        return usb_host_audio_mic_tx_handler(hdl, ptr, len);
    } else {
        len = 0;
    }

    return len;
}

static int usb_host_audio_record_get_buf(const usb_dev usb_id, void *ptr, u32 len, u8 channel, u32 sample_rate)
{
    struct usb_host_spk_handle *hdl = uac_host_spk_handle[usb_id];

    if (channel == 0 && sample_rate == 0) {
        usb_host_audio_speaker_close(hdl);
        uac_host_spk_handle[usb_id] = NULL;
        return 0;
    }
    if (!hdl) {
        hdl = zalloc(sizeof(struct usb_host_spk_handle));
        uac_host_spk_handle[usb_id] = hdl;
    }
    if (hdl && !hdl->play_dev) {
        usb_host_audio_speaker_open(hdl, sample_rate, len, channel);
    }
    if (hdl && hdl->play_dev && len > 0) {
        cbuf_write(&hdl->play_cbuf, ptr, len);
        os_sem_set(&hdl->rd_sem, 0);
        os_sem_post(&hdl->rd_sem);
    }

    return len;
}

static int usb_host_audio_api_init(void)
{
    usb_host_audio_init(0, usb_host_audio_play_put_buf, usb_host_audio_record_get_buf);
#if USB_MAX_HW_NUM > 1
    usb_host_audio_init(1, usb_host_audio_play_put_buf, usb_host_audio_record_get_buf);
#endif
    return 0;
}
late_initcall(usb_host_audio_api_init);

#endif
