CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread
TARGETS = thread_race prodcons

all: $(TARGETS)

thread_race: thread_race.c
	$(CC) $(CFLAGS) -o thread_race thread_race.c

prodcons: prodcons.c
	$(CC) $(CFLAGS) -o prodcons prodcons.c

clean:
	rm -f $(TARGETS)

run_race_unsync:
	./thread_race 4 1000000 unsync

run_race_mutex:
	./thread_race 4 1000000 mutex

run_race_atomic:
	./thread_race 4 1000000 atomic

run_prodcons:
	./prodcons -P 2 -C 2 -N 100000 -B 64

.PHONY: all clean run_race_unsync run_race_mutex run_race_atomic run_prodcons
