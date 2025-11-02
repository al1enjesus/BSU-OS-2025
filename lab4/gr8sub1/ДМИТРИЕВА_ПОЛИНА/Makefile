# Makefile для Lab 4 - Управление памятью
# ДМИТРИЕВА_ПОЛИНА, gr8sub1

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
TARGETS = memory_profiler file_analyzer page_faults_demo

all: $(TARGETS)

memory_profiler: src/memory_profiler.c
	$(CC) $(CFLAGS) -o memory_profiler src/memory_profiler.c

file_analyzer: src/mmap_vs_read.c
	$(CC) $(CFLAGS) -o file_analyzer src/mmap_vs_read.c

page_faults_demo: src/page_faults_demo.c
	$(CC) $(CFLAGS) -o page_faults_demo $<

clean:
	rm -f $(TARGETS)
	rm -f *.o
	rm -f test_*.bin
	rm -f test_comparison.bin

test: all
	@echo "=== Testing memory_profiler ==="
	./memory_profiler
	@echo ""
	@echo "=== Testing file_analyzer ==="
	./file_analyzer
	@echo ""
	@echo "=== Testing page_faults_demo ==="
	./page_faults_demo

.PHONY: all clean test
