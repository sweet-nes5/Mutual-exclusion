CC=gcc
CFLAGS=-Wall -g -std=c11 -D_POSIX_C_SOURCE

CPPFLAGS= -D_GNU_SOURCE -DNDEBUG
LDLIBS=-lrt -pthread

-D_POSIX_C_SOURCE=200809L

ALL = main

all : $(ALL)

rl_lock_library.o : rl_lock_library.c rl_lock_library.h

main : main.c rl_lock_library.o

clean:
		-rm *.o $(ALL) 
cleanall:
		rm -rf *~ $(ALL)		
