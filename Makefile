#Sample Makefile for Malloc
CC=gcc
CFLAGS=-g -O0 -fPIC -Werror -Wall
#CFLAGS = 

TESTS=t-test1 test1
HEADERS= ma_util.h buddy_util.h buddy.h malloc.h free.h calloc.h realloc.h

all: ${TESTS} libmalloc.so

clean:
	rm -rf *.o *.so ${TESTS} *.log

libmalloc.so: malloc.o free.o calloc.o realloc.o buddy.o buddy_util.o ma_util.o memalign.o posix_memalign.o reallocarray.o
	$(CC) $(CFLAGS) -shared -Wl,--unresolved-symbols=ignore-all $^ -o $@

test1: test1.o
	$(CC) $(CFLAGS) $< -o $@

t-test1: t-test1.o
	$(CC) $(CFLAGS) $< -o $@ -lpthread

%: %.c
	$(CC) $(CFLAGS) $< -o $@ -lpthread

# For every XYZ.c file, generate XYZ.o.
%.o: %.c ${HEADERS}
	$(CC) $(CFLAGS) $< -c -o $@


check1:	libmalloc.so test1 t-test1
	LD_PRELOAD=`pwd`/libmalloc.so ./t-test1

check:	libmalloc.so ${TESTS}
	LD_PRELOAD=`pwd`/libmalloc.so ./test1

debug1: libmalloc.so test1 t-test1
	LD_PRELOAD=`pwd`/libmalloc.so valgrind -v --tool=memcheck --leak-check=full --show-leak-kinds=all --log-file=valgrind.log ./t-test1

dist: clean
	dir=`basename $$PWD`; cd ..; tar cvf $$dir.tar ./$$dir; gzip $$dir.tar
