CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread -std=c11
SRCDIR = src
TARGETS = thread_race prodcons

all: $(TARGETS)

thread_race: $(SRCDIR)/thread_race.c
	$(CC) $(CFLAGS) $(SRCDIR)/thread_race.c -o thread_race

prodcons: $(SRCDIR)/prodcons.c
	$(CC) $(CFLAGS) $(SRCDIR)/prodcons.c -o prodcons

run_race_unsync: thread_race
	@echo "=== Тест гонки данных (unsync) ==="
	./thread_race 4 1000000 unsync

run_race_mutex: thread_race
	@echo "=== Тест с мьютексом (mutex) ==="
	./thread_race 4 1000000 mutex

run_race_atomic: thread_race
	@echo "=== Тест с атомиками (atomic) ==="
	./thread_race 4 1000000 atomic

run_prodcons: prodcons
	@echo "=== Тест Producer-Consumer ==="
	./prodcons -P 2 -C 2 -N 100000 -B 64

run_prodcons_large: prodcons
	@echo "=== Тест Producer-Consumer (большой) ==="
	./prodcons -P 4 -C 4 -N 1000000 -B 128

run_all_tests: all
	@echo "=== Запуск всех тестов ==="
	@make run_race_unsync
	@make run_race_mutex  
	@make run_race_atomic
	@make run_prodcons

debug: CFLAGS += -g -DDEBUG
debug: clean all

clean:
	rm -f $(TARGETS) *.o

help:
	@echo "Доступные команды:"
	@echo "  all              - собрать все программы"
	@echo "  clean            - очистить скомпилированные файлы"
	@echo "  debug            - собрать с отладочной информацией"
	@echo "  run_race_unsync  - запуск теста гонки данных (unsync)"
	@echo "  run_race_mutex   - запуск теста с мьютексом"
	@echo "  run_race_atomic  - запуск теста с атомиками"
	@echo "  run_prodcons     - запуск теста producer-consumer"
	@echo "  run_all_tests    - запуск всех тестов"
	@echo "  help             - показать эту справку"

.PHONY: all clean debug run_race_unsync run_race_mutex run_race_atomic run_prodcons run_prodcons_large run_all_tests help