#ifndef _BDVOICE_H_
#define _BDVOICE_H_

#include "generic/typedef.h"
#include "http/http_cli.h"


enum  bdvoice_error {
    BD_VOICE_NONE_ERROR = HERROR_OK,
    BD_VOICE_MEM_ERROR = HERROR_MEM,
    BD_VOICE_URL_ERROR = HERROR_HEADER,
    BD_VOICE_RESPOND_ERROR = HERROR_RESPOND,
    BD_VOICE_SOCK_ERROR = HERROR_SOCK,
    BD_VOICE_BREAK_STATUS = HERROR_CALLBACK,
    BD_VOICE_HTTP_PARM_ERROR = HERROR_PARAM,
    BD_VOICE_BODY_ANALYSIS = HERROR_BODY_ANALYSIS,
    BD_VOICE_PARM_INPU_ERROR = -10,
    BD_VOICE_HTTP_JSON_ERROR = -11,
    BD_VOICE_UNAUTHOR_ERROR = -12,
    BD_VOICE_SERVER_RESP_ERROR = -13,
};


struct bdvoice_object {
    char *api_key;
    char *secret_key;
    char *token;
    int checkin_flag;

    char *cuid;
    unsigned char spd;	//0-15
    unsigned char pit;	//0-15
    unsigned char vol;	//0-15
    unsigned char per;	//0-4
    u32 len;
    char *databuf;

    char *result;
    int result_len;
    int error_no;

    httpcli_ctx ctx;

    int play_flag;
    char url[1024];
};

extern int bdvoice_authorize(struct bdvoice_object *bd);
extern int bdvoice_transform(struct bdvoice_object *bd);
extern void bdvoice_free(struct bdvoice_object *bd);
extern void bdvoice_break(struct bdvoice_object *bd);
extern void set_bdvoice_object_timeout(struct bdvoice_object *bd, u32 ms);
extern int init_bdvoice_object(struct bdvoice_object *bd, const char *api_key, const char *secret_key,
                               const char *cuid, u8 spd, u8 pit, u8 vol, u8 per);


#endif
