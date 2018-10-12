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

struct ss_epoll_data_s {
    int    type;     // 0 : unix fd, 1 ss socket event fd;
    int    fd;
    void * pdata;    // epoll_data_t * or struct ss_socket_m *;
};

struct ss_call_que g_ss_call_que;
int g_ff_epfd = -1;


struct ss_events {
    struct ss_list link;
    int ss_events;
    struct ss_socket_m * p_ss_socket;
};

struct ss_epoll_ctrl_s {
    int epollfd;
    int eventfd;
    struct ss_list link;
};


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
            remain = remain - tmp;
            cnt    = cnt    + tmp;
        }

        p_ss_socket->event_s = p_ss_socket->event_s | EPOLLOUT;
        if ( eventfd_write(p_ss_socket->event_fd, (eventfd_t)(1) ) < 0 )
        {
            printf("eventfd_write failed! errno = %d\n", errno );
        }
    }

    pthread_mutex_unlock(&p_ss_socket->lock);
}








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

            p_ff_event_data->ff_events    = 0;
            p_ff_event_data->ff_socket_fd = p_ss_socket->socket_ff;
            p_ff_event_data->ss_socket_s  = p_ss_socket;

            ss_parm.ff_events     = pevent->events;
        }
        else
        {
            ss_parm.ff_events     = 0;
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

        event.events  = pevent->events;
        event.data.ptr = (void *)pdata;

        ret = epoll_ctl(epfd - 10000, op, p_ss_socket->event_fd, &event);

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

    sem_destroy(&pcall->sem);

    return 0;
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
            if ( NULL == parm->ff_event_data )
            {
                ret = ff_epoll_ctl(g_ff_epfd, parm->ff_opt, parm->ff_socket_fd, NULL );
            }
            else
            {
                struct epoll_event event;
                
                event.data.ptr = (void *)parm->ff_event_data;
                event.events   = parm->ff_events;
                
                ret = ff_epoll_ctl(g_ff_epfd, parm->ff_opt, parm->ff_socket_fd, &event);
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
    struct ss_call_s * pcall;
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

    for ( ; pcall != NULL ; pcall = pcall->pnext )
    {
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

