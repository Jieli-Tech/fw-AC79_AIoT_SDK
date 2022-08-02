#ifndef _ALI_API_H_
#define _ALI_API_H_

enum ALI_SDK_EVENT {
    ALI_SPEAK_END     = 0x01,
    ALI_MEDIA_END     = 0x02,
    ALI_PLAY_PAUSE    = 0x03,
    ALI_PREVIOUS_SONG = 0x04,
    ALI_NEXT_SONG     = 0x05,
    ALI_VOLUME_CHANGE = 0X06,
    ALI_VOLUME_INCR   = 0x07,
    ALI_VOLUME_DECR   = 0x08,
    ALI_VOLUME_MUTE   = 0x09,
    ALI_RECORD_START  = 0x0a,
    ALI_RECORD_SEND   = 0x0b,
    ALI_RECORD_STOP   = 0x0c,
    ALI_VOICE_MODE    = 0x0d,
    ALI_MEDIA_STOP    = 0x0e,
    ALI_BIND_DEVICE   = 0x0f,
    ALI_RECORD_ERR    = 0x10,
    ALI_COLLECT_RESOURCE = 0x12,
    ALI_PLAY_CONTIUE  = 0x13,
    ALI_SET_VOLUME    = 0x14,
    ALI_SPEAK_START   = 0x15,
    ALI_MEDIA_START   = 0x16,
    ALI_RECORD_BREAK  = 0x17,
    ALI_WS_CONNECT_MSG = 0x18,
    ALI_WS_DISCONNECT_MSG = 0x19,
    ALI_RECV_CHAT     = 0x1a,
    ALI_QUIT    	  = 0xff,
};


void ag_ws_on_connect() ;

void ag_ws_on_disconnect();

#endif
