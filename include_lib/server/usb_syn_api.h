#ifndef __usb_syn_api_h
#define __usb_syn_api_h

typedef struct _speaker_funapi {
    void (*output)(u8 *obuf, u32 Len);
    u32(*getlen)(void);
} speaker_funapi;


typedef struct __adc_usb_syn_ops {
    void (*open)(u8 *ptr, u16 frame_len, u16 nch, u16 samplebits); //samplebits: 16 or 32
    u32(*need_buf)();
    void (*adc_isr_run)(u8 *ptr, u8 *inbuf, u32 inLen);
    u32(*usb_isr_run)(u8 *ptr);
    void (*set_frameSize)(u8 *ptr, u16 frame_len);
} adc_usb_syn_ops;


typedef struct __dac_usb_syn_ops {
    void (*open)(u8 *ptr, u32 target_val, speaker_funapi *sf);
    u32(*need_buf)();
    u32(*run)(u8 *ptr, u8 *inbuf, u32 inlen);
} dac_usb_syn_ops;


dac_usb_syn_ops *get_dac_usbsyn_ops();
adc_usb_syn_ops *get_adc_usbsyn_ops();

#endif
