#ifndef __IOT_RSA_H__
#define __IOT_RSA_H__

#ifdef __cplusplus
extern "C" {
#endif

int speech_rsa_encrypt(const char *in, int in_size, char *out);
int speech_rsa_decrypt(const char *in, int in_size, char *out, int out_max_size);

#ifdef __cplusplus
}
#endif

#endif /* !__IOT_RSA_H__ */
