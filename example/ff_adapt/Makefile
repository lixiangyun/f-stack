TOPDIR=..

DEBUG=-O0 -gdwarf-2 -g3

ifndef DEBUG
HOST_CFLAGS = -O2
else
HOST_CFLAGS = ${DEBUG} 
endif

LIBS= -Wl,--no-whole-archive -lrt -lm -ldl -lpthread

TARGET="demo"
all:
	cc ${HOST_CFLAGS} -o ${TARGET}_socket main_socket.c main_epoll_m.c ${LIBS}

.PHONY: clean
clean:
	rm -f *.o ${TARGET} ${TARGET}_epoll
