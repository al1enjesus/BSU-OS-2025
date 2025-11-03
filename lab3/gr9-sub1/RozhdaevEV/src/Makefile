CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread
TARGETS = thread_race prodcons

all: $(TARGETS)

thread_race: thread_race.c
	$(CC) $(CFLAGS) -o $@ $<

prodcons: prodcons.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)

run_race_mutex: thread_race
	./thread_race 4 250000 mutex

run_race_atomic: thread_race
	./thread_race 4 250000 atomic

run_race_unsync: thread_race
	./thread_race 4 300000 unsync

run_prodcons: prodcons
	./prodcons -P 3 -C 2 -N 50000 -B 30

run_prodcons_large: prodcons
	./prodcons -P 4 -C 1 -N 80000 -B 20

.PHONY: all clean run_race_mutex run_race_atomic run_race_unsync run_prodcons run_prodcons_large
