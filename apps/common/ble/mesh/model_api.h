#ifndef __MODEL_API_H__
#define __MODEL_API_H__

//< Detail in "MshMDLv1.0.1"
#define SIG_MESH_GENERIC_ONOFF_CLIENT       0 // examples/generic_onoff_client.c
#define SIG_MESH_GENERIC_ONOFF_SERVER       1 // examples/generic_onoff_server.c
#define SIG_MESH_VENDOR_CLIENT              2 // examples/vendor_client.c
#define SIG_MESH_VENDOR_SERVER              3 // examples/vendor_server.c
#define SIG_MESH_ALIGENIE_SOCKET            4 // examples/AliGenie_socket.c
#define SIG_MESH_ALIGENIE_LIGHT             5 // examples/AliGenie_light.c
#define SIG_MESH_ALIGENIE_FAN               6 // examples/AliGenie_fan.c
#define SIG_MESH_TUYA_LIGHT                 7 // examples/TUYA_light.c
#define SIG_MESH_PROVISIONER                8 // examples/provisioner.c

// more...

//< Config whick example will use in <examples>
#define CONFIG_MESH_MODEL                   SIG_MESH_ALIGENIE_SOCKET


#define BYTE_LEN(x...)                      sizeof((u8 []) {x})

#define MAC_TO_LITTLE_ENDIAN(x) \
    (x & 0xff), \
    ((x >> 8) & 0xff), \
    ((x >> 16) & 0xff), \
    ((x >> 24) & 0xff), \
    ((x >> 32) & 0xff), \
    ((x >> 40) & 0xff)

#define MAC_TO_BIG_ENDIAN(x) \
    ((x >> 40) & 0xff), \
    ((x >> 32) & 0xff), \
    ((x >> 24) & 0xff), \
    ((x >> 16) & 0xff), \
    ((x >> 8) & 0xff), \
    (x & 0xff)


#endif /* __MODEL_API_H__ */
