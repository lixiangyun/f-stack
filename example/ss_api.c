
#include <sys/timerfd.h>
#include "ss_api.h"
#include "ss_inner.h"

int ss_socket(int domain, int type, int protocol)
{
    int ret;
    struct ss_call_s ss_call;
    struct ss_parm_socket ss_parm;

    if ( type & SOCK_NONBLOCK )
    {
        type &= ~SOCK_NONBLOCK;
    }

    ss_parm.domain   = domain;
    ss_parm.protocol = protocol;
    ss_parm.type     = type;

    ss_call.call_idx = SS_CALL_SOCKET;
    ss_call.param    = (void *)&ss_parm;

    ret = ss_call_remote(&ss_call);
    if ( ret < 0)
    {
        printf("call ff socket failed!(ret %d , errno %d)\n", ss_call.ret, ss_call.err );
        return -1;
    }
    
    errno = ss_call.err;
    return ss_call.ret;
}

int ss_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen)
{
    int ret;
    struct ss_call_s ss_call;
    struct ss_parm_setsockopt ss_parm;

    ss_parm.s       = s;
    ss_parm.level   = level;
    ss_parm.optname = optname;
    ss_parm.optval  = optval;
    ss_parm.optlen  = optlen;

    ss_call.call_idx = SS_CALL_SETSOCKOPT;
    ss_call.param    = (void *)&ss_parm;

    ret = ss_call_remote(&ss_call);
    if ( ret < 0)
    {
        printf("call ff setsockopt opt failed!(ret %d , errno %d)\n",
                ss_call.ret, ss_call.err );
        return -1;
    }

    errno = ss_call.err;
    return ss_call.ret;
}

int ss_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen)
{
    int ret;
    struct ss_call_s ss_call;
    struct ss_parm_getsockopt ss_parm;

    ss_parm.s       = s;
    ss_parm.level   = level;
    ss_parm.optname = optname;
    ss_parm.optval  = optval;
    ss_parm.optlen  = optlen;

    ss_call.call_idx = SS_CALL_GETSOCKOPT;
    ss_call.param    = (void *)&ss_parm;

    ret = ss_call_remote(&ss_call);
    if ( ret < 0)
    {
        printf("call ff getsockopt failed!(ret %d , errno %d)\n", 
                ss_call.ret, ss_call.err );
        return -1;
    }

    errno = ss_call.err;
    return ss_call.ret;
}

int ss_listen(int s, int backlog)
{
    int ret;
    struct ss_call_s ss_call;
    struct ss_parm_listen ss_parm;

    ss_parm.s   = s;
    ss_parm.backlog = backlog;

    ss_call.call_idx = SS_CALL_LISTEN;
    ss_call.param    = (void *)&ss_parm;

    ret = ss_call_remote(&ss_call);
    if ( ret < 0)
    {
        printf("call ff listen failed!(ret %d , errno %d)\n", 
            ss_call.ret, ss_call.err );
        return -1;
    }

    errno = ss_call.err;
    return ss_call.ret;
}

int ss_bind(int s, const struct sockaddr *addr, socklen_t addrlen)
{
    int ret;
    struct ss_call_s ss_call;
    struct ss_parm_bind ss_parm;

    ss_parm.s       = s;
    ss_parm.addr    = addr;
    ss_parm.addrlen = addrlen;

    ss_call.call_idx = SS_CALL_BIND;
    ss_call.param    = (void *)&ss_parm;

    ret = ss_call_remote(&ss_call);
    if ( ret < 0)
    {
        printf("call ff bind failed!(ret %d , errno %d)\n",
                ss_call.ret, ss_call.err );
        return -1;
    }

    errno = ss_call.err;
    return ss_call.ret;
}

int ss_connect(int s, const struct sockaddr *name, socklen_t namelen)
{
    int ret;
    struct ss_call_s ss_call;
    struct ss_parm_connect ss_parm;

    ss_parm.s       = s;
    ss_parm.name    = name;
    ss_parm.namelen = namelen;

    ss_call.call_idx = SS_CALL_CONNECT;
    ss_call.param    = (void *)&ss_parm;

    ret = ss_call_remote(&ss_call);
    if ( ret < 0)
    {
        printf("call ff socket failed!(ret %d , errno %d)\n", 
            ss_call.ret, ss_call.err );
        return -1;
    }

    errno = ss_call.err;
    return ss_call.ret;
}

int ss_close(int fd)
{
    int ret;
    struct ss_call_s ss_call;
    struct ss_parm_close ss_parm;

    ss_parm.fd = fd;

    ss_call.call_idx = SS_CALL_CLOSE;
    ss_call.param    = (void *)&ss_parm;

    ret = ss_call_remote(&ss_call);
    if ( ret < 0)
    {
        printf("call ff socket failed!(ret %d , errno %d)\n", 
            ss_call.ret, ss_call.err );
        return -1;
    }

    errno = ss_call.err;
    return ss_call.ret;
}

