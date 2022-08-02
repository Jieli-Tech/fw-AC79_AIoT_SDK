#ifndef __HTTP_H__
#define __HTTP_H__
#include "list.h"
#include "os/os_api.h"
#ifdef __cplusplus
extern "C" {
#endif

#include "wrhash.h"

#define CGI_EXT ".cgi"
/*for cache use ,default is ony-day ,if not expire ,we
just result http-304 ,not modified*/
#define EXPIRE_TIME 24*60*60

#define SERVER "WebSrv\/1.1"
/*default listen port*/
#define  WEB_SRV_PORT 81

/*max pending-socket number*/
#define  WR_SOCK_BACKLOG 8

/*max heads ,http/1.1 support max:64 ,but
here we only support 40*/
#define	MAX_HTTP_HEADERS 40

/*we use it when read/write file*/
#define PAGE_SIZE 4096

/*when use GET-method ,the sum of header and url
shouldn't exceed this numberwhen use POST
the header should also could exceed this number
otherwise return http_code-400*/
#define MAX_HEADER_SIZE 2*1024

/*when use POST-method and use www-form-url-encoded
the content-length shouldn't exceed this number,
of course you can change this value ,but i suggest that
the value is not >= 64*1024
if you upload file ,this limit-size unwork ,and you sholud
write the corresponding module by yourself,and i strongly
recommend you write the code carefully to handle segment-data
problem*/
#define MAX_POST_SIZE 8*1024

/* the max size we read socket once*/
#define WR_TCP_QUAN 1460

#define BUFSIZ      512

struct stat {
    long 	int st_size;
    long 	st_mtime;
};

typedef struct {
    char *method;
    char *url;
    char *protocol;

    union {
        char *queryString;	/*for get*/
        char *postData;		/*for post*/
    };
    wr_mpool mp;
    wr_hashmap headMap;
    wr_hashmap paramMap;

    /*the file virtual_stat(if static file)*/
    struct stat st;
    void *sockhdl;
    int recvLen;
    char *curPos;
    char *paramEndPos;
} WrHttp;

struct WrContext {
    struct list_head cli_list_head;
    wr_hashmap mimeMap;
    wr_hashmap pageMap;
    volatile int ClientCnt;
    volatile int quitFlag;
    void *srv_sockhdl;
    OS_MUTEX mutex;
    int pid;

    char *index_html_fn;
};
extern struct WrContext WrContext;

struct CliContext {
    struct list_head entry;
    void *sock_hdl;
    char ipaddr[16];
    int pid;
    int quitFlag;
};

/*!!! functions export!!*/
/*defined in http.c*/
int		websrv_init(void);
int websrv_start(void);
int		websrv_uninit(void);
void websrv_disconnect_cli(char *ipaddr);
void	cgi_page_add(const char *pageName, void *);
void	*cgi_page_find(const char *pageName);
void websrv_index_html_reg(const char *fname, const char *contents, unsigned int len);


/*defined in misc.c*/
int UTF8toANSI(char *src);


/*defined in analysis.c*/
void print_header(const WrHttp *pHttp);
void print_param(const WrHttp *pHttp);
const char *get_head_info(const WrHttp *pHttp, const char *key);
const char *get_param_info(const WrHttp *pHttp, const char *key);


/*defined in request.c*/
const char *get_mime_type(const char *path);
int wr_send_msg(WrHttp *pHttp, const char *type, const char *buf, size_t len);
/*send file to pHttp->sock ,will change pHttp->filePath & st*/
int wr_send_file(WrHttp *pHttp, const char *filePath);
int wr_error_reply(const WrHttp *pHttp, int stscode);

#ifdef __cplusplus
}
#endif

#endif
