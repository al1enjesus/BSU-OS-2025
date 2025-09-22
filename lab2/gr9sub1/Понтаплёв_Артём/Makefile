CC = gcc
CFLAGS = -Wall -Wextra -O2
SRC_DIR = src
BIN_DIR = bin
TARGETS = $(BIN_DIR)/fork_example $(BIN_DIR)/ptree

all: $(TARGETS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/fork_example: $(SRC_DIR)/fork_example.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_DIR)/ptree: $(SRC_DIR)/ptree.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $<
	
clean:
	rm -f $(TARGETS)

run_fork: $(BIN_DIR)/fork_example
	./$(BIN_DIR)/fork_example

run_ptree: $(BIN_DIR)/ptree
	./$(BIN_DIR)/ptree

.PHONY: all clean run_fork run_ptree
