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

#include "ff_config.h"
#include "ff_api.h"
#include "ff_epoll.h"

#include "ss_api.h"
#include "ss_inner.h"


struct ss_socket_m g_ss_socket[SOCK_MAX_NUM];

struct ss_socket_m * ss_socket_m_alloc(void)
{
    int i;
    struct ss_socket_m * p_ss_socket;

    for ( i = SOCK_REL_IDX ; i < SOCK_MAX_NUM ; i++ )
    {
        p_ss_socket = (struct ss_socket_m *)&g_ss_socket[i];
        if ( p_ss_socket->socket_type == SS_UNUSED )
        {
            break;
        }
    }
    if ( i == SOCK_MAX_NUM )
    {
        return NULL;
    }

    p_ss_socket->socket_type = SS_CLIENT;
    p_ss_socket->event_s = 0;
    p_ss_socket->paccept = NULL;

    ss_buff_m_clean(&p_ss_socket->buff_r);
    ss_buff_m_clean(&p_ss_socket->buff_w);

    return p_ss_socket;
}

void ss_socket_m_free(struct ss_socket_m * p_ss_socket)
{
    if ( p_ss_socket->socket_type == SS_UNUSED )
    {
        return;
    }
    p_ss_socket->socket_type = SS_UNUSED;

    ss_buff_m_clean(&p_ss_socket->buff_r);
    ss_buff_m_clean(&p_ss_socket->buff_w);
}

int ss_socket_remote(struct ss_parm_socket * parm)
{
    int i;
    int fd;
    struct ss_socket_m * p_ss_socket;

    p_ss_socket = ss_socket_m_alloc();
    if ( p_ss_socket == NULL )
    {
        printf("no free socket!\n");
        return -1;
    }

    fd = ff_socket(parm->domain, parm->type, parm->protocol);
    if ( fd < 0)
    {
        printf("call ff socket failed!(ret %d , errno %d)\n",
                ss_call.ret, ss_call.err );

        ss_socket_m_free();

        p_ss_socket->socket_type = SS_UNUSED;
        return -1;
    }

    p_ss_socket->socket_ff = fd;

    return ret;
}


int ss_socket(int domain, int type, int protocol)
{
    int event_fd;
    int i;
    int ret;
    struct ss_socket_m * p_ss_socket;
    struct ss_call_s ss_call;
    struct ss_parm_socket ss_parm;

    p_ss_socket = ss_alloc_socket_fd();
    if ( p_ss_socket == NULL )
    {
        printf("no free socket!\n");
        return -1;
    }

    ss_parm.domain   = domain;
    ss_parm.protocol = protocol;
    ss_parm.type     = type;

    ss_call.call_idx = SS_CALL_SOCKET;
    ss_call.param    = (void *)&ss_parm;

    ret = ss_remote_call(&ss_call);
    if ( ret < 0)
    {
        printf("call ff socket failed!(ret %d , errno %d)\n", ss_call.ret, ss_call.err );
        return -1;
    }
    p_ss_socket->socket_ff = ss_call.ret;

    return p_ss_socket->socket_idx;
}

int ss_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen)
{
    int ret;
    struct ss_call_s ss_call;
    struct ss_parm_setsockopt ss_parm;
    struct ss_socket_m * p_ss_socket = &g_ss_socket[s];

    pthread_mutex_lock(&p_ss_socket->lock);
    if ( p_ss_socket->socket_type == SS_UNUSED )
    {
        pthread_mutex_unlock(&p_ss_socket->lock);
        printf("socket has been free! %d\n", s);
        return -1;
    }

    ss_parm.s       = p_ss_socket->socket_ff;
    ss_parm.level   = level;
    ss_parm.optname = optname;
    ss_parm.optval  = optval;
    ss_parm.optlen  = optlen;

    ss_call.call_idx = SS_CALL_SETSOCKOPT;
    ss_call.param    = (void *)&ss_parm;

    ret = ss_remote_call(&ss_call);
    if ( ret < 0)
    {
        printf("call ff setsockopt opt failed!(ret %d , errno %d)\n", ss_call.ret, ss_call.err );
        pthread_mutex_unlock(&p_ss_socket->lock);
        return -1;
    }
    pthread_mutex_unlock(&p_ss_socket->lock);

    return ret;
}

