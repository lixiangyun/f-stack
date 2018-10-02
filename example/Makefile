TOPDIR=..

ifeq ($(FF_PATH),)
	FF_PATH=${TOPDIR}
endif

ifeq ($(FF_DPDK),)
	FF_DPDK=${TOPDIR}/dpdk/x86_64-native-linuxapp-gcc
endif

#DEBUG=-O0 -gdwarf-2 -g3

ifndef DEBUG
HOST_CFLAGS = -O2
else
HOST_CFLAGS = ${DEBUG} 
endif

LIBS+= -L${FF_PATH}/lib -Wl,--whole-archive,-lfstack,--no-whole-archive
LIBS+= -L${FF_DPDK}/lib -Wl,--whole-archive,-ldpdk,--no-whole-archive
LIBS+= -Wl,--no-whole-archive -lrt -lm -ldl -lcrypto -pthread -lnuma

TARGET="helloworld"
all:
	cc ${HOST_CFLAGS} -I../lib -o ${TARGET} main.c ${LIBS}
	cc ${HOST_CFLAGS} -I../lib -o ${TARGET}_epoll main_epoll.c ${LIBS}
	cc ${HOST_CFLAGS} -I../lib -o ${TARGET}_epoll_os main_epoll_os.c ${LIBS}
	cc ${HOST_CFLAGS} -I../lib -o ${TARGET}_epoll_m main_epoll_m.c ${LIBS}

.PHONY: clean
clean:
	rm -f *.o ${TARGET} ${TARGET}_epoll
