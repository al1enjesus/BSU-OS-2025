# src/Makefile
CC = gcc
CFLAGS = -Wall -Wextra -O2

TARGETS = memory_info mmap_vs_read page_fault_demo memory_profiler

all: $(TARGETS)

memory_info: memory_info.c
	$(CC) $(CFLAGS) $< -o $@

mmap_vs_read: mmap_vs_read.c
	$(CC) $(CFLAGS) $< -o $@

page_fault_demo: page_fault_demo.c
	$(CC) $(CFLAGS) $< -o $@

memory_profiler: memory_profiler.c
	$(CC) $(CFLAGS) $< -o $@

# === БОНУС: Запуск профилировщика (с логом внутри make) ===
run_profiler: memory_profiler
	@mkdir -p logs screenshots
	@echo "=== Запуск memory_profiler ==="
	@echo "PID: $(PID), ARGS: $(ARGS)"
	./memory_profiler $(PID) $(ARGS) > logs/my_process.log 2>&1 || true

# === Остальные цели без изменений ===
run_b:
	@mkdir -p logs
	@echo "=== Задание B: mmap vs read ==="
	sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches' && sleep 1
	/usr/bin/time -v ./mmap_vs_read read > logs/mmap_read.log 2>&1
	/usr/bin/time -v ./mmap_vs_read mmap > logs/mmap_mmap.log 2>&1
	@echo "Логи: logs/mmap_*.log"

run_c:
	@mkdir -p logs
	@echo "=== Задание C: Page Faults ==="
	./page_fault_demo | tee logs/page_faults.log

clean:
	rm -f $(TARGETS) testfile.bin
	rm -f logs/*.log logs/*.csv
	rm -f screenshots/*.png

.PHONY: all clean run_b run_c run_profiler
