CC = gcc
CFLAGS = -O2 -Wall -pthread

TARGETS = mem_analyze test_disk_io

all: $(TARGETS)

mem_analyze: mem_analyze.c
	$(CC) $(CFLAGS) mem_analyze.c -o mem_analyze

disk_io_test: test_disk_io.c
	$(CC) $(CFLAGS) test_disk_io.c -o disk_io_test

run_mem:
	@echo "=== Запуск mem_analyze ==="
	./mem_analyze

run_disk:
	@echo "=== Запуск disk_io_test ==="
	./disk_io_test

ps_info:
	@echo "=== Информация о памяти ==="
	ps -o pid,comm,vsz,rss -p $$PPID
	cat /proc/self/status | grep -E '^Vm'

maps:
	@echo "=== Карта памяти ==="
	cat /proc/self/maps | head -n 20

clean:
	rm -f $(TARGETS) *.o *.bin *.txt

.PHONY: all clean run_mem run_disk ps_info maps