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


int g_ff_epfd = -1;

struct ss_call_que g_ss_call_que = {PTHREAD_MUTEX_INITIALIZER,0,NULL,NULL};

struct ss_call_func {
    int idx;
    void (*func)(struct ss_call_s *);
};

struct ss_call_func ss_call_funcs[] = {
    { SS_CALL_SOCKET, ss_socket_call },
    { SS_CALL_SETSOCKOPT, ss_setsockopt_call },
    { SS_CALL_GETSOCKOPT, ss_getsockopt_call },
    { SS_CALL_LISTEN, ss_listen_call },
    { SS_CALL_BIND, ss_bind_call },
    { SS_CALL_CONNECT, ss_connect_call },
    { SS_CALL_CLOSE, ss_close_call },
    { SS_CALL_GETPEERNAME, ss_getpeername_call },
    { SS_CALL_GETSOCKNAME, ss_getsockname_call },
    { SS_CALL_EPOLL_CTL, ss_epoll_ctl_call },
};

struct ss_socket_m g_ss_socket[SOCK_MAX_NUM] = {0,0,0,0,NULL,NULL,NULL};

struct ss_socket_m * ss_socket_m_alloc(void)
{
    int i;
    struct ss_socket_m * p_socket;

    for ( i = SOCK_REL_IDX ; i < SOCK_MAX_NUM ; i++ )
    {
        p_socket = (struct ss_socket_m *)&g_ss_socket[i];
        if ( 0 == (p_socket->status & SS_USED_FLAG) )
        {
            break;
        }
    }
    if ( i == SOCK_MAX_NUM )
    {
        return NULL;
    }

    p_socket->status  = SS_USED_FLAG;
    p_socket->sockfd  = 0;
    p_socket->idx     = i;
    p_socket->ref++;
    
    p_socket->paccept = NULL;
    p_socket->pread   = ss_buff_alloc();
    p_socket->pwrite  = ss_buff_alloc();
    
    return p_socket;
}

void ss_socket_m_free(struct ss_socket_m * p_socket)
{
    p_socket->status  = 0;
    p_socket->paccept = NULL;
    p_socket->ref++;

    ss_buff_free(p_socket->pread);
    ss_buff_free(p_socket->pwrite);
}

struct ss_socket_m * ss_socket_m_get(int ss_fd)
{
    int ref = (ss_fd >> 16);
    struct ss_socket_m * p_socket = &g_ss_socket[ss_fd & SOCK_FD_MASK];

    if ( p_socket->ref != ref )
    {
        printf("get socket failed (fd %d, ref %d != %d)!\n", ss_fd,
                p_socket->ref, ref);
        return NULL;
    }

    return p_socket;
}

int ss_socket_m_event(int ss_fd)
{
    int event = 0;
    struct ss_socket_m * p_socket = ss_socket_m_get(ss_fd);

    if ( NULL == p_socket )
    {
        return EPOLLERR;
    }

    if ( p_socket->status & SS_CONN_STAT )
    {
        return EPOLLERR;
    }

    if (( p_socket->status & SS_SOCK_TYPE) == SS_SERVER )
    {
        if ( p_socket->paccept != NULL )
        {
            event |= EPOLLIN;
        }
    }
    else if (( p_socket->status & SS_SOCK_TYPE) == SS_CLIENT )
    {
        if ( ss_buff_size(p_socket->pread) > 0 )
        {
            event |= EPOLLIN;
        }

        if ( ss_buff_space(p_socket->pwrite) > 0 )
        {
            event |= EPOLLOUT;
        }
    }

    return event;
}


void ss_socket_call( struct ss_call_s * pcall )
{
    int fd;
    struct ss_parm_socket *parm = (struct ss_parm_socket *)pcall->param;
    struct ss_socket_m * p_socket;

    p_socket = ss_socket_m_alloc();
    if ( NULL == p_socket )
    {
        printf("no free socket!\n");

        pcall->ret = -1;
        pcall->err = ENOMEM;
        return;
    }

    fd = ff_socket(parm->domain, parm->type, parm->protocol);
    if ( fd < 0)
    {
        printf("call ff socket failed!(ret %d , errno %d)\n", fd, errno );
        
        ss_socket_m_free(p_socket);
        pcall->ret = -1;
    }
    else
    {
        p_socket->sockfd = fd;
        pcall->ret = p_socket->idx + (p_socket->ref << 16);
    }

    pcall->err = errno;
}

void ss_setsockopt_call( struct ss_call_s * pcall )
{
    int ret;
    struct ss_parm_setsockopt *parm = (struct ss_parm_setsockopt *)pcall->param;
    struct ss_socket_m * p_socket = ss_socket_m_get(parm->s);
    
    if ( NULL == p_socket )
    {
        pcall->ret = -1;
        pcall->err = EINVAL;
        return;
    }
    
    ret = ff_setsockopt(p_socket->sockfd, parm->level, parm->optname, (const void *)parm->optval, parm->optlen);
    pcall->ret = ret;
    pcall->err = errno;
}

