#ifndef _F_STACK_API_H
#define _F_STACK_API_H

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

typedef int (*loop_func_t)(void *arg);

#define linux_sockaddr sockaddr

int ff_init(int argc, char * const argv[]);

void ff_run(loop_func_t loop, void *arg);




int ff_socket(int domain, int type, int protocol);

int ff_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen);

int ff_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen);

int ff_listen(int s, int backlog);

int ff_bind(int s, const struct linux_sockaddr *addr, socklen_t addrlen);

int ff_accept(int s, struct linux_sockaddr *addr, socklen_t *addrlen);

int ff_connect(int s, const struct linux_sockaddr *name, socklen_t namelen);

int ff_close(int fd);

int ff_getpeername(int s, struct linux_sockaddr *name, socklen_t *namelen);

int ff_getsockname(int s, struct linux_sockaddr *name, socklen_t *namelen);



ssize_t ff_read(int d, void *buf, size_t nbytes);

ssize_t ff_readv(int fd, const struct iovec *iov, int iovcnt);

ssize_t ff_write(int fd, const void *buf, size_t nbytes);

ssize_t ff_writev(int fd, const struct iovec *iov, int iovcnt);


#ifdef __cplusplus
}
#endif
#endif
