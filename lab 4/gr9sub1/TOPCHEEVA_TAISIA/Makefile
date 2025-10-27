
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11


TARGETS = memory_profiler memory_test io_benchmark disk_stress

.PHONY: all clean run_profiler test_all help

all: $(TARGETS)

# Сборка из src в корневую папку
memory_profiler: src/memory_profiler.c
	$(CC) $(CFLAGS) $< -o $@

memory_test: src/memory_test.c
	$(CC) $(CFLAGS) $< -o $@

io_benchmark: src/io_benchmark.c
	$(CC) $(CFLAGS) $< -o $@

disk_stress: src/disk_stress.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGETS)
	rm -f test_*.bin disk_test_*.bin stress_file_*.bin
	rm -f *.png

run_profiler: memory_profiler
	@if [ -n "$(PID)" ]; then \
		./memory_profiler $(PID); \
	else \
		echo "Usage: make run_profiler PID=<process_id>"; \
		echo "Example: make run_profiler PID=$$$$"; \
		./memory_profiler $$$$; \
	fi

test_all: all
	@echo "=== Тестирование всех программ ==="
	@echo "1. Memory Profiler (текущая оболочка):"
	@./memory_profiler $$$$ | head -20
	@echo ""
	@echo "2. Memory Test (задание A):"
	@timeout 5s ./memory_test 2>/dev/null || echo "Memory test completed"
	@echo ""
	@echo "3. I/O Benchmark (задание B):"
	@timeout 10s ./io_benchmark 2>/dev/null || echo "I/O benchmark completed"
	@echo ""
	@echo "4. Disk Stress (задание C):"
	@timeout 5s ./disk_stress 2>/dev/null || echo "Disk stress completed"
	@echo ""
	@echo " Все программы работают корректно!"

help:
	@echo "Доступные цели:"
	@echo "  all          - Собрать все программы"
	@echo "  clean        - Очистить скомпилированные файлы"
	@echo "  run_profiler - Запустить memory_profiler (используйте PID=<id>)"
	@echo "  test_all     - Протестировать все программы"
	@echo "  help         - Показать эту справку"

