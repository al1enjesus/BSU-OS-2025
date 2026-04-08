CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread -std=c11
TARGETS = thread_race prodcons

all: $(TARGETS)

thread_race: src/thread_race.c
	$(CC) $(CFLAGS) -o $@ $<

prodcons: src/prodcons.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)

run_race: thread_race
	./thread_race 4 1000000 mutex
	./thread_race 4 1000000 atomic

run_prodcons: prodcons
	./prodcons -P 2 -C 2 -N 100000 -B 64

test: all run_race run_prodcons

.PHONY: all clean run_race run_prodcons test
