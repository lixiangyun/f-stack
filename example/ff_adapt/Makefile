TOPDIR=..

DEBUG=-O0 -gdwarf-2 -g3

ifndef DEBUG
HOST_CFLAGS = -O2
else
HOST_CFLAGS = ${DEBUG} 
endif

LIBS= -Wl,--no-whole-archive -lrt -lm -ldl -lpthread

TARGET="example"
all:
	cc ${HOST_CFLAGS} -o ${TARGET} main.c ss_sys.c ss_mbuff.c ss_util.c ss_api.c ff_api.c ff_epoll.c example.c ${LIBS}

.PHONY: clean
clean:
	rm -f *.o ${TARGET}
