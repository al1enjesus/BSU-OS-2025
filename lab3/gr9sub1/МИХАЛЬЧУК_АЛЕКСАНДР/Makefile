CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread -std=c99

all: thread_race prodcons

thread_race: src/thread_race.c
	$(CC) $(CFLAGS) src/thread_race.c -o thread_race

prodcons: src/prodcons.c
	$(CC) $(CFLAGS) src/prodcons.c -o prodcons

# Запуск тестов гонки данных
run_race: thread_race
	./thread_race 4 1000000 unsync
	./thread_race 4 1000000 mutex
	./thread_race 4 1000000 atomic

# Запуск producer-consumer
run_prodcons: prodcons
	./prodcons -P 2 -C 2 -N 100000 -B 64

# Отдельные цели для каждого режима
run_unsync: thread_race
	./thread_race 4 1000000 unsync

run_mutex: thread_race
	./thread_race 4 1000000 mutex

run_atomic: thread_race
	./thread_race 4 1000000 atomic

# Все тесты сразу
run_all: run_race run_prodcons

clean:
	rm -f thread_race prodcons

.PHONY: all clean run_race run_prodcons run_unsync run_mutex run_atomic run_all
