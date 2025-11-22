# Makefile для Lab 4 - Управление памятью и файловый I/O
# Вариант 1 (нечётные номера)

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -D_POSIX_C_SOURCE=200809L
LDFLAGS =

# Цели для Варианта 1
TARGETS = memory_info mmap_vs_read page_faults_demo memory_profiler

# Основная цель - собрать всё
all: $(TARGETS)

# Индивидуальные программы для Варианта 1
memory_info: memory_info.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

mmap_vs_read: mmap_vs_read.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

page_faults_demo: page_faults_demo.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

memory_profiler: memory_profiler.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Очистка
clean:
	rm -f $(TARGETS)
	rm -f *.o
	rm -f testfile.bin
	rm -f test_mmap_file.bin

# Запуск тестов
test: all
	@echo "=== Running tests for Variant 1 ==="
	@echo ""
	@echo "1. Memory Info (self)"
	@./memory_info &
	@PID=$$!; sleep 2; ./memory_info $$PID; kill $$PID 2>/dev/null || true
	@echo ""
	@echo "2. Page Faults Demo"
	@./page_faults_demo | head -n 50
	@echo ""
	@echo "All tests completed. Check output above."

# Запуск примеров
run_memory_info:
	./memory_info

run_mmap_vs_read:
	./mmap_vs_read testfile.bin --create-file 50

run_page_faults:
	./page_faults_demo

run_profiler:
	@echo "Usage: make run_profiler PID=<process_id>"
	@if [ -n "$(PID)" ]; then \
		./memory_profiler $(PID); \
	else \
		echo "Example: make run_profiler PID=$$$$"; \
		./memory_profiler $$$$; \
	fi

# Помощь
help:
	@echo "Lab 4 Makefile - Available targets:"
	@echo ""
	@echo "  all              - Build all programs"
	@echo "  clean            - Remove compiled binaries"
	@echo "  test             - Run basic tests"
	@echo ""
	@echo "Individual programs:"
	@echo "  memory_info      - Build memory info analyzer"
	@echo "  mmap_vs_read     - Build mmap vs read comparison"
	@echo "  page_faults_demo - Build page faults demonstration"
	@echo "  memory_profiler  - Build memory profiler (main utility)"
	@echo ""
	@echo "Run examples:"
	@echo "  run_memory_info  - Run memory info demo"
	@echo "  run_mmap_vs_read - Run mmap vs read comparison"
	@echo "  run_page_faults  - Run page faults demo"
	@echo "  run_profiler     - Run memory profiler (use PID=<pid>)"
	@echo ""
	@echo "Examples:"
	@echo "  make                    # Build everything"
	@echo "  make run_mmap_vs_read   # Run I/O comparison"
	@echo "  make run_page_faults    # Run page faults demo"
	@echo "  make run_profiler PID=1234  # Profile process 1234"

.PHONY: all clean test help \
        run_memory_info run_mmap_vs_read run_page_faults run_profiler
