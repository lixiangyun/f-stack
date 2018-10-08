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


ssize_t ss_buff_read(struct ss_buff * pbuff, char *buf, size_t nbytes)
{
    int copy_size;
    int copy_idx;

    if ( ( pbuff->read + nbytes) > pbuff->write )
    {
        copy_size = pbuff->write - pbuff->read;
    }
    else
    {
        copy_size = nbytes;
    }

    if ( 0 == copy_size )
    {
        return 0;
    }

    memcpy(buf, &pbuff->body[pbuff->read], copy_size );
    pbuff->read += copy_size;

    return copy_size;
}

ssize_t ss_buff_write(struct ss_buff * pbuff, const char *buf, size_t nbytes)
{
    int copy_size;

    if ( ( pbuff->write + nbytes ) > BUFF_MAX_LEN )
    {
        copy_size = BUFF_MAX_LEN - pbuff->write;
    }
    else
    {
        copy_size = nbytes;
    }

    if ( 0 == copy_size )
    {
        return 0;
    }

    memcpy( &pbuff->body[pbuff->write], buf, copy_size );
    pbuff->write += copy_size;

    return copy_size;
}

ssize_t ss_buff_m_read(struct ss_buff_m * pbuff, char *buf, size_t nbytes)
{
    size_t cnt = 0;
    size_t remain = nbytes;
    struct ss_buff * pcur = pbuff->pnext;

    while( pcur != NULL )
    {
        size_t tmp = ss_buff_read(pcur, buf + cnt, remain);

        if ( 0 == tmp )
        {
            pbuff->pnext = pcur->pnext;
            free(pcur);
            pcur = pbuff->pnext;

            continue;
        }

        cnt    += tmp;
        remain -= tmp;

        if ( 0 == remain )
        {
            break;
        }
    }

    if ( NULL == pbuff->pnext )
    {
        pbuff->ptail = NULL;
    }

    return cnt;
}

ssize_t ss_buff_m_readv(struct ss_buff_m * pbuff, const struct iovec *iov, int iovcnt)
{
    int i;
    ssize_t tmp;
    ssize_t cnt = 0;

    for( i = 0 ; i < iovcnt; i++ )
    {
        tmp = ss_buff_m_read(pbuff, iov[i].iov_base, iov[i].iov_len );
        cnt += tmp;
        if ( tmp < iov[i].iov_len )
        {
            break;
        }
    }

    return cnt;
}

ssize_t ss_buff_m_write(struct ss_buff_m * pbuff, const char *buf, size_t nbytes)
{
    size_t cnt = 0;
    size_t remain = nbytes;
    struct ss_buff * pcur = pbuff->ptail;

    for ( ; remain != 0 ; )
    {
        if ( NULL == pcur )
        {
alloc:
            pcur = (struct ss_buff *)malloc(sizeof(struct ss_buff));
            pcur->read  = 0;
            pcur->write = 0;
            pcur->pnext = NULL;

            if ( NULL == pbuff->ptail )
            {
                pbuff->pnext = pcur;
                pbuff->ptail = pcur;
            }
            else
            {
                pbuff->ptail->pnext = pcur;
                pbuff->ptail = pcur;
            }
        }

        size_t tmp = ss_buff_write(pcur, buf + cnt, remain );
        if ( 0 == tmp )
        {
            goto alloc;
        }

        cnt    += tmp;
        remain -= tmp;
    }

    return cnt;
}

ssize_t ss_buff_m_writev(struct ss_buff_m * pbuff, const struct iovec *iov, int iovcnt)
{
    int i;
    ssize_t tmp;
    ssize_t cnt = 0;

    for( i = 0 ; i < iovcnt; i++ )
    {
        tmp = ss_buff_m_write(pbuff, iov[i].iov_base, iov[i].iov_len );
        cnt += tmp;
        if ( tmp < iov[i].iov_len )
        {
            break;
        }
    }

    return cnt;
}

void ss_buff_m_clean(struct ss_buff_m * pbuffm)
{
    struct ss_buff * pbuff;
    struct ss_buff * pnext;

    for ( pbuff = pbuffm->pnext; pbuff != NULL ; pbuff = pnext )
    {
        pnext = pbuff->pnext;
        memset( pbuff, 0, sizeof(struct ss_buff) );
        free((void *)pbuff);
    }

    pbuffm->pnext = NULL;
    pbuffm->ptail = NULL;
}

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
    pthread_mutex_t lock;

    int socket_type;
    int socket_idx;

    int event_fd;
    int event_s;

    struct ss_accept_s * paccept;
    int socket_ff;

    int close_flag;

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
    int ff_socket_fd;
    struct ff_event_data * ff_event_data;
};

