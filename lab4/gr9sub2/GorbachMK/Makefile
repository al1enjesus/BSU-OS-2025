CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
TARGETS = memory_analysis io_comparison page_faults_demo memory_profiler

all: $(TARGETS)

memory_analysis: src/memory_analysis.c
	$(CC) $(CFLAGS) -o $@ $^

io_comparison: src/io_comparison.c
	$(CC) $(CFLAGS) -o $@ $^

page_faults_demo: src/page_faults_demo.c
	$(CC) $(CFLAGS) -o $@ $^

memory_profiler: src/memory_profiler.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGETS) testfile.bin

run_profiler: memory_profiler
	./memory_profiler $(PID)

.PHONY: all clean run_profiler
