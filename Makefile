CC = gcc
CFLAGS = -Wall -g -std=c11 -pedantic -D_POSIX_C_SOURCE=200809L
CPPFLAGS = -D_GNU_SOURCE -DNDEBUG
LDLIBS = -lrt -pthread

ALL = main

all: $(ALL)

rl_lock_library.o: rl_lock_library.c rl_lock_library.h

main: main.c rl_lock_library.o

clean:
	rm -rf *~
	rm -rf $(ALL) *.o

cleanall:
	rm -rf $(ALL) *.o