pthread_mutex_t g_ss_lock = PTHREAD_MUTEX_INITIALIZER;
int g_ff_2_ss[SOCK_MAX_NUM] = {0};
struct ss_socket_m g_ss_socket[SOCK_MAX_NUM];
struct ss_call_que g_ss_call_que = {PTHREAD_MUTEX_INITIALIZER,0,NULL,NULL};
int g_ff_epfd = -1;

int ss_remote_call( struct ss_call_s * pcall )
{
    int ret;

    ret = sem_init(&pcall->sem, 0, 0);
    if ( ret < 0 )
    {
        printf("sem init failed!");
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
        printf("sem wait failed!");
        return -1;
    }

    ret = pcall->ret;
    sem_destroy(&pcall->sem);

    return ret;
}

struct ss_socket_m * ss_alloc_socket_fd(void)
{
    int i;
    struct ss_socket_m * p_ss_socket;

    pthread_mutex_lock(&g_ss_lock);
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
        pthread_mutex_unlock(&g_ss_lock);
        printf("alloc socket fd failed!\n");
        return NULL;
    }

    p_ss_socket->socket_type = SS_CLIENT;
    p_ss_socket->event_s = 0;
    p_ss_socket->paccept = NULL;
    p_ss_socket->close_flag = 0;

    ss_buff_m_clean(&p_ss_socket->buff_r);
    ss_buff_m_clean(&p_ss_socket->buff_w);

    pthread_mutex_unlock(&g_ss_lock);

    return p_ss_socket;
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
        p_ss_socket->socket_type = SS_UNUSED;
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

    if (p_ss_socket->close_flag)
    {
        pthread_mutex_unlock(&p_ss_socket->lock);
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

    if (p_ss_socket->close_flag)
    {
        pthread_mutex_unlock(&p_ss_socket->lock);
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

    if (p_ss_socket->close_flag)
    {
        pthread_mutex_unlock(&p_ss_socket->lock);
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

    if (p_ss_socket->close_flag)
    {
        pthread_mutex_unlock(&p_ss_socket->lock);
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


void ff_epoll_callback_accept( struct ff_event_data * pdata )
{
    int ff_client_fd;
    struct ss_socket_m * p_ss_socket = pdata->ss_socket_s;
    struct ss_accept_s * p_ss_accept = NULL;

    if ( p_ss_socket->socket_type == SS_UNUSED )
    {
        printf("socket has been close! %d\n", p_ss_socket->socket_idx);
        return;
    }

    for (;;)
    {
        ff_client_fd = ff_accept(pdata->ff_socket_fd, NULL, NULL);
        if ( ff_client_fd < 0 )
        {
            break;
        }

        printf("ff_epoll_callback_accept ff_client_fd %d !\n", ff_client_fd);

        p_ss_accept = (struct ss_accept_s *)malloc(sizeof(struct ss_accept_s));
        p_ss_accept->socket_ff  = ff_client_fd;

        pthread_mutex_lock(&p_ss_socket->lock);
        p_ss_accept->pnext   = p_ss_socket->paccept;
        p_ss_socket->paccept = p_ss_accept;
        pthread_mutex_unlock(&p_ss_socket->lock);
    }

    if ( p_ss_accept )
    {
        pthread_mutex_lock(&p_ss_socket->lock);
        p_ss_socket->event_s = p_ss_socket->event_s | EPOLLIN;
        if ( eventfd_write(p_ss_socket->event_fd, (eventfd_t)(1) ) < 0 )
        {
            printf("eventfd_write failed! errno = %d\n", errno );
        }
        pthread_mutex_unlock(&p_ss_socket->lock);
    }
}

void ff_epoll_callback_connect(struct ff_event_data * pdata )
{
    int ret;
    struct ss_socket_m * p_ss_socket = pdata->ss_socket_s;
    char stbuf[4096];

    if ( p_ss_socket->socket_type == SS_UNUSED )
    {
        printf("socket has been close! %d\n", p_ss_socket->socket_idx);
        return;
    }

    ret = pthread_mutex_trylock(&p_ss_socket->lock);
    if ( ret != 0 )
    {
        return;
    }

    if ( pdata->ff_events & EPOLLIN )
    {
        ssize_t tmp;
        ssize_t cnt = 0;
        ssize_t remain;

        remain = ff_read(pdata->ff_socket_fd, stbuf, sizeof(stbuf));
        if ( remain < 0 )
        {
            p_ss_socket->close_flag = 1;
        }

        if ( remain > 0 )
        {
            ss_buff_m_write(&p_ss_socket->buff_r, stbuf, remain);
        }
        
        p_ss_socket->event_s = p_ss_socket->event_s | EPOLLIN;
        if ( eventfd_write(p_ss_socket->event_fd, (eventfd_t)(1) ) < 0 )
        {
            printf("eventfd_write failed! errno = %d\n", errno );
        }
    }

    if ( pdata->ff_events & EPOLLOUT )
    {
        ssize_t tmp;
        ssize_t cnt = 0;
        ssize_t remain;

        remain = ss_buff_m_read(&p_ss_socket->buff_w, stbuf, sizeof(stbuf));
        for ( ; remain > 0 ; )
        {
            tmp = ff_write(pdata->ff_socket_fd, &stbuf[cnt], remain );
            if ( tmp > 0 )
            {
                remain = remain - tmp;
                cnt    = cnt    + tmp;
            }
        }

        p_ss_socket->event_s = p_ss_socket->event_s | EPOLLOUT;
        if ( eventfd_write(p_ss_socket->event_fd, (eventfd_t)(1) ) < 0 )
        {
            printf("eventfd_write failed! errno = %d\n", errno );
        }
    }

    if (pdata->ff_events & EPOLLERR )
    {
        p_ss_socket->close_flag = 1;
        p_ss_socket->event_s = p_ss_socket->event_s | EPOLLERR;
        if ( eventfd_write(p_ss_socket->event_fd, (eventfd_t)(1) ) < 0 )
        {
            printf("eventfd_write failed! errno = %d\n", errno );
        }
    }

    pthread_mutex_unlock(&p_ss_socket->lock);
}

struct ss_epoll_data_s {
    int    type;     // 0 : unix fd, 1 ss socket event fd;
    int    fd;
    void * pdata;    // epoll_data_t * or struct ss_socket_m *;
};

int ss_epoll_create(int size)
{
    int epfd;

    epfd = epoll_create1(EPOLL_CLOEXEC);
    if ( epfd < 0 )
    {
        printf("ss epoll create failed! %d\n", errno );
        return -1;
    }

    return epfd + 10000;
}

int ss_epoll_ctl(int epfd, int op, int fd, struct epoll_event * pevent)
{
    int ret;

    if ( epfd < 10000 )
    {
        printf("ss_epoll_ctl failed! epfd = %d\n", epfd );
        return -1;
    }

    if ( ( SOCK_REL_IDX <= fd ) && ( fd < SOCK_MAX_NUM ) )
    {
        struct ff_event_data * p_ff_event_data = NULL;
        struct ss_call_s ss_call;
        struct ss_parm_epoll_ctl ss_parm;
        struct ss_socket_m * p_ss_socket = &g_ss_socket[fd];

        pthread_mutex_lock(&p_ss_socket->lock);
        if ( p_ss_socket->socket_type == SS_UNUSED )
        {
            pthread_mutex_unlock(&p_ss_socket->lock);
            printf("socket has been free! %d\n", fd);
            return -1;
        }

        if ( op != EPOLL_CTL_DEL )
        {
            p_ff_event_data = (struct ff_event_data *)malloc(sizeof(struct ff_event_data));
            if ( NULL == p_ff_event_data )
            {
                pthread_mutex_unlock(&p_ss_socket->lock);
                printf("malloc size failed! %lu\n", sizeof(struct ff_event_data));
                return -1;
            }

            if ( SS_SERVER == p_ss_socket->socket_type )
            {
                p_ff_event_data->ff_callback  = ff_epoll_callback_accept;
            }
            else
            {
                p_ff_event_data->ff_callback  = ff_epoll_callback_connect;
            }

            p_ff_event_data->ff_events    = pevent->events;
            p_ff_event_data->ff_socket_fd = p_ss_socket->socket_ff;
            p_ff_event_data->ss_socket_s  = p_ss_socket;
        }

        ss_parm.ff_socket_fd  = p_ss_socket->socket_ff;
        ss_parm.ff_event_data = p_ff_event_data;
        ss_parm.ff_opt        = op;

        ss_call.call_idx = SS_CALL_EPOLL_CTL;
        ss_call.param    = (void *)&ss_parm;

        ret = ss_remote_call(&ss_call);
        if ( ret < 0)
        {
            printf("call ff epoll ctl failed!(ret %d , errno %d)\n", ss_call.ret, ss_call.err );
            pthread_mutex_unlock(&p_ss_socket->lock);
            return -1;
        }

        struct epoll_event event;
        struct ss_epoll_data_s * pdata;

        pdata = (struct ss_epoll_data_s *)malloc(sizeof(struct ss_epoll_data_s));
        pdata->type   = 1;
        pdata->fd     = p_ss_socket->event_fd;
        pdata->pdata  = (void *)p_ss_socket;

        if ( op != EPOLL_CTL_DEL )
        {
            event.events  = pevent->events;
            event.data.ptr = (void *)pdata;
            ret = epoll_ctl(epfd - 10000, op, p_ss_socket->event_fd, &event);
        }
        else
        {
            ret = epoll_ctl(epfd - 10000, op, p_ss_socket->event_fd, NULL);
        }

        pthread_mutex_unlock(&p_ss_socket->lock);
    }
    else
    {
        struct epoll_event event;
        struct ss_epoll_data_s * pdata;

        pdata = (struct ss_epoll_data_s *)malloc(sizeof(struct ss_epoll_data_s));
        pdata->type   = 0;
        pdata->fd     = fd;
        pdata->pdata  = pevent->data.ptr;

        event.events  = pevent->events;
        event.data.ptr = (void *)pdata;

        ret = epoll_ctl(epfd - 10000, op, fd, &event);
    }

    return ret;
}

int ss_epoll_wait(int epfd, struct epoll_event * pevents, int maxevents, int timeout)
{
    int i;
    int cnt;
    struct epoll_event events[FF_MAX_EVENTS];

    if ( epfd < 10000 )
    {
        printf("ss_epoll_wait failed! epfd = %d\n", epfd );
        return -1;
    }

    cnt = epoll_wait(epfd - 10000, events, FF_MAX_EVENTS, timeout);
    if ( 0 >= cnt )
    {
        return cnt;
    }

    for ( i = 0 ; i < cnt; i++ )
    {
        struct ss_epoll_data_s * p_ss_epoll_data = (struct ss_epoll_data_s *)events[i].data.ptr;
        if ( 1 == p_ss_epoll_data->type )
        {
            struct ss_socket_m * p_ss_socket = (struct ss_socket_m *)p_ss_epoll_data->pdata;

            pevents[i].events   = p_ss_socket->event_s;
            pevents[i].data.fd  = p_ss_socket->socket_idx;

            while(1)
            {
                eventfd_t msg;
                if ( eventfd_read(p_ss_socket->event_fd, &msg) < 0 )
                {
                    break;
                }
            }
        }
        else
        {
            pevents[i].events   = events[i].events;
            pevents[i].data.ptr = p_ss_epoll_data->pdata;
        }
    }

    return cnt;
}

void ff_proccess_once( struct ss_call_s * pcall )
{
    int ret = 0;

    pcall->ret = 0;

    switch (pcall->call_idx)
    {
        case SS_CALL_SOCKET:
        {
            struct ss_parm_socket *parm = (struct ss_parm_socket *)pcall->param;
            ret = ff_socket(parm->domain, parm->type, parm->protocol);
        }break;
        case SS_CALL_SETSOCKOPT:
        {
            struct ss_parm_setsockopt *parm = (struct ss_parm_setsockopt *)pcall->param;
            ret = ff_setsockopt(parm->s, parm->level, parm->optname, (const void *)parm->optval, parm->optlen);
        }break;
        case SS_CALL_GETSOCKOPT:
        {
            struct ss_parm_getsockopt *parm = (struct ss_parm_getsockopt *)pcall->param;
            ret = ff_getsockopt(parm->s, parm->level, parm->optname, parm->optval, parm->optlen);
        }break;
        case SS_CALL_LISTEN:
        {
            struct ss_parm_listen *parm = (struct ss_parm_listen *)pcall->param;
            ret = ff_listen(parm->s, parm->backlog);
        }break;
        case SS_CALL_BIND:
        {
            struct ss_parm_bind *parm = (struct ss_parm_bind *)pcall->param;
            ret = ff_bind(parm->s, (const struct linux_sockaddr *)parm->addr, parm->addrlen);
        }break;
        case SS_CALL_CONNECT:
        {
            struct ss_parm_connect *parm = (struct ss_parm_connect *)pcall->param;
            ret = ff_connect(parm->s, (const struct linux_sockaddr *)parm->name, parm->namelen);
        }break;
        case SS_CALL_CLOSE:
        {
            struct ss_parm_close *parm = (struct ss_parm_close *)pcall->param;
            ret = ff_close(parm->fd);
        }break;
        case SS_CALL_GETPEERNAME:
        {
            struct ss_parm_getpeername *parm = (struct ss_parm_getpeername *)pcall->param;
            ret = ff_getpeername(parm->s, (struct linux_sockaddr *)parm->name, parm->namelen);
        }break;
        case SS_CALL_GETSOCKNAME:
        {
            struct ss_parm_getsockname *parm = (struct ss_parm_getsockname *)pcall->param;
            ret = ff_getsockname(parm->s, (struct linux_sockaddr *)parm->name, parm->namelen);
        }break;
        case SS_CALL_EPOLL_CTL:
        {
            struct ss_parm_epoll_ctl *parm = (struct ss_parm_epoll_ctl *)pcall->param;
            if ( NULL != parm->ff_event_data )
            {
                struct epoll_event event;
                event.data.ptr = (void *)parm->ff_event_data;
                event.events   = parm->ff_event_data->ff_events;
                ret = ff_epoll_ctl(g_ff_epfd, parm->ff_opt, parm->ff_socket_fd, &event);
            }
            else
            {
                ret = ff_epoll_ctl(g_ff_epfd, parm->ff_opt, parm->ff_socket_fd, NULL);
            }
        }break;
        default:
        {
            printf("call index is invaild! %d\n", pcall->call_idx );
            return;
        }
    }

    printf("ff remote proc %d , parm %p, ret %d \n", pcall->call_idx, pcall->param , pcall->ret );

    pcall->ret = ret;
    pcall->err = errno;

    ret = sem_post(&pcall->sem);
    if ( ret < 0 )
    {
        printf("sem post failed!\n");
    }
}

void ff_proccess_call()
{
    int ret;
    struct ss_call_s * pcall;
    struct ss_call_s * pcall_next;
    
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

    for ( ; pcall != NULL ; pcall = pcall_next )
    {
        pcall_next = pcall->pnext;
        ff_proccess_once(pcall);
    }
}

void ff_epoll_loop()
{
    int i;
    int nevents;
    struct epoll_event events[FF_MAX_EVENTS];

    nevents = ff_epoll_wait(g_ff_epfd, events, FF_MAX_EVENTS, 0);
    for ( i = 0; i < nevents ; i++ )
    {
        struct ff_event_data *pdata = (struct ff_event_data *)events[i].data.ptr;
        pdata->ff_events = events[i].events;

        pdata->ff_callback(pdata);
    }
}

int ff_loop(void *arg)
{
    ff_proccess_call();
    ff_epoll_loop();
    return 0;
}

void ss_run(void)
{
    ff_run(ff_loop, NULL);
}

int ss_init(int argc, char * argv[])
{
    int ret;
    int i;

    memset(&g_ss_socket,  0,sizeof(g_ss_socket));
    memset(&g_ff_2_ss,    0,sizeof(g_ff_2_ss));
    memset(&g_ss_call_que,0,sizeof(g_ss_call_que));

    for ( i = SOCK_REL_IDX ; i < SOCK_MAX_NUM ; i++ )
    {
        struct ss_socket_m * p_ss_socket;
        p_ss_socket = (struct ss_socket_m *)&g_ss_socket[i];

        p_ss_socket->socket_type = SS_UNUSED;
        p_ss_socket->socket_idx  = i;

        ret = pthread_mutex_init(&p_ss_socket->lock, NULL);
        if ( ret != 0 )
        {
            printf("pthread mutex init failed!\n");
            return -1;
        }

        p_ss_socket->event_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if ( p_ss_socket->event_fd < 0 )
        {
            printf("eventfd ret failed!\n");
            return -1;
        }
    }

    ret = pthread_mutex_init(&g_ss_call_que.lock, NULL);
    if ( ret != 0 )
    {
        printf("pthread mutex init failed!\n");
        return -1;
    }

    ret = ff_init(argc, argv);
    if ( ret < 0 )
    {
        printf("ff_init failed!\n");
        return -1;
    }

    g_ff_epfd = ff_epoll_create(0);
    if (g_ff_epfd < 0)
    {
        printf("ff_epoll_create failed\n");
        return -1;
    }

    return 0;
}

