#!/bin/bash

conf=config.ini
bin=./helloworld_epoll
num_procs=5
others="port 80"

for((proc_id=0; proc_id<${num_procs}; ++proc_id))
do
    if ((proc_id == 0))
    then
        echo "${bin} --conf ${conf} --proc-type=primary --proc-id=${proc_id} ${others}"
        ${bin} --conf ${conf} --proc-type=primary --proc-id=${proc_id} ${others} &
		sleep 5
    else
        echo "${bin} --conf ${conf} --proc-type=secondary --proc-id=${proc_id} ${others}"
        ${bin} --conf ${conf} --proc-type=secondary --proc-id=${proc_id} ${others} &
    fi
	sleep 5
done

#./helloworld_epoll --conf config.ini --proc-type=primary --proc-id=0 port 80
