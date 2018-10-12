
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>

#include "ff_epoll.h"

int ff_epoll_create(int size)
{
    printf("ff_epoll_create\n");
    return epoll_create1(EPOLL_CLOEXEC);
}

int ff_epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
    int on = 1;
    if (ioctl(fd, FIONBIO, &on) < 0)    
    {        
        printf("ioctl failed\n");
    }
    printf("ff_epoll_ctl epfd %d,op %d,fd %d\n", epfd, op, fd );
    return epoll_ctl(epfd, op, fd, event);
}

int ff_epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)
{
    int i;
    int cnt = epoll_wait(epfd, events, maxevents, timeout);    
    return cnt;
}

#ifdef __cplusplus
}
#endif


