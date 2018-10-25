#ifndef _SS_API_H
#define _SS_API_H

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

#include "ss_util.h"

#define SS_BUFF_MAX_LEN   (2*1024*1024)

struct ss_buff {
    int    read;
    int    write;
    char   body[SS_BUFF_MAX_LEN];
};


#define SS_MAX_EVENTS  32

#define SOCK_MAX_NUM   0xffff

#define SOCK_FD_MASK   0xffff

#define SOCK_REL_IDX   0xff00

#define EPOLL_MAX_NUM  0xff

struct ss_buff * ss_buff_alloc(void);

void ss_buff_free(struct ss_buff * pbuff);

ssize_t ss_buff_size(struct ss_buff * pbuff);
ssize_t ss_buff_space(struct ss_buff * pbuff);


/* buffer ¶ÁÐ´½Ó¿Ú */
ssize_t ss_buff_read(struct ss_buff * pbuff, char *buf, size_t nbytes);
ssize_t ss_buff_readv(struct ss_buff * pbuff, struct iovec *iov, int iovcnt);

ssize_t ss_buff_write(struct ss_buff * pbuff, char *buf, size_t nbytes);
ssize_t ss_buff_writev(struct ss_buff * pbuff, struct iovec *iov, int iovcnt);



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


int ss_epoll_create(int size);
int ss_epoll_ctl(int epfd, int op, int fd, struct epoll_event * pevent);
int ss_epoll_wait(int epfd, struct epoll_event * pevents, int maxevents, int timeout);

void ss_run(void);
int ss_init(int argc, char * argv[]);




#ifdef __cplusplus
}
#endif
#endif
