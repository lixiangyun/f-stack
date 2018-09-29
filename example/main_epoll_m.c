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

#define SOCK_MAX_NUM 65535

#define BUFF_MAX_LEN 4000

struct ss_buff {
    char   body[BUFF_MAX_LEN];
    int    write;
    int    read;
    int    total;
    struct ss_buff * pnext;
};

enum ss_socket_type {
    SS_UNUSED,
    SS_SERVER,
    SS_CLIENT,
};

struct ss_socket_m {
    pthread_mutex_t lock;
    
    int socket_type;
    int socket_idx;

    int event_fd;
    int event_flag;

    int accept_idx[512];
    int accept_num;

    int socket_ff;
    
    struct ss_buff * pnext;
    struct ss_buff * ptail;
};

struct ss_call {
    int call_idx;
    void * param;
    sem_t sem;
    int ret;
    int errno;
    struct ss_call * pnext;
};

struct ss_call_que {
    pthread_mutex_t lock;
    int calls;
    struct ss_call * pnext;
    struct ss_call * ptail;
};

enum ss_call_idx {
    SS_CALL_SOCKET,
    SS_CALL_SETSOCKOPT,
    SS_CALL_GETSOCKOPT,
    SS_CALL_LISTEN,
    SS_CALL_BIND,
    SS_CALL_CONNECT,
    SS_CALL_CLOSE,
    SS_CALL_GETPEERNAME,
    SS_CALL_GETSOCKNAME,    
};


int g_ff_2_ss[SOCK_MAX_NUM] = {0};

struct ss_socket_m g_ss_socket[SOCK_MAX_NUM] = {{PTHREAD_MUTEX_INITIALIZER, 0,0,0,0, {0},0, 0, NULL,NULL}};

struct ss_call_que g_ss_call_que = {PTHREAD_MUTEX_INITIALIZER,0,NULL,NULL};

void ss_proccess_once( struct ss_call * pcall )
{
    int ret;

    switch (pcall->call_idx)
    {
        case SS_CALL_SOCKET:
        {

        }break;
        case SS_CALL_SETSOCKOPT:
        {
            
        }break;
        case SS_CALL_GETSOCKOPT:
        {
            
        }break;
        case SS_CALL_LISTEN:
        {
            
        }break;
        case SS_CALL_BIND:
        {
            
        }break;
        case SS_CALL_CONNECT:
        {
            
        }break;
        case SS_CALL_CLOSE:
        {
            
        }break;
        case SS_CALL_GETPEERNAME:
        {
            
        }break;
        case SS_CALL_GETSOCKNAME:
        {
            
        }break;

        default:
        {
            printf("call index is invaild! %d\n", pcall->call_idx );
            return;
        }
    }

    // 


    ret = sem_post(&pcall->sem);
    if ( ret < 0 )
    {
        printf("sem post failed!\n")
    }
}

void ss_proccess_call()
{
    struct ss_call * pcall;
    if ( 0 == g_ss_call_que.calls )
    {
        return;
    }

    pthread_mutex_lock(&g_ss_call_que.lock);
    pcall = g_ss_call_que.pnext;
    g_ss_call_que.pnext = NULL;
    g_ss_call_que.ptail = NULL;
    g_ss_call_que.calls = 0;
    pthread_mutex_unlock(&g_ss_call_que.lock);

    for ( ; pcall != NULL ; pcall = pcall->pnext )
    {
        ss_proccess_once(pcall);
    }
}



int ss_remote_call( struct ss_call * pcall )
{
    int ret;
    
    ret = sem_init(&pcall->sem, 0, 0);
    if ( ret < 0 )
    {
        printf("sem init failed!")
        return -1;
    }

    pcall->pnext = NULL;

    pthread_mutex_lock(&g_ss_call_que.lock);
    if ( g_ss_call_que.ptail != NULL )
    {
        g_ss_call_que.ptail->pnext = pcall;
        g_ss_call_que.ptail = pcall;
    }
    else
    {
        g_ss_call_que.ptail = pcall;
        g_ss_call_que.pnext = pcall;
    }
    g_ss_call_que.calls++;
    pthread_mutex_unlock(&g_ss_call_que.lock);

    ret = sem_wait(&pcall->sem);
    if ( ret < 0 )
    {
        printf("sem wait failed!")
        return -1;
    }

    ret = pcall->ret;

    sem_destroy(&pcall->sem);

    return ret;
}


struct ss_parm_socket {
    int domain;
    int type;
    int protocol;
};

struct ss_parm_setsockopt {
    int s;
    int level;
    int optname;
    void *optval;
    socklen_t optlen;
};

struct ss_parm_getsockopt {
    int s;
    int level;
    int optname;
    void *optval;
    socklen_t *optlen;
};

struct ss_parm_listen {
    int s;
    int backlog;
};

struct ss_parm_bind {
    int s;
    struct sockaddr *addr;
    socklen_t addrlen;
};

struct ss_parm_connect {
    int s;
    struct sockaddr *name;
    socklen_t namelen;
};


struct ss_parm_close {
    int fd;
};

struct ss_parm_getpeername {
    int s;
    struct sockaddr *name;
    socklen_t *namelen;
};

struct ss_parm_getsockname {
    int s;
    struct sockaddr *name;
    socklen_t *namelen;
};


int ss_socket(int domain, int type, int protocol);

int ss_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen);

int ss_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen);

int ss_listen(int s, int backlog);

int ss_bind(int s, const struct sockaddr *addr, socklen_t addrlen);

int ss_connect(int s, const struct sockaddr *name, socklen_t namelen);

int ss_close(int fd);

int ss_getpeername(int s, struct sockaddr *name, socklen_t *namelen);

int ss_getsockname(int s, struct sockaddr *name, socklen_t *namelen);







int ss_accept(int s, struct sockaddr *addr, socklen_t *addrlen);



ssize_t ss_read(int d, void *buf, size_t nbytes);

ssize_t ss_readv(int fd, const struct iovec *iov, int iovcnt);

ssize_t ss_write(int fd, const void *buf, size_t nbytes);

ssize_t ss_writev(int fd, const struct iovec *iov, int iovcnt);



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
