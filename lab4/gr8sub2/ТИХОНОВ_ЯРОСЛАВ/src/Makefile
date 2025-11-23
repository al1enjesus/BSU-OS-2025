CC = gcc
CFLAGS = -O2 -Wall -pthread

TARGETS = mem_analyze test_disk_io io_compare memory_profiler

all: $(TARGETS)

mem_analyze: mem_analyze.c
	$(CC) $(CFLAGS) mem_analyze.c -o mem_analyze

test_disk_io: test_disk_io.c
	$(CC) $(CFLAGS) test_disk_io.c -o test_disk_io

io_compare: io_compare.c
	$(CC) $(CFLAGS) io_compare.c -o io_compare

memory_profiler: memory_profiler.c
	$(CC) $(CFLAGS) memory_profiler.c -o memory_profiler

run_mem:
	@echo "=== Запуск mem_analyze ==="
	./mem_analyze

run_disk:
	@echo "=== Запуск disk_io_test ==="
	./test_disk_io

run_compare:
	@echo "=== Запуск io_compare ==="
	./io_compare

run_profiler:
	@echo "=== Запуск memory_profiler ==="
	./memory_profiler

ps_info:
	@echo "=== Информация о памяти ==="
	ps -o pid,comm,vsz,rss -p $$PPID
	cat /proc/self/status | grep -E '^Vm'

maps:
	@echo "=== Карта памяти ==="
	cat /proc/self/maps | head -n 20

clean:
	rm -f $(TARGETS) *.o *.bin *.txt

.PHONY: all clean run_mem run_disk run_compare run_profiler ps_info maps
