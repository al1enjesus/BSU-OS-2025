CC=g++
CFLAGS=-Wall -Wextra -O2 -pthread -std=c++11
TARGETS=thread_race prodcons

all: $(TARGETS)

thread_race: src/thread_race.cpp
	$(CC) $(CFLAGS) src/thread_race.cpp -o thread_race

prodcons: src/prodcons.cpp
	$(CC) $(CFLAGS) src/prodcons.cpp -o prodcons

run_race_unsync: thread_race
	./thread_race 4 1000000 unsync

run_race_mutex: thread_race
	./thread_race 4 1000000 mutex

run_race_atomic: thread_race
	./thread_race 4 1000000 atomic

run_prodcons: prodcons
	./prodcons -P 2 -C 2 -N 100000 -B 64

clean:
	rm -f $(TARGETS)

.PHONY: all clean run_race_unsync run_race_mutex run_race_atomic run_prodcons
