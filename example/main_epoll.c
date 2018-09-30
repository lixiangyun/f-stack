#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>

#include "ff_config.h"
#include "ff_api.h"
#include "ff_epoll.h"



#define MAX_EVENTS 512


char html[] = 
"HTTP/1.1 200 OK\r\n"
"Server: F-Stack\r\n"
"Date: Sat, 25 Feb 2017 09:26:33 GMT\r\n"
"Content-Type: text/html\r\n"
"Content-Length: 439\r\n"
"Last-Modified: Tue, 21 Feb 2017 09:44:03 GMT\r\n"
"Connection: keep-alive\r\n"
"Accept-Ranges: bytes\r\n"
"\r\n"
"<!DOCTYPE html>\r\n"
"<html>\r\n"
"<head>\r\n"
"<title>Welcome to F-Stack!</title>\r\n"
"<style>\r\n"
"    body {  \r\n"
"        width: 35em;\r\n"
"        margin: 0 auto; \r\n"
"        font-family: Tahoma, Verdana, Arial, sans-serif;\r\n"
"    }\r\n"
"</style>\r\n"
"</head>\r\n"
"<body>\r\n"
"<h1>Welcome to F-Stack!</h1>\r\n"
"\r\n"
"<p>For online documentation and support please refer to\r\n"
"<a href=\"http://F-Stack.org/\">F-Stack.org</a>.<br/>\r\n"
"\r\n"
"<p><em>Thank you for using F-Stack.</em></p>\r\n"
"</body>\r\n"
"</html>";

int sockfdlist[10] = {-1};
int sockfdcnt = 0;

int epfd;
int flag = 0;


int newsocket( int port )
{
    int ret;
    int sockfd = -1;
    
    sockfd = ff_socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        printf("ff_socket failed\n");
        return -1;
    }

    int on = 1;
    ret = ff_ioctl(sockfd, FIONBIO, &on);
    if (ret < 0) 
    {
        printf("ff_bind failed\n");
        ff_close(sockfd);
        return -1;
    }

    struct sockaddr_in my_addr;
    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    ret = ff_bind(sockfd, (struct linux_sockaddr *)&my_addr, sizeof(my_addr));
    if (ret < 0) 
    {
        printf("ff_bind failed\n");
        ff_close(sockfd);
        return -1;
    }

    ret = ff_listen(sockfd, MAX_EVENTS);
    if (ret < 0) 
    {
        printf("ff_listen failed\n");
        ff_close(sockfd);
        return -1;
    }

    printf("sockfd:%d, port:%d\n", sockfd, port);

    sockfdlist[sockfdcnt] = sockfd;
    sockfdcnt++;

    return sockfd;
}

int checksockfd(int sockfd)
{
    for(int i = 0 ; i < sockfdcnt ; i++ )
    {
        if ( sockfd == sockfdlist[i] )
        {
            return sockfd;
        }
    }
    return -1;
}

void epolladd(int sockfd)
{
    int ret;
    struct epoll_event ev;
    
    ev.data.fd = sockfd;
    ev.events  = EPOLLIN | EPOLLERR;
    ret = ff_epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);
    if ( ret < 0 )
    {
        printf("ff_epoll_ctl failed:%d, %s\n", errno, strerror(errno));
    }
}

int loop(void *arg)
{
    int i;
    int sockfd;
    int nevents;
    struct epoll_event ev;
    struct epoll_event events[MAX_EVENTS];

    /*
    if ( 10000 == flag )
    {
        sockfd = newsocket(81);
        if ( sockfd > 0 )
        {
            epolladd(sockfd);
        }
    }*/

    flag++;
    
    nevents = ff_epoll_wait(epfd, events, MAX_EVENTS, 0);
    for ( i = 0; i < nevents ; ++i ) 
    {
        sockfd = checksockfd(events[i].data.fd);
        if ( sockfd != -1 ) 
        {
            while (1) 
            {
                int nclientfd;
                nclientfd = ff_accept(sockfd, NULL, NULL);
                if (nclientfd < 0) 
                {
                    break;
                }

                printf("accept client fd %d.\n", nclientfd);
                epolladd(nclientfd);
            }
        } 
        else
        {
            if (events[i].events & EPOLLERR ) 
            {
                /* Simply close socket */
                ff_epoll_ctl(epfd, EPOLL_CTL_DEL,  events[i].data.fd, NULL);
                ff_close(events[i].data.fd);

                printf("connect error %d close.\n", events[i].data.fd);
            } 
            else if (events[i].events & EPOLLIN )
            {
                char buf[4096];
                size_t writelen;
                size_t readlen = ff_read( events[i].data.fd, buf, sizeof(buf));
                if( readlen > 0 )
                {
                    writelen = ff_write( events[i].data.fd, html, sizeof(html) );
                } 
                else 
                {
                    ff_epoll_ctl(epfd, EPOLL_CTL_DEL,  events[i].data.fd, NULL);
                    ff_close( events[i].data.fd);

                    printf("connect read %d close.\n", events[i].data.fd);
                }
            }
            else
            {
                printf("unknown event: %8.8X\n", events[i].events);
            }
        }
    }
}

int main(int argc, char * argv[])
{
    int i;
    short port;
    int  sockfd;
    
    ff_init(argc, argv);

    for ( i = 0 ; i < argc ; i++ )
    {
        if ( 0 == strcmp(argv[i],"port") )
        {
            port = (short)atoi(argv[i+1]);
        }
    }

    epfd = ff_epoll_create(0);
    if (epfd < 0)
    {
        printf("ff_epoll_create failed\n");
        return -1;
    }
    
    sockfd = newsocket(port);
    if (sockfd < 0)
    {
        printf("ff_socket failed\n");
        return -1;
    }

    epolladd(sockfd);

    ff_run(loop, NULL);

    return 0;
}
