#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>


#define MAX_EVENTS 512


int sockfd;

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

pthread_t tid[512];

void * loop(void * arg)
{
    int ret;
    struct epoll_event ev;
    struct epoll_event events[MAX_EVENTS];
    int epfd;

    epfd = epoll_create1(EPOLL_CLOEXEC);
    if ( epfd < 0 )
    {
        printf("epoll create failed\n");
        exit(1);
    }

    ev.data.fd = sockfd;
    ev.events  = EPOLLIN;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);
    if (ret < 0) 
    {
        printf("listen failed\n");
        exit(1);
    }

    for (;;)
    {
        /* Wait for events to happen */
        int nevents = epoll_wait(epfd,  events, MAX_EVENTS, 0);
        int i;
        
        for ( i = 0; i < nevents ; ++i ) 
        {
            /* Handle new connect */
            if (events[i].data.fd == sockfd) 
            {
                while (1) 
                {
                    int nclientfd = accept(sockfd, NULL, NULL);
                    if (nclientfd < 0) {
                        break;
                    }
        
                    printf("accept client fd %d.\n", nclientfd);
        
                    /* Add to event list */
                    ev.data.fd = nclientfd;
                    ev.events  = EPOLLIN;
                    if (epoll_ctl(epfd, EPOLL_CTL_ADD, nclientfd, &ev) != 0) 
                    {
                        printf("epoll_ctl failed:%d, %s\n", errno, strerror(errno));
                        break;
                    }
                }
            } 
            else
            { 
                if (events[i].events & EPOLLERR ) 
                {
                    /* Simply close socket */
                    epoll_ctl(epfd, EPOLL_CTL_DEL,  events[i].data.fd, NULL);
                    close(events[i].data.fd);
        
                    printf("connect %d close.\n", events[i].data.fd);
                } 
                else if (events[i].events & EPOLLIN )
                {
                    char buf[256];
                    size_t writelen;
                    size_t readlen = read( events[i].data.fd, buf, sizeof(buf));
                    //printf("request read len %d.\n", readlen);
                    if( readlen > 0 )
                    {
                        writelen = write( events[i].data.fd, html, sizeof(html));
                        //printf("response write len %d.\n", readlen);
                    } 
                    else 
                    {
                        epoll_ctl(epfd, EPOLL_CTL_DEL,  events[i].data.fd, NULL);
                        close( events[i].data.fd);
        
                        printf("connect %d close.\n", events[i].data.fd);
                    }
                }
                else
                {
                    printf("unknown event: %8.8X\n", events[i].events);
                }
            }
        }
    }
}

int main(int argc, char * argv[])
{
    int ret;
    int i;
    short port;
    struct epoll_event ev;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        printf("socket failed\n");
        exit(1);
    }
        
    printf("sockfd:%d\n", sockfd);
    for ( i = 0 ; i < argc ; i++ )
    {
        if ( 0 == strcmp(argv[i],"port") )
        {
            port = (short)atoi(argv[i+1]);
        }
    }

    printf("port  :%d\n", port);

    int on = 1;
    ret = ioctl(sockfd, FIONBIO, &on);
    if (ret < 0)
    {
        printf("ioctl failed\n");
        exit(1);
    }

    struct sockaddr_in my_addr;
    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    ret = bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr));
    if (ret < 0) 
    {
        printf("bind failed\n");
        exit(1);
    }

    ret = listen(sockfd, MAX_EVENTS);
    if (ret < 0) 
    {
        printf("listen failed\n");
        exit(1);
    }

    for (i = 0 ; i < 15; i++ )
    {
        pthread_create( &tid[i], NULL, loop, NULL);
    }

    loop(NULL);

    return 0;
}
