#ifndef _SS_INNER_H
#define _SS_INNER_H

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

#include "ss_api.h"
#include "ss_util.h"

#define SS_USED_FLAG  0x1
#define SS_SOCK_TYPE  0x2
#define SS_CONN_STAT  0x4

#define SS_SERVER   0x2
#define SS_CLIENT   0x0


struct ss_accept_s {
    int fd;
    struct sockaddr  addr;
    socklen_t        addrlen;    
    struct ss_accept_s * pnext;
};

struct ss_socket_m {
    int status;
    int idx;
    int ref;
    int sockfd;
    int ctl_ref;

    struct ss_accept_s * paccept;
    struct ss_buff * pread;
    struct ss_buff * pwrite;
};

struct ss_call_s {
    int call_idx;
    void * param;
    sem_t sem;
    int ret;
    int err;
    struct ss_call_s * pnext;
};

struct ss_call_que {
    pthread_mutex_t lock;
    int calls;
    struct ss_call_s * pnext;
    struct ss_call_s * ptail;
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
    SS_CALL_EPOLL_CTL,
    SS_CALL_END,
};

struct ss_parm_socket {
    int domain;
    int type;
    int protocol;
};

struct ss_parm_setsockopt {
    int s;
    int level;
    int optname;
    const void *optval;
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
    const struct sockaddr *addr;
    socklen_t addrlen;
};

struct ss_parm_connect {
    int s;
    const struct sockaddr *name;
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

struct ss_parm_epoll_ctl {
    int fd;
    int opt;
    int events;
};

extern pthread_mutex_t g_ss_lock;

extern struct ss_socket_m g_ss_socket[];

extern int g_ff_epfd;



struct ss_socket_m * ss_socket_m_alloc(void);

void ss_socket_m_free(struct ss_socket_m * p_socket);

struct ss_socket_m * ss_socket_m_get(int ss_fd);

int ss_socket_m_event(int ss_fd);

void ss_socket_call( struct ss_call_s * pcall );
void ss_setsockopt_call( struct ss_call_s * pcall );
void ss_getsockopt_call( struct ss_call_s * pcall );
void ss_listen_call( struct ss_call_s * pcall );
void ss_bind_call( struct ss_call_s * pcall );
void ss_connect_call( struct ss_call_s * pcall );
void ss_close_call( struct ss_call_s * pcall );
void ss_getpeername_call( struct ss_call_s * pcall );
void ss_getsockname_call( struct ss_call_s * pcall );
void ss_epoll_ctl_call( struct ss_call_s * pcall );


int ss_call_remote( struct ss_call_s * pcall );


#ifdef __cplusplus
}
#endif
#endif
