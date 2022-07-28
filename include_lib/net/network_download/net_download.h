//----------------------------------------------------------------------------//
/**
 ******************************************************************************
 * @file    net_download.h
 * @author
 * @version
 * @brief   This file provides the api of net audio download.
 ******************************************************************************
 * @attention
 *
 * Copyright(c) 2017, ZhuHai JieLi Technology Co.Ltd. All rights reserved.
 ******************************************************************************
 */

#ifndef NET_DOWNLOAD_H
#define NET_DOWNLOAD_H

#include "typedef.h"

struct net_download_parm {
    const char *url;
    u32 seek_low_range;
    u32 seek_high_range;
    u32 cbuf_size;
    u32 timeout_millsec;	//socket的连接和读数的超时
    u32 read_timeout;		//解码器读网络buf的超时(ms)
    u8 prio;
    u8 max_reconnect_cnt;
    u8 save_file;
    u8 no_wait_close;
    const char *file_dir;
    u32	dir_len;			//路径长度,不能大于128
    void *net_buf;			//填NULL由库分配
    u32 start_play_threshold;//下载缓存了多少个字节才开始播放
    u32 seek_threshold;//跳转范围超过此值就直接重建链接
};

enum {
    AI_SPEAK_PRIO,
    AI_ALARM_PRIO,
    AI_MEDIA_PRIO,
};

enum {
    NET_DOWNLOAD_STATUS_NONE = 0,
    NET_DOWNLOAD_CONNECT_OK = 1,
    NET_DOWNLOAD_COMPLETE = 2,
    NET_DOWNLOAD_CONNECT_FAIL = -1,
    NET_DOWNLOAD_RECONNECT_MAX = -2,
    NET_DOWNLOAD_CLOSED_MANUAL = -3,
};

int net_download_open(void **priv, struct net_download_parm *parm);
int net_download_read(void *priv, void *buf, u32 len);
int net_download_seek(void *priv, u32 offset, int orig);
int net_download_close(void *priv);
int net_download_check_ready(void *priv);
char *net_download_get_media_type(void *priv);
int net_download_get_file_len(void *priv);
u32 net_download_get_tmp_data_size(void *priv);
void net_download_set_tmp_data_threshold(void *priv, u32 threshold);
int net_download_set_pp(void *priv, u8 pp);
int net_download_set_read_timeout(void *priv, u32 timeout_ms);
void net_download_buf_inactive(void *priv);
int net_download_restart(void *priv, struct net_download_parm *parm);
int net_download_get_status(void *priv);

#endif

