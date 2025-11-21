CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
LDFLAGS =

TARGETS = memory_profiler memory_info io_benchmark disk_test

all: $(TARGETS)

memory_profiler: src/memory_profiler.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

memory_info: src/memory_info.c
	$(CC) $(CFLAGS) -D_GNU_SOURCE $< -o $@ $(LDFLAGS)

io_benchmark: src/io_benchmark.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

disk_test: src/disk_test.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGETS)
	rm -f *.o
	rm -f test*.bin
	rm -f testfile.bin

run_memory_profiler: memory_profiler
	@echo "=== Запуск memory_profiler ==="
	@echo "Использование: ./memory_profiler <PID>"
	@echo "Пример: ./memory_profiler $$$$"
	./memory_profiler $$$$

run_memory_info: memory_info
	./memory_info

run_io_benchmark: io_benchmark
	./io_benchmark

run_disk_test: disk_test
	./disk_test

full_test: all
	@echo "=== Полное тестирование всех программ ==="
	@echo ""
	@echo "1. memory_profiler:"
	@./memory_profiler $$$$
	@echo ""
	@echo "2. memory_info:"
	@./memory_info
	@echo ""
	@echo "3. io_benchmark:"
	@./io_benchmark
	@echo ""
	@echo "4. disk_test:"
	@./disk_test

help:
	@echo ""
	@echo "  all                 - Собрать все программы"
	@echo "  memory_profiler     - Собрать memory_profiler"
	@echo "  memory_info         - Собрать memory_info"
	@echo "  io_benchmark        - Собрать io_benchmark"
	@echo "  disk_test           - Собрать disk_test"
	@echo "  clean               - Удалить скомпилированные файлы"
	@echo "  full_test           - Полное тестирование"
	@echo ""
	@echo "  run_memory_profiler - Запуск memory_profiler"
	@echo "  run_memory_info     - Запуск memory_info"
	@echo "  run_io_benchmark    - Запуск io_benchmark"
	@echo "  run_disk_test       - Запуск disk_test"
	@echo ""
	@echo "  help       - Эта справка"

.PHONY: all clean test help full_test check monitor \
        run_memory_profiler run_memory_info run_io_benchmark run_disk_test
