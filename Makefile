CC = gcc
# Feature-test macros to expose POSIX APIs (clock_gettime/CLOCK_MONOTONIC)
# and platform-specific extras (e.g., MAP_ANON on Darwin, MAP_ANONYMOUS on Linux).
UNAME_S := $(shell uname -s)
CPPFLAGS = -D_POSIX_C_SOURCE=200809L

ifeq ($(UNAME_S),Linux)
CPPFLAGS += -D_GNU_SOURCE
endif

ifeq ($(UNAME_S),Darwin)
CPPFLAGS += -D_DARWIN_C_SOURCE
endif

CFLAGS = -Wall -Wextra -std=c11 -g
OPTFLAGS = -O2

all: bench debug

bench: main.c arena.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(OPTFLAGS) -o bench main.c arena.c

debug: main.c arena.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -O0 -o debug main.c arena.c

clean:
	rm -f bench debug *.o