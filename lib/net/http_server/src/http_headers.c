#include "http_server.h"
#include "lwip/sockets.h"

#define BAD_REQUEST_RSP \
"HTTP/1.1 400 Bad Request\r\n"\
"Content-type: text/html\r\n"\
"Connection: close\r\n"\
"\r\n"\
"<html><title>Bad Request</title><body><p>Your browser sent a bad request</p></body></html>"


#define NOT_IMPLEMENTED_RSP \
"HTTP/1.1 501 Not Implemented\r\n"\
"Content-type: text/html\r\n"\
"Connection: close\r\n"\
"\r\n"\
"<html><title>Not Implemented</title><body><p>The server either does not recognise the request method, or it lacks the ability to fulfill the request.</p></body></html>"


#define FORBIDDEN_RSP \
"HTTP/1.1 403 Forbidden\r\n"\
"Content-type: text/html\r\n"\
"Connection: close\r\n"\
"\r\n"\
"<html><title>Forbidden</title><body><p>You don't have acces to this content</p></body></html>"


#define NOT_FOUND \
"HTTP/1.1 404 File Not Found\r\n"\
"Content-Type: text/html\r\n"\
"Connection: close\r\n"\
"\r\n"\
"<html><title>File Not Found</title><body><p>The server could not find the resourse</p></body></html>"


#define FILE_FOUND_DIR_200 \
"HTTP/1.1 200 OK\r\n"\
"Content-Type: %s\r\n"\
"Connection: close\r\n"\
"\r\n"


#define FILE_FOUND_200 \
"HTTP/1.1 200 OK\r\n"\
"Content-Type: %s\r\n"\
"Content-Length: %d\r\n"\
"Accept-Ranges: bytes\r\n"\
"Connection: close\r\n"\
"\r\n"


#define FILE_FOUND_206 \
"HTTP/1.1 206 Partial Content\r\n"\
"Content-Type: %s\r\n"\
"Content-Length: %d\r\n"\
"Accept-Ranges: bytes\r\n"\
"Content-Range: bytes %d-%d/%d\r\n"\
"Connection: close\r\n"\
"\r\n"


#define UNEXPECTED_END_OF_TRANSMISSION "Error unexpected end of transmission"

void unexpected_end_tran(void *sock_hdl)
{
    sock_send(sock_hdl, UNEXPECTED_END_OF_TRANSMISSION, strlen(UNEXPECTED_END_OF_TRANSMISSION), 0);
}

void badRequest(void *sock_hdl)
{
    sock_send(sock_hdl, BAD_REQUEST_RSP, strlen(BAD_REQUEST_RSP), 0);
}

void notImplemented(void *sock_hdl)
{
    sock_send(sock_hdl, NOT_IMPLEMENTED_RSP, strlen(NOT_IMPLEMENTED_RSP), 0);
}

void forbidden(void *sock_hdl)
{
    sock_send(sock_hdl, FORBIDDEN_RSP, strlen(FORBIDDEN_RSP), 0);
}

void notFound(void *sock_hdl)
{
    sock_send(sock_hdl, NOT_FOUND, strlen(NOT_FOUND), 0);
}

void fileFound(void *sock_hdl, char *cType, int content_len, int lowRange, int highRange)
{
    char buf[512];

    if (content_len == 0) {
        sprintf(buf, FILE_FOUND_DIR_200, cType);
    } else {
        if (lowRange == 0 && highRange == 0) {
            sprintf(buf, FILE_FOUND_200, cType, content_len);
        } else {
            int file_len = content_len;
            if (highRange == -1) {
                highRange =  content_len - 1;
                content_len = content_len - lowRange;
            } else {
                content_len = highRange - lowRange + 1;
            }
            sprintf(buf, FILE_FOUND_206, cType, content_len, lowRange, highRange, file_len);
        }
    }

    printf("FILE_FOUND: %s \r\n", buf);
    sock_send(sock_hdl, buf, strlen(buf), 0);
}
