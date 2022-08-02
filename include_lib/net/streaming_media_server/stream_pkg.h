
#ifndef _STREAM_PKG_H_
#define _STREAM_PKG_H_

#include "server/vpkg_server.h"
#include "server/stream_pkg.h"

int strm_jpeg_get_aframe(void *ptr, unsigned int rd_len, unsigned int *ret_len);
int strm_jpeg_get_pcma_aframe(void *ptr, unsigned int len, unsigned int *ret_len);
int strm_jpeg_get_vframe(void *ptr, unsigned int len, unsigned int *rsize,
                         unsigned char *is_frame_end);
int strm_jpeg_vframe_buf_empty(struct strm_pkg_fd *strm_fd);
int strm_jpeg_aframe_buf_empty(struct strm_pkg_fd *strm_fd);
int strm_jpeg_get_frame_rate(unsigned int *frame_rate);
int strm_jpeg_get_screen_width(unsigned int *screen_width);
int strm_jpeg_get_screen_height(unsigned int *screen_height);
int strm_jpeg_pcm_get_sample_rate(unsigned int *sample_rate);
int strm_jpeg_pcm_get_channel_num(unsigned int *channel_num);
int strm_jpeg_pcm_get_bit_per_sample(unsigned int *bit_per_sample);

int strm_h264_get_pcm_aframe(void *ptr, unsigned int len, unsigned int *ret_len);
int strm_h264_get_pcma_aframe(void *ptr, unsigned int len, unsigned int *ret_len);
//int strm_h264_get_vframe(void *ptr, unsigned int len);
int strm_h264_get_vframe(void *ptr, unsigned int len, unsigned char drop_f);
int strm_h264_vframe_buf_empty(struct strm_pkg_fd *strm_fd);
int strm_h264_aframe_buf_empty(struct strm_pkg_fd *strm_fd);
int strm_h264_get_frame_rate(unsigned int *frame_rate);
int strm_h264_get_screen_width(unsigned int *screen_width);
int strm_h264_get_screen_height(unsigned int *screen_height);
int strm_h264_pcm_get_sample_rate(unsigned int *sample_rate);
int strm_h264_pcm_get_channel_num(unsigned int *channel_num);
int strm_h264_pcm_get_bit_per_sample(unsigned int *bit_per_sample);

#endif