int ss_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen)
{
    int ret;
    struct ss_call_s ss_call;
    struct ss_parm_getsockopt ss_parm;
    struct ss_socket_m * p_ss_socket = &g_ss_socket[s];

    pthread_mutex_lock(&p_ss_socket->lock);
    if ( p_ss_socket->socket_type == SS_UNUSED )
    {
        pthread_mutex_unlock(&p_ss_socket->lock);
        printf("socket has been free! %d\n", s);
        return -1;
    }

    ss_parm.s       = p_ss_socket->socket_ff;
    ss_parm.level   = level;
    ss_parm.optname = optname;
    ss_parm.optval  = optval;
    ss_parm.optlen  = optlen;

    ss_call.call_idx = SS_CALL_GETSOCKOPT;
    ss_call.param    = (void *)&ss_parm;

    ret = ss_remote_call(&ss_call);
    if ( ret < 0)
    {
        printf("call ff getsockopt failed!(ret %d , errno %d)\n", ss_call.ret, ss_call.err );
        pthread_mutex_unlock(&p_ss_socket->lock);
        return -1;
    }
    pthread_mutex_unlock(&p_ss_socket->lock);

    return ret;
}

int ss_listen(int s, int backlog)
{
    int ret;
    struct ss_call_s ss_call;
    struct ss_parm_listen ss_parm;
    struct ss_socket_m * p_ss_socket = &g_ss_socket[s];

    pthread_mutex_lock(&p_ss_socket->lock);
    if ( p_ss_socket->socket_type == SS_UNUSED )
    {
        pthread_mutex_unlock(&p_ss_socket->lock);
        printf("socket has been free! %d\n", s);
        return -1;
    }

    ss_parm.s   = p_ss_socket->socket_ff;
    ss_parm.backlog = backlog;

    ss_call.call_idx = SS_CALL_LISTEN;
    ss_call.param    = (void *)&ss_parm;

    ret = ss_remote_call(&ss_call);
    if ( ret < 0)
    {
        printf("call ff listen failed!(ret %d , errno %d)\n", ss_call.ret, ss_call.err );
        pthread_mutex_unlock(&p_ss_socket->lock);
        return -1;
    }

    p_ss_socket->socket_type = SS_SERVER;

    pthread_mutex_unlock(&p_ss_socket->lock);

    return ret;
}

int ss_bind(int s, const struct sockaddr *addr, socklen_t addrlen)
{
    int ret;
    struct ss_call_s ss_call;
    struct ss_parm_bind ss_parm;
    struct ss_socket_m * p_ss_socket = &g_ss_socket[s];

    pthread_mutex_lock(&p_ss_socket->lock);
    if ( p_ss_socket->socket_type == SS_UNUSED )
    {
        pthread_mutex_unlock(&p_ss_socket->lock);
        printf("socket has been free! %d\n", s);
        return -1;
    }

    ss_parm.s       = p_ss_socket->socket_ff;
    ss_parm.addr    = addr;
    ss_parm.addrlen = addrlen;

    ss_call.call_idx = SS_CALL_BIND;
    ss_call.param    = (void *)&ss_parm;

    ret = ss_remote_call(&ss_call);
    if ( ret < 0)
    {
        printf("call ff bind failed!(ret %d , errno %d)\n", ss_call.ret, ss_call.err );
        pthread_mutex_unlock(&p_ss_socket->lock);
        return -1;
    }
    pthread_mutex_unlock(&p_ss_socket->lock);

    return ret;
}

int ss_connect(int s, const struct sockaddr *name, socklen_t namelen)
{
    int ret;
    struct ss_call_s ss_call;
    struct ss_parm_connect ss_parm;
    struct ss_socket_m * p_ss_socket = &g_ss_socket[s];

    pthread_mutex_lock(&p_ss_socket->lock);
    if ( p_ss_socket->socket_type == SS_UNUSED )
    {
        pthread_mutex_unlock(&p_ss_socket->lock);
        printf("socket has been free! %d\n", s);
        return -1;
    }

    ss_parm.s       = p_ss_socket->socket_ff;
    ss_parm.name    = name;
    ss_parm.namelen = namelen;

    ss_call.call_idx = SS_CALL_CONNECT;
    ss_call.param    = (void *)&ss_parm;

    ret = ss_remote_call(&ss_call);
    if ( ret < 0)
    {
        printf("call ff socket failed!(ret %d , errno %d)\n", ss_call.ret, ss_call.err );
        pthread_mutex_unlock(&p_ss_socket->lock);
        return -1;
    }

    pthread_mutex_unlock(&p_ss_socket->lock);

    return ret;
}

