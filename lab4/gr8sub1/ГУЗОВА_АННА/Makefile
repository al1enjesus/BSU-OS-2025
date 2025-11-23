CC := gcc
CFLAGS := -Wall -Wextra -O2 -std=c11
LDFLAGS :=

PROGS := memory_info mmap_vs_read page_faults_demo io_benchmark memory_profiler

.PHONY: all clean help run_memory_info run_profiler run_io_bench run_mmap_vs_read run_page_faults
all: $(PROGS)

memory_info: src/memory_info.c
	$(CC) $(CFLAGS) $< -o $@
	
mmap_vs_read: src/mmap_vs_read.c
	$(CC) $(CFLAGS) $< -o $@

io_benchmark: src/io_benchmark.c
	$(CC) $(CFLAGS) $< -o $@

memory_profiler: src/memory_profiler.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(PROGS) *.o test_*.bin testfile.bin *.csv *.dat

help:
	@echo "Targets:"
	@echo "  make all"
	@echo "  make clean"
	@echo "  make run_memory_info"
	@echo "  make run_profiler"
	@echo "  make run_io_bench"
	@echo "  make run_mmap_vs_read"
	@echo "  make run_page_faults"

run_memory_info: memory_info
	./memory_info

run_profiler: memory_profiler
	./memory_profiler $$

run_io_bench: io_benchmark
	./io_benchmark --size 100

run_mmap_vs_read: mmap_vs_read
	./mmap_vs_read testfile.bin --create-file 100 && ./mmap_vs_read testfile.bin