void ss_getsockopt_call( struct ss_call_s * pcall )
{
    int ret;
    struct ss_parm_getsockopt *parm = (struct ss_parm_getsockopt *)pcall->param;
    struct ss_socket_m * p_socket = ss_socket_m_get(parm->s);
    
    if ( NULL == p_socket )
    {
        pcall->ret = -1;
        pcall->err = EINVAL;
        return;
    }

    ret = ff_getsockopt(p_socket->sockfd, parm->level, parm->optname, parm->optval, parm->optlen);
    pcall->ret = ret;
    pcall->err = errno;
}

void ss_listen_call( struct ss_call_s * pcall )
{
    int ret;
    struct ss_parm_listen *parm = (struct ss_parm_listen *)pcall->param;
    struct ss_socket_m * p_socket = ss_socket_m_get(parm->s);
    
    if ( NULL == p_socket )
    {
        pcall->ret = -1;
        pcall->err = EINVAL;
        return;
    }
    
    ret = ff_listen(p_socket->sockfd, parm->backlog);

    pcall->ret = ret;
    pcall->err = errno;
}

void ss_bind_call( struct ss_call_s * pcall )
{
    int ret;
    struct ss_parm_bind *parm = (struct ss_parm_bind *)pcall->param;
    struct ss_socket_m * p_socket = ss_socket_m_get(parm->s);
    
    if ( NULL == p_socket )
    {
        pcall->ret = -1;
        pcall->err = EINVAL;
        return;
    }
    
    ret = ff_bind(p_socket->sockfd, (const struct linux_sockaddr *)parm->addr, parm->addrlen);
    if ( 0 == ret )
    {
        p_socket->status |= SS_SERVER;
    }
    
    pcall->ret = ret;
    pcall->err = errno;
}

void ss_connect_call( struct ss_call_s * pcall )
{
    int ret;
    struct ss_parm_connect *parm = (struct ss_parm_connect *)pcall->param;
    struct ss_socket_m * p_socket = ss_socket_m_get(parm->s);
    
    if ( NULL == p_socket )
    {
        pcall->ret = -1;
        pcall->err = EINVAL;
        return;
    }
    
    ret = ff_connect(p_socket->sockfd, (const struct linux_sockaddr *)parm->name, parm->namelen);
    if ( 0 == ret )
    {
        p_socket->status |= SS_CLIENT;
    }

    pcall->ret = ret;
    pcall->err = errno;
}

void ss_close_call( struct ss_call_s * pcall )
{
    int ret;
    struct ss_parm_close *parm = (struct ss_parm_close *)pcall->param;
    struct ss_socket_m * p_socket = ss_socket_m_get(parm->fd);
    
    if ( NULL == p_socket )
    {
        pcall->ret = -1;
        pcall->err = EINVAL;
        return;
    }
    
    ret = ff_close(p_socket->sockfd);
    if ( ret == 0 )
    {
        ss_socket_m_free(p_socket);
    }

    pcall->ret = ret;
    pcall->err = errno;
}

void ss_getpeername_call( struct ss_call_s * pcall )
{
    int ret;
    struct ss_parm_getpeername *parm = (struct ss_parm_getpeername *)pcall->param;
    struct ss_socket_m * p_socket = ss_socket_m_get(parm->s);
    
    if ( NULL == p_socket )
    {
        pcall->ret = -1;
        pcall->err = EINVAL;
        return;
    }
    
    ret = ff_getpeername(parm->s, (struct linux_sockaddr *)parm->name, parm->namelen);

    pcall->ret = ret;
    pcall->err = errno;
}

void ss_getsockname_call( struct ss_call_s * pcall )
{
    int ret;
    struct ss_parm_getsockname *parm = (struct ss_parm_getsockname *)pcall->param;
    struct ss_socket_m * p_socket = ss_socket_m_get(parm->s);
    
    if ( NULL == p_socket )
    {
        pcall->ret = -1;
        pcall->err = EINVAL;
        return;
    }
    
    ret = ff_getsockname(parm->s, (struct linux_sockaddr *)parm->name, parm->namelen);

    pcall->ret = ret;
    pcall->err = errno;
}

void ss_epoll_ctl_call( struct ss_call_s * pcall )
{
    int ret;
    struct ss_parm_epoll_ctl *parm = (struct ss_parm_epoll_ctl *)pcall->param;
    struct ss_socket_m * p_socket = ss_socket_m_get(parm->fd);
    
    if ( NULL == p_socket )
    {
        pcall->ret = -1;
        pcall->err = EINVAL;
        return;
    }

    printf("ff epoll ctl opt %d, sockfd %d, events %d\n", parm->opt, p_socket->sockfd, parm->events );
    
    if ( EPOLL_CTL_DEL == parm->opt )
    {
        ret = ff_epoll_ctl(g_ff_epfd, parm->opt, p_socket->sockfd, NULL );
    }
    else
    {
        struct epoll_event event;
        
        event.data.fd  = parm->fd;
        event.events   = parm->events;
        
        ret = ff_epoll_ctl(g_ff_epfd, parm->opt, p_socket->sockfd, &event );
    }

    pcall->ret = ret;
    pcall->err = errno;
}


