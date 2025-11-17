CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread

all: thread_race prodcons

thread_race: thread_race.c
	$(CC) $(CFLAGS) $^ -o $@

prodcons: prodcons.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f thread_race prodcons

run_race:
	./thread_race 4 1000000 unsync
	./thread_race 4 1000000 mutex
	./thread_race 4 1000000 atomic

run_prodcons:
	./prodcons -P 2 -C 2 -N 100000 -B 64
