#ifndef _VSARM_USER_SETTINGS_H_
#define _VSARM_USER_SETTINGS_H_



#define HAVE_PBKDF2
#define WOLFSSL_AES_DIRECT
#define  HAVE_AES_KEYWRAP
#define WOLFSSL_CMAC
#define WOLFSSL_KEY_GEN
#define CONFIG_ECC
#define HAVE_COMP_KEY
#define HAVE_INTEL_RDRAND
#define IS_INTEL_RDRAND(f) 1
#define CONFIG_NO_RC4

/* Enables blinding mode, to prevent timing attacks */
#define WC_RSA_BLINDING

#define WOLFSSL_SHA384             //启用S​​HA-384支持
#define WOLFSSL_SHA512             //启用S​​HA-512支持
//#define NO_SHA256                   //禁用SHA-256支持

//----------------------------------------------------------------
#define HAVE_EXTENDED_MASTER
#define HAVE_TLS_EXTENSIONS
#define HAVE_SUPPORTED_CURVES
#define HAVE_AESGCM                //启用AES-GCM支持。
#define USE_FAST_MATH
#define ECC_TIMING_RESISTANT
#define NO_HC128                //从构建中删除流密码扩展
//----------------------------------------------------------------

#define NO_RABBIT               //从构建中删除流密码扩展
#define NO_DSA                  //由于 DSA已被淘汰，NO_DSA删除了它。
//#define NO_MD4                  //从构建中删除了MD4，MD4损坏并且不应使用。
#define HAVE_ECC
#define HAVE_HASHDRBG
//#define NO_WOLFSSL_CLIENT      //删除特定于客户端的调用，并且仅用于服务器版本。如果您出于大小考虑要删除一些呼叫，则仅应使用此选项。定义后可减1k
#define NO_WOLFSSL_SERVER       //删除特定于服务器端的调用

#if 0
#define SMALL_SESSION_CACHE
#else
#define NO_SESSION_CACHE        //来限制wolfSSL使用的SSL会话缓存的大小。这会将默认会话缓存从33个会话减少到6个会话，并节省约2.5 kB
#endif

#undef  WOLFSSL_SMALL_STACK
#define WOLFSSL_SMALL_STACK      //可以用于堆栈大小小的设备。这会增加Wolfcrypt / src / integer.c中动态内存的使用，但会导致性能降低。

#ifdef USE_FAST_MATH
#undef  TFM_TIMING_RESISTANT
#define TFM_TIMING_RESISTANT     //可使用快速数学（当被定义USE_FAST_MATH上系统）具有小的堆栈大小。这将摆脱大型静态数组
#endif

//#undef  NO_FILESYSTEM
//#define NO_FILESYSTEM

#undef  NO_WRITEV
#define NO_WRITEV             //禁用对writev（） 语义的模拟

//#undef  NO_MAIN_DRIVER
//#define NO_MAIN_DRIVER      //在正常的构建环境中，使用NO_MAIN_DRIVER来确定是单独调用测试应用程序还是通过测试套件驱动程序应用程序调用。

//#undef  NO_PWDBASED
//#define NO_PWDBASED    //禁用基于密码的密钥派生功能，例如PKCS＃12中的PBKDF1，PBKDF2和PBKDF

//#define USE_CERT_BUFFERS_1024    //启用位于<wolfssl_root> /wolfssl/certs_test.h中的1024位测试证书和密钥缓冲区。在没有文件系统的嵌入式系统上进行测试和移植时很有用。
//#define USE_CERT_BUFFERS_2048      //启用<wolfssl_root> /wolfssl/certs_test.h中的2048位测试证书和密钥缓冲区。在没有文件系统的嵌入式系统上进行测试和移植时很有用



//#undef DEBUG_WOLFSSL


#define WOLFSSL_USER_MUTEX
//#define WOLFSSL_USER_FILESYSTEM
#define SINGLE_THREADED  //是一个关闭互斥量使用的开关。wolfSSL当前仅将一个用于会话高速缓存。如果对wolfSSL的使用始终是单线程的，则可以将其打开
#define NO_WOLFSSL_DIR
#define NO_FILESYSTEM
#define NO_DEV_RANDOM

//#define WOLFSSL_CERT_GEN

#define IGNORE_NAME_CONSTRAINTS
//#define NO_CERTS
#define XSTRNCASECMP
//#define CTYPE_USER
#define XFILE      void*
#define XBADFILE   NULL
#define XFOPEN(...) XBADFILE
#define XFREAD(...)      0
#define XFCLOSE(...)


#undef  WOLFSSL_GENERAL_ALIGNMENT
#define WOLFSSL_GENERAL_ALIGNMENT   4

#define HAVE_AES_CBC

//------------------not sure---------------------
// #define TFM_TIMING_RESISTANT
// #define WOLFSSL_SP_SMALL

// #define NO_SHA512                   //禁用SHA-512支持
// #define NO_SHA384                   //禁用SHA-512支持

// #define NO_PSK                  //关闭预共享密钥扩展的使用。默认情况下是内置的。定义后可减7K
// #define NO_DES3                 //删除了对DES3加密的使用。默认情况下内置DES3，因为某些较旧的服务器仍在使用它，而SSL 3.0则需要它。
// #define NO_RC4                  //从构建中删除对ARC4流密码的使用。默认情况下，ARC4是内置的，因为它仍然很流行并且被广泛使用。定义后可减5k
// #define WOLFSSL_SHA3_SMALL
//#define NO_RSA                  //从构建中删除对ARC4流密码的使用。默认情况下，ARC4是内置的，因为它仍然很流行并且被广泛使用。定义后可减5k
//------------------not sure---------------------

#endif /* _VSARM_USER_SETTINGS_H_ */
