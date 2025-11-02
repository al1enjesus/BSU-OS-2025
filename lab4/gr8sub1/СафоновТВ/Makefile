CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGETS = bin/info bin/mmap_vs_read bin/page_faults bin/memory_profiler
SRC_DIR = src
BIN_DIR = bin

all: $(TARGETS)

$(BIN_DIR)/info: $(SRC_DIR)/info.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_DIR)/mmap_vs_read: $(SRC_DIR)/mmap_vs_read.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_DIR)/page_faults: $(SRC_DIR)/page_faults.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_DIR)/memory_profiler: $(SRC_DIR)/memory_profiler.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean