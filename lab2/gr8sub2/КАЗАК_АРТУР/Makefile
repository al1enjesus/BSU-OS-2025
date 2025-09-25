CC=gcc
CFLAGS=-Wall -Wextra -O2
BIN_DIR=bin
BINS=$(BIN_DIR)/fork_example $(BIN_DIR)/ptree
all: $(BIN_DIR) $(BINS)
$(BIN_DIR): ; mkdir -p $(BIN_DIR)
$(BIN_DIR)/fork_example: src/fork_example.c ; $(CC) $(CFLAGS) $< -o $@
$(BIN_DIR)/ptree: src/ptree.c ; $(CC) $(CFLAGS) $< -o $@
clean: ; rm -rf $(BIN_DIR) *.log
run_fork_c: all ; $(BIN_DIR)/fork_example | tee fork_c.log
run_fork_py: ; python3 src/fork_example.py | tee fork_py.log
run_ptree_c: all ; $(BIN_DIR)/ptree
run_ptree_py: ; python3 src/ptree.py
.PHONY: all clean run_fork_c run_fork_py run_ptree_c run_ptree_py