int ss_close(int fd)
{
    int ret;
    struct ss_call_s ss_call;
    struct ss_parm_close ss_parm;
    struct ss_socket_m * p_ss_socket = &g_ss_socket[fd];

    pthread_mutex_lock(&p_ss_socket->lock);
    if ( p_ss_socket->socket_type == SS_UNUSED )
    {
        pthread_mutex_unlock(&p_ss_socket->lock);
        printf("socket has been free! %d\n", fd);
        return -1;
    }
    ss_parm.fd = p_ss_socket->socket_ff;

    ss_call.call_idx = SS_CALL_CLOSE;
    ss_call.param    = (void *)&ss_parm;

    ret = ss_remote_call(&ss_call);
    if ( ret < 0)
    {
        printf("call ff socket failed!(ret %d , errno %d)\n", ss_call.ret, ss_call.err );
        pthread_mutex_unlock(&p_ss_socket->lock);
        return -1;
    }

    p_ss_socket->socket_type = SS_UNUSED;

    ss_buff_m_clean(&p_ss_socket->buff_r);
    ss_buff_m_clean(&p_ss_socket->buff_w);

    pthread_mutex_unlock(&p_ss_socket->lock);

    return ret;
}

int ss_getpeername(int s, struct sockaddr *name, socklen_t *namelen)
{
    int ret;
    struct ss_call_s ss_call;
    struct ss_parm_getpeername ss_parm;
    struct ss_socket_m * p_ss_socket = &g_ss_socket[s];

    pthread_mutex_lock(&p_ss_socket->lock);
    if ( p_ss_socket->socket_type == SS_UNUSED )
    {
        pthread_mutex_unlock(&p_ss_socket->lock);
        printf("socket has been free! %d\n", s);
        return -1;
    }

    ss_parm.s        = p_ss_socket->socket_ff;
    ss_parm.name     = name;
    ss_parm.namelen  = namelen;

    ss_call.call_idx = SS_CALL_GETPEERNAME;
    ss_call.param    = (void *)&ss_parm;

    ret = ss_remote_call(&ss_call);
    if ( ret < 0)
    {
        printf("call ff socket failed!(ret %d , errno %d)\n", ss_call.ret, ss_call.err );
        pthread_mutex_unlock(&p_ss_socket->lock);
        return -1;
    }
    pthread_mutex_unlock(&p_ss_socket->lock);

    return ret;
}

int ss_getsockname(int s, struct sockaddr *name, socklen_t *namelen)
{
    int ret;
    struct ss_call_s ss_call;
    struct ss_parm_getsockname ss_parm;
    struct ss_socket_m * p_ss_socket = &g_ss_socket[s];

    pthread_mutex_lock(&p_ss_socket->lock);
    if ( p_ss_socket->socket_type == SS_UNUSED )
    {
        pthread_mutex_unlock(&p_ss_socket->lock);
        printf("socket has been free! %d\n", s);
        return -1;
    }

    ss_parm.s        = p_ss_socket->socket_ff;
    ss_parm.name     = name;
    ss_parm.namelen  = namelen;

    ss_call.call_idx = SS_CALL_GETSOCKNAME;
    ss_call.param    = (void *)&ss_parm;

    ret = ss_remote_call(&ss_call);
    if ( ret < 0)
    {
        printf("call ff socket failed!(ret %d , errno %d)\n", ss_call.ret, ss_call.err );
        pthread_mutex_unlock(&p_ss_socket->lock);
        return -1;
    }
    pthread_mutex_unlock(&p_ss_socket->lock);

    return ret;
}

