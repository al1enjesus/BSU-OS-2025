CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread

all: thread_race prodcons

thread_race: thread_race.c
	$(CC) $(CFLAGS) $< -o $@

prodcons: prodcons.c
	$(CC) $(CFLAGS) $< -o $@

run_race_mutex: thread_race
	./thread_race 4 1000000 mutex

run_race_atomic: thread_race
	./thread_race 4 1000000 atomic

run_prodcons_small: prodcons
	./prodcons -P 2 -C 2 -N 100000 -B 64

run_prodcons_big: prodcons
	./prodcons -P 4 -C 4 -N 1000000 -B 256

clean:
	rm -f thread_race prodcons

.PHONY: all run_race_mutex run_race_atomic run_prodcons_small run_prodcons_big clean

