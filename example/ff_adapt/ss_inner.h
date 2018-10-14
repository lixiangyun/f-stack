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



enum ss_socket_type {
    SS_UNUSED,
    SS_SERVER,
    SS_CLIENT,
};

struct ss_accept_s {
    int socket_ff;
    struct ss_accept_s * pnext;
};

struct ss_socket_m {

    int status;
    int idx;

    struct ss_accept_s * paccept;
    int socketfd;

    struct ss_buff_m buff_r;
    struct ss_buff_m buff_w;
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
    SS_CALL_EPOLL_CTL
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

struct ff_event_data {
    int ff_socket_fd;
    int ff_events;
    struct ss_socket_m * ss_socket_s;
    void (* ff_callback)(struct ff_event_data *);
};

struct ss_parm_epoll_ctl {
    int ff_opt;
    int ff_events;
    int ff_socket_fd;
    struct ff_event_data * ff_event_data;
};



extern pthread_mutex_t g_ss_lock;
extern struct ss_socket_m g_ss_socket[SOCK_MAX_NUM];


extern struct ss_call_que g_ss_call_que;
extern int g_ff_epfd;




int ss_remote_call( struct ss_call_s * pcall );


#ifdef __cplusplus
}
#endif
#endif
