CC=gcc
CFLAGS=-Wall -g -std=c11 -D_POSIX_C_SOURCE=200809L
CPPFLAGS=-D_GNU_SOURCE -DNDEBUG
LDLIBS=-lrt -pthread

ALL = main

all : $(ALL)

rl_lock_library.o : rl_lock_library.c rl_lock_library.h

main : main.c rl_lock_library.o
	@echo "Building $@"
	@$(CC) $(CFLAGS) $(CPPFLAGS) $^ -o $@ $(LDLIBS) -v

clean:
		-rm *.o $(ALL)

cleanall:
		rm -rf *~ $(ALL)

