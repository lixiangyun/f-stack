
#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
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

#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>

#include "ff_api.h"


int ff_init(int argc, char * const argv[])
{
    printf("ff_init\n");
    return 0;    
}

void ff_run(loop_func_t loop, void *arg)
{
    printf("ff_run\n");
    for(;;)
    {
        loop(arg);
    }
}

int ff_socket(int domain, int type, int protocol)
{
    printf("ff_socket %d\n", domain);
    return socket(domain,type,protocol);
}

int ff_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen)
{
    printf("ff_setsockopt fd %d\n", s);
    return setsockopt(s, level, optname, optval, optlen);
}

int ff_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen)
{
    printf("ff_getsockopt fd %d\n", s);
    return getsockopt(s, level, optname, optval, optlen);
}

int ff_listen(int s, int backlog)
{
    printf("ff_listen fd %d\n", s);
    return listen(s, backlog);
}

int ff_bind(int s, const struct linux_sockaddr *addr, socklen_t addrlen)
{
    printf("ff_bind fd %d\n", s);
    return bind(s, addr, addrlen );
}

int ff_accept(int s, struct linux_sockaddr *addr, socklen_t *addrlen)
{
    printf("ff_accept fd %d\n", s);
    return accept(s, addr, addrlen);
}

int ff_connect(int s, const struct linux_sockaddr *name, socklen_t namelen)
{
    printf("ff_connect fd %d\n", s);
    return connect(s, name, namelen);
}

int ff_close(int fd)
{
    printf("ff_close fd %d\n", fd);
    return close(fd);
}

int ff_getpeername(int s, struct linux_sockaddr *name, socklen_t *namelen)
{
    printf("ff_getpeername fd %d\n", s);
    return getpeername(s, name, namelen);
}

int ff_getsockname(int s, struct linux_sockaddr *name, socklen_t *namelen)
{
    printf("ff_getsockname fd %d\n", s);
    return getsockname(s, name, namelen);
}

ssize_t ff_read(int d, void *buf, size_t nbytes)
{
    ssize_t cnt = read(d,buf,nbytes);
    //printf("ff_read fd %d, size %lu\n", d, cnt);
    return cnt;
}

ssize_t ff_readv(int fd, const struct iovec *iov, int iovcnt)
{
    //printf("ff_readv fd %d\n", fd);
    return readv(fd,iov,iovcnt);
}

ssize_t ff_write(int fd, const void *buf, size_t nbytes)
{
    ssize_t cnt = write(fd,buf,nbytes);
    //printf("ff_write fd %d, size %lu\n", fd, cnt);
    return cnt;
}

ssize_t ff_writev(int fd, const struct iovec *iov, int iovcnt)
{
    //printf("ff_writev fd %d\n", fd);
    return writev(fd,iov,iovcnt);
}

#ifdef __cplusplus
}
#endif

