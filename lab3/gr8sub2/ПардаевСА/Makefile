# Простой Makefile для Лабораторной работы 3
CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread

all: thread_race prodcons_sem

thread_race: src/thread_race.c
	$(CC) $(CFLAGS) src/thread_race.c -o thread_race

prodcons_sem: src/prodcons_sem.c
	$(CC) $(CFLAGS) src/prodcons_sem.c -o prodcons_sem

clean:
	rm -f thread_race prodcons_sem

test: all
	@echo "=== Тестирование thread_race ==="
	./thread_race 4 1000000 mutex
	./thread_race 4 1000000 atomic
	@echo "=== Тестирование producer-consumer ==="
	./prodcons_sem -P 2 -C 2 -N 10000 -B 64

.PHONY: all clean test
