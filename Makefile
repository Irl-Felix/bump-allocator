CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
OPTFLAGS = -O2

all: bench debug

bench: main.c arena.c
	$(CC) $(CFLAGS) $(OPTFLAGS) -o bench main.c arena.c

debug: main.c arena.c
	$(CC) $(CFLAGS) -O0 -o debug main.c arena.c

clean:
	rm -f bench debug *.o