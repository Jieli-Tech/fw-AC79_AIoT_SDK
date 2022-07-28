#ifndef _BDSPEECH_H_
#define _BDSPEECH_H_

#include "generic/typedef.h"
#include "http/http_cli.h"

enum  bdspeech_error {
    BD_SPEECH_NONE_ERROR = HERROR_OK,
    BD_SPEECH_MEM_ERROR = HERROR_MEM,
    BD_SPEECH_URL_ERROR = HERROR_HEADER,
    BD_SPEECH_RESPOND_ERROR = HERROR_RESPOND,
    BD_SPEECH_SOCK_ERROR = HERROR_SOCK,
    BD_SPEECH_BREAK_STATUS = HERROR_CALLBACK,
    BD_SPEECH_HTTP_PARM_ERROR = HERROR_PARAM,
    BD_SPEECH_BODY_ANALYSIS = HERROR_BODY_ANALYSIS,
    BD_SPEECH_PARM_INPU_ERROR = -10,
    BD_SPEECH_HTTP_JSON_ERROR = -11,
    BD_SPEECH_UNAUTHOR_ERROR = -12,
    BD_SPEECH_SERVER_RESP_ERROR = -13,
};


struct bdspeech_object {
    char *api_key;
    char *secret_key;
    char *token;
    const char *format;	//不能指向局部变量
    char *cuid;
    u16 dev_pid;	//support 1536(普通话,中英),1537(普通话,纯中),1737(英),1637(粤语),1837(四川话),1936(普通话远场识别)
    u16 rate;		//support 8000 or 16000
    u32 len;
    char *databuf;

    char *result;
    u32 result_len;

    httpcli_ctx ctx;
    int error_no;
    int checkin_flag;
};

extern int bdspeech_authorize(struct bdspeech_object *bd);
extern int bdspeech_transform(struct bdspeech_object *bd);
extern void bdspeech_free(struct bdspeech_object *bd);
extern void bdspeech_break(struct bdspeech_object *bd);
extern void set_bdspeech_object_timeout(struct bdspeech_object *bd, u32 ms);
extern int init_bdspeech_object(struct bdspeech_object *bd, const char *api_key,
                                const char *secret_key, const char *cuid,
                                u16 dev_pid, const char *format, u16 rate);

#endif
