CC ?= clang
CFLAGS ?= -std=c11 -Wall -Wextra -O2 -pthread
LDFLAGS ?= -pthread

BIN_DIR := bin
SRC_DIR := src

RACE := $(BIN_DIR)/thread_race
PRODCONS := $(BIN_DIR)/prodcons

all: $(RACE) $(PRODCONS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(RACE): $(BIN_DIR) $(SRC_DIR)/thread_race.c
	$(CC) $(CFLAGS) $(SRC_DIR)/thread_race.c -o $(RACE) $(LDFLAGS)

$(PRODCONS): $(BIN_DIR) $(SRC_DIR)/prodcons.c
	$(CC) $(CFLAGS) $(SRC_DIR)/prodcons.c -o $(PRODCONS) $(LDFLAGS)

run_race_unsync: $(RACE)
	$(RACE) 4 1000000 unsync

run_race_mutex: $(RACE)
	$(RACE) 4 1000000 mutex

run_race_atomic: $(RACE)
	$(RACE) 4 1000000 atomic

run_prodcons: $(PRODCONS)
	$(PRODCONS) -P 2 -C 2 -N 100000 -B 64

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean run_race_unsync run_race_mutex run_race_atomic run_prodcons