int ss_accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int ret;
    struct ss_accept_s * p_ss_accept;
    struct ss_socket_m * p_ss_client;
    struct ss_socket_m * p_ss_socket = &g_ss_socket[s];

    pthread_mutex_lock(&p_ss_socket->lock);
    if ( p_ss_socket->socket_type == SS_UNUSED )
    {
        pthread_mutex_unlock(&p_ss_socket->lock);
        printf("socket has been free! %d\n", s);
        return -1;
    }

    p_ss_accept = p_ss_socket->paccept;
    if ( NULL == p_ss_accept )
    {
        p_ss_socket->event_s = p_ss_socket->event_s & ~(EPOLLIN);
        pthread_mutex_unlock(&p_ss_socket->lock);
        return -1;
    }
    p_ss_socket->paccept = p_ss_accept->pnext;
    pthread_mutex_unlock(&p_ss_socket->lock);

    p_ss_client = ss_alloc_socket_fd();
    if ( p_ss_client == NULL )
    {
        printf("no free socket!\n");
        return -1;
    }

    p_ss_client->socket_ff = p_ss_accept->socket_ff;
    free(p_ss_accept);

    return p_ss_client->socket_idx;
}

ssize_t ss_read(int fd, void *buf, size_t nbytes)
{
    ssize_t cnt = 0;
    struct ss_socket_m * p_ss_socket = &g_ss_socket[fd];

    pthread_mutex_lock(&p_ss_socket->lock);
    if ( p_ss_socket->socket_type == SS_UNUSED )
    {
        pthread_mutex_unlock(&p_ss_socket->lock);
        printf("socket has been free! %d\n", fd);
        return -1;
    }

    cnt = ss_buff_m_read(&p_ss_socket->buff_r, buf, nbytes);
    if ( cnt == 0 )
    {
        p_ss_socket->event_s = p_ss_socket->event_s & ~(EPOLLIN);
        pthread_mutex_unlock(&p_ss_socket->lock);

        return 0;
    }

    pthread_mutex_unlock(&p_ss_socket->lock);

    return cnt;
}

ssize_t ss_readv(int fd, const struct iovec *iov, int iovcnt)
{
    ssize_t cnt = 0;
    struct ss_socket_m * p_ss_socket = &g_ss_socket[fd];

    pthread_mutex_lock(&p_ss_socket->lock);
    if ( p_ss_socket->socket_type == SS_UNUSED )
    {
        pthread_mutex_unlock(&p_ss_socket->lock);
        printf("socket has been free! %d\n", fd);
        return -1;
    }

    cnt = ss_buff_m_readv(&p_ss_socket->buff_r, iov, iovcnt);
    if ( cnt == 0 )
    {
        p_ss_socket->event_s = p_ss_socket->event_s & ~(EPOLLIN);
        pthread_mutex_unlock(&p_ss_socket->lock);

        return 0;
    }

    pthread_mutex_unlock(&p_ss_socket->lock);

    return cnt;
}

ssize_t ss_write(int fd, const void *buf, size_t nbytes)
{
    ssize_t cnt = 0;
    struct ss_socket_m * p_ss_socket = &g_ss_socket[fd];

    pthread_mutex_lock(&p_ss_socket->lock);
    if ( p_ss_socket->socket_type == SS_UNUSED )
    {
        pthread_mutex_unlock(&p_ss_socket->lock);
        printf("socket has been free! %d\n", fd);
        return -1;
    }

    cnt = ss_buff_m_write(&p_ss_socket->buff_w, buf, nbytes);
    if ( cnt == 0 )
    {
        p_ss_socket->event_s = p_ss_socket->event_s & ~(EPOLLOUT);
        pthread_mutex_unlock(&p_ss_socket->lock);

        return 0;
    }

    pthread_mutex_unlock(&p_ss_socket->lock);

    return cnt;
}

ssize_t ss_writev(int fd, const struct iovec *iov, int iovcnt)
{
    ssize_t cnt = 0;
    struct ss_socket_m * p_ss_socket = &g_ss_socket[fd];

    pthread_mutex_lock(&p_ss_socket->lock);
    if ( p_ss_socket->socket_type == SS_UNUSED )
    {
        pthread_mutex_unlock(&p_ss_socket->lock);
        printf("socket has been free! %d\n", fd);
        return -1;
    }

    cnt = ss_buff_m_writev(&p_ss_socket->buff_w, iov, iovcnt);
    if ( cnt == 0 )
    {
        p_ss_socket->event_s = p_ss_socket->event_s & ~(EPOLLOUT);
        pthread_mutex_unlock(&p_ss_socket->lock);

        return 0;
    }

    pthread_mutex_unlock(&p_ss_socket->lock);

    return cnt;
}




