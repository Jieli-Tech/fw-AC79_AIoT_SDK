#ifndef _MEDIA_AVMODULE_H_
#define _MEDIA_AVMODULE_H_

#define __AUDIO_MODULE_EXPORT(n, module) \
    int n##_##module##_probe(void) \
    { \
        return 1; \
    }


#define AUDIO_DECODER_EXPORT(name) \
    __AUDIO_MODULE_EXPORT(name, decoder)

#define AUDIO_ENCODER_EXPORT(name) \
    __AUDIO_MODULE_EXPORT(name, encoder)

#define AUDIO_HWACCEL_EXPORT(name) \
    __AUDIO_MODULE_EXPORT(name, hwaccel)


#define __AUDIO_MODULE_PROBE(x, module) \
    ({ \
        int ret; \
        extern int x##_##module##_probe(); \
        ret = x##_##module##_probe(); \
        printf("module probe : %s_%s\n", #x, #module);\
        ret; \
    })

/*--no-whole-archive时，调用probe函数进行链接文件*/
#define AUDIO_DECODER_PROBE(name) \
    __AUDIO_MODULE_PROBE(name, decoder)

#define AUDIO_ENCODER_PROBE(name) \
    __AUDIO_MODULE_PROBE(name, encoder)

#define AUDIO_PACKAGE_PROBE(name) \
    __AUDIO_MODULE_PROBE(name, package)

#define AUDIO_HWACCEL_PROBE(name) \
    __AUDIO_MODULE_PROBE(name, hwaccel)


#endif
