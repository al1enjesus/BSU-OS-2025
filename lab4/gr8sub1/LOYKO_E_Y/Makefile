CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
LDFLAGS =

TARGETS = memory_info mmap_vs_read page_faults_demo io_benchmark memory_profiler

all: $(TARGETS)

memory_info: src/memory_info.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

mmap_vs_read: src/mmap_vs_read.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

page_faults_demo: src/page_faults_demo.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

io_benchmark: src/io_benchmark.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

memory_profiler: src/memory_profiler.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGETS)
	rm -f *.o
	rm -f test*.bin
	rm -f testfile.bin

test: all
	@echo "=== Running basic tests ==="
	@./memory_info

.PHONY: all clean test