int ss_getpeername(int s, struct sockaddr *name, socklen_t *namelen)
{
    int ret;
    struct ss_call_s ss_call;
    struct ss_parm_getpeername ss_parm;

    ss_parm.s        = s;
    ss_parm.name     = name;
    ss_parm.namelen  = namelen;

    ss_call.call_idx = SS_CALL_GETPEERNAME;
    ss_call.param    = (void *)&ss_parm;

    ret = ss_call_remote(&ss_call);
    if ( ret < 0)
    {
        printf("call ff socket failed!(ret %d , errno %d)\n", 
            ss_call.ret, ss_call.err );
        return -1;
    }

    errno = ss_call.err;
    return ss_call.ret;
}

int ss_getsockname(int s, struct sockaddr *name, socklen_t *namelen)
{
    int ret;
    struct ss_call_s ss_call;
    struct ss_parm_getsockname ss_parm;

    ss_parm.s        = s;
    ss_parm.name     = name;
    ss_parm.namelen  = namelen;

    ss_call.call_idx = SS_CALL_GETSOCKNAME;
    ss_call.param    = (void *)&ss_parm;

    ret = ss_call_remote(&ss_call);
    if ( ret < 0)
    {
        printf("call ff socket failed!(ret %d , errno %d)\n", ss_call.ret, ss_call.err );
        return -1;
    }

    errno = ss_call.err;
    return ss_call.ret;
}

int ss_accept(int s, struct sockaddr * paddr, socklen_t * paddrlen)
{
    int fd;
    struct ss_accept_s * p_accept;
    struct ss_socket_m * p_socket = ss_socket_m_get(s);

    if ( NULL == p_socket )
    {
        printf("socket free! %d\n", s);
        errno = ECONNABORTED;
        return -1;
    }

    for (;;)
    {
        p_accept = p_socket->paccept;
        if ( NULL == p_accept )
        {
            errno = EAGAIN;
            return -1;
        }
        if ( ss_atomic64_cas((long *)&(p_socket->paccept), (long)p_accept, (long)p_accept->pnext ) )
        {
            break;
        }
    }
    
    if ( NULL != paddr )
    {
        memcpy(paddr, &p_accept->addr, p_accept->addrlen);
    }

    if( NULL != paddrlen )
    {
        *paddrlen = p_accept->addrlen;
    }
    
    fd = p_accept->fd;
    free(p_accept);

    return fd;
}

ssize_t ss_read(int fd, void *buf, size_t nbytes)
{
    ssize_t cnt = 0;
    struct ss_socket_m * p_socket = ss_socket_m_get(fd);
    if ( NULL == p_socket )
    {
        printf("socket free! %d\n", fd);
        return 0;
    }

    if ( p_socket->status & SS_CONN_STAT )
    {
        printf("socket disconnect! %d\n", fd);
        return 0;
    }

    cnt = ss_buff_read(p_socket->pread, buf, nbytes);
    if ( cnt == 0 )
    {
        errno = EAGAIN;
        return -1;
    }

    return cnt;
}

ssize_t ss_readv(int fd, const struct iovec *iov, int iovcnt)
{
    ssize_t cnt = 0;
    struct ss_socket_m * p_socket = ss_socket_m_get(fd);
    if ( NULL == p_socket )
    {
        printf("socket free! %d\n", fd);
        return 0;
    }

    if ( p_socket->status & SS_CONN_STAT )
    {
        printf("socket disconnect! %d\n", fd);
        return 0;
    }

    cnt = ss_buff_readv(p_socket->pread, (struct iovec *)iov, iovcnt);
    if ( cnt == 0 )
    {
        errno = EAGAIN;
        return -1;
    }

    return cnt;
}

ssize_t ss_write(int fd, const void *buf, size_t nbytes)
{
    ssize_t cnt = 0;
    struct ss_socket_m * p_socket = ss_socket_m_get(fd);
    if ( NULL == p_socket )
    {
        printf("socket free! %d\n", fd);
        return 0;
    }

    if ( p_socket->status & SS_CONN_STAT )
    {
        printf("socket disconnect! %d\n", fd);
        return 0;
    }

    cnt = ss_buff_write(p_socket->pwrite, (char *)buf, nbytes);
    if ( cnt == 0 )
    {
        errno = EAGAIN;
        return -1;
    }

    return cnt;
}

