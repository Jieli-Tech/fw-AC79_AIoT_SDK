/**
 * Copyright (2017) Baidu Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * File: event.c
 * Auth: Renhe Zhang (v_zhangrenhe@baidu.com)
 * Desc: Duer Application Key definition.
 *       Blocking loop function.
 */

#ifndef BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_DCS3_LINUX_DUERAPP_EVENT_H
#define BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_DCS3_LINUX_DUERAPP_EVENT_H


enum duer_kbd_events {
    DUER_SPEAK_END     = 0x01,
    DUER_MEDIA_END     = 0x02,
    DUER_PLAY_PAUSE    = 0x03,
    DUER_PREVIOUS_SONG = 0x04,
    DUER_NEXT_SONG     = 0x05,
    DUER_VOLUME_CHANGE = 0X06,
    DUER_VOLUME_INCR   = 0x07,
    DUER_VOLUME_DECR   = 0x08,
    DUER_VOLUME_MUTE   = 0x09,
    DUER_RECORD_START  = 0x0a,
    DUER_RECORD_SEND   = 0x0b,
    DUER_RECORD_STOP   = 0x0c,
    DUER_VOICE_MODE    = 0x0d,
    DUER_MEDIA_STOP    = 0x0e,
    DUER_BIND_DEVICE   = 0x0f,
    DUER_COLLECT_RES   = 0x10,
    DUER_QUIT    	   = 0xff,
};

void duer_event_loop();

#endif // BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_DCS3_LINUX_DUERAPP_EVENT_H
