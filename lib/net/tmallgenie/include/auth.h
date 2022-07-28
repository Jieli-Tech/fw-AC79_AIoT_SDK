#ifndef __AUTH_H
#define __AUTH_H


int aligenie_set_app_key(const char *app_key);

int aligenie_set_ext(const char *ext);

int aligenie_set_schema(const char *schema);

int aligenie_set_user_id(const char *user_id);

int aligenie_set_utd_id(const char *utd_id);

int aligenie_get_authcode(char *_authcode);

#endif // __AUTH_H
