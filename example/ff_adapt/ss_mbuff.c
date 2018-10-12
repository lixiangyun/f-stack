#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>

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

            if ( NULL == pbuff->ptail )
            {
                pbuff->pnext = pcur;
            }
            pcur->pnext  = pbuff->ptail;
            pbuff->ptail = pcur;
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


