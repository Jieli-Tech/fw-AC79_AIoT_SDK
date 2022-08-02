#ifndef __NET_UPDATE_H__
#define __NET_UPDATE_H__

#define CONFIG_UPGRADE_OTA_FILE_NAME        "update-ota.ufw"

void *net_fopen(char *path, char *mode);//net_fopen支持写flash固件升级和写到SD卡，当名字字符有CCONFIG_UPGRADE_OTA_FILE_NAME时是固件升级
int net_fwrite(void *file, unsigned char *buf, int len, int end);
int net_fread(void *file, char *buf, int len);
int net_flen(void *file);
int net_fclose(void *file, char is_socket_err);
int net_update_request(void);


#endif

