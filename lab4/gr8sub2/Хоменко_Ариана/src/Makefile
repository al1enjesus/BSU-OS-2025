# Makefile для Lab 4 - Вариант 2
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
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
	@echo "2. I/O Benchmark"
	@./io_benchmark --size 10
	@echo ""
	@echo "All tests completed. Check output above."

# Запуск примеров
run_memory_info:
	./memory_info

run_io_benchmark:
	./io_benchmark --size 100

run_profiler:
	@echo "Usage: make run_profiler PID=<process_id>"
	@if [ -n "$(PID)" ]; then \
		./memory_profiler $(PID); \
	else \
		echo "Example: make run_profiler PID=$$$$"; \
		./memory_profiler $$$$; \
	fi

# Бенчмарк I/O
benchmark: io_benchmark
	@echo "Running I/O benchmarks..."
	@echo "This will create test files and measure performance."
	@echo ""
	./io_benchmark --size 100

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
	@echo "  io_benchmark     - Build I/O benchmark"
	@echo "  memory_profiler  - Build memory profiler (main utility)"
	@echo ""
	@echo "Run examples:"
	@echo "  run_memory_info  - Run memory info demo"
	@echo "  run_io_benchmark - Run I/O benchmark"
	@echo "  run_profiler     - Run memory profiler (use PID=<pid>)"
	@echo ""
	@echo "  benchmark        - Run full I/O benchmark suite"
	@echo ""
	@echo "Examples:"
	@echo "  make                    # Build everything"
	@echo "  make run_io_benchmark   # Run I/O benchmark"
	@echo "  make run_profiler PID=1234  # Profile process 1234"

.PHONY: all clean test help benchmark run_memory_info run_io_benchmark run_profiler
