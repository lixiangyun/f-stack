#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>

#include "ss_api.h"

struct ss_buff * ss_buff_alloc(void)
{
    struct ss_buff * pbuff;

    pbuff = (struct ss_buff *)malloc(sizeof(struct ss_buff));
    if ( NULL == pbuff )
    {
        return NULL;
    }

    memset( pbuff, 0, sizeof(struct ss_buff) );

    pbuff->read = 0;
    pbuff->write = 0;

    return pbuff;
}

void ss_buff_free(struct ss_buff * pbuff)
{
    memset( pbuff, 0, sizeof(struct ss_buff) );
    free(pbuff);
}

ssize_t ss_buff_size(struct ss_buff * pbuff)
{
    if ( pbuff->write >= pbuff->read )
    {
        return (ssize_t)(pbuff->write - pbuff->read);
    }
    else
    {
        return (ssize_t)(SS_BUFF_MAX_LEN + pbuff->write - pbuff->read);
    }
}

ssize_t ss_buff_space(struct ss_buff * pbuff)
{
    return (ssize_t)(SS_BUFF_MAX_LEN - 1 - ss_buff_size(pbuff));
}

ssize_t ss_buff_write(struct ss_buff * pbuff, char *buf, size_t nbytes)
{
    ssize_t remain, cnt;

    remain = ss_buff_space(pbuff);
    cnt = ( remain < nbytes ) ? remain : nbytes ;
    if ( cnt == 0 )
    {
        return 0;
    }

    if ( ( pbuff->write + cnt ) > SS_BUFF_MAX_LEN )
    {
        remain = (SS_BUFF_MAX_LEN - pbuff->write);
        memcpy(&pbuff->body[pbuff->write], buf, remain);
        memcpy(&pbuff->body[0], buf + remain, cnt - remain);
    }
    else
    {
        memcpy(&pbuff->body[pbuff->write], buf, cnt);
    }

    pbuff->write = ( pbuff->write + cnt ) % SS_BUFF_MAX_LEN;

    return cnt;
}

ssize_t ss_buff_read(struct ss_buff * pbuff, char *buf, size_t nbytes)
{
    ssize_t size, cnt, remain;

    size = ss_buff_size(pbuff);
    cnt  = ( size < nbytes ) ? size : nbytes ;
    if ( cnt == 0 )
    {
        return 0;
    }

    if ( ( pbuff->read + cnt ) > SS_BUFF_MAX_LEN )
    {
        remain = (SS_BUFF_MAX_LEN - pbuff->read);
        memcpy(buf, &pbuff->body[pbuff->read], remain);
        memcpy(buf + remain, &pbuff->body[0], cnt - remain);
    }
    else
    {
        memcpy(buf, &pbuff->body[pbuff->read], cnt);
    }

    pbuff->read = ( pbuff->read + cnt ) % SS_BUFF_MAX_LEN;

    return cnt;
}

ssize_t ss_buff_writev(struct ss_buff * pbuff, struct iovec *iov, int iovcnt)
{
    int i;
    ssize_t tmp;
    ssize_t cnt = 0;

    for( i = 0 ; i < iovcnt; i++ )
    {
        tmp = ss_buff_write(pbuff, iov[i].iov_base, iov[i].iov_len );
        cnt += tmp;
        if ( tmp < iov[i].iov_len )
        {
            break;
        }
    }

    return cnt;
}

ssize_t ss_buff_readv(struct ss_buff * pbuff, struct iovec *iov, int iovcnt)
{
    int i;
    ssize_t tmp;
    ssize_t cnt = 0;

    for( i = 0 ; i < iovcnt; i++ )
    {
        tmp = ss_buff_read(pbuff, iov[i].iov_base, iov[i].iov_len );
        cnt += tmp;
        if ( tmp < iov[i].iov_len )
        {
            break;
        }
    }

    return cnt;
}


