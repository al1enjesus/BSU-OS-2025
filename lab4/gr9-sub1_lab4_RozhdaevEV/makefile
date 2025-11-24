# Lab 4 Memory Profiler - Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
TARGETS = memory_info page_faults_demo mmap_vs_read io_benchmark memory_profiler

.PHONY: all clean test help

all: $(TARGETS)

memory_info: memory_info.c
	$(CC) $(CFLAGS) -o $@ $<

page_faults_demo: page_faults_demo.c
	$(CC) $(CFLAGS) -o $@ $<

mmap_vs_read: mmap_vs_read.c
	$(CC) $(CFLAGS) -o $@ $<

io_benchmark: io_benchmark.c
	$(CC) $(CFLAGS) -o $@ $<

memory_profiler: memory_profiler.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)
	rm -f *.bin test_*

test: all
	@echo "Running basic tests..."
	@./memory_info &
	@PID=$$!; \
	sleep 1; \
	./memory_info $$PID; \
	wait

help:
	@echo "Available targets:"
	@echo "  all    - Build all programs"
	@echo "  clean  - Remove built programs"
	@echo "  test   - Run basic test"
	@echo "  help   - Show this help"
