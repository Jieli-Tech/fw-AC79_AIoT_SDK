#include "http_server.h"
#include "lwip/sockets.h"
#include "sock_api.h"
#include "fs/fs.h"
#include "stdlib.h"

enum FD_TYPES {
    FILES,
    DIRS,
    ERROR,
};

#define HDR_URI_SZ      2048
#define HDR_METODE_SZ   5
#define HDR_VERSION_SZ  10

struct headers {
    char version[HDR_VERSION_SZ];
    char metode[HDR_METODE_SZ];
    char uri[HDR_URI_SZ];
    int lowRange;
    int highRange;
};

char *__attribute__((weak)) http_server_cType_ext(char *fileName)
{
    return ("text/plain; charset=UTF-8");
}

static char *cType(char *fileName)
{
    char *c = strrchr(fileName, '.');

    if (c == NULL) {
        return http_server_cType_ext(fileName);
    }

    char ext[5];
    memset(ext, 0, 5);
    strncpy(ext, c + 1, 4);

    if (!strcmp(ext, "htm") || !strcmp(ext, "html")) {
        return ("text/html; charset=UTF-8");
    } else if (!strcmp(ext, "jpg") || !strcmp(ext, "jpeg") || !strcmp(ext, "JPG") || !strcmp(ext, "JPEG")) {
        return ("image/jpeg");
    } else if (!strcmp(ext, "gif") || !strcmp(ext, "GIF")) {
        return ("image/gif");
    } else if (!strcmp(ext, "png") || !strcmp(ext, "PNG")) {
        return ("image/png");
    } else if (!strcmp(ext, "txt") || !strcmp(ext, "TXT")) {
        return ("text/plain; charset=UTF-8");
    } else if (!strcmp(ext, "mov") || !strcmp(ext, "MOV") || !strcmp(ext, "mp4") || !strcmp(ext, "MP4") || !strcmp(ext, "avi") || !strcmp(ext, "AVI")) {
        return ("video/quicktime");
    } else {
        return http_server_cType_ext(fileName);
    }
}

static int requestMethod(char *method, void *sock_hdl)
{
    int OK = 1;

    if (strcmp("GET", method)) {
        if (!strcmp("POST", method)) {
            puts("requestMethod notImplemented\n");
            notImplemented(sock_hdl);
        } else {
            printf("requestMethod badRequest:%s\n", method);
            badRequest(sock_hdl);
        }

        OK = -1;
    }

    return (OK);
}

static int getHeader(struct headers *hdr, void *sock_hdl)
{
    char *token = NULL;
    char requestBuffer[MAX_HTTP_BUFFER];
    int numbytes;
    char *find_n = NULL;
    char *find_p = NULL;

    if ((numbytes = sock_recv(sock_hdl, requestBuffer, sizeof(requestBuffer) - 1, 0)) == -1) {
        puts("Request is not received\n");
        return -1;
    }

    if (numbytes == 0) {
//        puts("No messages are available or peer has performed an orderly shutdown.\n");
        return -1;
    }

    requestBuffer[numbytes] = '\0';

    hdr->lowRange = hdr->highRange = 0;
    token = strstr(requestBuffer, "Range: bytes=");
    if (token) {
        hdr->lowRange = atoi(token + strlen("Range: bytes="));

        hdr->highRange = -1;
        if (!strstr(token, "-\r\n")) {
            hdr->highRange = atoi(strstr(token, "-") + strlen("-"));
        }
    }

    find_p = requestBuffer;

    memset(&hdr->metode, 0, sizeof(hdr->metode));
    find_n = strchr(find_p, ' ');
    if (!find_n) {
        return -1;
    }
    *(find_n++) = 0;
    strncpy(hdr->metode, find_p, sizeof(hdr->metode));
    hdr->metode[sizeof(hdr->metode) - 1] = 0;
    find_p = find_n;

    memset(&hdr->uri, 0, sizeof(hdr->uri));
    find_n = strchr(find_p, ' ');
    if (!find_n) {
        return -1;
    }
    *(find_n++) = 0;
    strncpy(hdr->uri, find_p, sizeof(hdr->uri));
    hdr->uri[sizeof(hdr->uri) - 1] = 0;
    find_p = find_n;

    memset(&hdr->version, 0, sizeof(hdr->version));
    find_n = strchr(find_p, '\r');
    if (!find_n) {
        return -1;
    }
    *(find_n++) = 0;
    strncpy(hdr->version, find_p, sizeof(hdr->version));
    hdr->version[sizeof(hdr->version) - 1] = 0;
    find_p = find_n;

    printf("http getHeader: %s, %s, %s, %d, %d\n",  hdr->metode, hdr->uri, hdr->version, hdr->lowRange, hdr->highRange);

    return 0;
}

static int getFDType(char *path)
{
    u8  is_dir;

    enum FD_TYPES fd;

    if (http_fattrib_isidr(path, &is_dir) == 0) {
        if (is_dir) {
            fd = DIRS;
        } else {
            fd = FILES;
        }
    } else {
        fd = ERROR;
    }

    return (fd);
}

static int sendFile(char *file, void *sock_hdl, int lowRange, int highRange)
{
    int buf_size, sz, ret = 0;
    http_file_hdl *pFile;
    char *buffer;
    int ret1 = 0;
    int range_len = 0;
    u32 http_send_len = 0;

    if ((pFile = http_fopen(file, "r")) == NULL) {
        unexpected_end_tran(sock_hdl);
        return -1;
    }

    fileFound(sock_hdl, cType(file), http_flen(pFile), lowRange, highRange);

    buf_size = 4 * 1024;
    buffer = (char *)malloc(buf_size);
    if (buffer == NULL) {
        buf_size = 1460;
        buffer = (char *)malloc(buf_size);
        if (buffer == NULL) {
            http_fclose(pFile);
            return -1;
        }
    }

    if (lowRange) {
        http_fseek(pFile, lowRange, SEEK_SET);
    }
    if (highRange && highRange != - 1) {
        range_len =  highRange - lowRange;
    }

    while ((sz = http_fread(buffer, 1, buf_size, pFile)) > 0) {
        if (sock_send(sock_hdl, buffer, sz, 0) <= 0) {
            puts("sendFile sock error,exit!\n");
            ret = -1;
            break;
        }
        http_send_len += sz;
        if (range_len && (http_send_len >= range_len)) {
            printf("http cli range_len reach %d, %d  \r\n", range_len, http_send_len);
            break;
        }
    }

    free(buffer);

    http_fclose(pFile);

    return ret;
}

extern void http_cli_thread_kill(struct http_cli_t *cli);

void acceptPetition(struct http_cli_t *cli)
{
    int ret;
    struct headers hdr;

    ret = getHeader(&hdr, cli->sock_hdl);
    if (ret < 0) {
        goto EXIT;
    }

    if (hdr.uri[0] == '\0') {
        puts("getHeader.uri  badRequest\n");
        badRequest(cli->sock_hdl);
        goto EXIT;
    }

    if (requestMethod(hdr.metode, cli->sock_hdl) == -1) {
        notImplemented(cli->sock_hdl);
        goto EXIT;
    }

    /*printf("hdr.uri = %s\n", hdr.uri);*/

    if (sendFile(hdr.uri + 1, cli->sock_hdl, hdr.lowRange, hdr.highRange) == -1) {
        goto EXIT;
    }

EXIT:
//    puts("acceptPetition exit!\n");

    http_cli_thread_kill(cli);
}
