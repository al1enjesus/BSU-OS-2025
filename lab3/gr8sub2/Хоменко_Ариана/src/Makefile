CC=gcc
CFLAGS=-Wall -Wextra -O2 -pthread

all: thread_race prodcons prodcons_sem

thread_race: thread_race.c
	$(CC) $(CFLAGS) thread_race.c -o thread_race

prodcons: prodcons.c
	$(CC) $(CFLAGS) prodcons.c -o prodcons

prodcons_sem: prodcons_sem.c
	$(CC) $(CFLAGS) prodcons_sem.c -o prodcons_sem

run_race_unsync: thread_race
	./thread_race 4 1000000 unsync

run_race_mutex: thread_race
	./thread_race 4 1000000 mutex

run_race_atomic: thread_race
	./thread_race 4 1000000 atomic

run_prodcons: prodcons
	./prodcons -P 2 -C 2 -N 100000 -B 64

run_prodcons_sem: prodcons_sem
	./prodcons_sem -P 2 -C 2 -N 100000 -B 64

clean:
	rm -f thread_race prodcons prodcons_sem

.PHONY: all clean run_race_unsync run_race_mutex run_race_atomic run_prodcons run_prodcons_sem
