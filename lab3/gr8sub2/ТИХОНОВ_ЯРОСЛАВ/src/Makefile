CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread

N ?= 4
M ?= 1000000
MODE ?= mutex


.PHONY: all clean run_race run_prodcons


all: thread_race prodcons


thread_race: thread_race.c
	$(CC) $(CFLAGS) -o $@ $^


prodcons: prodcons.c
	$(CC) $(CFLAGS) -o $@ $^


run_race: thread_race
	@echo "Running: ./thread_race $(N) $(M) $(MODE)"
	./thread_race $(N) $(M) $(MODE)


run_prodcons: prodcons
	@echo "Running: ./prodcons -P 2 -C 2 -N $(M) -B 64"
	./prodcons -P 2 -C 2 -N $(M) -B 64


clean:
	rm -f thread_race prodcons