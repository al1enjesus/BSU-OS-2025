CC=gcc
CFLAGS=-Wall -Wextra -O2
TARGETS=memory_analysis io_benchmark disk_load memory_profiler

all: $(TARGETS)

memory_analysis: src/memory_analysis.c
	$(CC) $(CFLAGS) -o $@ $<

io_benchmark: src/io_benchmark.c
	$(CC) $(CFLAGS) -o $@ $<

disk_load: src/disk_load.c
	$(CC) $(CFLAGS) -o $@ $<

memory_profiler: src/memory_profiler.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS) *.bin test_file_*

run_analysis: memory_analysis
	./memory_analysis

run_io_test: io_benchmark
	./io_benchmark

run_profiler: memory_profiler
	@echo "Usage: ./memory_profiler <PID> [--watch]"

.PHONY: all clean run_analysis run_io_test run_profiler
