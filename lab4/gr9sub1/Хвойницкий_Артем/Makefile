CC=gcc
CFLAGS=-Wall -Wextra -O2
LDFLAGS=

BIN_A=./a_memmap
BIN_B=./b_io_bench
BIN_C=./c_disk_stress
BIN_P=./memory_profiler

SRC_DIR=./src

all: $(BIN_A) $(BIN_B) $(BIN_C) $(BIN_P)

$(BIN_A): $(SRC_DIR)/a_memmap.c
	$(CC) $(CFLAGS) $< -o $@

$(BIN_B): $(SRC_DIR)/b_io_bench.c
	$(CC) $(CFLAGS) $< -o $@

$(BIN_C): $(SRC_DIR)/c_disk_stress.c
	$(CC) $(CFLAGS) $< -o $@

$(BIN_P): $(SRC_DIR)/memory_profiler.c
	$(CC) $(CFLAGS) $< -o $@

run_profiler: $(BIN_P)
	./memory_profiler $$

clean:
	rm -f $(BIN_A) $(BIN_B) $(BIN_C) $(BIN_P) *.o *.bin test_*.bin stress.bin
