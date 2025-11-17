# Makefile для Lab 4 - Управление памятью и файловый I/O

CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS =

# Цели
TARGETS = memory_info io_benchmark memory_profiler

# Основная цель - собрать всё
all: $(TARGETS)

# Индивидуальные программы
memory_info: memory_info.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

io_benchmark: io_benchmark.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

memory_profiler: memory_profiler.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Очистка
clean:
	rm -f $(TARGETS)
	rm -f *.o
	rm -f test*.bin
	rm -f testfile.bin

# Запуск тестов
test: all
	@echo "=== Running tests ==="
	@echo ""
	@echo "1. Memory Info (self)"
	@./memory_info &
	@PID=$$!; sleep 2; ./memory_info $$PID; kill $$PID 2>/dev/null || true
	@echo ""
	@echo "2. IO Benchmark (100MB)"
	@./io_benchmark
	@echo ""
	@echo "All tests completed. Check output above."

# Запуск примеров
run_memory_info:
	./memory_info

run_profiler:
	firefox & sleep 2; ./memory_profiler $$(pgrep firefox) --watch

# Бенчмарк всех методов I/O
benchmark: io_benchmark
	@echo "Running I/O benchmarks..."
	@echo "This will create test files and measure performance."
	@echo ""
	./io_benchmark

# Помощь
help:
	@echo "Lab 4 Makefile - Available targets:"
	@echo ""
	@echo "  all              - Build all programs"
	@echo "  clean            - Remove compiled binaries and test files"
	@echo "  test             - Run basic tests"
	@echo ""
	@echo "Individual programs:"
	@echo "  memory_info      - Build memory info analyzer"
	@echo "  memory_profiler  - Build memory profiler (main utility)"
	@echo ""
	@echo "Run examples:"
	@echo "  run_memory_info  - Run memory info demo"
	@echo "  run_profiler     - Run memory profiler (firefox)"
	@echo "  benchmark        - Run full I/O benchmark suite"
	@echo ""

.PHONY: all clean test help benchmark \
        run_memory_info \
		run_profiler
