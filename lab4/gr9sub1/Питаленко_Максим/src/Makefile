# Makefile for Lab 4 - Complete Version

CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGETS = memory_info mmap_vs_read page_faults_demo memory_profiler io_benchmark

all: $(TARGETS)

memory_info: memory_info.c
	$(CC) $(CFLAGS) -o memory_info memory_info.c

mmap_vs_read: mmap_vs_read.c
	$(CC) $(CFLAGS) -o mmap_vs_read mmap_vs_read.c

page_faults_demo: page_faults_demo.c
	$(CC) $(CFLAGS) -o page_faults_demo page_faults_demo.c

memory_profiler: memory_profiler.c
	$(CC) $(CFLAGS) -o memory_profiler memory_profiler.c

io_benchmark: io_benchmark.c
	$(CC) $(CFLAGS) -o io_benchmark io_benchmark.c

clean:
	rm -f $(TARGETS) *.bin test_*

run_memory_info:
	./memory_info

run_mmap_test:
	./mmap_vs_read testfile.bin --create-file 10
	./mmap_vs_read testfile.bin

run_page_faults:
	./page_faults_demo

run_profiler:
	./memory_profiler $$

run_benchmark:
	./io_benchmark --size 50

test_all: clean all
	@echo "=== Testing memory_info ==="
	./memory_info &
	@sleep 2
	@echo ""
	@echo "=== Testing page_faults_demo ==="
	./page_faults_demo
	@echo ""
	@echo "=== Testing memory_profiler ==="
	./memory_profiler $$
	@echo ""
	@echo "=== All tests completed ==="

.PHONY: all clean run_memory_info run_mmap_test run_page_faults run_profiler run_benchmark test_all