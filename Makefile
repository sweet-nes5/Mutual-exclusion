CC = gcc
CFLAGS = -Wall -g -std=c11 -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE -DNDEBUG
LDFLAGS = -lrt -lpthread -lc

OBJS = main.o rl_lock_library.o

all: main

main: $(OBJS)
	@echo "Building main"
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) && echo "Success" || echo "Failed"

%.o: %.c
	@echo "Compiling $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Cleaning"
	@rm -f $(OBJS) main
