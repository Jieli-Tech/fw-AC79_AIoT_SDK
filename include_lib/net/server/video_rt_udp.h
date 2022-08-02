#ifndef __VIDEO_RT_UDP_H
#define __VIDEO_RT_UDP_H




int rt_stream_set_sockaddr(struct sockaddr_in *addr, u32 isforward);
int rt_stream_rm_sockaddr(struct sockaddr_in *addr, u32 isforward);



struct rt_stream_info *net_rt_vpkg_open(const char *path, const char *mode);
int net_rt_vpkg_close(struct rt_stream_info *info);
int net_rt_send_frame(struct rt_stream_info *info, char *buffer, size_t len);




#endif
