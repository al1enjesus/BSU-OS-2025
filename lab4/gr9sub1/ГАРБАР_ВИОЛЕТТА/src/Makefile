CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -D_POSIX_C_SOURCE=199309L

.PHONY: all clean

all: memory_profiler io_benchmark disk_io_workload memory_analysis

memory_profiler: memory_profiler.c
	$(CC) $(CFLAGS) -o $@ $<

io_benchmark: io_benchmark.c
	$(CC) $(CFLAGS) -o $@ $<

disk_io_workload: disk_io_workload.c
	$(CC) $(CFLAGS) -o $@ $<

memory_analysis: memory_analysis.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f memory_profiler io_benchmark disk_io_workload memory_analysis
	rm -f workload_file_*.bin
	rm -f test_*.bin
