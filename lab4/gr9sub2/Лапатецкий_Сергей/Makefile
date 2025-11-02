# Makefile для Lab 4 - Управление памятью и файловый I/O

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -D_POSIX_C_SOURCE=199309L
LDFLAGS =

# Цели
TARGETS = ./src/memory_info ./src/io_benchmark ./src/memory_profiler

# Основная цель - собрать всё
all: $(TARGETS)

# Индивидуальные программы
./src/memory_info: ./src/memory_info.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

./src/io_benchmark: ./src/io_benchmark.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

./src/memory_profiler: ./src/memory_profiler.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Очистка
clean:
	rm -f $(TARGETS)
	rm -f ./src/*.o
	rm -f ./src/test*.bin
	rm -f ./src/testfile.bin

# Запуск примеров
run_memory_info: ./src/memory_info
	./src/memory_info

run_io_benchmark: ./src/io_benchmark
	./src/io_benchmark --size 100

run_profiler: ./src/memory_profiler
	@echo "Usage: make run_profiler PID=<process_id>"
	@if [ -n "$(PID)" ]; then \
		./src/memory_profiler $(PID); \
	else \
		echo "Example: make run_profiler PID=$$$$"; \
		./src/memory_profiler $$$$; \
	fi

# Бенчмарк всех методов I/O
benchmark: ./src/io_benchmark
	@echo "Running I/O benchmarks..."
	@echo "This will create test files and measure performance."
	@echo ""
	./src/io_benchmark --size 100

# Помощь
help:
	@echo "Lab 4 Makefile - Available targets:"
	@echo ""
	@echo "  all              - Build all programs"
	@echo "  clean            - Remove compiled binaries and test files"
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
	@echo "  make run_profiler PID=1234  # Profile process 1234"
	@echo "  make benchmark          # Run I/O benchmarks"

# Установка (опционально - копирование в ~/bin)
install: all
	@echo "Installing to ~/bin..."
	@mkdir -p ~/bin
	@cp memory_profiler ~/bin/
	@echo "Installed memory_profiler to ~/bin/"
	@echo "Make sure ~/bin is in your PATH"

.PHONY: all clean help install benchmark \
        run_memory_info run_io_benchmark run_profiler
