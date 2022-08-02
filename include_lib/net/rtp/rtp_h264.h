#ifndef RTP_H264_H
#define RTP_H264_H

#include "rtp_common.h"
#include "server/rt_stream_pkg.h"

int rtp_h264_init();
int rtp_h264_send_frame(struct rt_stream_info *info, u8 *data, u32 size, u32 fps);
int rtp_h264_send_frame_with_ts(struct rt_stream_info *info, u8 *buf, u32 size, u32 ts);
#endif