ssize_t ss_writev(int fd, const struct iovec *iov, int iovcnt)
{
    ssize_t cnt = 0;
    struct ss_socket_m * p_socket = ss_socket_m_get(fd);
    if ( NULL == p_socket )
    {
        printf("socket free! %d\n", fd);
        return 0;
    }

    if ( p_socket->status & SS_CONN_STAT )
    {
        printf("socket disconnect! %d\n", fd);
        return 0;
    }

    cnt = ss_buff_writev(p_socket->pwrite, (struct iovec *)iov, iovcnt);
    if ( cnt == 0 )
    {
        errno = EAGAIN;
        return -1;
    }

    return cnt;
}

struct ss_epoll_ctrl_s {
    int epoll_fd;
    int sock_fd[EPOLL_MAX_NUM];
};

struct ss_epoll_ctrl_s g_epoll_m[EPOLL_MAX_NUM] = {0,{0}};

int ss_epoll_create(int size)
{
    int i, epfd;

    for ( i = 0 ; i < EPOLL_MAX_NUM ; i++ )
    {
        if ( ss_atomic_cas(&g_epoll_m[i].epoll_fd, 0, 1) )
        {
            break;
        }
    }

    errno = 0;
    epfd = epoll_create1(EPOLL_CLOEXEC);
    if ( epfd < 0 )
    {
        printf("epoll create failed! %d\n", errno );
        return -1;
    }

    g_epoll_m[i].epoll_fd = epfd;

    return 10000 + i;
}

int ss_epoll_ctl(int epfd, int op, int fd, struct epoll_event * pevent)
{
    int ret;
    int events;
    struct ss_epoll_ctrl_s * p_epoll;

    if ( epfd < 10000 )
    {
        printf("ss_epoll_ctl failed! epfd = %d\n", epfd );
        return -1;
    }
    
    p_epoll = &g_epoll_m[epfd - 10000];
    errno = 0;

    if ( SOCK_REL_IDX <= ( fd & SOCK_FD_MASK ) )
    {
        int i;
        struct ss_call_s ss_call;
        struct ss_parm_epoll_ctl ss_parm;
        
        ss_parm.fd     = fd;
        ss_parm.opt    = op;
        
        if ( op == EPOLL_CTL_DEL )
        {
            ss_parm.events = 0;
        }
        else
        {
            ss_parm.events = pevent->events;
        }

        ss_call.call_idx = SS_CALL_EPOLL_CTL;
        ss_call.param    = (void *)&ss_parm;

        ret = ss_call_remote(&ss_call);
        if ( ret < 0)
        {
            printf("epoll ctl failed! (ret %d , errno %d) \n", 
                    ss_call.ret, ss_call.err );
            return -1;
        }

        if ( op == EPOLL_CTL_DEL )
        {
            for ( i = 0 ; i < EPOLL_MAX_NUM ; i++ )
            {
                if ( p_epoll->sock_fd[i] == fd )
                {
                    p_epoll->sock_fd[i] = 0;
                    break;
                }
            }
        }
        else if ( op == EPOLL_CTL_ADD )
        {
            // ·ÀÖØ¸´×¢²á
            for ( i = 0 ; i < EPOLL_MAX_NUM ; i++ )
            {
                if ( p_epoll->sock_fd[i] == fd )
                {
                    return 0;
                }
            }

            for ( i = 0 ; i < EPOLL_MAX_NUM ; i++ )
            {
                if ( p_epoll->sock_fd[i] == 0 )
                {
                    p_epoll->sock_fd[i] = fd;
                    break;
                }
            }
        }

        return 0;
    }
    else
    {
        ret = epoll_ctl(p_epoll->epoll_fd, op, fd, pevent);
    }

    return ret;
}

int ss_epoll_wait(int epfd, struct epoll_event * pevents, int maxevents, int timeout)
{
    int event;
    int fd;
    int i;
    int cnt;
    struct ss_epoll_ctrl_s * p_epoll;
    struct epoll_event events[SS_MAX_EVENTS];

    if ( epfd < 10000 )
    {
        printf("ss_epoll_ctl failed! epfd = %d\n", epfd );
        return -1;
    }
    
    p_epoll = &g_epoll_m[epfd - 10000];

    errno = 0;
    cnt = epoll_wait(p_epoll->epoll_fd, events, SS_MAX_EVENTS, 1);
    if ( cnt > 0 )
    {
        for ( i = 0 ; i < cnt; i++ )
        {
            pevents[i].events  = events[i].events;
            pevents[i].data.fd = events[i].data.fd;
        }
    }

    for ( i = 0 ; i < EPOLL_MAX_NUM; i++ )
    {
        fd = p_epoll->sock_fd[i];
        if ( fd == 0 )
        {
            continue;
        }

        event = ss_socket_m_event(fd);
        if ( event > 0 )
        {
            pevents[cnt].events  = event;
            pevents[cnt].data.fd = fd;
            cnt++;
        }
    }

    return cnt;
}


