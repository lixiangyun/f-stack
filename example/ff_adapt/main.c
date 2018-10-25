#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "example.h"
#include "ss_api.h"


int main(int argc, char * argv[])
{
    int i;
    int flag = 0;
    pthread_t tid;

    ss_init(argc, argv);

    pthread_create(&tid, NULL, (void *(*)(void*))ss_run, NULL);

    for ( i = 0 ; i < argc ; i++ )
    {
        if ( 0 == strcmp(argv[i],"s") )
        {
            flag = 1;
        }

        if ( 0 == strcmp(argv[i],"c") )
        {
            flag = 0;
        }
    }

    if ( flag )
    {
        // s p 1100
        server_init(argc, argv);
    }
    else
    {
        // c a 127.0.0.1 p 1100
        client_init(argc, argv);
    }

    return 0;
}

