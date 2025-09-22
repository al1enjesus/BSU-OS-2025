CC=gcc
CFLAGS=-Wall -Wextra -O2 -pthread

all: thread_race prodcons

thread_race: thread_race.c
	$(CC) $(CFLAGS) thread_race.c -o thread_race

prodcons: prodcons.c
	$(CC) $(CFLAGS) prodcons.c -o prodcons

run_race_unsync: thread_race
	./thread_race 4 1000000 unsync

run_race_mutex: thread_race
	./thread_race 4 1000000 mutex

run_race_atomic: thread_race
	./thread_race 4 1000000 atomic

run_prodcons: prodcons
	./prodcons -P 2 -C 2 -N 100000 -B 64

clean:
	rm -f thread_race prodcons

.PHONY: all clean run_race_unsync run_race_mutex run_race_atomic run_prodcons


