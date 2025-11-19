CC?=gcc
CFLAGS?=-O2 -Wall -Wextra -std=c11

BIN_DIR:=bin
SRC_DIR:=src

BINS:=$(BIN_DIR)/memory_profiler $(BIN_DIR)/io_bench $(BIN_DIR)/page_faults

all: $(BINS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/memory_profiler: $(SRC_DIR)/memory_profiler.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BIN_DIR)/io_bench: $(SRC_DIR)/io_bench.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BIN_DIR)/page_faults: $(SRC_DIR)/page_faults.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean