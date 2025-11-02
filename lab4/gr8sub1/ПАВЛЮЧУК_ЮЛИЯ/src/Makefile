CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -O2
TARGETS = memory_profiler memory_demo file_analyzer page_fault_analyzer

all: $(TARGETS)

memory_profiler: memory_profiler.c
	$(CC) $(CFLAGS) -o $@ $^

memory_demo: memory_demo.c
	$(CC) $(CFLAGS) -o $@ $^

file_analyzer: file_analyzer.c
	$(CC) $(CFLAGS) -o $@ $^

page_fault_analyzer: page_fault_analyzer.c
	$(CC) $(CFLAGS) -o $@ $^

run_A: memory_demo memory_profiler
	@echo "=== Задание A: Анализ памяти ==="
	./memory_demo &
	sleep 2
	./memory_profiler $$(pgrep memory_demo) --map --libs
	killall memory_demo 2>/dev/null || true

run_B: file_analyzer
	@echo "=== Задание B: File I/O сравнение ==="
	./file_analyzer

run_C: page_fault_analyzer
	@echo "=== Задание C: Page faults анализ ==="
	./page_fault_analyzer
	

clean:
	rm -f $(TARGETS) testfile.bin *.csv

.PHONY: all clean run_A run_B run_C
