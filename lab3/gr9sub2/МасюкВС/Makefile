CC       = gcc
CFLAGS   = -O2 -pthread -Wall -Wextra -std=c99

SRC_DIR  = src
BIN_DIR  = bin

THREAD_RACE = $(BIN_DIR)/thread_race
PRODCONS    = $(BIN_DIR)/prodcons

.PHONY: all build clean test_race test_prodcons race_unsync race_mutex race_atomic fulltest

all: build

build: $(THREAD_RACE) $(PRODCONS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(THREAD_RACE): $(SRC_DIR)/thread_race.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(PRODCONS): $(SRC_DIR)/prodcons.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@

# ---- Тесты гонки данных ----
test_race: $(THREAD_RACE)
	$(THREAD_RACE) 4 1000000 unsync
	$(THREAD_RACE) 4 1000000 mutex
	$(THREAD_RACE) 4 1000000 atomic

race_unsync: $(THREAD_RACE)
	$(THREAD_RACE) 4 1000000 unsync

race_mutex: $(THREAD_RACE)
	$(THREAD_RACE) 4 1000000 mutex

race_atomic: $(THREAD_RACE)
	$(THREAD_RACE) 4 1000000 atomic

# ---- Тест Producer–Consumer ----
test_prodcons: $(PRODCONS)
	$(PRODCONS) -P 2 -C 2 -N 100000 -B 64

# ---- Все тесты ----
fulltest: test_race test_prodcons

clean:
	rm -rf $(BIN_DIR)