int ss_call_remote( struct ss_call_s * pcall )
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

    sem_destroy(&pcall->sem);

    return 0;
}

void ss_call_proc( struct ss_call_s * pcall )
{
    int ret = 0;

    pcall->ret = 0;

    if ( pcall->call_idx < SS_CALL_END )
    {
        ss_call_funcs[pcall->call_idx].func(pcall);
    }
    
    printf("ff remote call idx %d , parm %p, ret %d \n", pcall->call_idx, pcall->param , pcall->ret );

    ret = sem_post(&pcall->sem);
    if ( ret < 0 )
    {
        printf("sem post failed!\n");
    }
}

void ss_call_loop()
{
    struct ss_call_s * pcall;
    struct ss_call_s * pnext;
    
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

    for ( ; pcall != NULL ; pcall = pnext )
    {
        pnext = pcall->pnext;
        ss_call_proc(pcall);
    }
}

void ss_buff_accept( struct ss_socket_m * p_socket )
{
    int client_fd;
    struct ss_accept_s * p_accept = NULL;
    struct ss_socket_m * p_cli;

    for (;;)
    {
        p_accept = (struct ss_accept_s *)malloc(sizeof(struct ss_accept_s));
        
        client_fd = ff_accept(p_socket->sockfd, (struct linux_sockaddr *)&p_accept->addr, &p_accept->addrlen);
        if ( client_fd < 0 )
        {
            free(p_accept);
            break;
        }

        printf("ss buff accept get client fd %d !\n", client_fd);
        
        p_cli = ss_socket_m_alloc();
        p_cli->status |= SS_CLIENT;
        p_cli->sockfd  = client_fd;

        p_accept->fd    = ( p_cli->idx + (p_cli->ref << 16) );
        
        p_accept->pnext   = p_socket->paccept;
        p_socket->paccept = p_accept;
    }
}

void ss_buff_connect(struct ss_socket_m * p_socket, int events )
{
    char stbuf[4096];

    if ( events & EPOLLIN )
    {
        ssize_t cnt, remain;

        remain = ss_buff_space(p_socket->pread);
        cnt = (remain < sizeof(stbuf)) ? remain : sizeof(stbuf);

        cnt = ff_read(p_socket->sockfd, stbuf, cnt );
        if ( 0 == cnt || ( cnt < 0 && errno != EAGAIN ) )
        {
            printf("ff connect read disable!\n");
            p_socket->status |= SS_CONN_STAT;
            return;
        }
        
        if ( cnt > 0 )
        {
            ss_buff_write(p_socket->pread, stbuf, cnt );
        }
    }

    if ( events & EPOLLOUT )
    {
        ssize_t cnt, tmp, remain;

        remain = ss_buff_size(p_socket->pwrite);
        remain = (remain < sizeof(stbuf)) ? remain : sizeof(stbuf);

        remain = ss_buff_read(p_socket->pwrite, stbuf, remain );
        for ( cnt = 0 ; remain > 0 ; )
        {
            tmp = ff_write(p_socket->sockfd, &stbuf[cnt], remain );
            if ( 0 == tmp || ( tmp < 0 && errno != EAGAIN ) )
            {
                printf("ff connect write disable!\n");
                p_socket->status |= SS_CONN_STAT;
                return;
            }
            else if ( tmp > 0 )
            {
                remain = remain - tmp;
                cnt    = cnt    + tmp;
            }
        }
    }

    if ( events & EPOLLERR )
    {
        printf("ff connect event disable!\n");
        p_socket->status |= SS_CONN_STAT;
    }
}


void ss_ffep_loop()
{
    int i;
    int nevents;
    struct epoll_event events[SS_MAX_EVENTS];

    nevents = ff_epoll_wait(g_ff_epfd, events, SS_MAX_EVENTS, 0);
    for ( i = 0; i < nevents ; i++ )
    {
        int fd     = events[i].data.fd;
        int event  = events[i].events;
        
        struct ss_socket_m * p_socket = ss_socket_m_get(fd);
        if ( NULL == p_socket )
        {
            continue;
        }

        if (( p_socket->status & SS_SOCK_TYPE) == SS_CLIENT )
        {
            ss_buff_connect(p_socket, event );
        }
        else if (( p_socket->status & SS_SOCK_TYPE) == SS_SERVER )
        {
            ss_buff_accept(p_socket);
        }
    }
}

static int ss_loop(void *arg)
{
    ss_call_loop();
    ss_ffep_loop();
    return 0;
}

// for init api
void ss_run(void)
{
    ff_run(ss_loop, NULL);
}

int ss_init(int argc, char * argv[])
{
    int ret;
    int i;

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

